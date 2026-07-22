cmake_minimum_required(VERSION 3.22)
project(KohaOfflineCirculation VERSION 2.0.0 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Sql)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(KohaOfflineCirculation
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    borrowersearch.cpp borrowersearch.h borrowersearch.ui
    koc.qrc
)

target_link_libraries(KohaOfflineCirculation PRIVATE Qt6::Widgets Qt6::Sql)

# Single source of truth for the app version is the project() line above
target_compile_definitions(KohaOfflineCirculation PRIVATE KOC_VERSION="${PROJECT_VERSION}")

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
