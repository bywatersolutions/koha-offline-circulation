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

#ifndef KOHALIBRARIES_H
#define KOHALIBRARIES_H

#include <QList>
#include <QObject>
#include <QString>

class QNetworkAccessManager;

struct KohaLibrary {
    QString code;
    QString name;
};

/* Fetches the list of libraries from a Koha server's public API so
 * the branch code can be picked from a list instead of typed. The
 * public namespace needs no credentials, so this works the same with
 * every connection method. */
class KohaLibraries : public QObject
{
    Q_OBJECT

    public:
        explicit KohaLibraries( QObject *parent = 0 );

        void start( const QString & baseUrl );

        const QList<KohaLibrary> & libraries() const;

    signals:
        void finished( bool ok, const QString & message );

    protected slots:
        void onReplyFinished();

    private:
        QNetworkAccessManager *mNetwork;
        QList<KohaLibrary> mLibraries;
};

#endif // KOHALIBRARIES_H
