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

#include "mainwindow.h"
#include "borrowersearch.h"
#include "kocfile.h"
#include "kohadownload.h"
#include "kohasettingsdialog.h"

MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent)
{
  TITLE = "Koha Offline Circulation";
  VERSION = KOC_VERSION;
  FILE_VERSION = "1.0";
  DATETIME_FORMAT = "yyyy-MM-dd hh-mm-ss zzz";

  setupUi(this);
  setupActions();

  // The stock "OK" label doesn't say what these buttons do
  buttonBoxIssues->button( QDialogButtonBox::Ok )->setText( tr("Commit") );
  buttonBoxReturns->button( QDialogButtonBox::Ok )->setText( tr("Commit") );

  mRecentFilesMenu = new QMenu( this );
  actionOpen_Recent->setMenu( mRecentFilesMenu );
  updateRecentFilesMenu();

  mKohaDownload = new KohaDownload( this );
  connect( mKohaDownload, SIGNAL( progress( const QString & ) ),
           this, SLOT( kohaDownloadProgress( const QString & ) ) );
  connect( mKohaDownload, SIGNAL( finished( bool, const QString & ) ),
           this, SLOT( kohaDownloadFinished( bool, const QString & ) ) );
  mDownloadProgress = 0;
  mDownloadRunning = false;
  mDownloadInteractive = false;

  mStatLabel = new QLabel;
  statusBar()->addPermanentWidget(mStatLabel);

  readSettings();
  lineEditIssuesBorrowerCardnumber->setFocus();

  QSettings settings;
  borrowersDbFilePath = settings.value("borrowersDbFilePath").toString();
  defaultKocSavePath = settings.value("defaultKocSavePath").toString();
  if ( ! defaultKocSavePath.isEmpty() ) {
      mFilePath = defaultKocSavePath + "/" + QDateTime::currentDateTime().toString( DATETIME_FORMAT ) + ".koc";
      this->setWindowTitle( TITLE + " - " + mFilePath );
  }

  updateSettingsDisplay();
}

void MainWindow::setupActions()
{
  /* File Menu Actions */
  // Quit via close() rather than qApp->quit() so closeEvent fires
  // and the window geometry gets saved
  connect(actionQuit, SIGNAL(triggered(bool)),
          this, SLOT(close()));
  actionQuit->setShortcut(tr("Ctrl+Q"));

  connect(actionOpen, SIGNAL(triggered(bool)),
          this, SLOT(loadFile()));
  actionOpen->setShortcut(tr("Ctrl+O"));

  connect(actionSave, SIGNAL(triggered(bool)),
          this, SLOT(saveFile()));
  actionSave->setShortcut(tr("Ctrl+S"));

  connect(actionSaveAs, SIGNAL(triggered(bool)),
          this, SLOT(saveFileAs()));
  actionSaveAs->setShortcut(tr("Ctrl+Shift+S"));

  connect(actionClose, SIGNAL(triggered(bool)),
          this, SLOT(closeFile()));
  actionClose->setShortcut(tr("Ctrl+W"));

  connect(actionNew, SIGNAL(triggered(bool)),
          this, SLOT(newFile()));
  actionNew->setShortcut(tr("Ctrl+N"));

  /* Settings Menu Actions */
  connect(actionSelectBorrowersDB, SIGNAL(triggered(bool)),
          this, SLOT(selectBorrowersDbFile()));
  connect(actionSet_Default_KOC_Save_Path, SIGNAL(triggered(bool)),
          this, SLOT(selectDefaultKocSavePath()));
  connect(actionKohaConnectionSettings, SIGNAL(triggered(bool)),
          this, SLOT(showKohaSettings()));
  connect(actionDownloadBorrowersDB, SIGNAL(triggered(bool)),
          this, SLOT(downloadBorrowersDb()));

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

MainWindow::~MainWindow(){}

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
  // Refuse to commit scanned items without a cardnumber, Koha can't
  // process transactions that have no borrower
  if ( lineEditIssuesBorrowerCardnumber->text().isEmpty()
       && listWidgetIssuesScannedBarcodes->count() > 0 ) {
    QMessageBox::warning(this, tr("No Borrower Cardnumber"),
                         tr("These items cannot be committed without a borrower cardnumber.\n"
                            "Enter or search for the borrower's cardnumber first."),
                         QMessageBox::Ok);
    lineEditIssuesBorrowerCardnumber->setFocus();
    return;
  }

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
	// A payment without a cardnumber can't be processed by Koha
	if ( lineEditIssuesBorrowerCardnumber->text().isEmpty() ) {
		QMessageBox::warning(this, tr("No Borrower Cardnumber"),
							tr("A payment cannot be recorded without a borrower cardnumber.\n"
								"Enter or search for the borrower's cardnumber first."),
							QMessageBox::Ok);
		lineEditIssuesBorrowerCardnumber->setFocus();
		return;
	}

	bool ok;
	QString paymentString = QInputDialog::getText(this, tr("Pay Fines") + " - " + TITLE,
                                             tr("Amount:"), QLineEdit::Normal,
                                             "0.00", &ok);
	if ( ok ) {
		float finePayment = paymentString.toFloat(&ok);
		if ( ok ) {
			paymentString = KocFile::formatPayment( finePayment );

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

    updateReturnsCount();
  }
}

void MainWindow::returnsDeleteItemBarcode() {
  qDeleteAll(listWidgetReturnsScannedBarcodes->selectedItems());
  lineEditReturnsItemBarcode->setFocus();

  updateReturnsCount();
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

  updateReturnsCount();

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
  if ( ! closeFile() ) return;

  saveFileAs();

  statusBar()->showMessage(tr("Starting new file."), 3000);
}

bool MainWindow::closeFile()
{
  // Transactions are saved to disk as they're committed, but confirm
  // anyway in case the last save failed
  if ( tableWidgetHistory->rowCount() > 0 ) {
      int ret = QMessageBox::question(this, tr("Close File"),
                                      tr("Close the current file? The transaction list will be cleared."),
                                      QMessageBox::Yes | QMessageBox::No);
      if ( ret != QMessageBox::Yes ) return false;
  }

  mFilePath = "";

  clearHistory();
  cancelReturns();
  cancelIssues();

  statusBar()->showMessage(tr("File closed."), 3000);

  return true;
}

void MainWindow::loadFile()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 "",
                                                 tr("Koha Offline Circulation Files (*.koc)"));
  if ( filename.isEmpty() ) return;

  loadFile( filename );
}

void MainWindow::loadFile(const QString &filename)
{
  if ( ! closeFile() ) return;

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
    mFilePath = filename;

	QTextStream stream( &file );
	stream.readLine(); // Ignore the header line
	
	QString line = stream.readLine();
	while( !line.isEmpty() ) {

		KocTransaction transaction = KocFile::parseLine( line );

	    int row = tableWidgetHistory->rowCount();
		tableWidgetHistory->insertRow(row);
		tableWidgetHistory->setItem(row, COLUMN_TYPE, new QTableWidgetItem(transaction.type));
		tableWidgetHistory->setItem(row, COLUMN_DATE, new QTableWidgetItem(transaction.date));

		if ( transaction.type == "issue" ) {
			tableWidgetHistory->setItem(row, COLUMN_CARDNUMBER, new QTableWidgetItem(transaction.cardnumber));
			tableWidgetHistory->setItem(row, COLUMN_BARCODE, new QTableWidgetItem(transaction.barcode));
		} else if ( transaction.type == "return" ) {
			tableWidgetHistory->setItem(row, COLUMN_BARCODE, new QTableWidgetItem(transaction.barcode));
		} else if ( transaction.type == "payment" ) {
			tableWidgetHistory->setItem(row, COLUMN_CARDNUMBER, new QTableWidgetItem(transaction.cardnumber));
			tableWidgetHistory->setItem(row, COLUMN_PAYMENT, new QTableWidgetItem(transaction.payment));
		}

		line = stream.readLine();
	}

	file.close();
    statusBar()->showMessage(tr("File successfully loaded."), 3000);
	this->setWindowTitle( TITLE + " - " + mFilePath );

	addRecentFile( filename );
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
    ts << KocFile::headerLine( FILE_VERSION, VERSION ) << Qt::endl;

	int rowCount = tableWidgetHistory->rowCount();

	for ( int row = 0; row < rowCount; row++ ) {

		KocTransaction transaction;
		transaction.type = tableWidgetHistory->item( row, COLUMN_TYPE )->text();
		transaction.date = tableWidgetHistory->item( row, COLUMN_DATE )->text();

		if ( transaction.type == "issue" ) {
			transaction.cardnumber = tableWidgetHistory->item( row, COLUMN_CARDNUMBER )->text();
			transaction.barcode = tableWidgetHistory->item( row, COLUMN_BARCODE )->text();
		} else if ( transaction.type == "return" ) {
			transaction.barcode = tableWidgetHistory->item( row, COLUMN_BARCODE )->text();
		} else if ( transaction.type == "payment" ) {
			transaction.cardnumber = tableWidgetHistory->item( row, COLUMN_CARDNUMBER )->text();
			transaction.payment = tableWidgetHistory->item( row, COLUMN_PAYMENT )->text();
		}

		ts << KocFile::serializeLine( transaction ) << Qt::endl;
	}

	file.close();
    statusBar()->showMessage(tr("File saved successfully."), 3000);
  } else {
    statusBar()->showMessage(tr("Failed to save file."), 3000);

    QMessageBox::critical(this, tr("Failed to Save File"),
                          tr("Could not write the file:\n%1\n\n%2\n\n"
                             "Transactions are not being saved! "
                             "Check the drive and use File, Save As to pick a working location.")
                              .arg(name, file.errorString()));
  }
}

void MainWindow::saveFileAs()
{
  // Start the dialog in the default KOC save path if one is set,
  // otherwise it opens in the working directory ( Program Files on Windows )
  QString suggestedFilePath = QDateTime::currentDateTime().toString( DATETIME_FORMAT ) + ".koc";
  if ( ! defaultKocSavePath.isEmpty() ) {
      suggestedFilePath = defaultKocSavePath + "/" + suggestedFilePath;
  }

  mFilePath = QFileDialog::getSaveFileName(this, TITLE + " - " + tr("Save File"),
                            suggestedFilePath,
                            tr("Koha Offline Circulation Files (*.koc)"));
  if ( mFilePath.isEmpty() ) return;

  if ( ! mFilePath.endsWith( ".koc" ) ) mFilePath += ".koc";

  this->setWindowTitle( TITLE + " - " + mFilePath );

  addRecentFile( mFilePath );

  saveFile(mFilePath);
}

void MainWindow::addRecentFile( const QString &path )
{
  QSettings settings;
  QStringList recentFiles = settings.value("recentFiles").toStringList();

  recentFiles.removeAll( path );
  recentFiles.prepend( path );
  while ( recentFiles.count() > 10 ) recentFiles.removeLast();

  settings.setValue("recentFiles", recentFiles);

  updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
  QSettings settings;
  QStringList recentFiles = settings.value("recentFiles").toStringList();

  mRecentFilesMenu->clear();
  for ( const QString &path : recentFiles ) {
      QAction *action = mRecentFilesMenu->addAction( path );
      action->setData( path );
      connect( action, SIGNAL(triggered(bool)),
               this, SLOT(openRecentFile()) );
  }

  actionOpen_Recent->setEnabled( ! recentFiles.isEmpty() );
}

void MainWindow::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( ! action ) return;

  QString path = action->data().toString();

  if ( ! QFile::exists( path ) ) {
      QMessageBox::warning(this, tr("File Not Found"),
                           tr("This file no longer exists:\n%1").arg( path ),
                           QMessageBox::Ok);

      QSettings settings;
      QStringList recentFiles = settings.value("recentFiles").toStringList();
      recentFiles.removeAll( path );
      settings.setValue("recentFiles", recentFiles);

      updateRecentFilesMenu();

      return;
  }

  loadFile( path );
}

void MainWindow::selectBorrowersDbFile() {
    qDebug() << "MainWindow::selectBorrowersDbFile()";

    QString selectedFile = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                       "",
                                                       tr("KOC Borrowers DB Files (*.db)"));

    // Cancelling the dialog returns an empty string, don't wipe out the stored path
    if ( selectedFile.isEmpty() ) return;

    borrowersDbFilePath = selectedFile;

    QSettings settings;
    settings.setValue("borrowersDbFilePath", borrowersDbFilePath);

    updateSettingsDisplay();

    qDebug() << "Borrowers DB File Select: " + borrowersDbFilePath;

    statusBar()->showMessage(tr("Borrowers DB File Selected: ") + borrowersDbFilePath, 3000);
}

void MainWindow::selectDefaultKocSavePath() {
    qDebug() << "MainWindow::selectDefaultKocSavePath()";

    QString startPath = defaultKocSavePath.isEmpty() ? QDir::homePath() : defaultKocSavePath;
    QString selectedPath = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                       startPath,
                                                       QFileDialog::ShowDirsOnly);

    // Cancelling the dialog returns an empty string, don't wipe out the stored path
    if ( selectedPath.isEmpty() ) return;

    defaultKocSavePath = selectedPath;

    QSettings settings;
    settings.setValue("defaultKocSavePath", defaultKocSavePath);

    updateSettingsDisplay();

    // Use the new path immediately if the current file has never been saved,
    // otherwise it wouldn't take effect until the next launch
    if ( mFilePath.isEmpty() || ! QFile::exists( mFilePath ) ) {
        mFilePath = defaultKocSavePath + "/" + QDateTime::currentDateTime().toString( DATETIME_FORMAT ) + ".koc";
        this->setWindowTitle( TITLE + " - " + mFilePath );
    }

    qDebug() << "Default KOC Save Path: " + defaultKocSavePath;

    statusBar()->showMessage(tr("Default KOC Save Path Selected: ") + defaultKocSavePath, 3000);
}

void MainWindow::showKohaSettings() {
    KohaSettingsDialog dialog( this );
    dialog.exec();
}

void MainWindow::downloadBorrowersDb() {
    startKohaDownload( true );
}

void MainWindow::startKohaDownload( bool interactive )
{
    if ( mDownloadRunning ) return;

    QSettings settings;

    KohaDownload::Config config;
    config.baseUrl = settings.value("kohaBaseUrl").toString();
    config.userid = settings.value("kohaUserid").toString();
    config.password = settings.value("kohaPassword").toString();
    config.useReports = settings.value("kohaUseReports", true).toBool();
    config.borrowersReportId = settings.value("kohaBorrowersReportId").toInt();
    config.issuesReportId = settings.value("kohaIssuesReportId").toInt();

    bool configured = ! config.baseUrl.isEmpty() && ! config.userid.isEmpty()
        && ( ! config.useReports || ( config.borrowersReportId > 0 && config.issuesReportId > 0 ) );

    if ( ! configured ) {
        if ( interactive ) {
            QMessageBox::information(this, tr("Koha Connection Not Configured"),
                                     tr("Set the Koha staff URL, credentials, and download method first."));
            showKohaSettings();
        }
        return;
    }

    mDownloadRunning = true;
    mDownloadInteractive = interactive;
    mDownloadTargetPath = borrowersDbTargetPath();

    if ( interactive ) {
        // Range 0,0 shows a busy indicator, there's no meaningful total
        mDownloadProgress = new QProgressDialog( tr("Contacting Koha..."), QString(), 0, 0, this );
        mDownloadProgress->setWindowModality( Qt::WindowModal );
        mDownloadProgress->setMinimumDuration( 0 );
        mDownloadProgress->setValue( 0 );
    }

    statusBar()->showMessage( tr("Downloading borrowers database from Koha...") );

    mKohaDownload->start( config, mDownloadTargetPath );
}

QString MainWindow::borrowersDbTargetPath()
{
    if ( ! borrowersDbFilePath.isEmpty() ) return borrowersDbFilePath;

    QString dir = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
    QDir().mkpath( dir );

    return dir + "/borrowers.db";
}

void MainWindow::kohaDownloadProgress( const QString & message )
{
    if ( mDownloadProgress ) mDownloadProgress->setLabelText( message );

    statusBar()->showMessage( message );
}

void MainWindow::kohaDownloadFinished( bool ok, const QString & message )
{
    mDownloadRunning = false;

    if ( mDownloadProgress ) {
        mDownloadProgress->close();
        mDownloadProgress->deleteLater();
        mDownloadProgress = 0;
    }

    if ( ok ) {
        // Remember the path so the lookups and the search dialog find it
        if ( borrowersDbFilePath.isEmpty() ) {
            borrowersDbFilePath = mDownloadTargetPath;
            QSettings settings;
            settings.setValue("borrowersDbFilePath", borrowersDbFilePath);
        }

        // The lookup connection may still have the replaced file open,
        // close it so the next lookup reopens the fresh database
        if ( QSqlDatabase::contains() ) {
            QSqlDatabase::database( QSqlDatabase::defaultConnection, false ).close();
        }

        updateSettingsDisplay();

        statusBar()->showMessage( message, 5000 );
        if ( mDownloadInteractive ) {
            QMessageBox::information(this, tr("Download Complete"), message);
        }
    } else {
        statusBar()->showMessage( tr("Koha download failed."), 5000 );
        if ( mDownloadInteractive ) {
            QMessageBox::critical(this, tr("Download Failed"), message);
        }
    }
}


void MainWindow::findBorrower() {
    qDebug() << "MainWindow::findBorrower()";
    qDebug() << "Using Borrowers DB File: " + borrowersDbFilePath;

    // Clear the previous borrower first so a failed lookup can't leave
    // the last borrower's details displayed next to the new cardnumber
    clearBorrowerDetails();

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
		QString borrowerCardnumber = lineEditIssuesBorrowerCardnumber->text();

		/* Get Borrower Details */
		// Bind the cardnumber rather than concatenating it into the SQL,
		// values containing a quote broke the query
		QSqlQuery query( db );
		query.prepare( "SELECT * FROM borrowers WHERE cardnumber = ?" );
		query.addBindValue( borrowerCardnumber );
		query.exec();

		QSqlRecord record = query.record();
		if ( ! query.next() ) {
			borrowerDetailsName->setText( tr("( no borrower found )") );
			statusBar()->showMessage( tr("No borrower found with cardnumber ") + borrowerCardnumber, 5000 );
			return;
		}
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

		// Koha's create_koc_db.pl doesn't export state or zipcode, skip
		// empty parts so the address doesn't show dangling separators
		QString cityLine = city;
		if ( ! state.isEmpty() ) cityLine += ", " + state;

		QStringList addressParts;
		if ( ! streetaddress.isEmpty() ) addressParts << streetaddress;
		if ( ! cityLine.isEmpty() ) addressParts << cityLine;
		if ( ! zipcode.isEmpty() ) addressParts << zipcode;
		QString address = addressParts.join( "\n" );
	
		borrowerDetailsName->setText( name );
		borrowerDetailsAddress->setText( address );
		borrowerDetailsPhone->setText( phone );
		borrowerDetailsDOB->setText( dateofbirth );
		borrowerDetailsFines->setText( total_fines );

		/* Get Borrower's Previous Issues */
		QSqlQuery query2( db );
		query2.prepare( "SELECT * FROM issues WHERE borrowernumber = ?" );
		query2.addBindValue( borrowernumber );
		query2.exec();

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
                                        TITLE + " " + VERSION + ".\n\n"
                                                "(c) 2010 Kyle M Hall\n\n"
                                                "http://kylehall.info/"
	);
}

/* Settings Related Functions */
void MainWindow::writeSettings()
{
  QSettings settings;
  settings.setValue("MainWindow/Geometry", saveGeometry());
  settings.setValue("MainWindow/Properties", saveState());
}

void MainWindow::readSettings()
{
  QSettings settings;

  // Restore the window size and position from the last run,
  // fall back to maximized on the first run
  if ( settings.contains("MainWindow/Geometry") ) {
      restoreGeometry(settings.value("MainWindow/Geometry").toByteArray());
  } else {
      this->showMaximized();
  }

  restoreState(settings.value("MainWindow/Properties").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  writeSettings();

  QMainWindow::closeEvent(event);
}

void MainWindow::updateReturnsCount()
{
  labelReturnsCount->setText( tr("Items scanned: %1").arg( listWidgetReturnsScannedBarcodes->count() ) );
}

void MainWindow::updateSettingsDisplay()
{
  QString kocPath = defaultKocSavePath.isEmpty() ? tr("not set") : defaultKocSavePath;
  QString dbPath = borrowersDbFilePath.isEmpty() ? tr("not set") : borrowersDbFilePath;

  mStatLabel->setText( tr("KOC save path: %1  |  Borrowers DB: %2").arg( kocPath, dbPath ) );
}

