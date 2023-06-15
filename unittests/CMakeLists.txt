cmake_minimum_required(VERSION 3.20)

find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
set(memcheck_command 
  ${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS}
  --error-exitcode=1 --leak-check=full --show-leak-kinds=all --track-origins=yes)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

enable_testing()
find_package(GTest REQUIRED CONFIG)

add_subdirectory(Container)
add_subdirectory(Common)
add_subdirectory(Lexer)