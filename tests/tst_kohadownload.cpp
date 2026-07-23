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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUrlQuery>

#include <functional>

#include "kohadownload.h"
#include "mockhttpserver.h"

class TestKohaDownload : public QObject
{
    Q_OBJECT

    private slots:
        void restMode();
        void restIncrementalMode();
        void restAuthFailureHint();
        void reportMode();
        void reportTruncationDetected();
        void reportBadJson();

    private:
        int countRows( const QString & dbPath, const QString & table );
};

int TestKohaDownload::countRows( const QString & dbPath, const QString & table )
{
    int count = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "kohadownload_test" );
        db.setDatabaseName( dbPath );
        if ( db.open() ) {
            QSqlQuery query( db );
            if ( query.exec( "SELECT COUNT(*) FROM " + table ) && query.next() ) {
                count = query.value( 0 ).toInt();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase( "kohadownload_test" );

    return count;
}

void TestKohaDownload::restMode()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    // 1000 rows is the client's page size, a full first page forces it
    // to request a second one
    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( status )
        QUrlQuery query( url );
        int page = query.queryItemValue( "_page" ).toInt();

        QJsonArray rows;
        if ( url.path() == "/api/v1/patrons" ) {
            int count = page == 1 ? 1000 : 7;
            for ( int i = 0; i < count; i++ ) {
                QJsonObject patron;
                patron.insert( "patron_id", ( page - 1 ) * 1000 + i + 1 );
                patron.insert( "cardnumber", QString( "CARD%1" ).arg( ( page - 1 ) * 1000 + i + 1 ) );
                patron.insert( "surname", "Surname" );
                patron.insert( "account_balance", 1.5 );
                rows.append( patron );
            }
        } else if ( url.path() == "/api/v1/checkouts" ) {
            for ( int i = 0; i < 2; i++ ) {
                QJsonObject item;
                item.insert( "callnumber", "FIC" );
                item.insert( "item_type", "BK" );
                QJsonObject biblio;
                biblio.insert( "title", "A Title" );
                item.insert( "biblio", biblio );

                QJsonObject checkout;
                checkout.insert( "patron_id", i + 1 );
                checkout.insert( "due_date", "2026-07-30T23:59:00" );
                checkout.insert( "item", item );
                rows.append( checkout );
            }
        }
        *body = QJsonDocument( rows ).toJson( QJsonDocument::Compact );
    };

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    KohaDownload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.useReports = false;

    QTemporaryDir dir;
    QString dbPath = dir.filePath( "borrowers.db" );

    download.start( config, dbPath );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QVERIFY2( finishedSpy.first().at( 0 ).toBool(),
              qPrintable( finishedSpy.first().at( 1 ).toString() ) );

    QCOMPARE( countRows( dbPath, "borrowers" ), 1007 );
    QCOMPARE( countRows( dbPath, "issues" ), 2 );

    // Two patron pages plus one checkout page
    QCOMPARE( server.requests.count(), 3 );
    QVERIFY( server.requestHeaders.first().value( "authorization" ).startsWith( "Basic " ) );
    QCOMPARE( server.requestHeaders.first().value( "x-koha-embed" ), QByteArray( "account_balance" ) );
    QCOMPARE( server.requestHeaders.last().value( "x-koha-embed" ), QByteArray( "item,item.biblio" ) );
}

void TestKohaDownload::restIncrementalMode()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( status )
        QJsonArray rows;
        if ( url.path() == "/api/v1/patrons" ) {
            QJsonObject patron;
            patron.insert( "patron_id", 1000 );
            patron.insert( "cardnumber", "NEWCARD" );
            patron.insert( "surname", "New" );
            rows.append( patron );
        }
        *body = QJsonDocument( rows ).toJson( QJsonDocument::Compact );
    };

    // Seed an existing database the incremental download merges into
    QTemporaryDir dir;
    QString dbPath = dir.filePath( "borrowers.db" );

    KohaPatron existing;
    existing.borrowernumber = "1";
    existing.cardnumber = "OLDCARD";
    QString error;
    QVERIFY2( BorrowersDb::write( dbPath, { existing }, {}, &error ), qPrintable( error ) );

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    KohaDownload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.useReports = false;
    config.updatedSince = "2026-07-01T00:00:00";

    download.start( config, dbPath );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QVERIFY2( finishedSpy.first().at( 0 ).toBool(),
              qPrintable( finishedSpy.first().at( 1 ).toString() ) );

    // The old patron survives the merge alongside the changed one
    QCOMPARE( countRows( dbPath, "borrowers" ), 2 );

    // Only the patrons request carries the updated_on filter
    QUrlQuery patronsQuery( server.requests.first() );
    QVERIFY( patronsQuery.queryItemValue( "q", QUrl::FullyDecoded ).contains( "updated_on" ) );
    QUrlQuery checkoutsQuery( server.requests.last() );
    QVERIFY( ! checkoutsQuery.hasQueryItem( "q" ) );
}

void TestKohaDownload::restAuthFailureHint()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        *status = 401;
        *body = "{}";
    };

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    KohaDownload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "wrong";
    config.useReports = false;

    QTemporaryDir dir;
    download.start( config, dir.filePath( "borrowers.db" ) );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), false );
    QVERIFY( finishedSpy.first().at( 1 ).toString().contains( "RESTBasicAuth" ) );
}

void TestKohaDownload::reportMode()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( status )
        QUrlQuery query( url );
        int reportId = query.queryItemValue( "id" ).toInt();

        QJsonArray rows;
        if ( reportId == 11 ) {
            for ( int i = 0; i < 12; i++ ) {
                QJsonArray row;
                row.append( i + 1 );
                row.append( QString( "CARD%1" ).arg( i + 1 ) );
                row.append( "Surname" );
                row.append( "Firstname" );
                row.append( "" );
                row.append( "" );
                row.append( "" );
                row.append( "1990-01-01" );
                row.append( "2.5" );
                rows.append( row );
            }
        } else if ( reportId == 22 ) {
            for ( int i = 0; i < 3; i++ ) {
                QJsonArray row;
                row.append( i + 1 );
                row.append( "2026-07-30 23:59:00" );
                row.append( "FIC" );
                row.append( "A Title" );
                row.append( "BK" );
                rows.append( row );
            }
        }
        *body = QJsonDocument( rows ).toJson( QJsonDocument::Compact );
    };

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    KohaDownload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.useReports = true;
    config.borrowersReportId = 11;
    config.issuesReportId = 22;

    QTemporaryDir dir;
    QString dbPath = dir.filePath( "borrowers.db" );

    download.start( config, dbPath );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QVERIFY2( finishedSpy.first().at( 0 ).toBool(),
              qPrintable( finishedSpy.first().at( 1 ).toString() ) );

    QCOMPARE( countRows( dbPath, "borrowers" ), 12 );
    QCOMPARE( countRows( dbPath, "issues" ), 3 );

    // Credentials go to the report service as query parameters, in both
    // the Koha 24.05+ and the older spellings
    QCOMPARE( server.requests.count(), 2 );
    QUrlQuery firstRequest( server.requests.first() );
    QCOMPARE( firstRequest.queryItemValue( "id" ), QString( "11" ) );
    QCOMPARE( firstRequest.queryItemValue( "login_userid" ), QString( "koha" ) );
    QCOMPARE( firstRequest.queryItemValue( "login_password" ), QString( "koha" ) );
    QCOMPARE( firstRequest.queryItemValue( "userid" ), QString( "koha" ) );
    QCOMPARE( firstRequest.queryItemValue( "password" ), QString( "koha" ) );
}

void TestKohaDownload::reportTruncationDetected()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    // Exactly 10 rows is Koha's default SvcMaxReportRows limit
    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        QJsonArray rows;
        for ( int i = 0; i < 10; i++ ) {
            QJsonArray row;
            row.append( i + 1 );
            rows.append( row );
        }
        *body = QJsonDocument( rows ).toJson( QJsonDocument::Compact );
    };

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    KohaDownload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.useReports = true;
    config.borrowersReportId = 11;
    config.issuesReportId = 22;

    QTemporaryDir dir;
    download.start( config, dir.filePath( "borrowers.db" ) );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), false );
    QVERIFY( finishedSpy.first().at( 1 ).toString().contains( "SvcMaxReportRows" ) );
}

void TestKohaDownload::reportBadJson()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    // A wrong URL or report ID typically returns an HTML page
    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "<html>login page</html>";
    };

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    KohaDownload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.useReports = true;
    config.borrowersReportId = 11;
    config.issuesReportId = 22;

    QTemporaryDir dir;
    download.start( config, dir.filePath( "borrowers.db" ) );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), false );
    QVERIFY( finishedSpy.first().at( 1 ).toString().contains( "JSON" ) );
}

QTEST_GUILESS_MAIN(TestKohaDownload)

#include "tst_kohadownload.moc"
