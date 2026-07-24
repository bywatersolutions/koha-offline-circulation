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

#include <QtTest>
#include <QSettings>

#include "credentialstore.h"

/* Exercises the QSettings fallback and migration behavior with the
 * keychain forced off, real keychain access can't run headless on the
 * CI runners. */
class TestCredentialStore : public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();
        void cleanup();
        void fallbackRoundTrip();
        void readsLegacyPlaintext();
        void removeClearsFallback();
};

void TestCredentialStore::initTestCase()
{
    qputenv( "KOC_NO_KEYCHAIN", "1" );

    // Keep the test's QSettings away from the real application's
    QCoreApplication::setOrganizationName( "Koha Offline Circulation Tests" );
    QCoreApplication::setApplicationName( "tst_credentialstore" );
}

void TestCredentialStore::cleanup()
{
    QSettings settings;
    settings.clear();
}

void TestCredentialStore::fallbackRoundTrip()
{
    // false means the secret landed in the fallback, not the keychain
    QCOMPARE( CredentialStore::write( "testKey", "secret" ), false );
    QCOMPARE( CredentialStore::read( "testKey" ), QString( "secret" ) );

    QSettings settings;
    QCOMPARE( settings.value( "testKey" ).toString(), QString( "secret" ) );
}

void TestCredentialStore::readsLegacyPlaintext()
{
    // A password stored by an older version in plain QSettings is found
    QSettings settings;
    settings.setValue( "legacyKey", "oldSecret" );

    QCOMPARE( CredentialStore::read( "legacyKey" ), QString( "oldSecret" ) );
}

void TestCredentialStore::removeClearsFallback()
{
    CredentialStore::write( "testKey", "secret" );
    CredentialStore::remove( "testKey" );

    QCOMPARE( CredentialStore::read( "testKey" ), QString() );
}

QTEST_GUILESS_MAIN(TestCredentialStore)

#include "tst_credentialstore.moc"
