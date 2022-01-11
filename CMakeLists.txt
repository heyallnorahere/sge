cmake_minimum_required(VERSION 3.10)

if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(FATAL_ERROR "SGE must be built as a standalone project!")
endif()

set(SGE_LANGUAGES C CXX)
if(APPLE AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    list(APPEND SGE_LANGUAGES OBJC OBJCXX)
endif()

project(sge LANGUAGES ${SGE_LANGUAGES})
set(SGE_DESKTOP $<PLATFORM_ID:Windows,Linux,Darwin>)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# dependencies
find_package(Vulkan)
find_package(Mono REQUIRED)
add_subdirectory("vendor")

# mono DLL macro
macro(copy_required_dlls target_name)
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${Mono_DLL} $<TARGET_FILE_DIR:${target_name}>)
    endif()
endmacro()

# sge targets
add_subdirectory("sge")
add_subdirectory("sgm")
add_subdirectory("sandbox")

if(MSVC)
    set_directory_properties(PROPERTIES VS_STARTUP_PROJECT sgm)
endif()

# folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

macro(get_targets DIRECTORY TARGETS)
    get_property(SUBDIRECTORIES DIRECTORY ${DIRECTORY} PROPERTY SUBDIRECTORIES)
    foreach(SUBDIRECTORY ${SUBDIRECTORIES})
        get_targets(${SUBDIRECTORY} ${TARGETS})
    endforeach()

    get_property(DIRECTORY_TARGETS DIRECTORY ${DIRECTORY} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${TARGETS} ${DIRECTORY_TARGETS})
endmacro()

get_targets("${CMAKE_SOURCE_DIR}/vendor" DEPENDENCIES)
foreach(DEPENDENCY ${DEPENDENCIES})
    set(FOLDER_NAME "dependencies")

    get_target_property(CURRENT_FOLDER ${DEPENDENCY} FOLDER)
    if(NOT CURRENT_FOLDER STREQUAL "CURRENT_FOLDER-NOTFOUND")
        set(FOLDER_NAME "${FOLDER_NAME}/${CURRENT_FOLDER}")
    endif()

    set_target_properties(${DEPENDENCY} PROPERTIES FOLDER ${FOLDER_NAME})
endforeach()
