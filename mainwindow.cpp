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

#include <QtGui>
#include <QtSql>

#include "mainwindow.h"
#include "borrowersearch.h"

MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent)
{
  TITLE = "Koha Offline Circulation";
  VERSION = "1.0";
  FILE_VERSION = "1.0";
  DATETIME_FORMAT = "yyyy-MM-dd hh:mm:ss zzz";

  setupUi(this);
  setupActions();

  mStatLabel = new QLabel;
  statusBar()->addPermanentWidget(mStatLabel);

  this->showMaximized();
  lineEditIssuesBorrowerCardnumber->setFocus();
}

void MainWindow::setupActions()
{
  /* File Menu Actions */
  connect(actionQuit, SIGNAL(triggered(bool)),
          qApp, SLOT(quit()));
  actionQuit->setShortcut(tr("Ctrl+Q"));

  connect(actionOpen, SIGNAL(triggered(bool)),
          this, SLOT(loadFile()));
  actionOpen->setShortcut(tr("Ctrl+O"));

  connect(actionSave, SIGNAL(triggered(bool)),
          this, SLOT(saveFile()));
  actionSave->setShortcut(tr("Ctrl+S"));

  connect(actionSaveAs, SIGNAL(triggered(bool)),
          this, SLOT(saveFileAs()));
  actionSaveAs->setShortcut(tr("Ctrl+A"));

  connect(actionClose, SIGNAL(triggered(bool)),
          this, SLOT(closeFile()));
  actionClose->setShortcut(tr("Ctrl+C"));

  connect(actionNew, SIGNAL(triggered(bool)),
          this, SLOT(newFile()));
  actionNew->setShortcut(tr("Ctrl+N"));

  /* Help Menu Actions */
  connect(actionAbout, SIGNAL(triggered(bool)),
          this, SLOT(about()));
          
  /* Issues Tab Actions */
  connect(pushButtonIssuesAcceptCardnumber, SIGNAL(clicked()),
          this, SLOT(issuesAcceptCardnumber()));
  connect(lineEditIssuesBorrowerCardnumber, SIGNAL(returnPressed()),
          this, SLOT(issuesAcceptCardnumber()));

  connect(pushButtonAddItemBarcode, SIGNAL(clicked()),
          this, SLOT(issuesAddItem()));
  connect(lineEditIssuesItemBarcode, SIGNAL(returnPressed()),
          this, SLOT(issuesAddItem()));

  connect(pushButtonIssuesDeleteSelectedItem, SIGNAL(clicked()),
          this, SLOT(issuesDeleteItemBarcode()));

  connect(buttonBoxIssues, SIGNAL(accepted()),
          this, SLOT(commitIssues()));
  connect(buttonBoxIssues, SIGNAL(rejected()),
          this, SLOT(cancelIssues()));

  connect(pushButtonPayFines, SIGNAL(clicked()),
          this, SLOT(issuesPayFine()));

  connect(pushButtonIssuesSearchBorrowers, SIGNAL(clicked()),
          this, SLOT(issuesSearchBorrowers()));

  /* Returns Tab Actions */
  connect(pushButtonReturnsAddItemBarcode, SIGNAL(clicked()),
          this, SLOT(returnsAddItem()));
  connect(lineEditReturnsItemBarcode, SIGNAL(returnPressed()),
          this, SLOT(returnsAddItem()));

  connect(pushButtonReturnsDeleteSelectedItem, SIGNAL(clicked()),
          this, SLOT(returnsDeleteItemBarcode()));

  connect(buttonBoxReturns, SIGNAL(accepted()),
          this, SLOT(commitReturns()));
  connect(buttonBoxReturns, SIGNAL(rejected()),
          this, SLOT(cancelReturns()));

  /* History Tab Actions */
  connect(pushButtonHistoryDeleteSelectedItem, SIGNAL(clicked()),
          this, SLOT(historyDeleteRow()));
}

MainWindow::~MainWindow()
{
}

/* Issues Related Functions */
void MainWindow::issuesAcceptCardnumber() {
  findBorrower();
  lineEditIssuesItemBarcode->setFocus();
}

void MainWindow::issuesAddItem() {
  QString itemBarcode = lineEditIssuesItemBarcode->text();

  if ( itemBarcode.isEmpty() ) {
    commitIssues();
  } else {
    listWidgetIssuesScannedBarcodes->addItem( itemBarcode );
    lineEditIssuesItemBarcode->clear();
    lineEditIssuesItemBarcode->setFocus();
  }
}

void MainWindow::issuesDeleteItemBarcode() {
  qDeleteAll(listWidgetIssuesScannedBarcodes->selectedItems());
  lineEditIssuesItemBarcode->setFocus();
}

void MainWindow::commitIssues() {
  while ( QListWidgetItem *item = listWidgetIssuesScannedBarcodes->takeItem(0) ) {
    QTableWidgetItem *borrowerCardnumber = new QTableWidgetItem( lineEditIssuesBorrowerCardnumber->text() );
    QTableWidgetItem *type = new QTableWidgetItem("issue");
    QTableWidgetItem *dateTime = new QTableWidgetItem( QDateTime::currentDateTime().toString( DATETIME_FORMAT ) );
    QTableWidgetItem *itemBarcode = new QTableWidgetItem( item->text() );

    int row = tableWidgetHistory->rowCount();

	tableWidgetHistory->insertRow(row);
	tableWidgetHistory->setItem(row, COLUMN_TYPE, type);
	tableWidgetHistory->setItem(row, COLUMN_CARDNUMBER, borrowerCardnumber);
	tableWidgetHistory->setItem(row, COLUMN_BARCODE, itemBarcode);
	tableWidgetHistory->setItem(row, COLUMN_DATE, dateTime);
  }

  cancelIssues();
  saveFile();
}

void MainWindow::cancelIssues() {
  lineEditIssuesItemBarcode->clear();
  lineEditIssuesBorrowerCardnumber->clear();
  listWidgetIssuesScannedBarcodes->clear();

  clearBorrowerDetails();

  lineEditIssuesBorrowerCardnumber->setFocus();
}

void MainWindow::clearBorrowerDetails() {

	borrowerDetailsName->setText( "" );
	borrowerDetailsAddress->setText( "" );
	borrowerDetailsPhone->setText( "" );
	borrowerDetailsDOB->setText( "" );
	borrowerDetailsFines->setText( "" );

	int rowCount = borrowerDetailsCurrentIssues->rowCount();
	for ( int row = 0; row < rowCount; row++ ) {
		borrowerDetailsCurrentIssues->removeRow( 0 );
	}
}

void MainWindow::issuesPayFine() {
	bool ok;
	QString paymentString = QInputDialog::getText(this, tr("Pay Fines") + " - " + TITLE,
                                             tr("Amount:"), QLineEdit::Normal,
                                             "0.00", &ok);
	if ( ok ) {
		float finePayment = paymentString.toFloat(&ok);
		if ( ok ) {
			paymentString.sprintf("%.2f", round(finePayment*100)/100);

			QTableWidgetItem *borrowerCardnumber = new QTableWidgetItem( lineEditIssuesBorrowerCardnumber->text() );
	    	QTableWidgetItem *type = new QTableWidgetItem("payment");
		    QTableWidgetItem *dateTime = new QTableWidgetItem( QDateTime::currentDateTime().toString( DATETIME_FORMAT ) );
	    	QTableWidgetItem *payment = new QTableWidgetItem( paymentString );

    		int row = tableWidgetHistory->rowCount();

			tableWidgetHistory->insertRow(row);
			tableWidgetHistory->setItem(row, COLUMN_TYPE, type);
			tableWidgetHistory->setItem(row, COLUMN_CARDNUMBER, borrowerCardnumber);
			tableWidgetHistory->setItem(row, COLUMN_PAYMENT, payment);
			tableWidgetHistory->setItem(row, COLUMN_DATE, dateTime);

			saveFile();
		} else {
			QMessageBox::warning(this, tr("Invalid Payment Amount"),
                                    tr("The payment amount was not a valid number.\nPlease try again."),
                                    QMessageBox::Ok);
			issuesPayFine();
		}
	}
}

void MainWindow::issuesSearchBorrowers() {
	BorrowerSearch *dialog = new BorrowerSearch( this );

	connect( dialog, SIGNAL( useBorrower( const QString & ) ),
		this, SLOT( useBorrower( const QString & ) ) );

//	dialog->setModal(true);
	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}

void MainWindow::useBorrower( const QString &borrowerCardnumber ) {
	lineEditIssuesBorrowerCardnumber->setText( borrowerCardnumber );
	issuesAcceptCardnumber();
}

/* Returns Related Functions */
void MainWindow::returnsAddItem() {
  QString itemBarcode = lineEditReturnsItemBarcode->text();

  if ( itemBarcode.isEmpty() ) {
    commitReturns();
  } else {
    listWidgetReturnsScannedBarcodes->addItem( itemBarcode );
    lineEditReturnsItemBarcode->clear();
    lineEditReturnsItemBarcode->setFocus();
  }
}

void MainWindow::returnsDeleteItemBarcode() {
  qDeleteAll(listWidgetReturnsScannedBarcodes->selectedItems());
  lineEditReturnsItemBarcode->setFocus();
}

void MainWindow::commitReturns() {
  while ( QListWidgetItem *item = listWidgetReturnsScannedBarcodes->takeItem(0) ) {
    QTableWidgetItem *type = new QTableWidgetItem("return");
    QTableWidgetItem *dateTime = new QTableWidgetItem( QDateTime::currentDateTime().toString( DATETIME_FORMAT ) );
    QTableWidgetItem *itemBarcode = new QTableWidgetItem( item->text() );

    int row = tableWidgetHistory->rowCount();

	tableWidgetHistory->insertRow(row);
	tableWidgetHistory->setItem(row, COLUMN_TYPE, type);
	tableWidgetHistory->setItem(row, COLUMN_BARCODE, itemBarcode);
	tableWidgetHistory->setItem(row, COLUMN_DATE, dateTime);
  }

  cancelReturns();
  saveFile();
}

void MainWindow::cancelReturns() {
  lineEditReturnsItemBarcode->clear();
  listWidgetReturnsScannedBarcodes->clear();

  lineEditReturnsItemBarcode->setFocus();
}

/* History Related Functions */
void MainWindow::historyDeleteRow() {
	QList<QTableWidgetItem *> selectedItems = tableWidgetHistory->selectedItems();

	QListIterator<QTableWidgetItem *> i(selectedItems);
	while ( i.hasNext() ) {
		QTableWidgetItem *item = i.next();

		if ( tableWidgetHistory->column(item) == COLUMN_DATE ) {
			int row = tableWidgetHistory->row(item);
			tableWidgetHistory->removeRow( row );
		}
	}

	saveFile();
}

void MainWindow::clearHistory() {
	int rowCount = tableWidgetHistory->rowCount();
	for ( int row = 0; row < rowCount; row++ ) {
		tableWidgetHistory->removeRow( 0 );
	}
}

/* File Related Functions */
void MainWindow::newFile()
{
  closeFile();
  saveFileAs();

  statusBar()->showMessage(tr("Starting new file."), 3000);
}

void MainWindow::closeFile()
{
  mFilePath = "";

  clearHistory();
  cancelReturns();
  cancelIssues();

  statusBar()->showMessage(tr("File closed."), 3000);
}

void MainWindow::loadFile()
{
  closeFile();

  QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 "",
                                                 tr("Koha Offline Circulation Files (*.koc)"));
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
    mFilePath = filename;

	QTextStream stream( &file );
	stream.readLine(); // Ignore the header line
	
	QString line = stream.readLine();
	while( !line.isEmpty() ) {

	    QStringList parts = line.split("\t");
		QString date = parts.takeFirst();
		QString type = parts.takeFirst();
	
	    int row = tableWidgetHistory->rowCount();
		tableWidgetHistory->insertRow(row);
		tableWidgetHistory->setItem(row, COLUMN_TYPE, new QTableWidgetItem(type));
		tableWidgetHistory->setItem(row, COLUMN_DATE, new QTableWidgetItem(date));

		if ( type == "issue" ) {
			QString cardnumber = parts.takeFirst();
			QString barcode = parts.takeFirst();
			tableWidgetHistory->setItem(row, COLUMN_CARDNUMBER, new QTableWidgetItem(cardnumber));
			tableWidgetHistory->setItem(row, COLUMN_BARCODE, new QTableWidgetItem(barcode));
		} else if ( type == "return" ) {
			QString barcode = parts.takeFirst();
			tableWidgetHistory->setItem(row, COLUMN_BARCODE, new QTableWidgetItem(barcode));
		} else if ( type == "payment" ) {
			QString cardnumber = parts.takeFirst();
			QString payment = parts.takeFirst();
			tableWidgetHistory->setItem(row, COLUMN_CARDNUMBER, new QTableWidgetItem(cardnumber));
			tableWidgetHistory->setItem(row, COLUMN_PAYMENT, new QTableWidgetItem(payment));
		}

		line = stream.readLine();
	}

	file.close();
    statusBar()->showMessage(tr("File successfully loaded."), 3000);
	this->setWindowTitle( TITLE + " - " + mFilePath );
  }
}

void MainWindow::saveFile()
{
  if(mFilePath.isEmpty()) 
    saveFileAs();
  else
    saveFile(mFilePath);
}

void MainWindow::saveFile(const QString &name)
{
  QFile file(name);

  if ( file.open(QIODevice::WriteOnly|QIODevice::Text) ) {
	QTextStream ts( &file );
    ts << "Version=" << FILE_VERSION << "\tGenerator=kocQt4\tGeneratorVersion=" << VERSION << endl;

	int rowCount = tableWidgetHistory->rowCount();

	for ( int row = 0; row < rowCount; row++ ) {

		QTableWidgetItem *type = tableWidgetHistory->item( row, COLUMN_TYPE );
		QString typeText = type->text();

		QTableWidgetItem *date = tableWidgetHistory->item( row, COLUMN_DATE );
		QString dateText = date->text();


		ts << dateText << "\t" << typeText << "\t";

		if ( typeText == "issue" ) {
			QTableWidgetItem *cardnumber = tableWidgetHistory->item( row, COLUMN_CARDNUMBER );
			QString cardnumberText = cardnumber->text();

			QTableWidgetItem *barcode = tableWidgetHistory->item( row, COLUMN_BARCODE );
			QString barcodeText = barcode->text();

			ts << cardnumberText << "\t" << barcodeText << endl;
		} else if ( typeText == "return" ) {
			QTableWidgetItem *barcode = tableWidgetHistory->item( row, COLUMN_BARCODE );
			QString barcodeText = barcode->text();

			ts << barcodeText << endl;
		} else if ( typeText == "payment" ) {
			QTableWidgetItem *cardnumber = tableWidgetHistory->item( row, COLUMN_CARDNUMBER );
			QString cardnumberText = cardnumber->text();

			QTableWidgetItem *payment = tableWidgetHistory->item( row, COLUMN_PAYMENT );
			QString paymentText = payment->text();

			ts << cardnumberText << "\t" << paymentText << endl;
		}

	}

	file.close();
    statusBar()->showMessage(tr("File saved successfully."), 3000);
  } else {
    statusBar()->showMessage(tr("Failed to save file."), 3000);
  }
}

void MainWindow::saveFileAs()
{
  mFilePath = QFileDialog::getSaveFileName(this, TITLE + " - " + tr("Save File"),
                            QDateTime::currentDateTime().toString( DATETIME_FORMAT ) + ".koc",
                            tr("Koha Offline Circulation Files (*.koc)"));
  if ( mFilePath.isEmpty() ) return;

  if ( ! mFilePath.endsWith( ".koc" ) ) mFilePath += ".koc";

  this->setWindowTitle( TITLE + " - " + mFilePath );

  saveFile(mFilePath);
}

void MainWindow::findBorrower() {
	qDebug() << "MainWindow::findBorrower()";

	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );

	db.setDatabaseName( "borrowers.db" );

	if ( db.open() ) {
		QString borrowerCardnumber = lineEditIssuesBorrowerCardnumber->text();

		/* Get Borrower Details */	
		QString borrowerQuery = "SELECT * FROM borrowers WHERE cardnumber = " + borrowerCardnumber;
		QSqlQuery query( borrowerQuery );
	
		qDebug() << "Borrower Search SQL: " + borrowerQuery;
	
		QSqlRecord record = query.record();
		query.next();
		QString borrowernumber = query.value(record.indexOf("borrowernumber")).toString();
		QString lastname = query.value(record.indexOf("surname")).toString();
		QString firstname = query.value(record.indexOf("firstname")).toString();
		QString dateofbirth = query.value(record.indexOf("dateofbirth")).toString();
		QString phone = query.value(record.indexOf("phone")).toString();
		QString streetaddress = query.value(record.indexOf("address")).toString();
		QString city = query.value(record.indexOf("city")).toString();
		QString state = query.value(record.indexOf("state")).toString();
		QString zipcode = query.value(record.indexOf("zipcode")).toString();
		QString total_fines = query.value(record.indexOf("total_fines")).toString();

		QString name = firstname + " " + lastname;
		QString address = streetaddress + "\n" + city + ", " + state + "\n" + zipcode;
	
		borrowerDetailsName->setText( name );
		borrowerDetailsAddress->setText( address );
		borrowerDetailsPhone->setText( phone );
		borrowerDetailsDOB->setText( dateofbirth );
		borrowerDetailsFines->setText( total_fines );

		/* Get Borrower's Previous Issues */
		QString issuesQuery = "SELECT * FROM issues WHERE borrowernumber = " + borrowernumber;
		QSqlQuery query2( issuesQuery );
	
		qDebug() << "Borrower Issues Search SQL: " + issuesQuery;
	
		record = query2.record();
		while (query2.next()) {
			QString itemcallnumber = query2.value(record.indexOf("itemcallnumber")).toString();
			QString itemtype = query2.value(record.indexOf("itemtype")).toString();
			QString title = query2.value(record.indexOf("title")).toString();
			QString date_due = query2.value(record.indexOf("date_due")).toString();

			addBorrowerPreviousIssue( itemcallnumber, itemtype, title, date_due );
		}

	}
}

void MainWindow::addBorrowerPreviousIssue( const QString & itemcallnumber, const QString & itemtype, const QString & title, const QString & datedue ) {

    QTableWidgetItem *itemcallnumberItem = new QTableWidgetItem( itemcallnumber );
    QTableWidgetItem *itemtypeItem = new QTableWidgetItem( itemtype );
    QTableWidgetItem *titleItem = new QTableWidgetItem( title );
    QTableWidgetItem *datedueItem = new QTableWidgetItem( datedue );

    int row = borrowerDetailsCurrentIssues->rowCount();

	borrowerDetailsCurrentIssues->insertRow(row);
	borrowerDetailsCurrentIssues->setItem(row, COLUMN_DATEDUE, datedueItem);
	borrowerDetailsCurrentIssues->setItem(row, COLUMN_TITLE, titleItem);
	borrowerDetailsCurrentIssues->setItem(row, COLUMN_ITEMTYPE, itemtypeItem);
	borrowerDetailsCurrentIssues->setItem(row, COLUMN_CALLNUMBER, itemcallnumberItem);

}

/* Help Related Functions */
void MainWindow::about()
{
  QMessageBox::about(this, tr("About Koha Offline Circulation"), 
					tr(	"Koha Offline Circulation 1.0.\n\n"
                                                "(c) 2010 Kyle Hall, Mill Run Technology Solutions\n\n"
                                                "http://millruntech.com/"
						)
	);
}

/* Settings Related Functions */
void MainWindow::writeSettings()
{
  QSettings settings;
  settings.setValue("MainWindow/Size", size());
  settings.setValue("MainWindow/Properties", saveState());
}

void MainWindow::readSettings()
{
  QSettings settings;
  resize(settings.value("MainWindow/Size", sizeHint()).toSize());
  restoreState(settings.value("MainWindow/Properties").toByteArray());
}

