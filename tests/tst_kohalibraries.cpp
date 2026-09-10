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
#include <QUrlQuery>

#include "kohalibraries.h"
#include "mockhttpserver.h"

class TestKohaLibraries : public QObject
{
    Q_OBJECT

    private slots:
        void librariesLoaded();
        void notFoundHint();
        void badJson();
};

void TestKohaLibraries::librariesLoaded()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "[{\"library_id\":\"CPL\",\"name\":\"Centerville\"},"
                "{\"library_id\":\"MPL\",\"name\":\"Midway\"}]";
    };

    KohaLibraries libraries;
    QSignalSpy finishedSpy( &libraries, SIGNAL( finished( bool, QString ) ) );

    // A trailing slash on the staff URL is tolerated, like the other clients
    libraries.start( server.baseUrl() + "/" );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QVERIFY2( finishedSpy.first().at( 0 ).toBool(),
              qPrintable( finishedSpy.first().at( 1 ).toString() ) );

    QCOMPARE( libraries.libraries().count(), 2 );
    QCOMPARE( libraries.libraries().at( 0 ).code, QString( "CPL" ) );
    QCOMPARE( libraries.libraries().at( 0 ).name, QString( "Centerville" ) );
    QCOMPARE( libraries.libraries().at( 1 ).code, QString( "MPL" ) );
    QCOMPARE( libraries.libraries().at( 1 ).name, QString( "Midway" ) );

    // The public namespace needs no credentials, and -1 turns paging off
    QCOMPARE( server.requests.count(), 1 );
    QCOMPARE( server.requests.first().path(), QString( "/api/v1/public/libraries" ) );
    QUrlQuery query( server.requests.first() );
    QCOMPARE( query.queryItemValue( "_per_page" ), QString( "-1" ) );
    QCOMPARE( query.queryItemValue( "_order_by" ), QString( "name" ) );
    QVERIFY( ! server.requestHeaders.first().contains( "authorization" ) );
}

void TestKohaLibraries::notFoundHint()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    // A wrong URL or a Koha with RESTPublicAPI disabled answers 404
    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        *status = 404;
        *body = "{}";
    };

    KohaLibraries libraries;
    QSignalSpy finishedSpy( &libraries, SIGNAL( finished( bool, QString ) ) );

    libraries.start( server.baseUrl() );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), false );
    QVERIFY( finishedSpy.first().at( 1 ).toString().contains( "RESTPublicAPI" ) );
    QVERIFY( libraries.libraries().isEmpty() );
}

void TestKohaLibraries::badJson()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    // A URL that isn't a Koha API typically returns an HTML page
    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "<html>login page</html>";
    };

    KohaLibraries libraries;
    QSignalSpy finishedSpy( &libraries, SIGNAL( finished( bool, QString ) ) );

    libraries.start( server.baseUrl() );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), false );
    QVERIFY( finishedSpy.first().at( 1 ).toString().contains( "Koha staff URL" ) );
}

QTEST_GUILESS_MAIN(TestKohaLibraries)

#include "tst_kohalibraries.moc"
