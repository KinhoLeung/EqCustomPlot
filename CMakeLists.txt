cmake_minimum_required(VERSION 3.19)
project(EqCustomPlot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(EqCustomPlot
    MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    eq.cpp
    eq.h
    eqcustomplot.cpp
    eqcustomplot.h
)

target_link_libraries(EqCustomPlot
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)

enable_testing()

add_executable(eq_math_test
    tests/eq_math_test.cpp
    eq.cpp
    eq.h
)
target_include_directories(eq_math_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(eq_math_test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
add_test(NAME eq_math_test COMMAND eq_math_test)

include(GNUInstallDirs)

install(TARGETS EqCustomPlot
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_generate_deploy_app_script(
        TARGET EqCustomPlot
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
