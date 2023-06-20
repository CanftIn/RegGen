cmake_minimum_required(VERSION 3.6)

include_directories(.)

file(GLOB DRIVER_LIST *.cc)

foreach(FILE_PATH ${DRIVER_LIST})
  STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
  message(STATUS "driver files found: ${FILE_NAME}.cc")
  add_executable(rg_driver ${FILE_NAME}.cc)
  target_link_libraries(rg_driver RegGen)
endforeach()
