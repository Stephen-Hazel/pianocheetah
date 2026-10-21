# _fpak/CMakeLists.txt - build all the apps in one go
#                        (app dirs are siblings of _fpak)
cmake_minimum_required(VERSION 3.31)
project(pcall LANGUAGES CXX)

foreach(d pianocheetah midicfg mid2song txt2song mod2song ll
          sfz2syn synsnd midimp song2wav)
   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../${d} ${d})
endforeach()
