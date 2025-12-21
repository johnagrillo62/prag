

function(enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /wd4625  # copy constructor deleted
            /wd4626  # assignment operator deleted
            /wd4820  # padding added
            /wd4514  # unreferenced inline function removed
            /wd5045  # spectre
            /wd4061  # defautls fix later	 
        )
    endif()
endfunction()

# Add subdirectories for libraries
add_subdirectory(ast)
add_subdirectory(input)
add_subdirectory(output)


# Main executable
add_executable(prag prag.cpp)
target_include_directories(prag PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}./)
target_link_libraries(prag PRIVATE ast inputs outputs)

# config
add_executable(config config.cpp)
target_include_directories(config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}  )
target_link_libraries(config PRIVATE ast inputs outputs)


# config
add_executable(cpp-reflect cpp_reflect.cpp)
target_include_directories(cpp-reflect PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cpp-reflect PRIVATE ast inputs outputs)


# enum_gen
add_executable(cpp-enum cpp_enum.cpp )
target_include_directories(cpp-enum PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cpp-enum PRIVATE ast inputs outputs)

# enum_gen
#add_executable(validate validate.cpp )
#target_include_directories(validate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#target_link_libraries(validate PRIVATE ast inputs outputs)



# Apply warning disables
enable_warnings(prag)
enable_warnings(ast)
enable_warnings(inputs)
enable_warnings(outputs)
enable_warnings(cpp-enum)

