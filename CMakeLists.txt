cmake_minimum_required (VERSION 3.0)
project(KIT_LPP)

set(CMAKE_C_STANDARD 90)
add_compile_options(-pedantic-errors)

add_executable(mppl
  src/analyzer.c
  src/ast.c
  src/casl2_codegen.c
  src/crossref.c
  src/ir.c
  src/lexer.c
  src/main.c
  src/message.c
  src/parser.c
  src/pretty_printer.c
  src/source.c
  src/utility.c)
