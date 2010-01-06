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

	/* Returns Related */
    void returnsAddItem();

	void returnsDeleteItemBarcode();

	void commitReturns();
	void cancelReturns();

  private:
    QString mFilePath;
    QLabel *mStatLabel;

  private:
	QString DATETIME_FORMAT;
	int COLUMN_TYPE;
	int COLUMN_CARDNUMBER;
	int COLUMN_BARCODE;
	int COLUMN_PAYMENT;
	int COLUMN_DATE;
};

#endif // MAINWINDOW_H
