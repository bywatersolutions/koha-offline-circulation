/*
* Copyright 2026 Kyle M Hall <kyle@bywatersolutions.com>
*
* This file is part of Koha Offline Circulation.
*
* Koha Offline Circulation is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Koha Offline Circulation is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Koha Offline Circulation.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "updateinstaller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopedPointer>
#include <QTemporaryDir>

#if defined( Q_OS_WIN )
#include <windows.h>
#include <shellapi.h>
#pragma comment( lib, "shell32" )
#else
#include <cstdio>
#endif

UpdateInstaller::UpdateInstaller( QObject *parent )
 : QObject( parent ), mReply( 0 ), mFile( 0 ), mHash( QCryptographicHash::Sha256 )
{
    mNetwork = new QNetworkAccessManager( this );
    downloadDir = QDir::tempPath();
}

UpdateInstaller::Kind UpdateInstaller::installKind()
{
#if defined( Q_OS_WIN )
    // The installer leaves its uninstaller beside the exe, the portable
    // zip has nothing to run an update through
    if ( QFile::exists( QCoreApplication::applicationDirPath() + "/unins000.exe" ) ) return WindowsInstaller;
#elif defined( Q_OS_MACOS )
    if ( QCoreApplication::applicationDirPath().endsWith( ".app/Contents/MacOS" ) ) return MacDmg;
#elif defined( Q_OS_LINUX )
    // The AppImage runtime sets this to the path of the running file. A
    // Flatpak bundle has no repository to update from, so it's unsupported
    if ( ! qEnvironmentVariableIsEmpty( "APPIMAGE" ) ) return LinuxAppImage;
#endif

    return Unsupported;
}

int UpdateInstaller::pickAsset( const QList<UpdateAsset> & assets, Kind kind )
{
    QString suffix;
    switch ( kind ) {
        case WindowsInstaller: suffix = "-setup.exe"; break;
        case MacDmg: suffix = "-macos.dmg"; break;
        case LinuxAppImage: suffix = "-x86_64.AppImage"; break;
        case Unsupported: return -1;
    }

    for ( int i = 0; i < assets.count(); i++ ) {
        if ( assets.at( i ).name.endsWith( suffix ) ) return i;
    }

    return -1;
}

void UpdateInstaller::download( const UpdateAsset & asset )
{
    mAsset = asset;
    mHash.reset();

    // Written under the asset's own name, the installers are recognised by it
    mFile = new QSaveFile( QDir( downloadDir ).filePath( asset.name ), this );
    if ( ! mFile->open( QIODevice::WriteOnly ) ) {
        QString error = tr("Could not save the update to %1: %2").arg( mFile->fileName(), mFile->errorString() );
        delete mFile;
        mFile = 0;
        emit failed( error );
        return;
    }

    QNetworkRequest request( asset.url );
    request.setTransferTimeout( 60000 );

    mReply = mNetwork->get( request );
    connect( mReply, SIGNAL( readyRead() ),
             this, SLOT( onReadyRead() ) );
    connect( mReply, SIGNAL( downloadProgress( qint64, qint64 ) ),
             this, SIGNAL( downloadProgress( qint64, qint64 ) ) );
    connect( mReply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void UpdateInstaller::cancel()
{
    if ( mReply ) mReply->abort();
}

void UpdateInstaller::onReadyRead()
{
    // Streamed to disk and hashed as it arrives, an AppImage is too big
    // to hold in memory
    QByteArray data = mReply->readAll();
    mHash.addData( data );
    mFile->write( data );
}

void UpdateInstaller::onReplyFinished()
{
    QNetworkReply *reply = mReply;
    mReply = 0;
    reply->deleteLater();

    // Deleting an uncommitted QSaveFile removes its temporary file, so a
    // failed or unverified download leaves nothing behind
    QScopedPointer<QSaveFile> file( mFile );
    mFile = 0;

    if ( reply->error() != QNetworkReply::NoError ) {
        emit failed( tr("Could not download the update: %1").arg( reply->errorString() ) );
        return;
    }

    QByteArray data = reply->readAll();
    mHash.addData( data );
    file->write( data );

    if ( mAsset.sha256.isEmpty() ) {
        emit failed( tr("The release doesn't publish a checksum for %1, so the download can't be verified.")
                     .arg( mAsset.name ) );
        return;
    }

    if ( mHash.result() != mAsset.sha256 ) {
        emit failed( tr("The downloaded update doesn't match the checksum published for it. Try again later.") );
        return;
    }

    if ( ! file->commit() ) {
        emit failed( tr("Could not save the update to %1: %2").arg( file->fileName(), file->errorString() ) );
        return;
    }

    emit ready( file->fileName() );
}

#if ! defined( Q_OS_WIN )
// Starts the new version once this process has exited, so the two never
// run side by side
static void relaunchAfterExit( const QStringList & command )
{
    QStringList arguments = { "-c", "while kill -0 \"$0\" 2>/dev/null; do sleep 0.2; done; exec \"$@\"",
                              QString::number( QCoreApplication::applicationPid() ) };
    QProcess::startDetached( "/bin/sh", arguments + command );
}
#endif

#if defined( Q_OS_MACOS )
static bool runTool( const QString & program, const QStringList & arguments, QString * output = 0 )
{
    QProcess process;
    process.setProcessChannelMode( QProcess::MergedChannels );
    process.start( program, arguments );
    if ( ! process.waitForFinished( 300000 ) ) return false;
    if ( output ) *output = QString::fromUtf8( process.readAll() );

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

// Empty for an ad-hoc signed development build
static QString teamIdentifier( const QString & bundle )
{
    QString output;
    runTool( "/usr/bin/codesign", { "-dv", "--verbose=2", bundle }, &output );

    QRegularExpressionMatch match = QRegularExpression( "^TeamIdentifier=(\\S+)$",
                                                        QRegularExpression::MultilineOption ).match( output );
    QString team = match.hasMatch() ? match.captured( 1 ) : QString();

    return team == "not" ? QString() : team;
}
#endif

bool UpdateInstaller::install( const QString & path, QString * error )
{
#if defined( Q_OS_WIN )
    // The installer asks for administrator rights, only ShellExecute can
    // raise the UAC prompt for it. RELAUNCH tells it to start the app
    // again once it's done, an IT deployment doesn't pass it
    std::wstring file = QDir::toNativeSeparators( path ).toStdWString();
    std::wstring parameters = L"/SILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /NORESTARTAPPLICATIONS /RELAUNCH=1";

    SHELLEXECUTEINFOW info = {};
    info.cbSize = sizeof( info );
    info.fMask = SEE_MASK_NOASYNC;
    info.lpVerb = L"open";
    info.lpFile = file.c_str();
    info.lpParameters = parameters.c_str();
    info.nShow = SW_SHOWNORMAL;

    if ( ! ShellExecuteExW( &info ) ) {
        DWORD code = GetLastError();
        if ( code == ERROR_CANCELLED ) {
            *error = tr("Installing the update needs a Windows administrator. Ask your IT staff to install the new version.");
        } else {
            *error = tr("Could not start the installer, Windows error %1.").arg( code );
        }
        return false;
    }

    return true;
#elif defined( Q_OS_MACOS )
    QString bundle = QDir( QCoreApplication::applicationDirPath() + "/../.." ).canonicalPath();
    QString parent = QFileInfo( bundle ).absolutePath();

    if ( ! QFileInfo( parent ).isWritable() ) {
        *error = tr("The app can't be replaced in %1 without an administrator.").arg( parent );
        return false;
    }

    QTemporaryDir mountPoint;
    if ( ! runTool( "/usr/bin/hdiutil", { "attach", "-nobrowse", "-readonly", "-noautoopen",
                                          "-mountpoint", mountPoint.path(), path } ) ) {
        *error = tr("Could not open the downloaded disk image.");
        return false;
    }

    // Copied next to the running app first, so the swap below is a rename
    // on the same disk
    QString staged = parent + "/.KohaOfflineCirculation-update.app";
    QDir( staged ).removeRecursively();

    QStringList apps = QDir( mountPoint.path() ).entryList( { "*.app" }, QDir::Dirs );
    bool copied = ! apps.isEmpty() && runTool( "/usr/bin/ditto", { mountPoint.path() + "/" + apps.first(), staged } );
    runTool( "/usr/bin/hdiutil", { "detach", mountPoint.path() } );
    QFile::remove( path );

    if ( ! copied ) {
        QDir( staged ).removeRecursively();
        *error = tr("Could not copy the new version out of the disk image.");
        return false;
    }

    // A signed release must come from the same developer as this copy,
    // a development build has no team to compare against
    QString currentTeam = teamIdentifier( bundle );
    if ( ! runTool( "/usr/bin/codesign", { "--verify", "--deep", "--strict", staged } )
         || ( ! currentTeam.isEmpty() && teamIdentifier( staged ) != currentTeam ) ) {
        QDir( staged ).removeRecursively();
        *error = tr("The new version's code signature could not be verified, it was not installed.");
        return false;
    }

    QString old = parent + "/.KohaOfflineCirculation-old.app";
    QDir( old ).removeRecursively();
    if ( ! QDir().rename( bundle, old ) ) {
        QDir( staged ).removeRecursively();
        *error = tr("Could not move the current version out of the way in %1.").arg( parent );
        return false;
    }
    if ( ! QDir().rename( staged, bundle ) ) {
        QDir().rename( old, bundle );
        QDir( staged ).removeRecursively();
        *error = tr("Could not move the new version into place in %1.").arg( parent );
        return false;
    }
    QDir( old ).removeRecursively();

    relaunchAfterExit( { "/usr/bin/open", "-n", bundle } );
    return true;
#elif defined( Q_OS_LINUX )
    QString target = qEnvironmentVariable( "APPIMAGE" );
    QString dir = QFileInfo( target ).absolutePath();

    if ( ! QFileInfo( dir ).isWritable() ) {
        *error = tr("The AppImage in %1 can't be replaced, the folder isn't writable.").arg( dir );
        return false;
    }

    // Staged beside the running AppImage so rename() can swap it in place,
    // the running copy keeps the file it was started from
    QString staged = dir + "/.KohaOfflineCirculation-update.AppImage";
    QFile::remove( staged );
    bool copied = QFile::copy( path, staged );
    QFile::remove( path );

    if ( ! copied ) {
        *error = tr("Could not copy the new version to %1.").arg( dir );
        return false;
    }

    QFile::setPermissions( staged, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                   | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther | QFile::ExeOther );

    if ( std::rename( QFile::encodeName( staged ).constData(), QFile::encodeName( target ).constData() ) != 0 ) {
        QFile::remove( staged );
        *error = tr("Could not replace %1 with the new version.").arg( target );
        return false;
    }

    relaunchAfterExit( { target } );
    return true;
#else
    Q_UNUSED( path )
    *error = tr("Updates can't be installed on this platform.");
    return false;
#endif
}
