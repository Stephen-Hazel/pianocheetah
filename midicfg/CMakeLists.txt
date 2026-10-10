cmake_minimum_required(VERSION 3.31)
set(QT_FORCE_CMP0156_TO_VALUE NEW)
project(midicfg VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS rc)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

set(PROJECT_SOURCES
   midicfg.cpp
   midicfg.h
   rc/midicfg.ui
   stv/os.cpp
   stv/ui.cpp
   stv/midi.cpp
)

qt_add_executable(midicfg rc/rc.qrc
   MANUAL_FINALIZATION
   ${PROJECT_SOURCES}
)

target_include_directories(midicfg PRIVATE
   ${PIPEWIRE_INCLUDE_DIRS})
target_compile_options(    midicfg PRIVATE
   ${PIPEWIRE_CFLAGS_OTHER})
target_link_libraries(     midicfg PRIVATE Qt6::Widgets Qt6::Svg
   asound ${PIPEWIRE_LIBRARIES})

install(TARGETS midicfg
   BUNDLE DESTINATION .
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(midicfg)
