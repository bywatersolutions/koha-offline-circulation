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

#ifndef KOHADOWNLOAD_H
#define KOHADOWNLOAD_H

#include <QObject>

#include "borrowersdb.h"

class QNetworkAccessManager;
class QNetworkRequest;

/* Downloads patron and checkout data from a Koha server and builds the
 * local borrowers database from it. Two modes:
 *
 * Report mode fetches two saved SQL reports through the svc/report
 * JSON service. One SQL execution server side, cached in memcached
 * between requests, and only needs the catalogue permission.
 *
 * REST mode pages through /api/v1/patrons and /api/v1/checkouts. No
 * server setup beyond RESTBasicAuth, but needs borrowers and circulate
 * permissions and inflates every row through the ORM. */
class KohaDownload : public QObject
{
    Q_OBJECT

    public:
        struct Config {
            QString baseUrl;
            QString userid;
            QString password;

            // Plugin mode downloads the prebuilt database from the
            // companion Koha plugin, the other modes assemble it here
            enum Method { MethodPlugin, MethodReports, MethodRest };
            Method method = MethodRest;

            int borrowersReportId = 0;
            int issuesReportId = 0;

            // REST mode only: fetch just the patrons changed since this
            // ISO timestamp and merge them into the existing database
            QString updatedSince;

            // Plugin and REST modes: authenticate with an OAuth2 client
            // credentials token instead of the username and password
            bool useOAuth = false;
            QString clientId;
            QString clientSecret;
        };

        explicit KohaDownload( QObject *parent = 0 );

        void start( const Config & config, const QString & outputPath );

    signals:
        void progress( const QString & message );
        void finished( bool ok, const QString & message );

    protected slots:
        void onReplyFinished();

    protected:
        void requestReport( int reportId );
        void requestToken();
        void requestRestPage( const QString & path, const QString & embed );
        void requestPluginDatabase();
        void setAuthorization( QNetworkRequest & request );
        void handleReportReply( const QByteArray & body );
        void handleTokenReply( const QByteArray & body );
        void handleRestReply( const QByteArray & body );
        void handlePluginReply( const QByteArray & body );
        void writeDatabase();
        void fail( const QString & message );

    private:
        enum Phase { PhaseBorrowersReport, PhaseIssuesReport, PhaseToken, PhasePatrons, PhaseCheckouts, PhasePluginDownload };

        QNetworkAccessManager *mNetwork;
        Config mConfig;
        QString mOutputPath;
        QString mAccessToken;
        Phase mPhase;
        int mPage;
        QList<KohaPatron> mPatrons;
        QList<KohaCheckout> mCheckouts;
};

#endif // KOHADOWNLOAD_H
