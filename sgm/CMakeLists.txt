cmake_minimum_required(VERSION 3.10)

set(SGM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE SGM_MANIFEST CONFIGURE_DEPENDS "${SGM_DIR}/*")
add_executable(sgm ${SGM_MANIFEST})

target_include_directories(sgm PRIVATE ${SGM_DIR})
target_link_libraries(sgm PRIVATE sge)
set_target_properties(sgm PROPERTIES CXX_STANDARD 17 FOLDER "tools")

copy_required_dlls(sgm)
if(SGE_BUILD_SCRIPTCORE)
    set(SGE_DEBUGGER_PATH "${CMAKE_SOURCE_DIR}/debugger/SGE.Debugger.csproj")
    add_custom_command(TARGET sgm PRE_BUILD
        COMMAND dotnet restore ${SGE_DEBUGGER_PATH}
        COMMAND ${MSBUILD_ARGS} -noLogo -v:m -p:Configuration=$<CONFIG> ${SGE_DEBUGGER_PATH}
        COMMENT "Building SGE.Debugger.exe")
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    target_precompile_headers(sgm PRIVATE "${SGM_DIR}/sgmpch.h")
endif()