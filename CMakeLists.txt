cmake_minimum_required(VERSION 3.10)

if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(FATAL_ERROR "SGE must be built as a standalone project!")
endif()

# todo: from git tag
set(SGE_VERSION 0.0.1)

set(SGE_LANGUAGES C CXX)
if(APPLE AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    list(APPEND SGE_LANGUAGES OBJC OBJCXX)
endif()

project(sge VERSION ${SGE_VERSION} LANGUAGES ${SGE_LANGUAGES})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# platform detection
if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    set(SGE_PLATFORM_WINDOWS ON)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    set(SGE_PLATFORM_LINUX ON)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    set(SGE_PLATFORM_MACOSX ON)
endif()
set(SGE_PLATFORM_DESKTOP ${SGE_PLATFORM_WINDOWS} OR ${SGE_PLATFORM_LINUX} OR ${SGE_PLATFORM_MACOSX})

# find packages
find_package(Vulkan)
find_package(Aftermath)

# options
include(CMakeDependentOption)

option(SGE_BUILD_SCRIPTCORE "Build SGE script core." ON)
cmake_dependent_option(SGE_USE_AFTERMATH "Use the NVIDIA Nsight Aftermath SDK" OFF "Aftermath_FOUND" OFF)
cmake_dependent_option(SGE_USE_VULKAN "Use the Vulkan graphics api." ON "SGE_PLATFORM_DESKTOP AND Vulkan_FOUND" OFF)

# dependencies
add_subdirectory("vendor")

# tools
add_subdirectory("tools")

# mono DLL macro
macro(copy_required_dlls target_name)
    if(WIN32)
        set(DLLS_TO_COPY "${CMAKE_SOURCE_DIR}/vendor/mono/bin/windows/coreclr.dll")
        if(SGE_USE_AFTERMATH)
            list(APPEND DLLS_TO_COPY ${Aftermath_DLLS})
        endif()

        foreach(DLL ${DLLS_TO_COPY})
            add_custom_command(TARGET ${target_name} POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${target_name}>)
        endforeach()
    endif()
endmacro()

# sge targets
add_subdirectory("sge")
add_subdirectory("sgm")
add_subdirectory("launcher")

# output binaries into ${CMAKE_SOURCE_DIR}/bin, subdirectory if not release
set_target_properties(sgm launcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_SOURCE_DIR}/bin/$<$<NOT:$<CONFIG:Release>>:$<CONFIG>>")

# managed code
if(SGE_BUILD_SCRIPTCORE)
    if(MSVC)
        include_external_msproject(scriptcore "${CMAKE_SOURCE_DIR}/scriptcore/scriptcore.csproj")
        set_target_properties(scriptcore PROPERTIES FOLDER "core")
    else()
        find_program(dotnet_EXECUTABLE NAMES dotnet
                     PATHS "/usr/share" "/usr/local/share"
                           "C:\\Program Files" "C:\\Program Files (x86)"
                     PATH_SUFFIXES dotnet)
        
        if(NOT ${dotnet_EXECUTABLE} STREQUAL "dotnet_EXECUTABLE-NOTFOUND")
            add_custom_target(scriptcore ALL
                COMMAND ${dotnet_EXECUTABLE} build -nologo -v:m -c $<CONFIG>
                "${CMAKE_SOURCE_DIR}/scriptcore/scriptcore.csproj"
                COMMENT "Building SGE.Scriptcore.dll")
        else()
            message(STATUS "Could not find the .NET Core executable - skipping build target")
        endif()
    endif()

    if(TARGET scriptcore)
        add_dependencies(sge scriptcore)
    endif()
endif()

if(MSVC)
    set_directory_properties(PROPERTIES VS_STARTUP_PROJECT launcher)
endif()

# add all dependencies to a folder in IDEs
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
