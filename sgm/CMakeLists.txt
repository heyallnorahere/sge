cmake_minimum_required(VERSION 3.10)

set(SGM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE SGM_MANIFEST CONFIGURE_DEPENDS "${SGM_DIR}/*")
add_executable(sgm ${SGM_MANIFEST})

target_include_directories(sgm PRIVATE ${SGM_DIR})
target_link_libraries(sgm PRIVATE sge)
set_target_properties(sgm PROPERTIES CXX_STANDARD 17 FOLDER "tools")

copy_required_dlls(sgm)
add_custom_command(TARGET sgm PRE_BUILD
    COMMAND dotnet build --nologo -v q -c $<CONFIG> "${CMAKE_SOURCE_DIR}/debugger/SGE.Debugger.csproj"
    COMMENT "Building SGE.Debugger.exe")

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    target_precompile_headers(sgm PRIVATE "${SGM_DIR}/sgmpch.h")
endif()