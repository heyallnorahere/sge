cmake_minimum_required(VERSION 3.10)

file(GLOB LAUNCHER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(launcher ${LAUNCHER_SOURCE})
target_link_libraries(launcher PRIVATE sge)
add_dependencies(launcher sgm)
copy_required_dlls(launcher)

set_target_properties(launcher PROPERTIES
    FOLDER "tools"
    CXX_STANDARD 17
    RUNTIME_OUTPUT_NAME "sgm-launcher")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    target_precompile_headers(launcher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/launcher_pch.h")
endif()