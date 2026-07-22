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

#ifndef KOCFILE_H
#define KOCFILE_H

#include <QString>

struct KocTransaction {
    QString date;
    QString type;
    QString cardnumber;
    QString barcode;
    QString payment;
};

/* The .koc file format, tab separated, one transaction per line.
 * The format is fixed by what Koha's process_koc.pl accepts, lines
 * written by this module must stay identical to those written by the
 * 1.x releases. */
namespace KocFile {
    QString headerLine( const QString & fileVersion, const QString & generatorVersion );
    QString serializeLine( const KocTransaction & transaction );
    KocTransaction parseLine( const QString & line );
    QString formatPayment( double amount );
}

#endif // KOCFILE_H
