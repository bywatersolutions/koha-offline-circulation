#include <QtGui>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent)
{
  DATETIME_FORMAT = "yyyy-MM-dd hh:mm:ss zzz";
  COLUMN_TYPE = 0;
  COLUMN_CARDNUMBER = 1;
  COLUMN_BARCODE = 2;
  COLUMN_PAYMENT = 3;
  COLUMN_DATE = 4;

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
	/* Should be equivilent to hitting the "OK" button */
    lineEditIssuesItemBarcode->setFocus();
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
/*
    int row = filesTable->rowCount();
    filesTable->insertRow(row);
    filesTable->setItem(row, 0, fileNameItem);
    filesTable->setItem(row, 1, sizeItem);
*/
  }
}

void MainWindow::cancelIssues() {
  lineEditIssuesItemBarcode->clear();
  lineEditIssuesBorrowerCardnumber->clear();
  listWidgetIssuesScannedBarcodes->clear();

  lineEditIssuesBorrowerCardnumber->setFocus();
}

/* Returns Related Functions */
void MainWindow::returnsAddItem() {
  QString itemBarcode = lineEditReturnsItemBarcode->text();

  if ( itemBarcode.isEmpty() ) {
	/* Should be equivilent to hitting the "OK" button */
    lineEditReturnsItemBarcode->setFocus();
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

}

void MainWindow::cancelReturns() {
  lineEditReturnsItemBarcode->clear();
  listWidgetReturnsScannedBarcodes->clear();

  lineEditReturnsItemBarcode->setFocus();
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

