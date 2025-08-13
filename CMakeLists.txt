cmake_minimum_required(VERSION 3.15)
project(KIT_LPP)

include(CTest)

set(_POSIX_C_SOURCE 200112L CACHE STRING "POSIX version to use")

set(gnu_like "$<C_COMPILER_ID:GNU,Clang>")
set(gnu_like_debug "$<AND:${gnu_like},$<CONFIG:Debug>>")

function(set_build_options target)
  set_target_properties(${target}
    PROPERTIES
      C_STANDARD 90
      C_EXTENSIONS OFF)
  target_compile_options(${target}
    PRIVATE
      "$<${gnu_like}:-pedantic-errors;-Wall;-Wextra>"
      "$<${gnu_like_debug}:-fsanitize=address;-fno-sanitize-recover;-fstack-protector>")
  target_link_options(${target}
    PRIVATE
      "$<${gnu_like_debug}:-fsanitize=address>")
  target_compile_definitions(${target}
    PRIVATE
      _POSIX_C_SOURCE=${_POSIX_C_SOURCE})
endfunction()

file(GLOB srcs src/*.c)
list(FILTER srcs EXCLUDE REGEX ".*\/main.c")

add_library(mpplc_lib STATIC ${srcs})
target_include_directories(mpplc_lib PUBLIC src)
set_build_options(mpplc_lib)

add_executable(mpplc src/main.c)
target_link_libraries(mpplc PRIVATE mpplc_lib)
set_build_options(mpplc)

if(BUILD_TESTING)
  enable_testing()

  file(GLOB test_srcs test/*.c)
  foreach(test_src ${test_srcs})
    cmake_path(GET test_src STEM test_stem)
    add_executable(test_${test_stem} ${test_src})
    target_link_libraries(test_${test_stem} PRIVATE mpplc_lib)
    set_build_options(test_${test_stem})
    add_test(NAME test_${test_stem} COMMAND test_${test_stem} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach()
endif()
