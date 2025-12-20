
file(GLOB_RECURSE LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(bhw ${LIB_SOURCES} )
target_include_directories(bhw PUBLIC ../ ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bhw PUBLIC ${ODBC_LIBRARIES})
