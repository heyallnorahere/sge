# searches for (64-bit) Mono in all usual locations
set(Mono_LIBRARY_HINTS "/usr/lib" "/usr/local/lib" "C:\\Program Files\\Mono\\lib" "/Library/Frameworks/Mono.framework/Versions/Current/lib")

find_path(Mono_INCLUDE_DIR "mono/jit/jit.h" HINTS "/usr/include" "/usr/local/include" "C:\\Program Files\\Mono\\include" "/Library/Frameworks/Mono.framework/Versions/Current/include" PATH_SUFFIXES "mono-2.0")
find_library(Mono_LIBRARY NAMES monosgen-2.0 mono-2.0-sgen HINTS ${Mono_LIBRARY_HINTS})
set(MONO_EXPORT_NAMES Mono_LIBRARY Mono_INCLUDE_DIR)

if(WIN32)
    find_file(Mono_DLL NAMES mono-2.0-sgen.dll HINTS "C:\\Program Files\\Mono\\bin")
    list(APPEND MONO_EXPORT_NAMES Mono_DLL)
endif()

if(NOT MSVC)
    find_file(Mono_MSBUILD NAMES MSBuild.dll PATHS ${Mono_LIBRARY_HINTS} PATH_SUFFIXES "mono/msbuild/Current/bin")
    list(APPEND MONO_EXPORT_NAMES Mono_MSBUILD)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mono DEFAULT_MSG ${MONO_EXPORT_NAMES})
mark_as_advanced(${MONO_EXPORT_NAMES})