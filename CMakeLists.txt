cmake_minimum_required(VERSION 3.10)

if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(FATAL_ERROR "SGE must be built as a standalone project!")
endif()

set(SGE_LANGUAGES C CXX)
if(APPLE AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    list(APPEND SGE_LANGUAGES OBJC OBJCXX)
endif()

project(sge LANGUAGES ${SGE_LANGUAGES})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

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

# managed code
if(SGE_BUILD_SCRIPTCORE)
    find_program(dotnet_EXECUTABLE NAMES dotnet
                 PATHS "/usr/share" "/usr/local/share"
                       "C:\\Program Files" "C:\\Program Files (x86)"
                 PATH_SUFFIXES dotnet)
                 
    if(MSVC)
        add_custom_target(nuget-restore ALL COMMAND ${dotnet_EXECUTABLE} restore
            -v:m "${CMAKE_SOURCE_DIR}/SGECore.sln")
        set_target_properties(nuget-restore PROPERTIES FOLDER "dependencies")

        set(MANAGED_TARGET_NAMES scriptcore example-scripts)
        foreach(MANAGED_PROJECT ${MANAGED_TARGET_NAMES})
            include_external_msproject(${MANAGED_PROJECT}
                "${CMAKE_SOURCE_DIR}/${MANAGED_PROJECT}/${MANAGED_PROJECT}.csproj")
            add_dependencies(${MANAGED_PROJECT} nuget-restore)
        endforeach()

        set_target_properties(scriptcore PROPERTIES FOLDER "core")
        set_target_properties(example-scripts PROPERTIES FOLDER "examples")
    else()
        
        if(NOT ${dotnet_EXECUTABLE} STREQUAL "dotnet_EXECUTABLE-NOTFOUND")
            add_custom_target(managed-code ALL
                COMMAND ${dotnet_EXECUTABLE} build -nologo -v:m -c $<CONFIG>
                "${CMAKE_SOURCE_DIR}/SGECore.sln")

            set(MANAGED_TARGET_NAMES managed-code)
        endif()
    endif()

    if(DEFINED MANAGED_TARGET_NAMES)
        add_dependencies(sge ${MANAGED_TARGET_NAMES})
    endif()
endif()

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
