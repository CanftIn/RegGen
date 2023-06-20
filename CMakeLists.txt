cmake_minimum_required(VERSION 3.6)
project(
  RegGen
  VERSION 0.0.1
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")
set(project_version "${${PROJECT_NAME}_VERSION}")

message(STATUS "Project '${PROJECT_NAME}', version: '${project_version}'")

option(REGGEN_OPT_BUILD_UNITTESTS "Build all RegGen unittests" ON)

# CMake helpers:
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(CTest)

include_directories(include)

# Create the RegGen C++ library
set(RegGen_SRCS)

set(STATIC_LIB_NAME ${PROJECT_NAME})

file(GLOB_RECURSE LIB_PATH
    ./lib/Common/*.cc
    ./lib/Container/*.cc
    ./lib/Lexer/*.cc
    ./lib/Parser/*.cc
  )
list(APPEND RegGen_SRCS ${LIB_PATH})

add_library(${STATIC_LIB_NAME} STATIC ${RegGen_SRCS})
#target_link_libraries(${STATIC_LIB_NAME})

if (REGGEN_OPT_BUILD_UNITTESTS)
  add_subdirectory(unittests #[[EXCLUDE_FROM_ALL]])
endif()

add_subdirectory(Driver)