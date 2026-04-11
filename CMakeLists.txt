cmake_minimum_required(VERSION 3.15)
project(KIT_LPP)

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

file(GLOB src CONFIGURE_DEPENDS src/*.c)
list(FILTER src EXCLUDE REGEX "src/main.c")

add_library(mpplc_core STATIC ${src})
target_link_libraries(mpplc_core mpplc_options)

add_executable(mpplc src/main.c)
target_link_libraries(mpplc mpplc_core mpplc_options)
