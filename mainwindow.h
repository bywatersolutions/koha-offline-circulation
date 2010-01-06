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

  protected:
    void writeSettings();
    void readSettings();

  protected:
    void saveFile(const QString&);

  protected slots:
	/* File Related */
    void newFile();
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

	static const int COLUMN_TYPE = 0;
	static const int COLUMN_CARDNUMBER = 1;
	static const int COLUMN_BARCODE = 2;
	static const int COLUMN_PAYMENT = 3;
	static const int COLUMN_DATE = 4;
};

#endif // MAINWINDOW_H
