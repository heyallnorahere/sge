cmake_minimum_required(VERSION 3.10)

file(GLOB TOOL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB TOOL_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

foreach(TOOL_SOURCE ${TOOL_SOURCES})
    get_filename_component(TOOL_NAME ${TOOL_SOURCE} NAME_WE)
    message(STATUS "Tool ${TOOL_NAME}: ${TOOL_SOURCE}")

    add_executable(${TOOL_NAME} ${TOOL_SOURCE} ${TOOL_HEADERS})
    set_target_properties(${TOOL_NAME} PROPERTIES
        FOLDER "tools"
        CXX_STANDARD 17)

    if(UNIX AND NOT APPLE)
        target_link_libraries(${TOOL_NAME} PRIVATE pthread stdc++fs spdlog)
    endif()
endforeach()