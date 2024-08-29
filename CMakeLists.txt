cmake_minimum_required(VERSION 3.15)
project(KIT_LPP)

add_executable(mpplc)

option(_POSIX_C_SOURCE "POSIX C source" 200809L)

file(GLOB src src/*.c)
target_sources(mpplc
  PRIVATE ${src})

set_target_properties(mpplc
  PROPERTIES
    C_STANDARD 90
    C_EXTENSIONS OFF)

set(gnu_like "$<C_COMPILER_ID:GNU,Clang>")
set(gnu_like_debug "$<AND:${gnu_like},$<CONFIG:Debug>>")
target_compile_options(mpplc
  PRIVATE
    "$<${gnu_like}:-pedantic-errors;-Wall;-Wextra>"
    "$<${gnu_like_debug}:-fsanitize=address,leak,undefined;-fno-sanitize-recover;-fstack-protector>")
target_link_options(mpplc
  PRIVATE
    "$<${gnu_like_debug}:-fsanitize=address,leak,undefined>")
target_compile_definitions(mpplc
  PRIVATE
    "-D_POSIX_C_SOURCE=${_POSIX_C_SOURCE}")
