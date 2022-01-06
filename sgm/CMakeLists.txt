cmake_minimum_required(VERSION 3.10)

set(SGM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE SGM_MANIFEST CONFIGURE_DEPENDS "${SGM_DIR}/*")
add_executable(sgm ${SGM_MANIFEST})
target_include_directories(sgm PRIVATE ${SGM_DIR})
target_link_libraries(sgm PRIVATE sge)
set_target_properties(sgm PROPERTIES CXX_STANDARD 17)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    target_precompile_headers(sgm PRIVATE "${SGM_DIR}/sgmpch.h")
    set_source_files_properties("${SGM_DIR}/main.cpp" SKIP_PRECOMPILE_HEADERS ON)
endif()

if(MSVC)
    set_target_properties(sgm PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()