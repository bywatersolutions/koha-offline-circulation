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
#include "kohalibraries.h"

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

    comboBoxBranchcode->lineEdit()->setPlaceholderText( tr("The branch transactions are recorded under, e.g. CPL") );
    comboBoxBranchcode->setEditText( settings.value("kohaBranchcode").toString() );

    // The list is only fetched on request, so opening the settings on
    // an offline machine doesn't hit the network
    mKohaLibraries = new KohaLibraries( this );
    connect( mKohaLibraries, SIGNAL( finished( bool, const QString & ) ),
             this, SLOT( librariesFetched( bool, const QString & ) ) );
    connect( pushButtonFetchLibraries, SIGNAL( clicked() ),
             this, SLOT( fetchLibraries() ) );
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
    // A picked entry reads "Name (CODE)", a typed one is the code itself
    int index = comboBoxBranchcode->findText( comboBoxBranchcode->currentText() );
    QString branchcode = index >= 0
        ? comboBoxBranchcode->itemData( index ).toString()
        : comboBoxBranchcode->currentText().trimmed().toUpper();
    settings.setValue( "kohaBranchcode", branchcode );
    settings.setValue( "kohaUploadPending", checkBoxUploadPending->isChecked() );

    settings.setValue( "kohaNightlyEnabled", checkBoxNightly->isChecked() );
    settings.setValue( "kohaNightlyTime", timeEditNightly->time().toString("HH:mm") );
    settings.setValue( "kohaDownloadOnLaunch", checkBoxOnLaunch->isChecked() );

    QDialog::accept();
}

void KohaSettingsDialog::fetchLibraries()
{
    QString baseUrl = lineEditBaseUrl->text().trimmed();
    if ( baseUrl.isEmpty() ) {
        QMessageBox::information(this, tr("Fetch Libraries"),
                                 tr("Enter the Koha staff interface URL first."));
        return;
    }

    pushButtonFetchLibraries->setEnabled( false );
    labelLibrariesStatus->setText( tr("Contacting Koha...") );

    mKohaLibraries->start( baseUrl );
}

void KohaSettingsDialog::librariesFetched( bool ok, const QString & message )
{
    pushButtonFetchLibraries->setEnabled( true );

    if ( ! ok ) {
        labelLibrariesStatus->clear();
        QMessageBox::warning(this, tr("Fetch Libraries"), message);
        return;
    }

    // Keep whatever was already picked or typed selected in the new list
    QString current = comboBoxBranchcode->currentText().trimmed();
    int currentIndex = comboBoxBranchcode->findText( current );
    if ( currentIndex >= 0 ) current = comboBoxBranchcode->itemData( currentIndex ).toString();

    comboBoxBranchcode->clear();
    for ( const KohaLibrary & library : mKohaLibraries->libraries() ) {
        comboBoxBranchcode->addItem( library.name + " (" + library.code + ")", library.code );
    }

    int index = comboBoxBranchcode->findData( current, Qt::UserRole, Qt::MatchFixedString );
    if ( index >= 0 ) {
        comboBoxBranchcode->setCurrentIndex( index );
    } else {
        // Nothing chosen yet shouldn't silently become the first library
        comboBoxBranchcode->setCurrentIndex( -1 );
        comboBoxBranchcode->setEditText( current );
    }

    labelLibrariesStatus->setText( message );
}
