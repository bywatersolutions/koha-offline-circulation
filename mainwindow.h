#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class QLabel;

class MainWindow : public QMainWindow, 
                   private Ui::MainWindow
{
  Q_OBJECT
  public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

  protected:
    void setupActions();

    void writeSettings();
    void readSettings();

    void saveFile(const QString&);

	void clearHistory();

  protected slots:
	/* File Related */
    void newFile();
	void closeFile();
    void loadFile();
    void saveFile();
    void saveFileAs();
    void about();

	/* Issues Related */
    void issuesAcceptCardnumber();
    void issuesAddItem();

	void issuesDeleteItemBarcode();

	void commitIssues();
	void cancelIssues();

	void issuesPayFine();

	/* Returns Related */
    void returnsAddItem();

	void returnsDeleteItemBarcode();

	void commitReturns();
	void cancelReturns();

	/* History Related */
	void historyDeleteRow();

  private:
    QString mFilePath;
    QLabel *mStatLabel;

  /* Private Constants */
  private:
	QString DATETIME_FORMAT;

	QString TITLE;
	QString VERSION;
	QString FILE_VERSION;

	static const int COLUMN_TYPE = 0;
	static const int COLUMN_CARDNUMBER = 1;
	static const int COLUMN_BARCODE = 2;
	static const int COLUMN_PAYMENT = 3;
	static const int COLUMN_DATE = 4;
};

#endif // MAINWINDOW_H
