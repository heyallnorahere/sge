cmake_minimum_required(VERSION 3.10)

add_subdirectory("spdlog")
add_subdirectory("SPIRV-Cross")
add_subdirectory("stb")
add_subdirectory("entt")
add_subdirectory("box2d")
add_subdirectory("json")

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio")

if(NOT GIT_SUBMODULES_SYNCED)
    find_package(Git REQUIRED)

    message(STATUS "Syncing Git submodules...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    message(STATUS "Synced Git submodules!")
    set(GIT_SUBMODULES_SYNCED ON CACHE BOOL "Have SGE's Git submodules been synced?" FORCE)
endif()

if(NOT SHADERC_DEPENDENCIES_SYNCED)
    find_package(Python3 REQUIRED)

    message(STATUS "Syncing shaderc dependencies...")
    execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/shaderc/utils/git-sync-deps")

    message(STATUS "Synced shaderc dependencies!")
    set(SHADERC_DEPENDENCIES_SYNCED ON CACHE BOOL "Have the shaderc dependencies been synced?" FORCE)
endif()
add_subdirectory("shaderc")

if(SGE_PLATFORM_DESKTOP)
    add_subdirectory("glfw")
endif()

if(SGE_USE_VULKAN)
    add_subdirectory("Vulkan-Headers")

    # volk (meta loader)
    set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
    add_library(volk STATIC "${VOLK_DIR}/volk.c" "${VOLK_DIR}/volk.h")
    target_link_libraries(volk PUBLIC Vulkan::Headers)
    target_include_directories(volk PUBLIC ${VOLK_DIR})

    # vma
    set(VMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator")
    file(GLOB VMA_SOURCE CONFIGURE_DEPENDS "${VMA_DIR}/src/VmaUsage.*")
    list(APPEND VMA_SOURCE "${VMA_DIR}/include/vk_mem_alloc.h")

    if(WIN32 AND ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
        list(APPEND VMA_SOURCE "${VMA_DIR}/src/vk_mem_alloc.natvis")
    endif()

    add_library(vma STATIC ${VMA_SOURCE})
    target_include_directories(vma PUBLIC "${VMA_DIR}/include")
    target_link_libraries(vma PUBLIC Vulkan::Headers)

    set_target_properties(vma PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON)

    target_compile_definitions(vma PRIVATE
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1
        VMA_DEBUG_ALWAYS_DEDICATED_MEMORY=0
        VMA_DEBUG_INITIALIZE_ALLOCATIONS=0
        VMA_DEBUG_GLOBAL_MUTEX=0
        VMA_DEBUG_DONT_EXCEED_MAX_MEMORY_ALLOCATION_COUNT=0
        VMA_RECORDING_ENABLED=0)
endif()

if(SGE_USE_AFTERMATH)
    add_subdirectory("aftermath-helpers")
endif()

include("imgui.cmake")