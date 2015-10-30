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

        void findBorrower();
        void addBorrowerPreviousIssue( const QString & itemcallnumber, const QString & itemtype, const QString & title, const QString & datedue );
        void clearBorrowerDetails();

    protected slots:
        /* File Related */
        void newFile();
        void closeFile();
        void loadFile();
        void saveFile();
        void saveFileAs();
        void selectBorrowersDbFile();
        void selectDefaultKocSavePath();
        void about();

        /* Issues Related */
        void issuesAcceptCardnumber();
        void issuesAddItem();

        void issuesDeleteItemBarcode();

        void commitIssues();
        void cancelIssues();

        void issuesPayFine();

        void issuesSearchBorrowers();
        void useBorrower( const QString & );

        /* Returns Related */
        void returnsAddItem();

        void returnsDeleteItemBarcode();

        void commitReturns();
        void cancelReturns();

        /* History Related */
        void historyDeleteRow();

    private:
        QString mFilePath;
        QString defaultKocSavePath;
        QLabel *mStatLabel;

        QString borrowersDbFilePath;

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

	static const int COLUMN_DATEDUE = 0;
	static const int COLUMN_TITLE = 1;
	static const int COLUMN_ITEMTYPE = 2;
	static const int COLUMN_CALLNUMBER = 3;
};

#endif // MAINWINDOW_H
