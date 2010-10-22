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

#include <QtGui>
#include <QtSql>
#include <QDebug>

#include "borrowersearch.h"

BorrowerSearch::BorrowerSearch(QWidget *parent) : QDialog(parent) {
  setupUi(this);

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
            borrowersDbFilePath = 'borrowers.db';
        }

	clearResults();

	QString lastnameSearch = nameLast->text();
	QString firstnameSearch = nameFirst->text();

	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );

        db.setDatabaseName( borrowersDbFilePath );

	if ( db.open() ) {
		QString borrowersQuery = "SELECT * FROM borrowers WHERE firstname LIKE '" 
						+ firstnameSearch + "%' AND surname LIKE '" + lastnameSearch 
						+ "%' ORDER BY firstname ASC";
		QSqlQuery query( borrowersQuery );
	
		qDebug() << "Borrower Search SQL: " + borrowersQuery;
	
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
			QString address = streetaddress + "\n" + city + ", " + state + "\n" + zipcode;
	
			qDebug() << query.at() << ":" << lastname << "," << firstname;
	
			addSearchResult( lastname, firstname, dateofbirth, address, cardnumber );
		}

		qDebug() << "Search Complete";

	} else {
		qDebug() << db.lastError();
		qFatal( "Failed to connect." );

		QMessageBox::warning(this, tr("Cannot Borrowers File"),
                                   tr("Unable to open the file borrowers.db.\n"
									"This file is needed in order to search for borrowers."
									"Please create the borrowers.db file and place it in"
									"the same directory as this program.\n\n"),
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

