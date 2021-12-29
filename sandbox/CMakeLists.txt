cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE SANDBOX_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(sandbox ${SANDBOX_MANIFEST})
target_link_libraries(sandbox PRIVATE sge)
set_target_properties(sandbox PROPERTIES CXX_STANDARD 17)