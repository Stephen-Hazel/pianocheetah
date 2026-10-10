cmake_minimum_required(VERSION 3.31)
set(QT_FORCE_CMP0156_TO_VALUE NEW)
project(pianocheetah VERSION 0.1 LANGUAGES CXX)

add_compile_definitions(USE_SYN)

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
   glo.h
   song.h
   pcheetah.h
   ctlNt.h
   pcheetah.cpp
   ctlNt.cpp
   dlg.cpp
   song.cpp
   sTime.cpp
   sDevTyp.cpp
   sDevice.cpp
   sFile.cpp
   sRecord.cpp
   sNote.cpp
   sChd.cpp
   sEdit.cpp
   sEdMs.cpp
   sUtil.cpp
   sReDo.cpp
   sCmd.cpp
   sDump.cpp
   stv/os.cpp
   stv/ui.cpp
   stv/timer.h
   stv/midi.h
   stv/midi.cpp
   stv/midiChd.cpp
   stv/syn.h
   stv/syn.cpp
   stv/snd.h
   stv/snd.cpp
   stv/wav.h
   stv/wav.cpp
   rc/dlgcfg.ui
   rc/dlgchd.ui
   rc/dlgctl.ui
   rc/dlgcue.ui
   rc/dlgfl.ui
   rc/dlgqua.ui
   rc/dlghlp.ui
   rc/dlgksg.ui
   rc/dlgmov.ui
   rc/dlgtdr.ui
   rc/dlgtpo.ui
   rc/dlgtsg.ui
   rc/pcheetah.ui
)

qt_add_executable(pianocheetah rc/rc.qrc
   MANUAL_FINALIZATION
   ${PROJECT_SOURCES}
)

target_include_directories(pianocheetah PRIVATE
 ${PIPEWIRE_INCLUDE_DIRS})
target_compile_options(    pianocheetah PRIVATE
 ${PIPEWIRE_CFLAGS_OTHER})
target_link_libraries(     pianocheetah PRIVATE Qt6::Widgets Qt6::Svg
 asound ${PIPEWIRE_LIBRARIES})

install(TARGETS pianocheetah
   BUNDLE DESTINATION .
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(pianocheetah)
