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

  connect(searchButtonBox, SIGNAL(accepted()),
          this, SLOT(acceptBorrower()));
  connect(searchButtonBox, SIGNAL(rejected()),
          this, SLOT(cancelSearch()));

}

void BorrowerSearch::searchBorrowers() {
	clearResults();

	QString lastnameSearch = nameLast->text();
	QString firstnameSearch = nameFirst->text();

	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );

	db.setDatabaseName( "borrowers.db" );

	if ( db.open() ) {

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

	QString borrowersQuery = "SELECT * FROM borrowers WHERE firstname LIKE '" 
					+ firstnameSearch + "%' AND surname LIKE '" + lastnameSearch 
					+ "%' ORDER BY firstname ASC, surname ASC";
	QSqlQuery query( borrowersQuery );

	qDebug() << "Borrower Search SQL: " + borrowersQuery;

	QSqlRecord record = query.record();
	while (query.next()) {
		QString lastname = query.value(record.indexOf("surname")).toString();
		QString firstname = query.value(record.indexOf("firstname")).toString();
		QString dateofbirth = query.value(record.indexOf("dateofbirth")).toString();
		QString streetaddress = query.value(record.indexOf("streetaddress")).toString();
		QString city = query.value(record.indexOf("city")).toString();
		QString state = query.value(record.indexOf("state")).toString();
		QString zipcode = query.value(record.indexOf("zipcode")).toString();
		QString cardnumber = query.value(record.indexOf("cardnumber")).toString();
		QString address = streetaddress + "\n" + city + ", " + state + "\n" + zipcode;

		qDebug() << query.at() << ":" << lastname << "," << firstname;

		addSearchResult( lastname, firstname, dateofbirth, address, cardnumber );
	}

	qDebug() << "Search Complete";

}

void BorrowerSearch::acceptBorrower() {
	emit useBorrower( "TestBorrower" );

}

void BorrowerSearch::cancelSearch() {

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

