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

#include "kocfile.h"

/* The expected strings in these tests are the .koc format as written
 * by the 1.x releases and accepted by Koha's process_koc.pl. If a
 * change breaks one of these tests, it breaks uploads to Koha. */
class TestKocFile : public QObject
{
    Q_OBJECT

    private slots:
        void headerLine();
        void serializeIssue();
        void serializeReturn();
        void serializePayment();
        void parseIssue();
        void parseReturn();
        void parsePayment();
        void parseShortLine();
        void roundTrip();
        void formatPayment();
};

void TestKocFile::headerLine()
{
    QCOMPARE( KocFile::headerLine( "1.0", "2.0.0" ),
              QString("Version=1.0\tGenerator=kocDesktop\tGeneratorVersion=2.0.0") );
}

void TestKocFile::serializeIssue()
{
    KocTransaction transaction;
    transaction.date = "2026-07-22 10-15-30 000";
    transaction.type = "issue";
    transaction.cardnumber = "23529001000463";
    transaction.barcode = "31000000123456";

    QCOMPARE( KocFile::serializeLine( transaction ),
              QString("2026-07-22 10-15-30 000\tissue\t23529001000463\t31000000123456") );
}

void TestKocFile::serializeReturn()
{
    KocTransaction transaction;
    transaction.date = "2026-07-22 10-16-02 000";
    transaction.type = "return";
    transaction.barcode = "31000000654321";

    QCOMPARE( KocFile::serializeLine( transaction ),
              QString("2026-07-22 10-16-02 000\treturn\t31000000654321") );
}

void TestKocFile::serializePayment()
{
    KocTransaction transaction;
    transaction.date = "2026-07-22 10-17-45 000";
    transaction.type = "payment";
    transaction.cardnumber = "23529001000463";
    transaction.payment = "5.00";

    QCOMPARE( KocFile::serializeLine( transaction ),
              QString("2026-07-22 10-17-45 000\tpayment\t23529001000463\t5.00") );
}

void TestKocFile::parseIssue()
{
    KocTransaction transaction =
        KocFile::parseLine( "2026-07-22 10-15-30 000\tissue\t23529001000463\t31000000123456" );

    QCOMPARE( transaction.date, QString("2026-07-22 10-15-30 000") );
    QCOMPARE( transaction.type, QString("issue") );
    QCOMPARE( transaction.cardnumber, QString("23529001000463") );
    QCOMPARE( transaction.barcode, QString("31000000123456") );
    QCOMPARE( transaction.payment, QString() );
}

void TestKocFile::parseReturn()
{
    KocTransaction transaction =
        KocFile::parseLine( "2026-07-22 10-16-02 000\treturn\t31000000654321" );

    QCOMPARE( transaction.date, QString("2026-07-22 10-16-02 000") );
    QCOMPARE( transaction.type, QString("return") );
    QCOMPARE( transaction.barcode, QString("31000000654321") );
    QCOMPARE( transaction.cardnumber, QString() );
    QCOMPARE( transaction.payment, QString() );
}

void TestKocFile::parsePayment()
{
    KocTransaction transaction =
        KocFile::parseLine( "2026-07-22 10-17-45 000\tpayment\t23529001000463\t5.00" );

    QCOMPARE( transaction.date, QString("2026-07-22 10-17-45 000") );
    QCOMPARE( transaction.type, QString("payment") );
    QCOMPARE( transaction.cardnumber, QString("23529001000463") );
    QCOMPARE( transaction.payment, QString("5.00") );
    QCOMPARE( transaction.barcode, QString() );
}

void TestKocFile::parseShortLine()
{
    // A truncated line shouldn't crash, missing fields stay empty
    KocTransaction transaction = KocFile::parseLine( "2026-07-22 10-15-30 000\tissue" );

    QCOMPARE( transaction.date, QString("2026-07-22 10-15-30 000") );
    QCOMPARE( transaction.type, QString("issue") );
    QCOMPARE( transaction.cardnumber, QString() );
    QCOMPARE( transaction.barcode, QString() );
}

void TestKocFile::roundTrip()
{
    QStringList lines = {
        "2026-07-22 10-15-30 000\tissue\t23529001000463\t31000000123456",
        "2026-07-22 10-16-02 000\treturn\t31000000654321",
        "2026-07-22 10-17-45 000\tpayment\t23529001000463\t5.00",
    };

    for ( const QString &line : lines ) {
        QCOMPARE( KocFile::serializeLine( KocFile::parseLine( line ) ), line );
    }
}

void TestKocFile::formatPayment()
{
    QCOMPARE( KocFile::formatPayment( 5.0 ), QString("5.00") );
    QCOMPARE( KocFile::formatPayment( 2.5 ), QString("2.50") );
    QCOMPARE( KocFile::formatPayment( 0.1 ), QString("0.10") );
    QCOMPARE( KocFile::formatPayment( 3.999 ), QString("4.00") );
    QCOMPARE( KocFile::formatPayment( 0.0 ), QString("0.00") );

    // No grouping separators, Koha parses the amount as a number
    QCOMPARE( KocFile::formatPayment( 1234.56 ), QString("1234.56") );
}

QTEST_APPLESS_MAIN(TestKocFile)

#include "tst_kocfile.moc"
