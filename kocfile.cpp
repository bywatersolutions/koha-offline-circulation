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

#include "kocfile.h"

#include <QStringList>

QString KocFile::headerLine( const QString & fileVersion, const QString & generatorVersion )
{
    return "Version=" + fileVersion + "\tGenerator=kocDesktop\tGeneratorVersion=" + generatorVersion;
}

QString KocFile::serializeLine( const KocTransaction & transaction )
{
    if ( transaction.type == "issue" ) {
        return transaction.date + "\t" + transaction.type + "\t" + transaction.cardnumber + "\t" + transaction.barcode;
    } else if ( transaction.type == "return" ) {
        return transaction.date + "\t" + transaction.type + "\t" + transaction.barcode;
    } else if ( transaction.type == "payment" ) {
        return transaction.date + "\t" + transaction.type + "\t" + transaction.cardnumber + "\t" + transaction.payment;
    }

    return transaction.date + "\t" + transaction.type;
}

QString KocFile::formatPayment( double amount )
{
    return QString::number( amount, 'f', 2 );
}

KocTransaction KocFile::parseLine( const QString & line )
{
    KocTransaction transaction;

    QStringList parts = line.split("\t");

    transaction.date = parts.takeFirst();
    if ( ! parts.isEmpty() ) transaction.type = parts.takeFirst();

    if ( transaction.type == "issue" ) {
        if ( ! parts.isEmpty() ) transaction.cardnumber = parts.takeFirst();
        if ( ! parts.isEmpty() ) transaction.barcode = parts.takeFirst();
    } else if ( transaction.type == "return" ) {
        if ( ! parts.isEmpty() ) transaction.barcode = parts.takeFirst();
    } else if ( transaction.type == "payment" ) {
        if ( ! parts.isEmpty() ) transaction.cardnumber = parts.takeFirst();
        if ( ! parts.isEmpty() ) transaction.payment = parts.takeFirst();
    }

    return transaction;
}
