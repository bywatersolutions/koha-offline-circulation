#ifndef BORROWERSEARCH_H
#define BORROWERSEARCH_H

#include <QDialog>
#include "ui_borrowersearch.h"

class BorrowerSearch : public QDialog, 
                   private Ui::BorrowerSearch
{
  Q_OBJECT

  public:
    BorrowerSearch(QWidget *parent = 0);
    ~BorrowerSearch();

  protected:
    void setupActions();

  protected slots:

  private:
	static const int COLUMN_CARDNUMBER = 0;
	static const int COLUMN_LASTNAME = 1;
	static const int COLUMN_FIRSTNAME = 2;
	static const int COLUMN_DOB = 3;
	static const int COLUMN_ADDRESS = 4;
};

#endif // BORROWERSEARCH_H
