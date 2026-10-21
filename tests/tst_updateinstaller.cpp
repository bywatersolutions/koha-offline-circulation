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

#include <QtTest>
#include <QTemporaryDir>

#include "updateinstaller.h"
#include "mockhttpserver.h"

class TestUpdateInstaller : public QObject
{
    Q_OBJECT

    private slots:
        void pickAsset();
        void downloadVerified();
        void downloadChecksumMismatch();
        void downloadWithoutChecksum();
        void downloadServerError();

    private:
        UpdateAsset servedAsset( MockHttpServer & server, const QByteArray & sha256 );
};

UpdateAsset TestUpdateInstaller::servedAsset( MockHttpServer & server, const QByteArray & sha256 )
{
    UpdateAsset asset;
    asset.name = "KohaOfflineCirculation-9.9.9-setup.exe";
    asset.url = QUrl( server.baseUrl() + "/download/setup.exe" );
    asset.sha256 = sha256;
    return asset;
}

void TestUpdateInstaller::pickAsset()
{
    // The asset names of a real release
    QList<UpdateAsset> assets;
    for ( const QString & name : { "KohaOfflineCirculation-2.5.0-macos.dmg",
                                   "KohaOfflineCirculation-2.5.0-setup.exe",
                                   "KohaOfflineCirculation-2.5.0-windows-portable.zip",
                                   "KohaOfflineCirculation-2.5.0-x86_64.AppImage",
                                   "KohaOfflineCirculation-2.5.0-x86_64.flatpak" } ) {
        UpdateAsset asset;
        asset.name = name;
        assets.append( asset );
    }

    QCOMPARE( UpdateInstaller::pickAsset( assets, UpdateInstaller::MacDmg ), 0 );
    QCOMPARE( UpdateInstaller::pickAsset( assets, UpdateInstaller::WindowsInstaller ), 1 );
    QCOMPARE( UpdateInstaller::pickAsset( assets, UpdateInstaller::LinuxAppImage ), 3 );
    QCOMPARE( UpdateInstaller::pickAsset( assets, UpdateInstaller::Unsupported ), -1 );

    // A release missing this platform's package offers nothing to install
    assets.removeAt( 1 );
    QCOMPARE( UpdateInstaller::pickAsset( assets, UpdateInstaller::WindowsInstaller ), -1 );
}

void TestUpdateInstaller::downloadVerified()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    QByteArray content = "installer bytes";
    server.handler = [&content]( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = content;
    };

    QTemporaryDir dir;
    UpdateInstaller installer;
    installer.downloadDir = dir.path();
    QSignalSpy readySpy( &installer, SIGNAL( ready( QString ) ) );
    QSignalSpy failedSpy( &installer, SIGNAL( failed( QString ) ) );

    installer.download( servedAsset( server, QCryptographicHash::hash( content, QCryptographicHash::Sha256 ) ) );
    QVERIFY( readySpy.wait( 10000 ) );
    QCOMPARE( failedSpy.count(), 0 );

    QString path = readySpy.first().at( 0 ).toString();
    QCOMPARE( path, dir.filePath( "KohaOfflineCirculation-9.9.9-setup.exe" ) );

    QFile file( path );
    QVERIFY( file.open( QIODevice::ReadOnly ) );
    QCOMPARE( file.readAll(), content );
}

void TestUpdateInstaller::downloadChecksumMismatch()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "tampered bytes";
    };

    QTemporaryDir dir;
    UpdateInstaller installer;
    installer.downloadDir = dir.path();
    QSignalSpy readySpy( &installer, SIGNAL( ready( QString ) ) );
    QSignalSpy failedSpy( &installer, SIGNAL( failed( QString ) ) );

    installer.download( servedAsset( server, QCryptographicHash::hash( "installer bytes", QCryptographicHash::Sha256 ) ) );
    QVERIFY( failedSpy.wait( 10000 ) );
    QCOMPARE( readySpy.count(), 0 );
    QVERIFY( failedSpy.first().at( 0 ).toString().contains( "checksum" ) );

    // Nothing that failed verification is left on disk
    QVERIFY( QDir( dir.path() ).isEmpty() );
}

void TestUpdateInstaller::downloadWithoutChecksum()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "installer bytes";
    };

    QTemporaryDir dir;
    UpdateInstaller installer;
    installer.downloadDir = dir.path();
    QSignalSpy failedSpy( &installer, SIGNAL( failed( QString ) ) );

    installer.download( servedAsset( server, QByteArray() ) );
    QVERIFY( failedSpy.wait( 10000 ) );
    QVERIFY( failedSpy.first().at( 0 ).toString().contains( "checksum" ) );
    QVERIFY( QDir( dir.path() ).isEmpty() );
}

void TestUpdateInstaller::downloadServerError()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        *status = 404;
        *body = "{}";
    };

    QTemporaryDir dir;
    UpdateInstaller installer;
    installer.downloadDir = dir.path();
    QSignalSpy failedSpy( &installer, SIGNAL( failed( QString ) ) );

    installer.download( servedAsset( server, QCryptographicHash::hash( "{}", QCryptographicHash::Sha256 ) ) );
    QVERIFY( failedSpy.wait( 10000 ) );
    QVERIFY( QDir( dir.path() ).isEmpty() );
}

QTEST_GUILESS_MAIN(TestUpdateInstaller)

#include "tst_updateinstaller.moc"
