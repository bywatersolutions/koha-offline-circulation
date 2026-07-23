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

    sendNext();
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
