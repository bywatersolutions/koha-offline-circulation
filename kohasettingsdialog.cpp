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
#include "credentialstore.h"

KohaSettingsDialog::KohaSettingsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);

    QSettings settings;
    lineEditBaseUrl->setText( settings.value("kohaBaseUrl").toString() );
    lineEditUserid->setText( settings.value("kohaUserid").toString() );
    // Secrets aren't read back from the keychain here, opening the
    // dialog shouldn't trigger keychain access prompts. Blank means
    // keep the stored value.
    lineEditPassword->setPlaceholderText( tr("Leave blank to keep the stored password") );
    lineEditClientSecret->setPlaceholderText( tr("Leave blank to keep the stored secret") );

    checkBoxUseToken->setChecked( settings.value("kohaUseToken", false).toBool() );
    lineEditClientId->setText( settings.value("kohaClientId").toString() );

    QString method = settings.value("kohaDownloadMethod").toString();
    if ( method.isEmpty() ) {
        // The pre plugin releases stored the method as a boolean
        method = settings.value("kohaUseReports", true).toBool() ? "reports" : "rest";
    }
    radioButtonPlugin->setChecked( method == "plugin" );
    radioButtonReports->setChecked( method == "reports" );
    radioButtonRest->setChecked( method == "rest" );
    spinBoxBorrowersReport->setValue( settings.value("kohaBorrowersReportId", 0).toInt() );
    spinBoxIssuesReport->setValue( settings.value("kohaIssuesReportId", 0).toInt() );

    lineEditBranchcode->setText( settings.value("kohaBranchcode").toString() );
    checkBoxUploadPending->setChecked( settings.value("kohaUploadPending", true).toBool() );

    checkBoxNightly->setChecked( settings.value("kohaNightlyEnabled", false).toBool() );
    timeEditNightly->setTime( QTime::fromString( settings.value("kohaNightlyTime", "22:00").toString(), "HH:mm" ) );
    checkBoxOnLaunch->setChecked( settings.value("kohaDownloadOnLaunch", false).toBool() );

    // Size to the content rather than the fixed size from the ui file,
    // which squeezes the form rows whenever the dialog grows a field
    adjustSize();
    setMinimumSize( sizeHint() );
}

void KohaSettingsDialog::accept()
{
    QSettings settings;
    settings.setValue( "kohaBaseUrl", lineEditBaseUrl->text().trimmed() );
    settings.setValue( "kohaUserid", lineEditUserid->text().trimmed() );
    if ( ! lineEditPassword->text().isEmpty() ) {
        CredentialStore::write( "kohaPassword", lineEditPassword->text() );
    }
    settings.setValue( "kohaUseToken", checkBoxUseToken->isChecked() );
    settings.setValue( "kohaClientId", lineEditClientId->text().trimmed() );
    if ( ! lineEditClientSecret->text().isEmpty() ) {
        CredentialStore::write( "kohaClientSecret", lineEditClientSecret->text() );
    }
    settings.setValue( "kohaDownloadMethod",
                       radioButtonPlugin->isChecked()  ? "plugin"
                       : radioButtonReports->isChecked() ? "reports"
                                                         : "rest" );
    settings.setValue( "kohaBorrowersReportId", spinBoxBorrowersReport->value() );
    settings.setValue( "kohaIssuesReportId", spinBoxIssuesReport->value() );
    settings.setValue( "kohaBranchcode", lineEditBranchcode->text().trimmed().toUpper() );
    settings.setValue( "kohaUploadPending", checkBoxUploadPending->isChecked() );

    settings.setValue( "kohaNightlyEnabled", checkBoxNightly->isChecked() );
    settings.setValue( "kohaNightlyTime", timeEditNightly->time().toString("HH:mm") );
    settings.setValue( "kohaDownloadOnLaunch", checkBoxOnLaunch->isChecked() );

    QDialog::accept();
}
