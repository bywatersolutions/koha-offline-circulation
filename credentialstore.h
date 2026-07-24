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

#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QString>

/* Stores secrets in the operating system keychain, the macOS Keychain,
 * the Windows Credential Manager, or the Linux Secret Service. When no
 * keychain is available it falls back to the plaintext QSettings
 * location older versions used, and a successful keychain write removes
 * any plaintext copy, which migrates old installs the next time their
 * settings are saved. Setting KOC_NO_KEYCHAIN in the environment forces
 * the fallback, the tests use that. */
namespace CredentialStore {
    QString read( const QString & key );
    bool write( const QString & key, const QString & secret );
    void remove( const QString & key );
}

#endif // CREDENTIALSTORE_H
