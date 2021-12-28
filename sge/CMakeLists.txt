cmake_minimum_required(VERSION 3.10)

set(VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
set(SGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SGE_INCLUDES ${SGE_DIR} "${VENDOR_DIR}/glm")
set(SGE_LIBS spdlog)

# glob source
file(GLOB_RECURSE SGE_MANIFEST CONFIGURE_DEPENDS "${SGE_DIR}/*.cpp" "${SGE_DIR}/*.h")

# add target
add_library(sge STATIC ${SGE_MANIFEST})
target_link_libraries(sge PUBLIC ${SGE_LIBS})
target_include_directories(sge PUBLIC ${SGE_INCLUDES})
set_target_properties(sge PROPERTIES CXX_STANDARD 17)

# setup pch
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    target_precompile_headers(sge PRIVATE "${SGE_DIR}/sgepch.h")
endif()