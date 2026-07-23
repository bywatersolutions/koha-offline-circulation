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

#include "borrowersdb.h"

class TestBorrowersDb : public QObject
{
    Q_OBJECT

    private slots:
        void patronFromApi();
        void checkoutFromApi();
        void patronFromReportRow();
        void checkoutFromReportRow();
        void write();
        void merge();
};

void TestBorrowersDb::patronFromApi()
{
    QJsonObject patron = QJsonDocument::fromJson( R"({
        "patron_id": 42,
        "cardnumber": "23529001000463",
        "surname": "O'Brien",
        "firstname": "Mary",
        "address": "123 Main St",
        "city": "Greenville",
        "phone": null,
        "date_of_birth": "1990-01-31",
        "account_balance": 5.5
    })" ).object();

    KohaPatron result = BorrowersDb::patronFromApi( patron );

    QCOMPARE( result.borrowernumber, QString("42") );
    QCOMPARE( result.cardnumber, QString("23529001000463") );
    QCOMPARE( result.surname, QString("O'Brien") );
    QCOMPARE( result.firstname, QString("Mary") );
    QCOMPARE( result.address, QString("123 Main St") );
    QCOMPARE( result.city, QString("Greenville") );
    QCOMPARE( result.phone, QString() );
    QCOMPARE( result.dateofbirth, QString("1990-01-31") );
    QCOMPARE( result.total_fines, 5.5 );
}

void TestBorrowersDb::checkoutFromApi()
{
    QJsonObject checkout = QJsonDocument::fromJson( R"({
        "patron_id": 42,
        "due_date": "2026-07-30T23:59:00",
        "item": {
            "callnumber": "FIC ROW",
            "item_type": "BK",
            "biblio": {
                "title": "A Book Title"
            }
        }
    })" ).object();

    KohaCheckout result = BorrowersDb::checkoutFromApi( checkout );

    QCOMPARE( result.borrowernumber, QString("42") );
    QCOMPARE( result.date_due, QString("2026-07-30 23:59:00") );
    QCOMPARE( result.itemcallnumber, QString("FIC ROW") );
    QCOMPARE( result.itemtype, QString("BK") );
    QCOMPARE( result.title, QString("A Book Title") );
}

void TestBorrowersDb::patronFromReportRow()
{
    // Report values arrive as strings or numbers depending on the column
    QJsonArray row = QJsonDocument::fromJson(
        R"([42, "23529001000463", "O'Brien", "Mary", "123 Main St", "Greenville", null, "1990-01-31", "5.5"])" )
        .array();

    KohaPatron result = BorrowersDb::patronFromReportRow( row );

    QCOMPARE( result.borrowernumber, QString("42") );
    QCOMPARE( result.cardnumber, QString("23529001000463") );
    QCOMPARE( result.surname, QString("O'Brien") );
    QCOMPARE( result.phone, QString() );
    QCOMPARE( result.total_fines, 5.5 );
}

void TestBorrowersDb::checkoutFromReportRow()
{
    QJsonArray row = QJsonDocument::fromJson(
        R"([42, "2026-07-30 23:59:00", "FIC ROW", "A Book Title", "BK"])" ).array();

    KohaCheckout result = BorrowersDb::checkoutFromReportRow( row );

    QCOMPARE( result.borrowernumber, QString("42") );
    QCOMPARE( result.date_due, QString("2026-07-30 23:59:00") );
    QCOMPARE( result.itemcallnumber, QString("FIC ROW") );
    QCOMPARE( result.title, QString("A Book Title") );
    QCOMPARE( result.itemtype, QString("BK") );
}

void TestBorrowersDb::write()
{
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    QString path = dir.filePath( "borrowers.db" );

    KohaPatron patron;
    patron.borrowernumber = "42";
    patron.cardnumber = "23529001000463";
    patron.surname = "O'Brien";
    patron.firstname = "Mary";
    patron.total_fines = 5.5;

    KohaCheckout checkout;
    checkout.borrowernumber = "42";
    checkout.date_due = "2026-07-30 23:59:00";
    checkout.title = "A Book Title";

    QString error;
    QVERIFY2( BorrowersDb::write( path, { patron }, { checkout }, &error ), qPrintable( error ) );
    QVERIFY( QFile::exists( path ) );
    QVERIFY( ! QFile::exists( path + ".tmp" ) );

    // Read it back the same way the app does
    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "borrowersdb_test" );
        db.setDatabaseName( path );
        QVERIFY( db.open() );

        QSqlQuery query( db );
        QVERIFY( query.exec( "SELECT surname, total_fines FROM borrowers WHERE cardnumber = '23529001000463'" ) );
        QVERIFY( query.next() );
        QCOMPARE( query.value( 0 ).toString(), QString("O'Brien") );
        QCOMPARE( query.value( 1 ).toDouble(), 5.5 );

        QVERIFY( query.exec( "SELECT title FROM issues WHERE borrowernumber = 42" ) );
        QVERIFY( query.next() );
        QCOMPARE( query.value( 0 ).toString(), QString("A Book Title") );

        db.close();
    }
    QSqlDatabase::removeDatabase( "borrowersdb_test" );

    // Writing again replaces the file rather than appending to it
    QVERIFY2( BorrowersDb::write( path, { patron }, {}, &error ), qPrintable( error ) );
    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "borrowersdb_test" );
        db.setDatabaseName( path );
        QVERIFY( db.open() );

        QSqlQuery query( db );
        QVERIFY( query.exec( "SELECT COUNT(*) FROM issues" ) );
        QVERIFY( query.next() );
        QCOMPARE( query.value( 0 ).toInt(), 0 );

        db.close();
    }
    QSqlDatabase::removeDatabase( "borrowersdb_test" );
}

void TestBorrowersDb::merge()
{
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    QString path = dir.filePath( "borrowers.db" );

    KohaPatron patron1;
    patron1.borrowernumber = "1";
    patron1.cardnumber = "CARD1";
    patron1.surname = "Unchanged";

    KohaPatron patron2;
    patron2.borrowernumber = "2";
    patron2.cardnumber = "CARD2";
    patron2.surname = "Before";

    KohaCheckout oldCheckout;
    oldCheckout.borrowernumber = "1";
    oldCheckout.title = "Old Title";

    QString error;
    QVERIFY2( BorrowersDb::write( path, { patron1, patron2 }, { oldCheckout }, &error ), qPrintable( error ) );

    // Patron 2 changed, patron 3 is new, and the checkouts are replaced
    patron2.surname = "After";

    KohaPatron patron3;
    patron3.borrowernumber = "3";
    patron3.cardnumber = "CARD3";
    patron3.surname = "New";

    KohaCheckout newCheckout;
    newCheckout.borrowernumber = "3";
    newCheckout.title = "New Title";

    QVERIFY2( BorrowersDb::merge( path, { patron2, patron3 }, { newCheckout }, &error ), qPrintable( error ) );

    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "borrowersdb_test" );
        db.setDatabaseName( path );
        QVERIFY( db.open() );

        QSqlQuery query( db );
        QVERIFY( query.exec( "SELECT COUNT(*) FROM borrowers" ) );
        QVERIFY( query.next() );
        QCOMPARE( query.value( 0 ).toInt(), 3 );

        QVERIFY( query.exec( "SELECT surname FROM borrowers WHERE borrowernumber = 2" ) );
        QVERIFY( query.next() );
        QCOMPARE( query.value( 0 ).toString(), QString( "After" ) );

        QVERIFY( query.exec( "SELECT title FROM issues" ) );
        QVERIFY( query.next() );
        QCOMPARE( query.value( 0 ).toString(), QString( "New Title" ) );
        QVERIFY( ! query.next() );

        db.close();
    }
    QSqlDatabase::removeDatabase( "borrowersdb_test" );

    // Merging into a missing database fails rather than creating an
    // empty one
    QVERIFY( ! BorrowersDb::merge( dir.filePath( "missing.db" ), { patron3 }, {}, &error ) );
}

QTEST_GUILESS_MAIN(TestBorrowersDb)

#include "tst_borrowersdb.moc"
