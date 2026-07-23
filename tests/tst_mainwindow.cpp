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
#include <QtWidgets>
#include <QTemporaryDir>

#include "mainwindow.h"

/* Widget level tests that push transactions through the real history
 * table and the real save and load paths, the part the KocFile unit
 * tests can't see. */
class TestMainWindow : public QObject
{
    Q_OBJECT

    private slots:
        void saveLoadRoundTrip();

    private:
        void addRow( MainWindow & window, const QString & type, const QString & cardnumber,
                     const QString & barcode, const QString & payment, const QString & date );
};

void TestMainWindow::addRow( MainWindow & window, const QString & type, const QString & cardnumber,
                             const QString & barcode, const QString & payment, const QString & date )
{
    int row = window.tableWidgetHistory->rowCount();
    window.tableWidgetHistory->insertRow( row );
    window.tableWidgetHistory->setItem( row, MainWindow::COLUMN_TYPE, new QTableWidgetItem( type ) );
    window.tableWidgetHistory->setItem( row, MainWindow::COLUMN_DATE, new QTableWidgetItem( date ) );
    if ( ! cardnumber.isEmpty() )
        window.tableWidgetHistory->setItem( row, MainWindow::COLUMN_CARDNUMBER, new QTableWidgetItem( cardnumber ) );
    if ( ! barcode.isEmpty() )
        window.tableWidgetHistory->setItem( row, MainWindow::COLUMN_BARCODE, new QTableWidgetItem( barcode ) );
    if ( ! payment.isEmpty() )
        window.tableWidgetHistory->setItem( row, MainWindow::COLUMN_PAYMENT, new QTableWidgetItem( payment ) );
}

void TestMainWindow::saveLoadRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    QString path = dir.filePath( "roundtrip.koc" );

    MainWindow saver;
    addRow( saver, "issue", "23529001000463", "31000000123456", "", "2026-07-23 10-15-30 000" );
    addRow( saver, "return", "", "31000000654321", "", "2026-07-23 10-16-00 000" );
    addRow( saver, "payment", "23529001000463", "", "5.00", "2026-07-23 10-17-00 000" );

    saver.saveFile( path );

    // The written file is the wire format Koha parses
    QFile file( path );
    QVERIFY( file.open( QIODevice::ReadOnly | QIODevice::Text ) );
    QStringList lines = QString::fromUtf8( file.readAll() ).split( "\n", Qt::SkipEmptyParts );
    QCOMPARE( lines.count(), 4 );
    QVERIFY( lines.at( 0 ).startsWith( "Version=1.0\tGenerator=kocDesktop\tGeneratorVersion=" ) );
    QCOMPARE( lines.at( 1 ), QString( "2026-07-23 10-15-30 000\tissue\t23529001000463\t31000000123456" ) );
    QCOMPARE( lines.at( 2 ), QString( "2026-07-23 10-16-00 000\treturn\t31000000654321" ) );
    QCOMPARE( lines.at( 3 ), QString( "2026-07-23 10-17-00 000\tpayment\t23529001000463\t5.00" ) );

    // And loading it back rebuilds the same table
    MainWindow loader;
    loader.loadFile( path );

    QCOMPARE( loader.tableWidgetHistory->rowCount(), 3 );
    QCOMPARE( loader.tableWidgetHistory->item( 0, MainWindow::COLUMN_TYPE )->text(), QString( "issue" ) );
    QCOMPARE( loader.tableWidgetHistory->item( 0, MainWindow::COLUMN_CARDNUMBER )->text(), QString( "23529001000463" ) );
    QCOMPARE( loader.tableWidgetHistory->item( 0, MainWindow::COLUMN_BARCODE )->text(), QString( "31000000123456" ) );
    QCOMPARE( loader.tableWidgetHistory->item( 1, MainWindow::COLUMN_TYPE )->text(), QString( "return" ) );
    QCOMPARE( loader.tableWidgetHistory->item( 1, MainWindow::COLUMN_BARCODE )->text(), QString( "31000000654321" ) );
    QCOMPARE( loader.tableWidgetHistory->item( 2, MainWindow::COLUMN_TYPE )->text(), QString( "payment" ) );
    QCOMPARE( loader.tableWidgetHistory->item( 2, MainWindow::COLUMN_PAYMENT )->text(), QString( "5.00" ) );

    // Loaded rows have no upload status yet
    QVERIFY( ! loader.tableWidgetHistory->item( 0, MainWindow::COLUMN_STATUS ) );
}

int main( int argc, char *argv[] )
{
    // No window system needed, and keep the test's QSettings away from
    // the real application's
    if ( qEnvironmentVariable( "QT_QPA_PLATFORM" ).isEmpty() ) {
        qputenv( "QT_QPA_PLATFORM", "offscreen" );
    }

    QApplication app( argc, argv );
    QCoreApplication::setOrganizationName( "Koha Offline Circulation Tests" );
    QCoreApplication::setApplicationName( "tst_mainwindow" );

    TestMainWindow test;
    return QTest::qExec( &test, argc, argv );
}

#include "tst_mainwindow.moc"
