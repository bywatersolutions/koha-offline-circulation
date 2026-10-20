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

#ifndef KOHANETWORK_H
#define KOHANETWORK_H

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QUrl>

/* Builds every request sent to Koha. Qt's default user agent is a bare
 * "Mozilla/5.0", which gives a firewall in front of Koha nothing to tell
 * the app apart from any other bot. */
inline QNetworkRequest kohaRequest( const QUrl & url )
{
    QNetworkRequest request( url );
    request.setHeader( QNetworkRequest::UserAgentHeader,
                       "KohaOfflineCirculation/" + QCoreApplication::applicationVersion() );
    return request;
}

#endif // KOHANETWORK_H
