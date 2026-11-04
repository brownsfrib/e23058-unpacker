# E23058 unpacker
#
#   cmake -S . -B build && cmake --build build -j
#
# Produces
#   lib/libE23058Data.so (+ _rdict.pcm, .rootmap)   data classes for ROOT sessions
#   bin/unpacker                                     the unpacker
#   bin/toBinary_FROG, bin/toBinary_HIRA             geometry text -> .dat
cmake_minimum_required(VERSION 3.9)
project(E23058 CXX)

find_package(ROOT 6.24 REQUIRED COMPONENTS Core RIO Tree)
include(${ROOT_USE_FILE})		# ROOT compile flags (incl. C++ standard) and ROOT_GENERATE_DICTIONARY

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include
                    ${PROJECT_SOURCE_DIR}/modules/include
                    ${PROJECT_SOURCE_DIR}/detector-specific/include)

# ---- data classes written to the tree (+ ROOT dictionary) -------------------
# To add PIXIE: add PixieData.h / src/PixieData.cpp here and a line in include/LinkDef.h
set(DATA_HEADERS FROGData.h HiRAData.h TDCData.h)
set(DATA_SOURCES src/FROGData.cpp src/HiRAData.cpp src/TDCData.cpp)

add_library(E23058Data SHARED ${DATA_SOURCES})
target_link_libraries(E23058Data ${ROOT_LIBRARIES})
ROOT_GENERATE_DICTIONARY(G__E23058Data ${DATA_HEADERS}
                         MODULE E23058Data
                         LINKDEF include/LinkDef.h
                         NOINSTALL)

# ---- unpacker ----------------------------------------------------------------
file(GLOB CORE_SOURCES     ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM CORE_SOURCES ${PROJECT_SOURCE_DIR}/src/FROGData.cpp
                              ${PROJECT_SOURCE_DIR}/src/HiRAData.cpp
                              ${PROJECT_SOURCE_DIR}/src/TDCData.cpp)
file(GLOB MODULE_SOURCES   ${PROJECT_SOURCE_DIR}/modules/src/*.cpp)
file(GLOB DETECTOR_SOURCES ${PROJECT_SOURCE_DIR}/detector-specific/src/*.cpp)

add_executable(unpacker unpacker.cxx ${CORE_SOURCES} ${MODULE_SOURCES} ${DETECTOR_SOURCES})
target_link_libraries(unpacker E23058Data ${ROOT_LIBRARIES})

# ---- geometry tools ------------------------------------------------------------
add_executable(toBinary_FROG tools/toBinary_FROG.cpp)
add_executable(toBinary_HIRA tools/toBinary_HIRA.cpp)
