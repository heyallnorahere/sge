cmake_minimum_required(VERSION 3.10)

# set up base variables
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")
set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets")
set(SGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SGE_INCLUDES ${SGE_DIR} "${VENDOR_DIR}/glm")
set(SGE_LIBS spdlog shaderc spirv-cross-glsl stb EnTT box2d)

# platform detection
if(SGE_DESKTOP)
    list(APPEND SGE_DEFINES SGE_PLATFORM_DESKTOP)
    list(APPEND SGE_LIBS glfw)
    list(APPEND ENABLED_PLATFORMS desktop)
endif()

if(WIN32)
    list(APPEND SGE_DEFINES SGE_PLATFORM_WINDOWS)
    list(APPEND ENABLED_PLATFORMS windows)
elseif(UNIX)
    if(APPLE)
        if(SGE_DESKTOP)
            list(APPEND SGE_DEFINES SGE_PLATFORM_MACOSX)
            list(APPEND ENABLED_PLATFORMS macosx)
        else()
            message(FATAL_ERROR "IOS is not supported yet - sorry!")
        endif()
    else()
        # we can assume its linux
        list(APPEND SGE_DEFINES SGE_PLATFORM_LINUX)
        list(APPEND SGE_LIBS pthread stdc++fs)
        list(APPEND ENABLED_PLATFORMS linux)
    endif()
endif()

# renderer api
if(Vulkan_FOUND)
    list(APPEND SGE_INCLUDES ${Vulkan_INCLUDE_DIR})
    list(APPEND SGE_LIBS ${Vulkan_LIBRARY} VulkanMemoryAllocator)
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
set_target_properties(sge PROPERTIES CXX_STANDARD 17)

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