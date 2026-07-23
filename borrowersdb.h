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

#ifndef BORROWERSDB_H
#define BORROWERSDB_H

#include <QString>
#include <QList>

class QJsonObject;
class QJsonArray;

struct KohaPatron {
    QString borrowernumber;
    QString cardnumber;
    QString surname;
    QString firstname;
    QString address;
    QString city;
    QString phone;
    QString dateofbirth;
    double total_fines = 0.0;
};

struct KohaCheckout {
    QString borrowernumber;
    QString date_due;
    QString itemcallnumber;
    QString title;
    QString itemtype;
};

/* Builds the same SQLite file Koha's create_koc_db.pl produces, so the
 * patron lookup code doesn't care where the file came from.
 *
 * The report row mappers expect the column order of the saved reports
 * documented in the README:
 *   borrowers: borrowernumber, cardnumber, surname, firstname, address,
 *              city, phone, dateofbirth, total_fines
 *   issues:    borrowernumber, date_due, itemcallnumber, title, itemtype */
namespace BorrowersDb {
    KohaPatron patronFromApi( const QJsonObject & patron );
    KohaCheckout checkoutFromApi( const QJsonObject & checkout );
    KohaPatron patronFromReportRow( const QJsonArray & row );
    KohaCheckout checkoutFromReportRow( const QJsonArray & row );

    bool write( const QString & filePath, const QList<KohaPatron> & patrons,
                const QList<KohaCheckout> & checkouts, QString * errorMessage );
    bool merge( const QString & filePath, const QList<KohaPatron> & patrons,
                const QList<KohaCheckout> & checkouts, QString * errorMessage );
}

#endif // BORROWERSDB_H
