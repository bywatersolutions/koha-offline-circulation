# Koha Offline Circulation

A desktop application for circulating items when your [Koha](https://koha-community.org/)
server is unreachable. Scan patron cards and item barcodes for checkouts, returns, and
fine payments while offline; transactions are written to a `.koc` file that can be
uploaded to Koha once you are back online (Circulation → Offline circulation file upload).

If a `borrowers.db` SQLite export is available, the app can also display patron details
and search for patrons by name while offline.

Licensed under the GPLv3. Copyright 2010 Kyle M Hall.

## Downloads

Grab the latest packages from the
[Releases page](https://github.com/bywatersolutions/koha-offline-circulation/releases):

| Platform | File | Notes |
|----------|------|-------|
| Windows | `KohaOfflineCirculation-x.y.z-setup.exe` | Installer. Requires Windows 10 or 11, 64-bit. |
| Windows | `KohaOfflineCirculation-x.y.z-windows-portable.zip` | No install needed — unzip and run. Good for locked-down machines. |
| macOS | `KohaOfflineCirculation-x.y.z-macos.dmg` | Universal (Apple Silicon + Intel). See Gatekeeper note below. |
| Linux | `KohaOfflineCirculation-x.y.z-x86_64.AppImage` | `chmod +x` the file and run it. Requires glibc 2.39+ (Ubuntu 24.04+, Debian 13+, Fedora 40+). |

Version 2.0.0 and later require a 64-bit OS. If you are on 32-bit Windows or
Windows 7/8, use the old
[v1.3.2 release](https://github.com/bywatersolutions/koha-offline-circulation/releases/tag/v1.3.2).

### macOS Gatekeeper note

This only applies to releases built without code signing (see the macOS code
signing section below) — signed and notarized releases open normally. For an
unsigned build, the first launch will be blocked. To open it:
open System Settings → Privacy & Security, scroll down, and click **Open Anyway**
next to the message about KohaOfflineCirculation. Alternatively, from a terminal:

```
xattr -cr "/Applications/Koha Offline Circulation.app"
```

## Usage tips

- **Settings → Set Default KOC Save Path** controls where new `.koc` files are saved
  and where the Save As dialog starts. If your circulation computers wipe their local
  drives (Deep Freeze and similar), point this at a USB drive so transactions survive
  a reboot.
- **Settings → Select Borrowers DB File** points the app at a `borrowers.db` SQLite
  file for offline patron lookup.

## Downloading patron data from Koha

The app can build its own `borrowers.db` directly from your Koha server:
**Settings → Koha Connection Settings** to configure, then **Settings →
Download Borrowers DB from Koha** to run it. The settings dialog can also
schedule a nightly download and refresh automatically at startup when the
database is more than a day old — recommended for machines that wipe their
drives on reboot (Deep Freeze and similar).

Two download methods are supported:

### Saved reports (recommended)

The fastest option, and the account only needs the `catalogue` permission.
One-time setup on the Koha server:

1. Raise the **`SvcMaxReportRows`** system preference (default is only 10)
   to a value above your patron count, e.g. 1000000.
2. Create two saved SQL reports and note their report IDs. The column
   order matters — use this SQL as-is:

   Borrowers report:

   ```sql
   SELECT b.borrowernumber, b.cardnumber, b.surname, b.firstname,
          b.address, b.city, b.phone, b.dateofbirth,
          COALESCE( ( SELECT SUM(a.amountoutstanding)
                      FROM accountlines a
                      WHERE a.borrowernumber = b.borrowernumber ), 0 ) AS total_fines
   FROM borrowers b
   ```

   Issues report:

   ```sql
   SELECT i.borrowernumber, i.date_due, it.itemcallnumber, bib.title,
          COALESCE( it.itype, bi.itemtype ) AS itemtype
   FROM issues i
   JOIN items it ON it.itemnumber = i.itemnumber
   JOIN biblio bib ON bib.biblionumber = it.biblionumber
   JOIN biblioitems bi ON bi.biblionumber = bib.biblionumber
   ```

3. Optionally set a cache expiry on both reports (e.g. an hour) — with
   memcached active, a whole fleet of circulation computers downloading at
   the same time costs the server a single SQL run.

### REST API

No reports needed: enable the **`RESTBasicAuth`** system preference and use
an account with the `borrowers` and `circulate` permissions. The app pages
through `/api/v1/patrons` and `/api/v1/checkouts`, which is heavier on the
server than report mode but requires no other setup. After the first full
download, REST mode only fetches the patrons changed since the last sync
and does a fresh full download weekly to pick up deletions.

Koha's `misc/cronjobs/create_koc_db.pl` remains a fine alternative for very
large systems — the app reads the file it produces the same way.

## Uploading transactions to Koha

Once you're back online, **File → Upload to Koha** sends the current file's
transactions straight to the server — no more copying the `.koc` file to a
machine with staff client access. It uses the same connection settings as
the download, plus a **Branch code** (in Settings → Koha Connection
Settings) that the transactions are recorded under.

By default transactions are processed immediately, the same as uploading a
`.koc` file through the staff client. Ticking "Queue uploads for staff
review" queues them under **Circulation → Pending offline circulation
actions** instead, where staff approve them before anything is applied.

Each row's result appears in the History tab's Status column. Rows marked
*sent* are skipped if you upload again, so retrying after a network failure
can't process a transaction twice — this matters most for fine payments,
where a duplicate would double-charge the patron. Rejected rows (unknown
barcode, unknown patron) show Koha's reason and stay eligible for retry.
The account needs the `circulate` permission to upload.

## Building from source

Requires Qt 6.5+ and CMake 3.22+.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

- **macOS**: `brew install cmake qt`, then the commands above produce
  `build/KohaOfflineCirculation.app`.
- **Linux**: install Qt 6 development packages (e.g. `qt6-base-dev` on Debian/Ubuntu),
  then the commands above.
- **Windows**: use the [Qt online installer](https://www.qt.io/download-qt-installer)
  with MSVC 2022, then configure with `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`.

## Releasing

Packages are built and published automatically by GitHub Actions:

1. Bump the version on the `project(...)` line in `CMakeLists.txt`.
2. Commit, then tag the commit `vX.Y.Z` (the workflow fails if the tag and CMake
   version disagree) and push the tag.
3. The workflow builds all platforms and attaches the four packages to a GitHub
   Release. Tags containing a suffix (e.g. `v2.1.0-rc1`) are marked as prereleases.

Every push and pull request also runs the full build and packaging on all three
platforms, so packaging breakage shows up before release time.

## macOS code signing

Builds are signed, notarized, and stapled automatically when these Actions
secrets are configured (Settings → Secrets and variables → Actions). They
are the same secrets, with the same names, used by the Olorin companion
app, so they can be shared as organization secrets. Without them the build
falls back to ad-hoc signing and users hit the Gatekeeper flow described
above.

| Secret | Contents |
|--------|----------|
| `APPLE_CERTIFICATE_P12` | The "Developer ID Application" certificate exported from Keychain Access as a .p12, base64 encoded (`base64 -i cert.p12 \| pbcopy`) |
| `APPLE_CERTIFICATE_PASSWORD` | The password chosen for that .p12 export |
| `APPLE_ID` | The Apple Account email of the enrolled developer |
| `APPLE_APP_SPECIFIC_PASSWORD` | An app-specific password generated at account.apple.com (Sign-In and Security → App-Specific Passwords) |
| `APPLE_TEAM_ID` | The 10-character Team ID from the developer account's membership page |

The app itself is stapled before the DMG is built, so a signed release
passes Gatekeeper even on machines with no internet access.

## The .koc file format

Plain text, tab-separated, one transaction per line, with a header line:

```
Version=1.0	Generator=kocDesktop	GeneratorVersion=2.0.0
2026-07-22 10-15-30 000	issue	23529001000463	31000000123456
2026-07-22 10-16-02 000	return	31000000654321
2026-07-22 10-17-45 000	payment	23529001000463	5.00
```

The format version (1.0) is unchanged since the 1.x releases, so files produced by
any version of this tool upload to Koha the same way.
