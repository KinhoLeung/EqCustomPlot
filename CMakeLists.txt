cmake_minimum_required(VERSION 3.19)
project(EqCustomPlot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg)

qt_standard_project_setup()
# Ensure resources (*.qrc) are compiled automatically
set(CMAKE_AUTORCC ON)

qt_add_executable(EqCustomPlot
    MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    eq.cpp
    eq.h
    eqcustomplot.cpp
    eqcustomplot.h
    resources.qrc
)

target_link_libraries(EqCustomPlot
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Svg
)

enable_testing()

add_executable(eq_math_test
    tests/eq_math_test.cpp
    eq.cpp
    eq.h
)
target_include_directories(eq_math_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(eq_math_test PRIVATE Qt::Core)
add_test(NAME eq_math_test COMMAND eq_math_test)

include(GNUInstallDirs)

install(TARGETS EqCustomPlot
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET EqCustomPlot
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
