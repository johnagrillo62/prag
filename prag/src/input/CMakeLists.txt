# List all parser source files explicitly
set(INPUT_SOURCES
    cpp_parser.cpp
    mdb_parser.cpp
    flatbuf_parser.cpp
    go_parser.cpp
    graphql_parser.cpp
    parser_registry.cpp
    parser_registry2.cpp

    protobuf_parser.cpp
    #python_parser.cpp
    rust_parser.cpp
    thrift_parser.cpp
    typescript_parser.cpp
)

# Include headers for IDE integration
file(GLOB INPUT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# Create static library
add_library(inputs STATIC ${INPUT_SOURCES} ${INPUT_HEADERS} "parser_registry2.cpp")

# Include directories for consumers
target_include_directories(inputs PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mini_yaml
    ${CMAKE_CURRENT_SOURCE_DIR}      # input/
    ${CMAKE_CURRENT_SOURCE_DIR}/..   # project root if needed
)

# Link against the AST library
target_link_libraries(inputs PRIVATE ast)

# Optional: compile flags
target_compile_options(inputs PRIVATE
    -Wall
#    -Wextra
#    -Wpedantic
     )


