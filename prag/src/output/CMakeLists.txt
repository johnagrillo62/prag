#output / CMakeLists.txt
#Static library for walker implementations, exporting only API headers

#Implementation source files
set(OUTPUT_SOURCES
    walker_registry.cpp
#add other walker implementation cpp files here if needed
)

#Public headers to export
set(OUTPUT_HEADERS
    walker_registry.h
    ast_walker.h

)

#Create the outputs static library(implementation + headers)
add_library(outputs STATIC ${OUTPUT_SOURCES})

#Include directories for consumers
target_include_directories(outputs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}       # output/ folder for walker headers
    ${CMAKE_CURRENT_SOURCE_DIR}/..    # project root if needed (for ast.h etc.)
)

#Link AST library so outputs can see ast.h
target_link_libraries(outputs PRIVATE ast)

#Ensure outputs builds after ast
add_dependencies(outputs ast)

#Compile options
target_compile_options(outputs PRIVATE
    -Wall
#- Wextra
#- Wpedantic
#- Wno - unused - parameter
)
