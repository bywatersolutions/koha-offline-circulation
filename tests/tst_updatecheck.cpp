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

#include "updatecheck.h"
#include "mockhttpserver.h"

class TestUpdateCheck : public QObject
{
    Q_OBJECT

    private slots:
        void isNewer();
        void updateAvailable();
        void upToDate();
        void serverError();
};

void TestUpdateCheck::isNewer()
{
    QVERIFY( UpdateCheck::isNewer( "v2.1.0", "2.0.0" ) );
    QVERIFY( UpdateCheck::isNewer( "2.1.0", "2.0.0" ) );
    QVERIFY( UpdateCheck::isNewer( "v10.0.0", "9.9.9" ) );
    QVERIFY( ! UpdateCheck::isNewer( "v2.1.0", "2.1.0" ) );
    QVERIFY( ! UpdateCheck::isNewer( "v2.0.0", "2.1.0" ) );
    QVERIFY( ! UpdateCheck::isNewer( "v2.0.0", "v2.1.0" ) );
}

void TestUpdateCheck::updateAvailable()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "{\"tag_name\":\"v9.9.9\",\"html_url\":\"https://example.org/releases\"}";
    };

    UpdateCheck check;
    check.apiUrl = server.baseUrl() + "/releases/latest";
    QSignalSpy finishedSpy( &check, SIGNAL( finished( bool, bool, QString, QString ) ) );

    check.start( "2.1.0" );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), true );
    QCOMPARE( finishedSpy.first().at( 1 ).toBool(), true );
    QCOMPARE( finishedSpy.first().at( 2 ).toString(), QString( "v9.9.9" ) );
    QCOMPARE( finishedSpy.first().at( 3 ).toString(), QString( "https://example.org/releases" ) );

    // GitHub's API rejects requests without a user agent
    QVERIFY( ! server.requestHeaders.first().value( "user-agent" ).isEmpty() );
}

void TestUpdateCheck::upToDate()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "{\"tag_name\":\"v2.1.0\",\"html_url\":\"https://example.org/releases\"}";
    };

    UpdateCheck check;
    check.apiUrl = server.baseUrl() + "/releases/latest";
    QSignalSpy finishedSpy( &check, SIGNAL( finished( bool, bool, QString, QString ) ) );

    check.start( "2.1.0" );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), true );
    QCOMPARE( finishedSpy.first().at( 1 ).toBool(), false );
}

void TestUpdateCheck::serverError()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        *status = 404;
        *body = "{}";
    };

    UpdateCheck check;
    check.apiUrl = server.baseUrl() + "/releases/latest";
    QSignalSpy finishedSpy( &check, SIGNAL( finished( bool, bool, QString, QString ) ) );

    check.start( "2.1.0" );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toBool(), false );
}

QTEST_GUILESS_MAIN(TestUpdateCheck)

#include "tst_updatecheck.moc"
