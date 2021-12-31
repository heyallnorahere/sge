cmake_minimum_required(VERSION 3.10)

if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(FATAL_ERROR "SGE must be built as a standalone project!")
endif()

project(sge)
set(SGE_DESKTOP $<PLATFORM_ID:Windows,Linux,Darwin>)

# dependencies
find_package(Vulkan)
add_subdirectory("vendor")

# my code
add_subdirectory("sge")
add_subdirectory("sandbox")