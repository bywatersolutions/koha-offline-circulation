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

#include "updatecheck.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVersionNumber>

static const char *DEFAULT_API_URL =
    "https://api.github.com/repos/bywatersolutions/koha-offline-circulation/releases/latest";

UpdateCheck::UpdateCheck( QObject *parent )
 : QObject( parent )
{
    mNetwork = new QNetworkAccessManager( this );
    apiUrl = DEFAULT_API_URL;
}

bool UpdateCheck::isNewer( const QString & latest, const QString & current )
{
    QString latestNumber = latest.startsWith( "v" ) ? latest.mid( 1 ) : latest;
    QString currentNumber = current.startsWith( "v" ) ? current.mid( 1 ) : current;

    return QVersionNumber::fromString( latestNumber ) > QVersionNumber::fromString( currentNumber );
}

void UpdateCheck::start( const QString & currentVersion )
{
    mCurrentVersion = currentVersion;

    QNetworkRequest request( ( QUrl( apiUrl ) ) );

    // GitHub's API rejects requests without a user agent
    request.setHeader( QNetworkRequest::UserAgentHeader, "KohaOfflineCirculation/" + currentVersion );
    request.setRawHeader( "Accept", "application/vnd.github+json" );
    request.setTransferTimeout( 30000 );

    QNetworkReply *reply = mNetwork->get( request );
    connect( reply, SIGNAL( finished() ),
             this, SLOT( onReplyFinished() ) );
}

void UpdateCheck::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( ! reply ) return;

    reply->deleteLater();

    if ( reply->error() != QNetworkReply::NoError ) {
        emit finished( false, false, QString(), QString() );
        return;
    }

    QJsonObject release = QJsonDocument::fromJson( reply->readAll() ).object();
    QString latest = release.value( "tag_name" ).toString();
    QString releaseUrl = release.value( "html_url" ).toString();

    if ( latest.isEmpty() ) {
        emit finished( false, false, QString(), QString() );
        return;
    }

    emit finished( true, isNewer( latest, mCurrentVersion ), latest, releaseUrl );
}
