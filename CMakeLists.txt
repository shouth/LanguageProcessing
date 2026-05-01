cmake_minimum_required(VERSION 3.15)
project(KIT_LPP LANGUAGES C)

add_library(mpplc_options INTERFACE)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(mpplc_options
    INTERFACE
      -std=c90 -pedantic-errors -Wall -Wextra
      $<$<CONFIG:Debug>:-fsanitize=address,leak,undefined -fno-sanitize-recover -fstack-protector>)
  target_link_options(mpplc_options
    INTERFACE
      $<$<CONFIG:Debug>:-fsanitize=address,leak,undefined>)
endif()

if(UNIX)
  target_compile_definitions(mpplc_options INTERFACE _POSIX_C_SOURCE=200809L)
endif()

file(GLOB src CONFIGURE_DEPENDS src/*.c)
list(FILTER src EXCLUDE REGEX "src/main.c")

add_library(mpplc_core STATIC ${src})
target_link_libraries(mpplc_core mpplc_options)
target_include_directories(mpplc_core PUBLIC src)

add_executable(mpplc src/main.c)
target_link_libraries(mpplc mpplc_core mpplc_options)

add_executable(tc task/1_tc.c)
target_link_libraries(tc mpplc_core mpplc_options)

add_executable(pp task/2_pp.c)
target_link_libraries(pp mpplc_core mpplc_options)

enable_testing()
file(GLOB tests CONFIGURE_DEPENDS test/*.c)
foreach(test ${tests})
  get_filename_component(test_name ${test} NAME_WE)
  add_executable(${test_name} ${test})
  target_link_libraries(${test_name} mpplc_core mpplc_options)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
