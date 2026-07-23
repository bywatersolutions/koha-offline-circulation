/*
* Copyright 2010 Kyle M Hall <kyle.m.hall@gmail.com>
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

#include <QtWidgets>
#include <QtSql>
#include <QDebug>

#include "borrowersearch.h"

BorrowerSearch::BorrowerSearch(QWidget *parent) : QDialog(parent) {
  setupUi(this);

  searchBorrowersButton->setIcon( QIcon::fromTheme( QIcon::ThemeIcon::SystemSearch,
                                                    QIcon(":/icons/images/icons/system-search.png") ) );

  setupActions();
}

BorrowerSearch::~BorrowerSearch() {
}

void BorrowerSearch::setupActions() {
  connect(searchBorrowersButton, SIGNAL(clicked()),
          this, SLOT(searchBorrowers()));

  connect(nameLast, SIGNAL(returnPressed()),
          this, SLOT(searchBorrowers()));
  connect(nameFirst, SIGNAL(returnPressed()),
          this, SLOT(searchBorrowers()));

  connect(pushButtonOK, SIGNAL(clicked()),
          this, SLOT(acceptBorrower()));
  connect(resultsTable, SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
          this, SLOT(acceptBorrower()));

  connect(pushButtonCancel, SIGNAL(clicked()),
          this, SLOT(cancelSearch()));
}

void BorrowerSearch::searchBorrowers() {
        QSettings settings;
        QString borrowersDbFilePath = settings.value("borrowersDbFilePath").toString();

        // If borrowersDbFilePath is not set,
        // Default to APP_DIR/borrowers.db so the program does not crash.
        if ( borrowersDbFilePath.isEmpty() ) {
            borrowersDbFilePath = "borrowers.db";
        }

	clearResults();

	QString lastnameSearch = nameLast->text();
	QString firstnameSearch = nameFirst->text();

	// SQLite creates a missing database file on open, so check the file
	// exists first, otherwise a bad path just returns no search results
	if ( ! QFile::exists( borrowersDbFilePath ) ) {
		QMessageBox::warning(this, tr("Cannot Open Borrowers File"),
                                   tr("The borrowers database file does not exist:\n%1\n\n"
									"This file is needed in order to search for borrowers. "
									"Set its location via Settings, Select Borrowers DB File.").arg(borrowersDbFilePath),
                                   QMessageBox::Ok);
		return;
	}

	// Reuse the existing connection, calling addDatabase repeatedly
	// adds a duplicate connection and Qt warns about it every time
	QSqlDatabase db = QSqlDatabase::contains()
		? QSqlDatabase::database( QSqlDatabase::defaultConnection, false )
		: QSqlDatabase::addDatabase( "QSQLITE" );

	if ( db.databaseName() != borrowersDbFilePath ) {
		db.close();
		db.setDatabaseName( borrowersDbFilePath );
	}

	if ( db.isOpen() || db.open() ) {
		// Bind the search terms rather than concatenating them into the
		// SQL, names containing an apostrophe broke the search
		QSqlQuery query( db );
		query.prepare( "SELECT * FROM borrowers WHERE firstname LIKE ? AND surname LIKE ? ORDER BY firstname ASC" );
		query.addBindValue( firstnameSearch + "%" );
		query.addBindValue( lastnameSearch + "%" );
		query.exec();

		QSqlRecord record = query.record();
		while (query.next()) {
			QString lastname = query.value(record.indexOf("surname")).toString();
			QString firstname = query.value(record.indexOf("firstname")).toString();
			QString dateofbirth = query.value(record.indexOf("dateofbirth")).toString();
			QString streetaddress = query.value(record.indexOf("address")).toString();
			QString city = query.value(record.indexOf("city")).toString();
			QString state = query.value(record.indexOf("state")).toString();
			QString zipcode = query.value(record.indexOf("zipcode")).toString();
			QString cardnumber = query.value(record.indexOf("cardnumber")).toString();

			// Koha's create_koc_db.pl doesn't export state or zipcode, skip
			// empty parts so the address doesn't show dangling separators
			QString cityLine = city;
			if ( ! state.isEmpty() ) cityLine += ", " + state;

			QStringList addressParts;
			if ( ! streetaddress.isEmpty() ) addressParts << streetaddress;
			if ( ! cityLine.isEmpty() ) addressParts << cityLine;
			if ( ! zipcode.isEmpty() ) addressParts << zipcode;
			QString address = addressParts.join( "\n" );
	
			qDebug() << query.at() << ":" << lastname << "," << firstname;
	
			addSearchResult( lastname, firstname, dateofbirth, address, cardnumber );
		}

		qDebug() << "Search Complete";

	} else {
		qDebug() << db.lastError();

		QMessageBox::warning(this, tr("Cannot Open Borrowers File"),
                                   tr("Unable to open the borrowers database file:\n%1\n\n"
									"This file is needed in order to search for borrowers. "
									"Set its location via Settings, Select Borrowers DB File.").arg(borrowersDbFilePath),
                                   QMessageBox::Ok);
	}

}

void BorrowerSearch::acceptBorrower() {
    QList<QTableWidgetItem *> itemList = resultsTable->selectedItems();

    if ( itemList.size() > 0 ) {
        QTableWidgetItem *item = itemList.takeFirst();
	QString cardnumber = item->text();

	emit useBorrower( cardnumber );
    }

    this->hide();
}

void BorrowerSearch::cancelSearch() {
	this->hide();
}

void BorrowerSearch::addSearchResult( const QString & lastname, const QString & firstname, const QString & dateofbirth, const QString & address, const QString & cardnumber ) {

    QTableWidgetItem *cardnumberItem = new QTableWidgetItem( cardnumber );
    QTableWidgetItem *lastnameItem = new QTableWidgetItem( lastname );
    QTableWidgetItem *firstnameItem = new QTableWidgetItem( firstname );
    QTableWidgetItem *dateofbirthItem = new QTableWidgetItem( dateofbirth );
    QTableWidgetItem *addressItem = new QTableWidgetItem( address );

    int row = resultsTable->rowCount();

	resultsTable->insertRow(row);
	resultsTable->setItem(row, COLUMN_CARDNUMBER, cardnumberItem);
	resultsTable->setItem(row, COLUMN_LASTNAME, lastnameItem);
	resultsTable->setItem(row, COLUMN_FIRSTNAME, firstnameItem);
	resultsTable->setItem(row, COLUMN_DOB, dateofbirthItem);
	resultsTable->setItem(row, COLUMN_ADDRESS, addressItem);

}

void BorrowerSearch::clearResults() {
	int rowCount = resultsTable->rowCount();
	for ( int row = 0; row < rowCount; row++ ) {
		resultsTable->removeRow( 0 );
	}
}

