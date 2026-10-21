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

#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

/* One downloadable file of a release. GitHub publishes a SHA-256 for
 * every release asset, an asset without one has an empty sha256. */
struct UpdateAsset {
    QString name;
    QUrl url;
    qint64 size = 0;
    QByteArray sha256;
};

/* Asks the GitHub releases API whether a newer version has been
 * published, and keeps the release's downloads so UpdateInstaller can
 * fetch the right one for this platform. */
class UpdateCheck : public QObject
{
    Q_OBJECT

    public:
        explicit UpdateCheck( QObject *parent = 0 );

        // Overridable so the tests can point it at a local server
        QString apiUrl;

        void start( const QString & currentVersion );

        static bool isNewer( const QString & latest, const QString & current );

        QList<UpdateAsset> assets() const;

    signals:
        void finished( bool ok, bool updateAvailable, const QString & latestVersion, const QString & releaseUrl );

    protected slots:
        void onReplyFinished();

    private:
        QNetworkAccessManager *mNetwork;
        QString mCurrentVersion;
        QList<UpdateAsset> mAssets;
};

#endif // UPDATECHECK_H
