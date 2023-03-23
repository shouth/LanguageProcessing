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

set(gnu_like "$<C_COMPILER_ID:GNU,Clang>")
set(gnu_like_debug "$<AND:${gnu_like},$<CONFIG:Debug>>")
target_compile_options(mppl
  PRIVATE
    "$<${gnu_like}:-pedantic-errors>"
    "$<${gnu_like_debug}:-fsanitize=address,leak,undefined;-fno-sanitize-recover;-fstack-protector>")
target_link_options(mppl
  PRIVATE
    "$<${gnu_like_debug}:-fsanitize=address,leak,undefined>")
