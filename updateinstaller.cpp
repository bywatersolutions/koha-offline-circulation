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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QScopedPointer>

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
