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

The app is not notarized with Apple, so the first launch will be blocked. To open it:
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
