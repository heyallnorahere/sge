cmake_minimum_required(VERSION 3.10)

option(SGE_BUILD_SCRIPTCORE "Build SGE script core." ON)
if(SGE_BUILD_SCRIPTCORE)
    if(MSVC)
        include_external_msproject(scriptcore "${CMAKE_CURRENT_SOURCE_DIR}/scriptcore.csproj")
    else()
        file(GLOB_RECURSE SCRIPTCORE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cs")
        file(GLOB_RECURSE EXCLUDED_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/obj/*.cs")
        list(REMOVE_ITEM SCRIPTCORE_SOURCE ${EXCLUDED_SOURCE})

        add_custom_target(scriptcore ALL
            COMMAND mono ${Mono_MSBUILD} -noLogo -v:q -p:Configuration=$<CONFIG>
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            BYPRODUCTS "${CMAKE_SOURCE_DIR}/assemblies/SGE.dll"
            SOURCES ${SCRIPTCORE_SOURCE})
    endif()
else()
    add_custom_target(scriptcore ALL echo "SGE_BUILD_SCRIPTORE was disabled.")
endif()

set_target_properties(scriptcore PROPERTIES FOLDER "core")