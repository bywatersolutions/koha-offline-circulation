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
#include <QUrlQuery>

#include "kohaupload.h"
#include "mockhttpserver.h"

class TestKohaUpload : public QObject
{
    Q_OBJECT

    private slots:
        void isoTimestamp();
        void uploadsEachTransaction();
        void rejectedTransactionContinues();
        void transportFailureStops();
        void pendingMode();
        void pluginBatch();
};

void TestKohaUpload::isoTimestamp()
{
    // The conversion is local time to UTC, so compare instants rather
    // than strings to stay independent of the machine's timezone
    QString iso = KohaUpload::isoTimestamp( "2026-07-22 10-15-30 000" );
    QDateTime roundTrip = QDateTime::fromString( iso, Qt::ISODate );
    QCOMPARE( roundTrip, QDateTime::fromString( "2026-07-22 10-15-30 000", "yyyy-MM-dd hh-mm-ss zzz" ) );

    // Unparseable input passes through untouched
    QCOMPARE( KohaUpload::isoTimestamp( "garbage" ), QString( "garbage" ) );
}

void TestKohaUpload::uploadsEachTransaction()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "Success.";
    };

    KocTransaction issue;
    issue.type = "issue";
    issue.date = "2026-07-22 10-15-30 000";
    issue.cardnumber = "CARD1";
    issue.barcode = "BC1";

    KocTransaction returnTransaction;
    returnTransaction.type = "return";
    returnTransaction.date = "2026-07-22 10-16-00 000";
    returnTransaction.barcode = "BC2";

    KocTransaction payment;
    payment.type = "payment";
    payment.date = "2026-07-22 10-17-00 000";
    payment.cardnumber = "CARD1";
    payment.payment = "5.00";

    KohaUpload upload;
    QSignalSpy resultSpy( &upload, SIGNAL( transactionResult( int, bool, QString ) ) );
    QSignalSpy finishedSpy( &upload, SIGNAL( finished( int, int ) ) );

    KohaUpload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.branchcode = "CPL";

    upload.start( config, { issue, returnTransaction, payment } );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toInt(), 3 );
    QCOMPARE( finishedSpy.first().at( 1 ).toInt(), 0 );
    QCOMPARE( resultSpy.count(), 3 );

    QCOMPARE( server.requests.count(), 3 );

    QUrlQuery first( server.requests.at( 0 ) );
    QCOMPARE( server.requests.at( 0 ).path(), QString( "/cgi-bin/koha/offline_circ/service.pl" ) );
    QCOMPARE( first.queryItemValue( "action" ), QString( "issue" ) );
    QCOMPARE( first.queryItemValue( "cardnumber" ), QString( "CARD1" ) );
    QCOMPARE( first.queryItemValue( "barcode" ), QString( "BC1" ) );
    QCOMPARE( first.queryItemValue( "branchcode" ), QString( "CPL" ) );
    QCOMPARE( first.queryItemValue( "pending" ), QString( "false" ) );
    QCOMPARE( first.queryItemValue( "login_userid" ), QString( "koha" ) );
    QVERIFY( ! first.queryItemValue( "timestamp" ).isEmpty() );

    QUrlQuery second( server.requests.at( 1 ) );
    QCOMPARE( second.queryItemValue( "action" ), QString( "return" ) );
    QCOMPARE( second.queryItemValue( "barcode" ), QString( "BC2" ) );
    QVERIFY( ! second.hasQueryItem( "cardnumber" ) );

    QUrlQuery third( server.requests.at( 2 ) );
    QCOMPARE( third.queryItemValue( "action" ), QString( "payment" ) );
    QCOMPARE( third.queryItemValue( "amount" ), QString( "5.00" ) );
}

void TestKohaUpload::rejectedTransactionContinues()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    // The second transaction is rejected by Koha, the rest still upload
    server.handler = [&server]( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = server.requests.count() == 2 ? "Borrower not found." : "Success.";
    };

    KocTransaction transaction;
    transaction.type = "return";
    transaction.date = "2026-07-22 10-16-00 000";
    transaction.barcode = "BC";

    KohaUpload upload;
    QSignalSpy resultSpy( &upload, SIGNAL( transactionResult( int, bool, QString ) ) );
    QSignalSpy finishedSpy( &upload, SIGNAL( finished( int, int ) ) );

    KohaUpload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.branchcode = "CPL";

    upload.start( config, { transaction, transaction, transaction } );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toInt(), 2 );
    QCOMPARE( finishedSpy.first().at( 1 ).toInt(), 1 );

    QCOMPARE( resultSpy.at( 1 ).at( 1 ).toBool(), false );
    QCOMPARE( resultSpy.at( 1 ).at( 2 ).toString(), QString( "Borrower not found." ) );
}

void TestKohaUpload::transportFailureStops()
{
    // Point at a port nothing listens on
    MockHttpServer server;
    QVERIFY( server.listen() );
    QString deadUrl = server.baseUrl();
    // The server object stays alive but we use an unlikely port instead
    deadUrl = "http://127.0.0.1:1";

    KocTransaction transaction;
    transaction.type = "return";
    transaction.date = "2026-07-22 10-16-00 000";
    transaction.barcode = "BC";

    KohaUpload upload;
    QSignalSpy finishedSpy( &upload, SIGNAL( finished( int, int ) ) );

    KohaUpload::Config config;
    config.baseUrl = deadUrl;
    config.userid = "koha";
    config.password = "koha";
    config.branchcode = "CPL";

    upload.start( config, { transaction, transaction, transaction } );
    QVERIFY( finishedSpy.wait( 30000 ) );

    // One failure, the remaining transactions are never attempted
    QCOMPARE( finishedSpy.first().at( 0 ).toInt(), 0 );
    QCOMPARE( finishedSpy.first().at( 1 ).toInt(), 1 );
}

void TestKohaUpload::pendingMode()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "Added.";
    };

    KocTransaction transaction;
    transaction.type = "return";
    transaction.date = "2026-07-22 10-16-00 000";
    transaction.barcode = "BC";

    KohaUpload upload;
    QSignalSpy finishedSpy( &upload, SIGNAL( finished( int, int ) ) );

    KohaUpload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.branchcode = "CPL";
    config.pending = true;

    upload.start( config, { transaction } );
    QVERIFY( finishedSpy.wait( 10000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toInt(), 1 );
    QCOMPARE( finishedSpy.first().at( 1 ).toInt(), 0 );

    QUrlQuery query( server.requests.first() );
    QCOMPARE( query.queryItemValue( "pending" ), QString( "true" ) );
}

void TestKohaUpload::pluginBatch()
{
    MockHttpServer server;
    QVERIFY( server.listen() );

    server.handler = []( const QUrl & url, int * status, QByteArray * body ) {
        Q_UNUSED( url )
        Q_UNUSED( status )
        *body = "{\"results\":["
                "{\"status\":\"queued\",\"message\":\"Added to the pending offline circulation queue\"},"
                "{\"status\":\"skipped\",\"message\":\"Already processed: queued\"},"
                "{\"status\":\"error\",\"message\":\"Borrower not found.\"}"
                "]}";
    };

    KocTransaction transaction;
    transaction.type = "return";
    transaction.date = "2026-07-24 10-16-00 000";
    transaction.barcode = "BC";

    KohaUpload upload;
    QSignalSpy resultSpy( &upload, SIGNAL( transactionResult( int, bool, QString ) ) );
    QSignalSpy finishedSpy( &upload, SIGNAL( finished( int, int ) ) );

    KohaUpload::Config config;
    config.baseUrl = server.baseUrl();
    config.userid = "koha";
    config.password = "koha";
    config.branchcode = "CPL";
    config.usePlugin = true;

    upload.start( config, { transaction, transaction, transaction } );
    QVERIFY( finishedSpy.wait( 10000 ) );

    // Queued and skipped both count as sent, the error doesn't
    QCOMPARE( finishedSpy.first().at( 0 ).toInt(), 2 );
    QCOMPARE( finishedSpy.first().at( 1 ).toInt(), 1 );
    QCOMPARE( resultSpy.count(), 3 );
    QCOMPARE( resultSpy.at( 1 ).at( 1 ).toBool(), true );
    QCOMPARE( resultSpy.at( 2 ).at( 1 ).toBool(), false );

    // One request carried the whole batch
    QCOMPARE( server.requests.count(), 1 );
    QCOMPARE( server.requests.first().path(), QString( "/api/v1/contrib/offlinecirc/transactions" ) );

    QJsonObject body = QJsonDocument::fromJson( server.requestBodies.first() ).object();
    QCOMPARE( body.value( "branchcode" ).toString(), QString( "CPL" ) );
    QCOMPARE( body.value( "pending" ).toBool(), false );
    QCOMPARE( body.value( "transactions" ).toArray().count(), 3 );
}

QTEST_GUILESS_MAIN(TestKohaUpload)

#include "tst_kohaupload.moc"
