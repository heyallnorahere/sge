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

if(SGE_PLATFORM_MACOSX)
    set(SGE_TARGET_ARCHITECTURE ${CMAKE_OSX_ARCHITECTURES})
else()
    set(SGE_TARGET_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(${SGE_TARGET_ARCHITECTURE} STREQUAL "x86_64" OR ${SGE_TARGET_ARCHITECTURE} STREQUAL "AMD64")
    set(SGE_CPU_ARCHITECTURE x64)
else()
    set(SGE_CPU_ARCHITECTURE ARM64)
endif()

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

# independent options
option(SGE_BUILD_SCRIPTCORE "Build the SGE scriptcore." ON)
option(SGE_BUILD_DEBUGGER "Build SGE.Debugger.exe" ON)
set(SGE_DEBUGGER_AGENT_ADDRESS "127.0.0.1" CACHE STRING "The address to host the mono debugger agent at.")
set(SGE_DEBUGGER_AGENT_PORT "62222" CACHE STRING "The port to host the mono debugger agent on.")

# find packages
find_package(Aftermath)
if(SGE_BUILD_SCRIPTCORE)
    find_package(DotnetCore REQUIRED)
endif()

# dependent options
include(CMakeDependentOption)
cmake_dependent_option(SGE_BUILD_DEBUGGER "Build SGE.Debugger.exe" ON "SGE_BUILD_SCRIPTCORE" OFF)
cmake_dependent_option(SGE_USE_AFTERMATH "Use the NVIDIA Nsight Aftermath SDK" OFF "Aftermath_FOUND" OFF)
cmake_dependent_option(SGE_USE_VULKAN "Use the Vulkan graphics api." ON "SGE_PLATFORM_DESKTOP" OFF)

if(SGE_BUILD_DEBUGGER)
    find_package(MSBuild REQUIRED)
endif()

# dependencies
add_subdirectory("vendor")

# tools
add_subdirectory("tools")

# mono DLL macro
macro(copy_required_dlls TARGET_NAME)
    if(WIN32)
        set(DLLS_TO_COPY "${CMAKE_SOURCE_DIR}/vendor/mono/bin/windows/mono-2.0-sgen.dll")
        if(SGE_USE_AFTERMATH)
            list(APPEND DLLS_TO_COPY ${Aftermath_DLLS})
        endif()

        foreach(DLL ${DLLS_TO_COPY})
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                               COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${TARGET_NAME}>)
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
    set(MANAGED_PROJECTS
        "${CMAKE_SOURCE_DIR}/scriptcore/scriptcore.csproj~core")

    foreach(PROJECT_DESC ${MANAGED_PROJECTS})
        string(FIND ${PROJECT_DESC} "~" SEPARATOR_POS REVERSE)
        string(LENGTH ${PROJECT_DESC} DESC_LENGTH)

        if(${SEPARATOR_POS} LESS 0)
            message(FATAL_ERROR "Malformed project description!")
        endif()

        string(SUBSTRING ${PROJECT_DESC} 0 ${SEPARATOR_POS} PROJECT_PATH)
        cmake_path(GET PROJECT_PATH FILENAME PROJECT_FILENAME)

        math(EXPR FOLDER_START "${SEPARATOR_POS} + 1")
        math(EXPR FOLDER_LENGTH "${DESC_LENGTH} - ${FOLDER_START}")
        string(SUBSTRING ${PROJECT_DESC} ${FOLDER_START} ${FOLDER_LENGTH} PROJECT_FOLDER)

        string(FIND ${PROJECT_FILENAME} "." SEPARATOR_POS REVERSE)
        if(${SEPARATOR_POS} LESS 0)
            message(FATAL_ERROR "Malformed project path!")
        endif()

        string(SUBSTRING ${PROJECT_FILENAME} 0 ${SEPARATOR_POS} MANAGED_PROJECT_NAME)
        if(MSVC)
            include_external_msproject(${MANAGED_PROJECT_NAME} ${PROJECT_PATH})
            set_target_properties(${MANAGED_PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_FOLDER})
        else()
            string(FIND ${PROJECT_PATH} "vendor" POS)

            if(${POS} LESS 0)
                add_custom_target(${MANAGED_PROJECT_NAME} ALL
                    COMMAND ${DOTNET_EXE} build --nologo -v m -c $<CONFIG> -a ${SGE_CPU_ARCHITECTURE} ${PROJECT_PATH}
                    COMMENT "Building managed project: ${MANAGED_PROJECT_NAME}")
            endif()
        endif()
    endforeach()

    add_dependencies(sge scriptcore)
endif()

if(MSVC)
    set_directory_properties(PROPERTIES VS_STARTUP_PROJECT launcher)
    set_target_properties(sgm launcher PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
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
