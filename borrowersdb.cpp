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

#include "borrowersdb.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

// Report rows arrive as JSON arrays whose values may be strings, numbers,
// or null depending on the column type
static QString jsonToString( const QJsonValue & value )
{
    if ( value.isString() ) return value.toString();
    if ( value.isDouble() ) return QString::number( value.toDouble() );

    return QString();
}

// Koha's REST API returns ISO 8601 datetimes, the app displays them as
// stored, so convert to the local, space separated form here
static QString isoToLocalDateTime( const QString & iso )
{
    QDateTime dateTime = QDateTime::fromString( iso, Qt::ISODate );
    if ( ! dateTime.isValid() ) return iso;

    return dateTime.toLocalTime().toString( "yyyy-MM-dd hh:mm:ss" );
}

KohaPatron BorrowersDb::patronFromApi( const QJsonObject & patron )
{
    KohaPatron result;

    result.borrowernumber = jsonToString( patron.value( "patron_id" ) );
    result.cardnumber = jsonToString( patron.value( "cardnumber" ) );
    result.surname = jsonToString( patron.value( "surname" ) );
    result.firstname = jsonToString( patron.value( "firstname" ) );
    result.address = jsonToString( patron.value( "address" ) );
    result.city = jsonToString( patron.value( "city" ) );
    result.phone = jsonToString( patron.value( "phone" ) );
    result.dateofbirth = jsonToString( patron.value( "date_of_birth" ) );
    result.total_fines = patron.value( "account_balance" ).toDouble();

    return result;
}

KohaCheckout BorrowersDb::checkoutFromApi( const QJsonObject & checkout )
{
    KohaCheckout result;

    result.borrowernumber = jsonToString( checkout.value( "patron_id" ) );
    result.date_due = isoToLocalDateTime( jsonToString( checkout.value( "due_date" ) ) );

    QJsonObject item = checkout.value( "item" ).toObject();
    result.itemcallnumber = jsonToString( item.value( "callnumber" ) );
    result.itemtype = jsonToString( item.value( "item_type" ) );

    QJsonObject biblio = item.value( "biblio" ).toObject();
    result.title = jsonToString( biblio.value( "title" ) );

    return result;
}

KohaPatron BorrowersDb::patronFromReportRow( const QJsonArray & row )
{
    KohaPatron result;

    result.borrowernumber = jsonToString( row.at( 0 ) );
    result.cardnumber = jsonToString( row.at( 1 ) );
    result.surname = jsonToString( row.at( 2 ) );
    result.firstname = jsonToString( row.at( 3 ) );
    result.address = jsonToString( row.at( 4 ) );
    result.city = jsonToString( row.at( 5 ) );
    result.phone = jsonToString( row.at( 6 ) );
    result.dateofbirth = jsonToString( row.at( 7 ) );
    result.total_fines = jsonToString( row.at( 8 ) ).toDouble();

    return result;
}

KohaCheckout BorrowersDb::checkoutFromReportRow( const QJsonArray & row )
{
    KohaCheckout result;

    result.borrowernumber = jsonToString( row.at( 0 ) );
    result.date_due = jsonToString( row.at( 1 ) );
    result.itemcallnumber = jsonToString( row.at( 2 ) );
    result.title = jsonToString( row.at( 3 ) );
    result.itemtype = jsonToString( row.at( 4 ) );

    return result;
}

bool BorrowersDb::write( const QString & filePath, const QList<KohaPatron> & patrons,
                         const QList<KohaCheckout> & checkouts, QString * errorMessage )
{
    // Build the new database beside the target and swap it into place at
    // the end, so a failed download can't destroy the working database
    QString tmpPath = filePath + ".tmp";
    QFile::remove( tmpPath );

    bool ok = true;
    QString error;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE", "borrowersdb_write" );
        db.setDatabaseName( tmpPath );

        if ( ! db.open() ) {
            ok = false;
            error = db.lastError().text();
        }

        if ( ok ) {
            QSqlQuery query( db );

            db.transaction();

            ok = query.exec( "CREATE TABLE borrowers ( borrowernumber INTEGER PRIMARY KEY, cardnumber TEXT, "
                             "surname TEXT, firstname TEXT, address TEXT, city TEXT, phone TEXT, "
                             "dateofbirth TEXT, total_fines REAL )" )
                 && query.exec( "CREATE INDEX borrowers_cardnumber ON borrowers ( cardnumber )" )
                 && query.exec( "CREATE TABLE issues ( borrowernumber INTEGER, date_due TEXT, "
                                "itemcallnumber TEXT, title TEXT, itemtype TEXT )" )
                 && query.exec( "CREATE INDEX issues_borrowernumber ON issues ( borrowernumber )" );

            if ( ok ) {
                query.prepare( "INSERT INTO borrowers ( borrowernumber, cardnumber, surname, firstname, "
                               "address, city, phone, dateofbirth, total_fines ) "
                               "VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ? )" );
                for ( const KohaPatron & patron : patrons ) {
                    query.addBindValue( patron.borrowernumber );
                    query.addBindValue( patron.cardnumber );
                    query.addBindValue( patron.surname );
                    query.addBindValue( patron.firstname );
                    query.addBindValue( patron.address );
                    query.addBindValue( patron.city );
                    query.addBindValue( patron.phone );
                    query.addBindValue( patron.dateofbirth );
                    query.addBindValue( patron.total_fines );
                    if ( ! query.exec() ) { ok = false; break; }
                }
            }

            if ( ok ) {
                query.prepare( "INSERT INTO issues ( borrowernumber, date_due, itemcallnumber, title, itemtype ) "
                               "VALUES ( ?, ?, ?, ?, ? )" );
                for ( const KohaCheckout & checkout : checkouts ) {
                    query.addBindValue( checkout.borrowernumber );
                    query.addBindValue( checkout.date_due );
                    query.addBindValue( checkout.itemcallnumber );
                    query.addBindValue( checkout.title );
                    query.addBindValue( checkout.itemtype );
                    if ( ! query.exec() ) { ok = false; break; }
                }
            }

            if ( ok ) {
                ok = db.commit();
            }

            if ( ! ok && error.isEmpty() ) {
                error = query.lastError().text();
                db.rollback();
            }

            db.close();
        }
    }
    QSqlDatabase::removeDatabase( "borrowersdb_write" );

    if ( ok ) {
        QFile::remove( filePath );
        ok = QFile::rename( tmpPath, filePath );
        if ( ! ok ) error = "Could not replace " + filePath;
    }

    if ( ! ok ) {
        QFile::remove( tmpPath );
        if ( errorMessage ) *errorMessage = error;
    }

    return ok;
}
