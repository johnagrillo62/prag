# AST library
add_library(ast STATIC
    ast.cpp
    ast.h
    
    languages.cpp
    languages.h
    ast_parser.h
    ast_walker.h
     "schema.h" "schema.cpp"  )

target_include_directories(ast PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_compile_options(ast PRIVATE -Wall
#-Wextra
#-Wpedantic
)
