cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE SANDBOX_MANIFEST CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(sandbox ${SANDBOX_MANIFEST})
target_link_libraries(sandbox PRIVATE sge)
set_target_properties(sandbox PROPERTIES CXX_STANDARD 17 FOLDER "examples")

if(MSVC)
    set_target_properties(sandbox PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()