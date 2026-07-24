cmake_minimum_required(VERSION 3.22)
project(KohaOfflineCirculation VERSION 2.4.0 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Sql Network LinguistTools)

# QtKeychain stores credentials in the macOS Keychain, the Windows
# Credential Manager, or the Linux Secret Service
include(FetchContent)
set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
set(QTKEYCHAIN_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(qtkeychain
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
    GIT_TAG 0.15.0)
# Force BUILD_TESTING off while qtkeychain configures, its test writes
# to a real keychain, which headless CI runners don't have. qtkeychain
# runs include(CTest) itself, so a plain variable shadow isn't enough.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(qtkeychain)
set(BUILD_TESTING ON CACHE BOOL "" FORCE)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(KohaOfflineCirculation
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    borrowersearch.cpp borrowersearch.h borrowersearch.ui
    kocfile.cpp kocfile.h
    borrowersdb.cpp borrowersdb.h
    kohadownload.cpp kohadownload.h
    kohasettingsdialog.cpp kohasettingsdialog.h kohasettingsdialog.ui
    kohaupload.cpp kohaupload.h
    updatecheck.cpp updatecheck.h
    credentialstore.cpp credentialstore.h
    koc.qrc
)

target_link_libraries(KohaOfflineCirculation PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network qt6keychain)
target_include_directories(KohaOfflineCirculation PRIVATE "${qtkeychain_SOURCE_DIR}" "${qtkeychain_BINARY_DIR}")

# Single source of truth for the app version is the project() line above
target_compile_definitions(KohaOfflineCirculation PRIVATE KOC_VERSION="${PROJECT_VERSION}")

# Translations are embedded under :/i18n and loaded in main.cpp; run
# the update_translations target after changing translatable strings
qt_add_translations(TARGETS KohaOfflineCirculation
    TS_FILES translations/koc_es.ts translations/koc_fr.ts)

if(WIN32)
    configure_file(koc.rc.in "${CMAKE_CURRENT_BINARY_DIR}/koc.rc" @ONLY)
    target_sources(KohaOfflineCirculation PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/koc.rc")
endif()

if(APPLE)
    set(KOC_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/images/icons/koha.icns")
    target_sources(KohaOfflineCirculation PRIVATE "${KOC_ICNS}")
    set_source_files_properties("${KOC_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

set_target_properties(KohaOfflineCirculation PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Koha Offline Circulation"
    MACOSX_BUNDLE_GUI_IDENTIFIER "info.kylehall.KohaOfflineCirculation"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "koha.icns"
)

install(TARGETS KohaOfflineCirculation
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION bin
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    qt_add_executable(tst_kocfile tests/tst_kocfile.cpp kocfile.cpp kocfile.h)
    target_include_directories(tst_kocfile PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(tst_kocfile PRIVATE Qt6::Core Qt6::Test)

    add_test(NAME kocfile COMMAND tst_kocfile)

    qt_add_executable(tst_borrowersdb tests/tst_borrowersdb.cpp borrowersdb.cpp borrowersdb.h)
    target_include_directories(tst_borrowersdb PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(tst_borrowersdb PRIVATE Qt6::Core Qt6::Sql Qt6::Test)

    add_test(NAME borrowersdb COMMAND tst_borrowersdb)

    qt_add_executable(tst_kohadownload tests/tst_kohadownload.cpp
        kohadownload.cpp kohadownload.h borrowersdb.cpp borrowersdb.h)
    target_include_directories(tst_kohadownload PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(tst_kohadownload PRIVATE Qt6::Core Qt6::Sql Qt6::Network Qt6::Test)

    add_test(NAME kohadownload COMMAND tst_kohadownload)

    qt_add_executable(tst_credentialstore tests/tst_credentialstore.cpp credentialstore.cpp credentialstore.h)
    target_include_directories(tst_credentialstore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${qtkeychain_SOURCE_DIR}" "${qtkeychain_BINARY_DIR}")
    target_link_libraries(tst_credentialstore PRIVATE Qt6::Core Qt6::Test qt6keychain)

    add_test(NAME credentialstore COMMAND tst_credentialstore)

    qt_add_executable(tst_kohaupload tests/tst_kohaupload.cpp kohaupload.cpp kohaupload.h)
    target_include_directories(tst_kohaupload PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(tst_kohaupload PRIVATE Qt6::Core Qt6::Network Qt6::Test)

    add_test(NAME kohaupload COMMAND tst_kohaupload)

    qt_add_executable(tst_updatecheck tests/tst_updatecheck.cpp updatecheck.cpp updatecheck.h)
    target_include_directories(tst_updatecheck PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(tst_updatecheck PRIVATE Qt6::Core Qt6::Network Qt6::Test)

    add_test(NAME updatecheck COMMAND tst_updatecheck)

    qt_add_executable(tst_mainwindow tests/tst_mainwindow.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        borrowersearch.cpp borrowersearch.h borrowersearch.ui
        kocfile.cpp kocfile.h
        borrowersdb.cpp borrowersdb.h
        kohadownload.cpp kohadownload.h
        kohasettingsdialog.cpp kohasettingsdialog.h kohasettingsdialog.ui
        kohaupload.cpp kohaupload.h
        updatecheck.cpp updatecheck.h
        credentialstore.cpp credentialstore.h
        koc.qrc)
    target_include_directories(tst_mainwindow PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${qtkeychain_SOURCE_DIR}" "${qtkeychain_BINARY_DIR}")
    target_compile_definitions(tst_mainwindow PRIVATE KOC_VERSION="0.0.0")
    target_link_libraries(tst_mainwindow PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network Qt6::Test qt6keychain)

    add_test(NAME mainwindow COMMAND tst_mainwindow)

    # Live test against a real Koha, skips itself unless KOHA_URL is set
    qt_add_executable(tst_kohadownload_integration tests/tst_kohadownload_integration.cpp
        kohadownload.cpp kohadownload.h borrowersdb.cpp borrowersdb.h kohaupload.cpp kohaupload.h)
    target_include_directories(tst_kohadownload_integration PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(tst_kohadownload_integration PRIVATE Qt6::Core Qt6::Sql Qt6::Network Qt6::Test)

    add_test(NAME kohadownload_integration COMMAND tst_kohadownload_integration)
endif()
