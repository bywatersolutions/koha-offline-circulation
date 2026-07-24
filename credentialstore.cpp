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

#include "credentialstore.h"

#include <QEventLoop>
#include <QSettings>

#include <qtkeychain/keychain.h>

static const char *SERVICE_NAME = "Koha Offline Circulation";

static bool keychainDisabled()
{
    return qEnvironmentVariableIsSet( "KOC_NO_KEYCHAIN" );
}

// QtKeychain jobs are asynchronous, spin until this one finishes
static bool runJob( QKeychain::Job & job )
{
    QEventLoop loop;
    QObject::connect( &job, SIGNAL( finished( QKeychain::Job * ) ),
                      &loop, SLOT( quit() ) );
    job.setAutoDelete( false );
    job.start();
    loop.exec();

    return job.error() == QKeychain::NoError;
}

QString CredentialStore::read( const QString & key )
{
    if ( ! keychainDisabled() ) {
        QKeychain::ReadPasswordJob job( SERVICE_NAME );
        job.setKey( key );
        if ( runJob( job ) && ! job.textData().isEmpty() ) {
            return job.textData();
        }
    }

    // The QSettings location older versions stored secrets in, write()
    // migrates it into the keychain the next time settings are saved
    QSettings settings;
    return settings.value( key ).toString();
}

bool CredentialStore::write( const QString & key, const QString & secret )
{
    QSettings settings;

    if ( ! keychainDisabled() ) {
        QKeychain::WritePasswordJob job( SERVICE_NAME );
        job.setKey( key );
        job.setTextData( secret );
        if ( runJob( job ) ) {
            // The plaintext copy is obsolete once the keychain has it
            settings.remove( key );
            return true;
        }
    }

    // No keychain available, keep the old plaintext behavior
    settings.setValue( key, secret );
    return false;
}

void CredentialStore::remove( const QString & key )
{
    if ( ! keychainDisabled() ) {
        QKeychain::DeletePasswordJob job( SERVICE_NAME );
        job.setKey( key );
        runJob( job );
    }

    QSettings settings;
    settings.remove( key );
}
