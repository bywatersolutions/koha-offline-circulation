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

#include "kohaupload.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

KohaUpload::KohaUpload( QObject *parent )
 : QObject( parent )
{
    mNetwork = new QNetworkAccessManager( this );
    mIndex = 0;
    mSent = 0;
    mFailed = 0;
    mCancelled = false;
}

QString KohaUpload::isoTimestamp( const QString & kocDate )
{
    // .koc timestamps are local time in the app's format, the service
    // expects ISO 8601 UTC and converts back to the server's timezone
    QDateTime local = QDateTime::fromString( kocDate, "yyyy-MM-dd hh-mm-ss zzz" );
    if ( ! local.isValid() ) return kocDate;

    return local.toUTC().toString( Qt::ISODate );
}

void KohaUpload::start( const Config & config, const QList<KocTransaction> & transactions )
{
    mConfig = config;
    while ( mConfig.baseUrl.endsWith( "/" ) ) mConfig.baseUrl.chop( 1 );
    mTransactions = transactions;
    mIndex = 0;
    mSent = 0;
    mFailed = 0;
    mCancelled = false;
    mAwaitingToken = false;

    if ( mConfig.usePlugin ) {
        if ( mConfig.useOAuth ) {
            mAwaitingToken = true;
            mAccessToken.clear();
            emit progress( tr("Requesting an access token...") );
            requestToken();
        } else {
            sendBatch();
        }
        return;
    }

    sendNext();
}

void KohaUpload::setAuthorization( QNetworkRequest & request )
{
    if ( mConfig.useOAuth ) {
        request.setRawHeader( "Authorization", "Bearer " + mAccessToken.toUtf8() );
    } else {
        request.setRawHeader( "Authorization",
                              "Basic " + QString( mConfig.userid + ":" + mConfig.password ).toUtf8().toBase64() );
    }
}

void KohaUpload::requestToken()
{
    QNetworkRequest request( ( QUrl( mConfig.baseUrl + "/api/v1/oauth/token" ) ) );
    request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
    request.setTransferTimeout( 60000 );

    QUrlQuery body;
    body.addQueryItem( "grant_type", "client_credentials" );
    body.addQueryItem( "client_id", mConfig.clientId );
    body.addQueryItem( "client_secret", mConfig.clientSecret );

    QNetworkReply *reply = mNetwork->post( request, body.toString( QUrl::FullyEncoded ).toUtf8() );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void KohaUpload::handleTokenReply( const QByteArray & body )
{
    QJsonObject response = QJsonDocument::fromJson( body ).object();
    mAccessToken = response.value( "access_token" ).toString();

    if ( mAccessToken.isEmpty() ) {
        batchFail( tr("The server did not issue an access token. Check the API client ID and secret, "
                      "and that the RESTOAuth2ClientCredentials system preference is enabled.") );
        return;
    }

    sendBatch();
}

void KohaUpload::sendBatch()
{
    QJsonArray transactionsJson;
    for ( const KocTransaction & transaction : mTransactions ) {
        QJsonObject entry;
        entry.insert( "timestamp", isoTimestamp( transaction.date ) );
        entry.insert( "action", transaction.type );
        if ( ! transaction.cardnumber.isEmpty() ) entry.insert( "cardnumber", transaction.cardnumber );
        if ( ! transaction.barcode.isEmpty() ) entry.insert( "barcode", transaction.barcode );
        if ( ! transaction.payment.isEmpty() ) entry.insert( "amount", transaction.payment );
        transactionsJson.append( entry );
    }

    QJsonObject body;
    body.insert( "branchcode", mConfig.branchcode );
    body.insert( "pending", mConfig.pending );
    body.insert( "transactions", transactionsJson );

    emit progress( tr("Uploading %1 transactions...").arg( mTransactions.count() ) );

    QNetworkRequest request( ( QUrl( mConfig.baseUrl + "/api/v1/contrib/offlinecirc/transactions" ) ) );
    request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
    setAuthorization( request );
    request.setTransferTimeout( 300000 );

    QNetworkReply *reply = mNetwork->post( request, QJsonDocument( body ).toJson( QJsonDocument::Compact ) );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void KohaUpload::handleBatchReply( const QByteArray & body )
{
    QJsonObject response = QJsonDocument::fromJson( body ).object();
    QJsonArray results = response.value( "results" ).toArray();

    if ( results.count() != mTransactions.count() ) {
        batchFail( tr("The plugin returned an unexpected response.") );
        return;
    }

    for ( int i = 0; i < results.count(); i++ ) {
        QJsonObject result = results.at( i ).toObject();
        QString status = result.value( "status" ).toString();
        QString message = result.value( "message" ).toString();

        // A skipped transaction was processed by an earlier upload,
        // that's the duplicate protection working
        bool ok = status == "processed" || status == "queued" || status == "skipped";
        if ( ok ) {
            mSent++;
        } else {
            mFailed++;
        }
        emit transactionResult( i, ok, message );
    }

    emit finished( mSent, mFailed );
}

void KohaUpload::batchFail( const QString & message )
{
    emit transactionResult( 0, false, message );
    emit finished( 0, mTransactions.count() );
}

void KohaUpload::cancel()
{
    mCancelled = true;
}

void KohaUpload::sendNext()
{
    if ( mCancelled || mIndex >= mTransactions.count() ) {
        emit finished( mSent, mFailed );
        return;
    }

    const KocTransaction & transaction = mTransactions.at( mIndex );

    QUrl url( mConfig.baseUrl + "/cgi-bin/koha/offline_circ/service.pl" );

    QUrlQuery query;

    // Koha 24.05 renamed the credential parameters to login_userid and
    // login_password, send both sets so older versions keep working
    query.addQueryItem( "login_userid", mConfig.userid );
    query.addQueryItem( "login_password", mConfig.password );
    query.addQueryItem( "userid", mConfig.userid );
    query.addQueryItem( "password", mConfig.password );

    query.addQueryItem( "branchcode", mConfig.branchcode );
    query.addQueryItem( "pending", mConfig.pending ? "true" : "false" );
    query.addQueryItem( "action", transaction.type );
    query.addQueryItem( "timestamp", isoTimestamp( transaction.date ) );
    if ( ! transaction.cardnumber.isEmpty() ) query.addQueryItem( "cardnumber", transaction.cardnumber );
    if ( ! transaction.barcode.isEmpty() ) query.addQueryItem( "barcode", transaction.barcode );
    if ( ! transaction.payment.isEmpty() ) query.addQueryItem( "amount", transaction.payment );
    url.setQuery( query );

    emit progress( tr("Uploading transaction %1 of %2...").arg( mIndex + 1 ).arg( mTransactions.count() ) );

    QNetworkRequest request( url );
    request.setTransferTimeout( 60000 );

    QNetworkReply *reply = mNetwork->get( request );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void KohaUpload::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( ! reply ) return;

    reply->deleteLater();

    if ( mConfig.usePlugin ) {
        if ( reply->error() != QNetworkReply::NoError ) {
            int status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();

            QString hint;
            if ( status == 404 ) {
                hint = tr("\n\nThe Offline Circulation plugin does not appear to be installed on this Koha server.");
            } else if ( status == 400 || status == 401 || status == 403 ) {
                hint = mConfig.useOAuth
                    ? tr("\n\nCheck the API client ID and secret, and that the "
                         "RESTOAuth2ClientCredentials system preference is enabled.")
                    : tr("\n\nCheck the username and password, and that the account has the "
                         "circulate permission.");
            }

            batchFail( reply->errorString() + hint );
            return;
        }

        QByteArray body = reply->readAll();
        if ( mAwaitingToken ) {
            mAwaitingToken = false;
            handleTokenReply( body );
        } else {
            handleBatchReply( body );
        }
        return;
    }

    int index = mIndex;
    mIndex++;

    if ( reply->error() != QNetworkReply::NoError ) {
        // A transport failure likely affects every remaining
        // transaction too, stop rather than fail them all one by one
        mFailed++;
        emit transactionResult( index, false, reply->errorString() );
        emit finished( mSent, mFailed );
        return;
    }

    QString body = QString::fromUtf8( reply->readAll() ).trimmed();

    if ( body.contains( "auth_status" ) ) {
        mFailed++;
        emit transactionResult( index, false, tr("Authentication failed, check the username and password.") );
        emit finished( mSent, mFailed );
        return;
    }

    bool ok = body.startsWith( "Success" ) || body.startsWith( "Added" );
    if ( ok ) {
        mSent++;
    } else {
        mFailed++;
    }
    emit transactionResult( index, ok, body );

    sendNext();
}
