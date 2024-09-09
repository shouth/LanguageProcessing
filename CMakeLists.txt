cmake_minimum_required(VERSION 3.15)
project(KIT_LPP)

set(mpplc_BUILD_TESTS OFF CACHE BOOL "Build tests")
set(_POSIX_C_SOURCE 200112L CACHE STRING "POSIX version to use")

set(gnu_like "$<C_COMPILER_ID:GNU,Clang>")
set(gnu_like_debug "$<AND:${gnu_like},$<CONFIG:Debug>>")

file(GLOB src src/*.c)

add_executable(mpplc ${src})

set_target_properties(mpplc
  PROPERTIES
    C_STANDARD 90
    C_EXTENSIONS OFF)

target_compile_options(mpplc
  PRIVATE
    "$<${gnu_like}:-pedantic-errors;-Wall;-Wextra>"
    "$<${gnu_like_debug}:-fsanitize=address,leak,undefined;-fno-sanitize-recover;-fstack-protector>")
target_link_options(mpplc
  PRIVATE
    "$<${gnu_like_debug}:-fsanitize=address,leak,undefined>")
target_compile_definitions(mpplc
  PRIVATE
    _POSIX_C_SOURCE=${_POSIX_C_SOURCE})

if(mpplc_BUILD_TESTS)
  add_subdirectory(test)
endif()
