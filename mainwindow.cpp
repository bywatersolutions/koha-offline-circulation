#include <QtGui>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent)
{
  DATETIME_FORMAT = "yyyy-MM-dd hh:mm:ss zzz";

  setupUi(this);
  setupActions();

  mStatLabel = new QLabel;
  statusBar()->addPermanentWidget(mStatLabel);

  lineEditIssuesBorrowerCardnumber->setFocus();
}

void MainWindow::setupActions()
{
  /* File Menu Actions */
  connect(actionQuit, SIGNAL(triggered(bool)),
          qApp, SLOT(quit()));
  connect(actionOpen, SIGNAL(triggered(bool)),
          this, SLOT(loadFile()));
  connect(actionSave, SIGNAL(triggered(bool)),
          this, SLOT(saveFile()));
  connect(actionSaveAs, SIGNAL(triggered(bool)),
          this, SLOT(saveFileAs()));

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
}

void MainWindow::cancelIssues() {
  lineEditIssuesItemBarcode->clear();
  lineEditIssuesBorrowerCardnumber->clear();
  listWidgetIssuesScannedBarcodes->clear();

  lineEditIssuesBorrowerCardnumber->setFocus();
}

void MainWindow::issuesPayFine() {
	bool ok;
	QString paymentString = QInputDialog::getText(this, tr("Fine Payment: Amount To Pay"),
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
		} else {
			QMessageBox::warning(this, tr("Invalid Payment Amount"),
                                    tr("The payment amount was not a valid number.\nPlease try again."),
                                    QMessageBox::Ok);
			issuesPayFine();
		}
	}
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
}

/* File Related Functions */
void MainWindow::newFile()
{
  mFilePath = "";
}

void MainWindow::loadFile()
{
  QString filename = QFileDialog::getOpenFileName(this);
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
    mFilePath = filename;
    statusBar()->showMessage(tr("File successfully loaded."), 3000);
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
  if (file.open(QIODevice::WriteOnly|QIODevice::Text)) {
    statusBar()->showMessage(tr("File saved successfully."), 3000);
  }
}

void MainWindow::saveFileAs()
{
  mFilePath = QFileDialog::getSaveFileName(this);
  if(mFilePath.isEmpty())
    return;
  saveFile(mFilePath);
}

/* Help Related Functions */
void MainWindow::about()
{
  QMessageBox::about(this, tr("About Koha Offline Circulation"), 
		tr("Koha Offline Circulation 1.0.\n"
		   "(c) 2010 Kyle Hall, Mill Run Technology Solutions"));
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

