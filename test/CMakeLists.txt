cmake_minimum_required(VERSION 3.15)

set(gnu_like "$<C_COMPILER_ID:GNU,Clang>")
set(gnu_like_debug "$<AND:${gnu_like},$<CONFIG:Debug>>")

file(GLOB tests *.c)
file(GLOB srcs ../src/*.c)
list(FILTER srcs EXCLUDE REGEX ".*main.c")

foreach(test ${tests})
  get_filename_component(test_name ${test} NAME_WE)
  add_executable(${test_name} ${test} ${srcs})
  target_include_directories(${test_name} PRIVATE ../src)
  target_compile_options(${test_name}
    PRIVATE
      "$<${gnu_like}:-pedantic-errors;-Wall;-Wextra>"
      "$<${gnu_like_debug}:-fsanitize=address,leak,undefined;-fno-sanitize-recover;-fstack-protector>")
  target_link_options(${test_name}
    PRIVATE
      "$<${gnu_like_debug}:-fsanitize=address,leak,undefined>")
endforeach()
