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

#include "kohalibraries.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

KohaLibraries::KohaLibraries( QObject *parent )
 : QObject( parent )
{
    mNetwork = new QNetworkAccessManager( this );
}

const QList<KohaLibrary> & KohaLibraries::libraries() const
{
    return mLibraries;
}

void KohaLibraries::start( const QString & baseUrl )
{
    QString base = baseUrl;
    while ( base.endsWith( "/" ) ) base.chop( 1 );

    mLibraries.clear();

    QUrl url( base + "/api/v1/public/libraries" );

    // A page size of -1 turns paging off, every library in one request
    QUrlQuery query;
    query.addQueryItem( "_per_page", "-1" );
    query.addQueryItem( "_order_by", "name" );
    url.setQuery( query );

    QNetworkRequest request( url );
    request.setTransferTimeout( 30000 );

    QNetworkReply *reply = mNetwork->get( request );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void KohaLibraries::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( ! reply ) return;

    reply->deleteLater();

    if ( reply->error() != QNetworkReply::NoError ) {
        int status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();

        QString hint;
        if ( status == 404 ) {
            hint = tr("\n\nCheck the Koha staff URL, and that the RESTPublicAPI system preference is enabled.");
        }

        emit finished( false, reply->errorString() + hint );
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson( reply->readAll() );
    if ( ! document.isArray() ) {
        emit finished( false, tr("The server did not return a list of libraries. Check the Koha staff URL.") );
        return;
    }

    QJsonArray rows = document.array();
    for ( const QJsonValue & row : rows ) {
        QJsonObject object = row.toObject();

        KohaLibrary library;
        library.code = object.value( "library_id" ).toString();
        library.name = object.value( "name" ).toString();
        mLibraries.append( library );
    }

    emit finished( true, tr("%1 libraries loaded.").arg( mLibraries.count() ) );
}
