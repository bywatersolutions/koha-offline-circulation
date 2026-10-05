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
#include <QtSql>
#include <QTemporaryDir>

#include "borrowersdb.h"
#include "mainwindow.h"

static QMessageBox *visibleMessageBox()
{
    for ( QWidget *widget : QApplication::topLevelWidgets() ) {
        QMessageBox *box = qobject_cast<QMessageBox *>( widget );
        if ( box && box->isVisible() ) return box;
    }

    return nullptr;
}

/* Clicks the chosen button on any message box that appears, and counts
 * the ones it answered. A modal box runs its own event loop, so a timer
 * is the only way to reach it, and stopping the timer on destruction
 * keeps a test that expected no box from arming one for the next test. */
class MessageBoxAnswer
{
    public:
        explicit MessageBoxAnswer( QMessageBox::StandardButton answer )
            : mAnswer( answer )
        {
            QObject::connect( &mTimer, &QTimer::timeout, [this]() { answerIfShown(); } );
            mTimer.start( 0 );
        }

        ~MessageBoxAnswer() { mTimer.stop(); }

        int answered() const { return mAnswered; }
        QString text() const { return mText; }

    private:
        void answerIfShown()
        {
            QMessageBox *box = visibleMessageBox();
            if ( ! box ) return;

            mAnswered++;
            mText = box->text();

            if ( QAbstractButton *button = box->button( mAnswer ) ) button->click();
        }

        QMessageBox::StandardButton mAnswer;
        int mAnswered = 0;
        QString mText;
        QTimer mTimer;
};

/* Widget level tests that push transactions through the real history
 * table and the real save and load paths, the part the KocFile unit
 * tests can't see. */
class TestMainWindow : public QObject
{
    Q_OBJECT

    private slots:
        void saveLoadRoundTrip();
        void patronNameFromBorrowersDb();
        void earlierSessionsGroupedNewestFirst();
        void theWindowFitsASmallScreen();
        void closingWithNothingPendingDoesNotAsk();
        void closingWithPendingBarcodesCanBeRefused();
        void closingWithPendingBarcodesCanBeConfirmed();
        void closingTheFileAsksAboutPendingBarcodesEvenAfterAnUpload();

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

void TestMainWindow::patronNameFromBorrowersDb()
{
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );

    // A borrowers database with a single patron in it
    QString dbPath = dir.filePath( "borrowers.db" );
    KohaPatron patron;
    patron.borrowernumber = "1";
    patron.cardnumber = "23529001000463";
    patron.firstname = "Edna";
    patron.surname = "Acosta";
    QString error;
    QVERIFY2( BorrowersDb::write( dbPath, { patron }, {}, &error ), qPrintable( error ) );

    QString path = dir.filePath( "names.koc" );
    MainWindow saver;
    addRow( saver, "issue", "23529001000463", "31000000123456", "", "2026-09-03 10-15-30 000" );
    addRow( saver, "return", "", "31000000654321", "", "2026-09-03 10-16-00 000" );
    addRow( saver, "issue", "NOSUCHCARD", "31000000111111", "", "2026-09-03 10-17-00 000" );
    saver.saveFile( path );

    // The file doesn't carry names, they come from the database on load
    MainWindow loader;
    loader.borrowersDbFilePath = dbPath;
    loader.loadFile( path );

    QCOMPARE( loader.tableWidgetHistory->item( 0, MainWindow::COLUMN_NAME )->text(), QString( "Edna Acosta" ) );
    QVERIFY( ! loader.tableWidgetHistory->item( 1, MainWindow::COLUMN_NAME ) );
    QCOMPARE( loader.tableWidgetHistory->item( 2, MainWindow::COLUMN_NAME )->text(), QString() );

    // Let go of the file so the temporary directory can be removed
    QSqlDatabase::database( QSqlDatabase::defaultConnection, false ).close();
}

void TestMainWindow::earlierSessionsGroupedNewestFirst()
{
    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    QString path = dir.filePath( "sessions.koc" );

    MainWindow saver;
    addRow( saver, "issue", "CARDA", "31000000000001", "", "2026-09-03 11-58-02 000" );
    addRow( saver, "issue", "CARDA", "31000000000002", "", "2026-09-03 11-58-02 000" );
    addRow( saver, "return", "", "31000000000003", "", "2026-09-03 12-00-00 000" );
    addRow( saver, "payment", "CARDA", "", "5.00", "2026-09-03 12-05-00 000" );
    // Stamped a few milliseconds apart, the way 2.4.0 and earlier wrote a batch
    addRow( saver, "issue", "CARDB", "31000000000004", "", "2026-09-03 12-10-30 000" );
    addRow( saver, "issue", "CARDB", "31000000000005", "", "2026-09-03 12-10-30 004" );
    addRow( saver, "issue", "CARDB", "31000000000006", "", "2026-09-03 12-10-30 009" );
    saver.saveFile( path );

    MainWindow loader;
    loader.loadFile( path );

    // Newest batch first, opened; the older one closed
    QTreeWidget *issues = loader.treeWidgetIssuesSessions;
    QCOMPARE( issues->topLevelItemCount(), 2 );
    QCOMPARE( issues->topLevelItem( 0 )->text( 0 ), QString( "2026-09-03 12:10:30" ) );
    QCOMPARE( issues->topLevelItem( 0 )->text( 1 ), QString( "CARDB" ) );
    QCOMPARE( issues->topLevelItem( 0 )->text( 3 ), QString( "3 items" ) );
    QCOMPARE( issues->topLevelItem( 0 )->childCount(), 3 );
    QCOMPARE( issues->topLevelItem( 0 )->child( 2 )->text( 0 ), QString( "31000000000006" ) );
    QVERIFY( issues->topLevelItem( 0 )->isExpanded() );
    QCOMPARE( issues->topLevelItem( 1 )->text( 1 ), QString( "CARDA" ) );
    QCOMPARE( issues->topLevelItem( 1 )->childCount(), 2 );
    QVERIFY( ! issues->topLevelItem( 1 )->isExpanded() );

    // Returns have their own list, the payment appears in neither
    QTreeWidget *returns = loader.treeWidgetReturnsSessions;
    QCOMPARE( returns->topLevelItemCount(), 1 );
    QCOMPARE( returns->topLevelItem( 0 )->text( 0 ), QString( "2026-09-03 12:00:00" ) );
    QCOMPARE( returns->topLevelItem( 0 )->childCount(), 1 );

    // Closing the file clears both lists
    loader.closeFile( true );
    QCOMPARE( issues->topLevelItemCount(), 0 );
    QCOMPARE( returns->topLevelItemCount(), 0 );
}

void TestMainWindow::theWindowFitsASmallScreen()
{
    MainWindow window;

    // A 300 pixel floor here used to set the minimum height of the whole
    // window, leaving the buttons off the bottom of a small screen
    QCOMPARE( window.borrowerDetailsCurrentIssues->minimumHeight(), 0 );

    // The borrower details are six stacked group boxes and will always want
    // more height than a small screen has, so they scroll rather than hold
    // the window open
    QScrollArea *area = window.findChild<QScrollArea *>( "scrollAreaBorrowerDetails" );
    QVERIFY2( area, "the borrower details column has to stay inside a scroll area" );
    QVERIFY( area->isAncestorOf( window.groupBoxBorrowerDetails ) );

    // A maximized window on a 1280x720 screen gets about 649 pixels once the
    // taskbar and the caption are gone. This measures 619, and the bound is
    // loose because font metrics differ between the three CI runners.
    int minimum = window.minimumSizeHint().height();
    QVERIFY2( minimum < 700, qPrintable( QString("window minimum height is %1").arg( minimum ) ) );
}

void TestMainWindow::closingWithNothingPendingDoesNotAsk()
{
    MainWindow window;
    window.show();

    // Set to answer No, so an unexpected prompt can't slip past
    MessageBoxAnswer answer( QMessageBox::No );

    QVERIFY( window.close() );
    QCOMPARE( answer.answered(), 0 );
}

void TestMainWindow::closingWithPendingBarcodesCanBeRefused()
{
    MainWindow window;
    window.show();
    window.listWidgetIssuesScannedBarcodes->addItem( "31000000000001" );
    window.listWidgetReturnsScannedBarcodes->addItem( "31000000000002" );

    MessageBoxAnswer answer( QMessageBox::No );

    QVERIFY( ! window.close() );
    QCOMPARE( answer.answered(), 1 );

    // Both tabs count toward the total, not just the one in front
    QVERIFY2( answer.text().contains( "2" ), qPrintable( answer.text() ) );

    // Refusing leaves the window and the barcodes exactly as they were
    QVERIFY( window.isVisible() );
    QCOMPARE( window.listWidgetIssuesScannedBarcodes->count(), 1 );
    QCOMPARE( window.listWidgetReturnsScannedBarcodes->count(), 1 );
}

void TestMainWindow::closingWithPendingBarcodesCanBeConfirmed()
{
    MainWindow window;
    window.show();
    window.listWidgetReturnsScannedBarcodes->addItem( "31000000000003" );

    MessageBoxAnswer answer( QMessageBox::Yes );

    QVERIFY( window.close() );
    QCOMPARE( answer.answered(), 1 );
}

void TestMainWindow::closingTheFileAsksAboutPendingBarcodesEvenAfterAnUpload()
{
    // A finished upload closes the file with the confirmation skipped,
    // because the transactions it uploaded are already on disk. Barcodes
    // scanned since then are not, so they still get a question
    MainWindow window;
    addRow( window, "issue", "CARDA", "31000000000001", "", "2026-09-03 11-58-02 000" );
    window.listWidgetReturnsScannedBarcodes->addItem( "31000000000009" );

    MessageBoxAnswer answer( QMessageBox::No );

    QVERIFY( ! window.closeFile( true ) );
    QCOMPARE( answer.answered(), 1 );
    QCOMPARE( window.tableWidgetHistory->rowCount(), 1 );
    QCOMPARE( window.listWidgetReturnsScannedBarcodes->count(), 1 );

    // With nothing scanned it stays silent, the way the upload flow needs
    window.listWidgetReturnsScannedBarcodes->clear();
    MessageBoxAnswer again( QMessageBox::No );

    QVERIFY( window.closeFile( true ) );
    QCOMPARE( again.answered(), 0 );
    QCOMPARE( window.tableWidgetHistory->rowCount(), 0 );
}

int main( int argc, char *argv[] )
{
    // No window system needed, and keep the test's QSettings away from
    // the real application's
    if ( qEnvironmentVariable( "QT_QPA_PLATFORM" ).isEmpty() ) {
        qputenv( "QT_QPA_PLATFORM", "offscreen" );
    }

    QApplication app( argc, argv );

    // Closing a window in a test must not take the whole application
    // down with it, the close tests do exactly that
    app.setQuitOnLastWindowClosed( false );

    QCoreApplication::setOrganizationName( "Koha Offline Circulation Tests" );
    QCoreApplication::setApplicationName( "tst_mainwindow" );

    // The close tests reach writeSettings(), keep the window geometry it
    // saves in a scratch directory so runs can't affect one another
    QTemporaryDir settingsDir;
    QSettings::setDefaultFormat( QSettings::IniFormat );
    QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, settingsDir.path() );

    TestMainWindow test;
    return QTest::qExec( &test, argc, argv );
}

#include "tst_mainwindow.moc"
