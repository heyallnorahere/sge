cmake_minimum_required(VERSION 3.10)

# set up base variables
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")
set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets")
set(SGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# find mono libs
if(SGE_PLATFORM_WINDOWS)
    set(MONO_PLATFORM windows)
elseif(SGE_PLATFORM_MACOSX)
    set(MONO_PLATFORM osx)
elseif(SGE_PLATFORM_LINUX)
    include(glibc)
    CHECK_GLIBC_VERSION()

    set(CURRENT_GLIBC_VERSION 0.0)
    file(GLOB AVAILABLE_GLIBC_VERSIONS "${VENDOR_DIR}/mono/lib/linux/*")
    foreach(DIR ${AVAILABLE_GLIBC_VERSIONS})
        get_filename_component(CURRENT_VERSION ${DIR} NAME)
        if(${CURRENT_VERSION} VERSION_GREATER_EQUAL ${CURRENT_GLIBC_VERSION} AND
           ${GLIBC_VERSION} VERSION_GREATER_EQUAL ${CURRENT_VERSION})
            set(CURRENT_GLIBC_VERSION ${CURRENT_VERSION})
        endif()
    endforeach()

    if(${CURRENT_GLIBC_VERSION} STREQUAL 0.0)
        message(FATAL_ERROR "No applicable mono binaries! Please compile them yourself.")
    endif()

    message(STATUS "Using mono binaries for GLIBC version ${CURRENT_GLIBC_VERSION}.")
    set(MONO_PLATFORM linux/${CURRENT_GLIBC_VERSION})
endif()
file(GLOB MONO_LIBS CONFIGURE_DEPENDS "${VENDOR_DIR}/mono/lib/${MONO_PLATFORM}/${CMAKE_SYSTEM_PROCESSOR}/*")

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    set(SGE_CPU_ARCHITECTURE x64)
else()
    set(SGE_CPU_ARCHITECTURE ARM64)
endif()

# base lists
set(SGE_INCLUDES ${SGE_DIR} "${VENDOR_DIR}/mono/include" "${VENDOR_DIR}/glm")
set(SGE_LIBS spdlog shaderc spirv-cross-glsl stb EnTT box2d sge-imgui nlohmann_json ${MONO_LIBS})
set(SGE_DEFINES SGE_VERSION="${SGE_VERSION}" SGE_CPU_ARCHITECTURE="${SGE_CPU_ARCHITECTURE}")

# nvidia nsight aftermath
if(SGE_USE_AFTERMATH)
    list(APPEND SGE_LIBS sge-aftermath-helpers)
    list(APPEND SGE_DEFINES SGE_AFTERMATH_LINKED)
endif()

# platform detection
if(SGE_PLATFORM_DESKTOP)
    list(APPEND SGE_DEFINES SGE_PLATFORM_DESKTOP)
    list(APPEND SGE_LIBS glfw)
    list(APPEND ENABLED_PLATFORMS desktop)

    if(SGE_PLATFORM_WINDOWS)
        list(APPEND SGE_DEFINES SGE_PLATFORM_WINDOWS)
        list(APPEND ENABLED_PLATFORMS windows)
    elseif(SGE_PLATFORM_MACOSX)
        list(APPEND SGE_DEFINES SGE_PLATFORM_MACOSX)
        list(APPEND ENABLED_PLATFORMS macosx)
    elseif(SGE_PLATFORM_LINUX)
        list(APPEND SGE_DEFINES SGE_PLATFORM_LINUX)
        list(APPEND SGE_LIBS pthread stdc++fs)
        list(APPEND ENABLED_PLATFORMS linux)
    endif()
else()
    message(FATAL_ERROR "Other systems are not currently supported!")
endif()

# graphics api
if(SGE_USE_VULKAN)
    list(APPEND SGE_LIBS ${Vulkan_LIBRARY} VulkanMemoryAllocator Vulkan-Headers)
    list(APPEND SGE_DEFINES SGE_USE_VULKAN)
    list(APPEND ENABLED_PLATFORMS vulkan)
endif()

# glob sources
file(GLOB_RECURSE SGE_MANIFEST CONFIGURE_DEPENDS "${SGE_DIR}/*")
file(GLOB_RECURSE PLATFORM_CODE CONFIGURE_DEPENDS "${SGE_DIR}/sge/platform/*")
list(REMOVE_ITEM SGE_MANIFEST ${PLATFORM_CODE})
foreach(PLATFORM_NAME ${ENABLED_PLATFORMS})
    file(GLOB_RECURSE GLOBBED_PLATFORM_CODE CONFIGURE_DEPENDS "${SGE_DIR}/sge/platform/${PLATFORM_NAME}/*")
    list(APPEND SGE_MANIFEST ${GLOBBED_PLATFORM_CODE})
endforeach()

# glob shaders
file(GLOB_RECURSE SGE_SHADERS CONFIGURE_DEPENDS "${ASSET_DIR}/shaders/*")
if(MSVC)
    source_group("Shaders" FILES ${SGE_SHADERS})
    set_source_files_properties(${SGE_SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
endif()

# add target
add_library(sge STATIC ${SGE_MANIFEST} ${SGE_SHADERS})
target_link_libraries(sge PUBLIC ${SGE_LIBS})
target_include_directories(sge PUBLIC ${SGE_INCLUDES})
target_compile_definitions(sge PUBLIC ${SGE_DEFINES} $<$<CONFIG:Debug>:SGE_DEBUG>)
set_target_properties(sge PROPERTIES CXX_STANDARD 17 FOLDER "core")

# setup pch
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    set(SGE_PCH "${SGE_DIR}/sgepch.h")
    message(STATUS "SGE: Using precompiled header: ${SGE_PCH}")
    target_precompile_headers(sge PRIVATE ${SGE_PCH})
endif()

message(STATUS "SGE: Linking against libraries: ${SGE_LIBS}")
message(STATUS "SGE: Including directories: ${SGE_INCLUDES}")
message(STATUS "SGE: Defining preprocessor macros: ${SGE_DEFINES}")
message(STATUS "SGE: Enabled platform code: ${ENABLED_PLATFORMS}")