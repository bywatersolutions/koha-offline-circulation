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
