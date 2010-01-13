/*
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

#ifndef BORROWERSEARCH_H
#define BORROWERSEARCH_H

#include <QDialog>

#include "ui_borrowersearch.h"

class BorrowerSearch : public QDialog, public Ui::BorrowerSearch {
  Q_OBJECT

  public:
    BorrowerSearch(QWidget *parent = 0);
    ~BorrowerSearch();

  signals:
	void useBorrower( const QString & );

  protected:
    void setupActions();

  protected slots:
	void searchBorrowers();
	void cancelSearch();

	void acceptBorrower();

  private:
	void addSearchResult( const QString & surname, const QString & firstname, const QString & dateofbirth, const QString & address, const QString & cardnumber );

	void clearResults();

	static const int COLUMN_CARDNUMBER = 0;
	static const int COLUMN_LASTNAME = 1;
	static const int COLUMN_FIRSTNAME = 2;
	static const int COLUMN_DOB = 3;
	static const int COLUMN_ADDRESS = 4;
};

#endif // BORROWERSEARCH_H
