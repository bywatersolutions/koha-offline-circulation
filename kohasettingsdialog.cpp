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

#include <QtWidgets>

#include "kohasettingsdialog.h"

KohaSettingsDialog::KohaSettingsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);

    QSettings settings;
    lineEditBaseUrl->setText( settings.value("kohaBaseUrl").toString() );
    lineEditUserid->setText( settings.value("kohaUserid").toString() );
    lineEditPassword->setText( settings.value("kohaPassword").toString() );

    bool useReports = settings.value("kohaUseReports", true).toBool();
    radioButtonReports->setChecked( useReports );
    radioButtonRest->setChecked( ! useReports );
    spinBoxBorrowersReport->setValue( settings.value("kohaBorrowersReportId", 0).toInt() );
    spinBoxIssuesReport->setValue( settings.value("kohaIssuesReportId", 0).toInt() );

    lineEditBranchcode->setText( settings.value("kohaBranchcode").toString() );
    checkBoxUploadPending->setChecked( settings.value("kohaUploadPending", false).toBool() );

    checkBoxNightly->setChecked( settings.value("kohaNightlyEnabled", false).toBool() );
    timeEditNightly->setTime( QTime::fromString( settings.value("kohaNightlyTime", "22:00").toString(), "HH:mm" ) );
    checkBoxOnLaunch->setChecked( settings.value("kohaDownloadOnLaunch", false).toBool() );
}

void KohaSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue( "kohaBaseUrl", lineEditBaseUrl->text().trimmed() );
    settings.setValue( "kohaUserid", lineEditUserid->text().trimmed() );
    settings.setValue( "kohaPassword", lineEditPassword->text() );
    settings.setValue( "kohaUseReports", radioButtonReports->isChecked() );
    settings.setValue( "kohaBorrowersReportId", spinBoxBorrowersReport->value() );
    settings.setValue( "kohaIssuesReportId", spinBoxIssuesReport->value() );
    settings.setValue( "kohaBranchcode", lineEditBranchcode->text().trimmed().toUpper() );
    settings.setValue( "kohaUploadPending", checkBoxUploadPending->isChecked() );

    settings.setValue( "kohaNightlyEnabled", checkBoxNightly->isChecked() );
    settings.setValue( "kohaNightlyTime", timeEditNightly->time().toString("HH:mm") );
    settings.setValue( "kohaDownloadOnLaunch", checkBoxOnLaunch->isChecked() );

    QDialog::accept();
}
