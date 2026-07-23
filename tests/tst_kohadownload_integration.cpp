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
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "kohadownload.h"
#include "kohaupload.h"

/* Live integration test against a real Koha server, normally a
 * koha-testing-docker instance prepared by tests/integration/setup-ktd.sh.
 * Skips itself unless KOHA_URL is set, so it's harmless in the normal
 * test run. Environment:
 *
 *   KOHA_URL                  staff interface URL, e.g. http://127.0.0.1:8081
 *   KOHA_USER, KOHA_PASSWORD  credentials, default koha/koha
 *   KOHA_EXPECTED_PATRONS     optional exact borrower count to assert
 *   KOHA_BORROWERS_REPORT_ID  saved report IDs, without them the report
 *   KOHA_ISSUES_REPORT_ID     mode test skips */
class TestKohaDownloadIntegration : public QObject
{
    Q_OBJECT

    private slots:
        void restMode();
        void reportMode();
        void uploadPending();

    private:
        void runDownload( KohaDownload::Config config );
        int countRows( const QString & dbPath, const QString & table );
};

int TestKohaDownloadIntegration::countRows( const QString & dbPath, const QString & table )
{
    int count = -1;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "integration_test" );
        db.setDatabaseName( dbPath );
        if ( db.open() ) {
            QSqlQuery query( db );
            if ( query.exec( "SELECT COUNT(*) FROM " + table ) && query.next() ) {
                count = query.value( 0 ).toInt();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase( "integration_test" );

    return count;
}

void TestKohaDownloadIntegration::runDownload( KohaDownload::Config config )
{
    config.baseUrl = qEnvironmentVariable( "KOHA_URL" );
    config.userid = qEnvironmentVariable( "KOHA_USER", "koha" );
    config.password = qEnvironmentVariable( "KOHA_PASSWORD", "koha" );

    QTemporaryDir dir;
    QVERIFY( dir.isValid() );
    QString dbPath = dir.filePath( "borrowers.db" );

    KohaDownload download;
    QSignalSpy finishedSpy( &download, SIGNAL( finished( bool, QString ) ) );

    download.start( config, dbPath );

    // A real download of the sample database can take a while
    QVERIFY( finishedSpy.wait( 300000 ) );
    QVERIFY2( finishedSpy.first().at( 0 ).toBool(),
              qPrintable( finishedSpy.first().at( 1 ).toString() ) );

    int patrons = countRows( dbPath, "borrowers" );
    int expected = qEnvironmentVariable( "KOHA_EXPECTED_PATRONS" ).toInt();
    if ( expected > 0 ) {
        QCOMPARE( patrons, expected );
    } else {
        QVERIFY( patrons > 0 );
    }

    // The sample data may have no current checkouts, the table just has
    // to exist and be readable
    QVERIFY( countRows( dbPath, "issues" ) >= 0 );
}

void TestKohaDownloadIntegration::restMode()
{
    if ( qEnvironmentVariable( "KOHA_URL" ).isEmpty() ) {
        QSKIP( "KOHA_URL is not set, skipping the live Koha integration test" );
    }

    KohaDownload::Config config;
    config.useReports = false;

    runDownload( config );
}

void TestKohaDownloadIntegration::reportMode()
{
    if ( qEnvironmentVariable( "KOHA_URL" ).isEmpty() ) {
        QSKIP( "KOHA_URL is not set, skipping the live Koha integration test" );
    }
    if ( qEnvironmentVariable( "KOHA_BORROWERS_REPORT_ID" ).isEmpty() ) {
        QSKIP( "KOHA_BORROWERS_REPORT_ID is not set, skipping the report mode test" );
    }

    KohaDownload::Config config;
    config.useReports = true;
    config.borrowersReportId = qEnvironmentVariable( "KOHA_BORROWERS_REPORT_ID" ).toInt();
    config.issuesReportId = qEnvironmentVariable( "KOHA_ISSUES_REPORT_ID" ).toInt();

    runDownload( config );
}

void TestKohaDownloadIntegration::uploadPending()
{
    if ( qEnvironmentVariable( "KOHA_URL" ).isEmpty() ) {
        QSKIP( "KOHA_URL is not set, skipping the live Koha integration test" );
    }

    KohaUpload::Config config;
    config.baseUrl = qEnvironmentVariable( "KOHA_URL" );
    config.userid = qEnvironmentVariable( "KOHA_USER", "koha" );
    config.password = qEnvironmentVariable( "KOHA_PASSWORD", "koha" );
    config.branchcode = qEnvironmentVariable( "KOHA_BRANCHCODE", "CPL" );

    // Pending mode only queues the transaction for review, nothing is
    // circulated, so a nonsense barcode is safe
    config.pending = true;

    KocTransaction transaction;
    transaction.type = "return";
    transaction.barcode = "KOC-INTEGRATION-TEST";
    transaction.date = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh-mm-ss zzz" );

    KohaUpload upload;
    QSignalSpy finishedSpy( &upload, SIGNAL( finished( int, int ) ) );

    upload.start( config, { transaction } );
    QVERIFY( finishedSpy.wait( 60000 ) );
    QCOMPARE( finishedSpy.first().at( 0 ).toInt(), 1 );
    QCOMPARE( finishedSpy.first().at( 1 ).toInt(), 0 );
}

QTEST_GUILESS_MAIN(TestKohaDownloadIntegration)

#include "tst_kohadownload_integration.moc"
