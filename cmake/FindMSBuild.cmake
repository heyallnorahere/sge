# searches for Visual Studio MSBuild on Windows, and Mono MSBuild on *nix systems.

if(WIN32)
    set(VISUAL_STUDIO_TYPES Community Professional Enterprise)
    foreach(TYPE ${VISUAL_STUDIO_TYPES})
        list(APPEND VISUAL_STUDIO_PATHS "C:\\Program Files\\Microsoft Visual Studio\\2022\\${TYPE}")
    endforeach()

    message(STATUS ${VISUAL_STUDIO_PATHS})
    find_program(MSBUILD_EXE "MSBuild"
        PATHS ${VISUAL_STUDIO_PATHS}
        PATH_SUFFIXES "\\MSBuild\\Current\\Bin\\")

    set(MSBUILD_ARGS "${MSBUILD_EXE}")
    set(MSBUILD_ERR_MESSAGE "Visual Studio 2022 not found! Please install Visual Studio at https://visualstudio.microsoft.com/downloads/.")
elseif(UNIX)
    set(MONO_PATHS
        "/Library/Frameworks/Mono.framework/Versions/Current/"
        "/usr/"
        "/usr/local/"
    )

    find_program(MSBUILD_EXE "mono"
        PATHS ${MONO_PATHS}
        PATH_SUFFIXES "bin/")

    find_file(MSBUILD_DLL "MSBuild.dll"
        PATHS ${MONO_PATHS}
        PATH_SUFFIXES "lib/mono/msbuild/Current/bin/")

    if(NOT "${MSBUILD_DLL}" STREQUAL "MSBUILD_DLL-NOTFOUND")
        set(MSBUILD_ARGS "${MSBUILD_EXE}" "${MSBUILD_DLL}")
    endif()

    set(MSBUILD_ERR_MESSAGE "Mono not found! Please install Mono at https://www.mono-project.com/download/stable/.")
endif()

set(MSBUILD_REQUIRED_VARS MSBUILD_ARGS MSBUILD_EXE)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MSBuild ${MSBUILD_ERR_MESSAGE} ${MSBUILD_REQUIRED_VARS})
mark_as_advanced(${MSBUILD_REQUIRED_VARS})