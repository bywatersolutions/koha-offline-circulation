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

#include "kohadownload.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

// Rows per REST request, large pages keep the request count down
// without asking the server to inflate the whole table at once
static const int REST_PAGE_SIZE = 1000;

// Koha's SvcMaxReportRows system preference defaults to this, a report
// that returns exactly this many rows was almost certainly truncated
static const int SVC_DEFAULT_ROW_LIMIT = 10;

KohaDownload::KohaDownload( QObject *parent )
 : QObject( parent )
{
    mNetwork = new QNetworkAccessManager( this );
    mPhase = PhasePatrons;
    mPage = 1;
}

void KohaDownload::start( const Config & config, const QString & outputPath )
{
    mConfig = config;
    while ( mConfig.baseUrl.endsWith( "/" ) ) mConfig.baseUrl.chop( 1 );
    mOutputPath = outputPath;
    mPatrons.clear();
    mCheckouts.clear();

    if ( mConfig.useReports ) {
        mPhase = PhaseBorrowersReport;
        emit progress( tr("Downloading borrowers report...") );
        requestReport( mConfig.borrowersReportId );
    } else {
        mPhase = PhasePatrons;
        mPage = 1;
        emit progress( tr("Downloading borrowers...") );
        requestRestPage( "/api/v1/patrons", "account_balance" );
    }
}

void KohaDownload::requestReport( int reportId )
{
    QUrl url( mConfig.baseUrl + "/cgi-bin/koha/svc/report" );

    QUrlQuery query;
    query.addQueryItem( "id", QString::number( reportId ) );
    query.addQueryItem( "userid", mConfig.userid );
    query.addQueryItem( "password", mConfig.password );
    url.setQuery( query );

    QNetworkRequest request( url );
    request.setTransferTimeout( 300000 );

    QNetworkReply *reply = mNetwork->get( request );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void KohaDownload::requestRestPage( const QString & path, const QString & embed )
{
    QUrl url( mConfig.baseUrl + path );

    QUrlQuery query;
    query.addQueryItem( "_per_page", QString::number( REST_PAGE_SIZE ) );
    query.addQueryItem( "_page", QString::number( mPage ) );
    url.setQuery( query );

    QNetworkRequest request( url );
    request.setRawHeader( "x-koha-embed", embed.toUtf8() );
    request.setRawHeader( "Authorization",
                          "Basic " + QString( mConfig.userid + ":" + mConfig.password ).toUtf8().toBase64() );
    request.setTransferTimeout( 300000 );

    QNetworkReply *reply = mNetwork->get( request );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void KohaDownload::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( ! reply ) return;

    reply->deleteLater();

    if ( reply->error() != QNetworkReply::NoError ) {
        int status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();

        QString hint;
        if ( status == 401 || status == 403 ) {
            hint = mConfig.useReports
                ? tr("\n\nCheck the username and password. The account needs the catalogue permission.")
                : tr("\n\nCheck the username and password, that the RESTBasicAuth system preference is "
                     "enabled, and that the account has the borrowers and circulate permissions.");
        }

        fail( reply->errorString() + hint );
        return;
    }

    QByteArray body = reply->readAll();

    if ( mConfig.useReports ) {
        handleReportReply( body );
    } else {
        handleRestReply( body );
    }
}

void KohaDownload::handleReportReply( const QByteArray & body )
{
    QJsonDocument document = QJsonDocument::fromJson( body );
    if ( ! document.isArray() ) {
        fail( tr("The report service did not return JSON rows. Check the report IDs.") );
        return;
    }

    QJsonArray rows = document.array();

    if ( mPhase == PhaseBorrowersReport ) {
        if ( rows.count() == SVC_DEFAULT_ROW_LIMIT ) {
            fail( tr("The borrowers report returned exactly %1 rows, which is Koha's default "
                     "SvcMaxReportRows limit. Raise the SvcMaxReportRows system preference on "
                     "the Koha server.").arg( SVC_DEFAULT_ROW_LIMIT ) );
            return;
        }

        for ( const QJsonValue & row : rows ) {
            mPatrons.append( BorrowersDb::patronFromReportRow( row.toArray() ) );
        }

        mPhase = PhaseIssuesReport;
        emit progress( tr("Downloading issues report...") );
        requestReport( mConfig.issuesReportId );
    } else {
        for ( const QJsonValue & row : rows ) {
            mCheckouts.append( BorrowersDb::checkoutFromReportRow( row.toArray() ) );
        }

        writeDatabase();
    }
}

void KohaDownload::handleRestReply( const QByteArray & body )
{
    QJsonDocument document = QJsonDocument::fromJson( body );
    if ( ! document.isArray() ) {
        fail( tr("The API did not return JSON rows. Check the Koha staff URL.") );
        return;
    }

    QJsonArray rows = document.array();

    if ( mPhase == PhasePatrons ) {
        for ( const QJsonValue & row : rows ) {
            mPatrons.append( BorrowersDb::patronFromApi( row.toObject() ) );
        }

        if ( rows.count() == REST_PAGE_SIZE ) {
            mPage++;
            emit progress( tr("Downloading borrowers... %1 so far").arg( mPatrons.count() ) );
            requestRestPage( "/api/v1/patrons", "account_balance" );
        } else {
            mPhase = PhaseCheckouts;
            mPage = 1;
            emit progress( tr("Downloading checkouts...") );
            requestRestPage( "/api/v1/checkouts", "item,item.biblio" );
        }
    } else {
        for ( const QJsonValue & row : rows ) {
            mCheckouts.append( BorrowersDb::checkoutFromApi( row.toObject() ) );
        }

        if ( rows.count() == REST_PAGE_SIZE ) {
            mPage++;
            emit progress( tr("Downloading checkouts... %1 so far").arg( mCheckouts.count() ) );
            requestRestPage( "/api/v1/checkouts", "item,item.biblio" );
        } else {
            writeDatabase();
        }
    }
}

void KohaDownload::writeDatabase()
{
    emit progress( tr("Writing borrowers database...") );

    QString error;
    if ( BorrowersDb::write( mOutputPath, mPatrons, mCheckouts, &error ) ) {
        emit finished( true, tr("Downloaded %1 borrowers and %2 checkouts.")
                                 .arg( mPatrons.count() ).arg( mCheckouts.count() ) );
    } else {
        fail( error );
    }
}

void KohaDownload::fail( const QString & message )
{
    emit finished( false, message );
}
