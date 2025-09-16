cmake_minimum_required(VERSION 3.19)
project(EqCustomPlot LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg PrintSupport)

qt_standard_project_setup()
# Ensure resources (*.qrc) are compiled automatically
set(CMAKE_AUTORCC ON)

qt_add_executable(EqCustomPlot
    MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    qcustomplot.cpp
    qcustomplot.h
    eq.cpp
    eq.h
    qcpaxistickerfreq.cpp
    qcpaxistickerfreq.h
    eqcustomplot.cpp
    eqcustomplot.h
    resources.qrc
)

target_link_libraries(EqCustomPlot
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::PrintSupport
)

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
