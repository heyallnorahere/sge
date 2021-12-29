cmake_minimum_required(VERSION 3.10)

project(sge)
set(SGE_DESKTOP $<PLATFORM_ID:Windows,Linux,Darwin>)

# dependencies
add_subdirectory("vendor")

# my code
add_subdirectory("sge")
add_subdirectory("sandbox")