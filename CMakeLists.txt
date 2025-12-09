cmake_minimum_required(VERSION 3.21)

project(FVD LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport OpenGL OpenGLWidgets)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIB3DS lib3ds)
endif()

find_library(LIB3DS_LIBRARY NAMES 3ds lib3ds HINTS ${LIB3DS_LIBRARY_DIRS})
if(NOT LIB3DS_LIBRARY)
    message(FATAL_ERROR "Could not find lib3ds")
endif()

set(SOURCES
    main.cpp
    core/logging.cpp
    core/undohandler.cpp
    core/undoaction.cpp
    core/trackhandler.cpp
    core/track.cpp
    core/subfunction.cpp
    core/smoothhandler.cpp
    core/sectionhandler.cpp
    core/section.cpp
    core/secstraight.cpp
    core/secgeometric.cpp
    core/secforced.cpp
    core/seccurved.cpp
    core/secbezier.cpp
    core/saver.cpp
    core/nolimitsimporter.cpp
    core/mnode.cpp
    core/function.cpp
    core/exportfuncs.cpp
    osx/common.cpp
    renderer/trackmesh.cpp
    renderer/mytexture.cpp
    renderer/myshader.cpp
    renderer/myframebuffer.cpp
    renderer/glviewwidget.cpp
    ui/transitionwidget.cpp
    ui/trackwidget.cpp
    ui/trackproperties.cpp
    ui/smoothui.cpp
    ui/qcustomplot.cpp
    ui/projectwidget.cpp
    ui/optionsmenu.cpp
    ui/objectexporter.cpp
    ui/mytreewidget.cpp
    ui/myqdoublespinbox.cpp
    ui/mainwindow.cpp
    ui/importui.cpp
    ui/graphwidget.cpp
    ui/graphhandler.cpp
    ui/exportui.cpp
    ui/draglabel.cpp
    ui/conversionpanel.cpp
    core/secnlcsv.cpp
    resources.qrc
)

set(HEADERS
    core/logging.h
    core/undohandler.h
    core/undoaction.h
    core/trackhandler.h
    core/track.h
    core/subfunction.h
    core/smoothhandler.h
    core/sectionhandler.h
    core/section.h
    core/secstraight.h
    core/secgeometric.h
    core/secforced.h
    core/seccurved.h
    core/secbezier.h
    core/saver.h
    core/nolimitsimporter.h
    core/mnode.h
    core/function.h
    core/exportfuncs.h
    osx/common.h
    renderer/trackmesh.h
    renderer/mytexture.h
    renderer/myshader.h
    renderer/myframebuffer.h
    renderer/glviewwidget.h
    renderer/qtglcompat.h
    ui/transitionwidget.h
    ui/trackwidget.h
    ui/trackproperties.h
    ui/smoothui.h
    ui/qcustomplot.h
    ui/projectwidget.h
    ui/optionsmenu.h
    ui/objectexporter.h
    ui/mytreewidget.h
    ui/myqdoublespinbox.h
    ui/mainwindow.h
    ui/importui.h
    ui/graphwidget.h
    ui/graphhandler.h
    ui/exportui.h
    ui/draglabel.h
    ui/conversionpanel.h
    lenassert.h
    core/secnlcsv.h
)

set(UIS
    ui/transitionwidget.ui
    ui/trackwidget.ui
    ui/trackproperties.ui
    ui/smoothui.ui
    ui/projectwidget.ui
    ui/optionsmenu.ui
    ui/objectexporter.ui
    ui/mainwindow.ui
    ui/importui.ui
    ui/graphwidget.ui
    ui/exportui.ui
    ui/conversionpanel.ui
)

add_executable(FVD
    ${SOURCES}
    ${HEADERS}
    ${UIS}
)

target_include_directories(FVD PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${LIB3DS_INCLUDE_DIRS}
)

if(APPLE)
    target_sources(FVD PRIVATE
        osx/Document.mm
        osx/MainDelagate.mm
        osx/NSApplicationMain.mm
        osx/Init.cpp
    )
    target_link_libraries(FVD PRIVATE "-framework Foundation" "-framework Cocoa")
endif()

target_link_libraries(FVD PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    OpenGL::GLU
    GLEW::GLEW
    ${LIB3DS_LIBRARY}
)
