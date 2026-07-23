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

#ifndef KOHAUPLOAD_H
#define KOHAUPLOAD_H

#include <QObject>

#include "kocfile.h"

class QNetworkAccessManager;

/* Uploads transactions to Koha's offline circulation service, one
 * request per transaction, the same endpoint the old KOCT browser
 * plugin used. Requests are GETs because Koha's CSRF protection
 * rejects credentialed POSTs to CGI scripts.
 *
 * With pending set the transactions are queued for staff review under
 * Circulation, Pending offline circulation actions; otherwise they are
 * processed immediately, like uploading a .koc file does. */
class KohaUpload : public QObject
{
    Q_OBJECT

    public:
        struct Config {
            QString baseUrl;
            QString userid;
            QString password;
            QString branchcode;
            bool pending = false;
        };

        explicit KohaUpload( QObject *parent = 0 );

        void start( const Config & config, const QList<KocTransaction> & transactions );
        void cancel();

        static QString isoTimestamp( const QString & kocDate );

    signals:
        void progress( const QString & message );
        void transactionResult( int index, bool ok, const QString & result );
        void finished( int sent, int failed );

    protected slots:
        void onReplyFinished();

    protected:
        void sendNext();

    private:
        QNetworkAccessManager *mNetwork;
        Config mConfig;
        QList<KocTransaction> mTransactions;
        int mIndex;
        int mSent;
        int mFailed;
        bool mCancelled;
};

#endif // KOHAUPLOAD_H
