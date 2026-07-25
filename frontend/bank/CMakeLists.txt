cmake_minimum_required(VERSION 3.16)

project(bank VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appbank
    main.cpp
)

qt_add_qml_module(appbank
    URI bank
    VERSION 1.0
    QML_FILES
        Main.qml
        Style.qml
        views/DashboardPage.qml
        views/LoginPage.qml
        views/SendMoneyPage.qml
        views/ReceiveMoneyPage.qml

    RESOURCES
        image/qr.png
)

# Tell Qt that Style.qml is a Singleton module
set_source_files_properties(Style.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set_target_properties(appbank PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appbank
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

install(TARGETS appbank
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)