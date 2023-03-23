cmake_minimum_required(VERSION 3.20)
project(KIT_LPP)

add_executable(mppl)

file(GLOB src src/*.c)
target_sources(mppl
  PRIVATE ${src})

set_target_properties(mppl
  PROPERTIES
    C_STANDARD 90
    C_EXTENSIONS OFF)
target_compile_options(mppl
  PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:
      -pedantic-errors>
    $<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:
      -fsanitize=address,leak,undefined -fno-sanitize-recover -fstack-protector>)
target_link_options(mppl
  PRIVATE
    $<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:
      -fsanitize=address,leak,undefined>)
