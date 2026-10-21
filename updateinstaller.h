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

#ifndef UPDATEINSTALLER_H
#define UPDATEINSTALLER_H

#include <QCryptographicHash>
#include <QObject>

#include "updatecheck.h"

class QNetworkAccessManager;
class QNetworkReply;
class QSaveFile;

/* Downloads the release asset that matches how this copy of the app was
 * installed and checks it against the SHA-256 GitHub published for it,
 * nothing is installed from a file that doesn't match. install() then
 * puts the new version in place and arranges for it to start once this
 * one has quit. */
class UpdateInstaller : public QObject
{
    Q_OBJECT

    public:
        enum Kind { WindowsInstaller, MacDmg, LinuxAppImage, Unsupported };

        explicit UpdateInstaller( QObject *parent = 0 );

        static Kind installKind();
        static int pickAsset( const QList<UpdateAsset> & assets, Kind kind );

        // Overridable so the tests can download into a temporary directory
        QString downloadDir;

        void download( const UpdateAsset & asset );
        void cancel();

        bool install( const QString & path, QString * error );

    signals:
        void downloadProgress( qint64 received, qint64 total );
        void ready( const QString & path );
        void failed( const QString & error );

    protected slots:
        void onReadyRead();
        void onReplyFinished();

    private:
        QNetworkAccessManager *mNetwork;
        QNetworkReply *mReply;
        QSaveFile *mFile;
        QCryptographicHash mHash;
        UpdateAsset mAsset;
};

#endif // UPDATEINSTALLER_H
