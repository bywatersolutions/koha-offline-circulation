/*
* Copyright 2026 Kyle M Hall <kyle@bywatersolutions.com>
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

#ifndef KOHASETTINGSDIALOG_H
#define KOHASETTINGSDIALOG_H

#include <QDialog>
#include "ui_kohasettingsdialog.h"

class KohaSettingsDialog : public QDialog,
                           private Ui::KohaSettingsDialog
{
    Q_OBJECT
    public:
        KohaSettingsDialog(QWidget *parent = 0);

    protected slots:
        void accept();
};

#endif // KOHASETTINGSDIALOG_H
