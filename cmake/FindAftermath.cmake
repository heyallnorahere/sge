# searches for the NVIDIA Nsight Aftermath SDK (https://developer.nvidia.com/nsight-aftermath)
# will search in vendor/nvidia-aftermath

set(AFTERMATH_DIR "${CMAKE_SOURCE_DIR}/vendor/nvidia-aftermath")
set(AFTERMATH_LIB_DIR "${AFTERMATH_DIR}/lib/x64")

find_path(Aftermath_INCLUDE_DIR "GFSDK_Aftermath.h" HINTS "${AFTERMATH_DIR}/include")
find_library(Aftermath_LIBRARY NAMES GFSDK_Aftermath_Lib.x64 HINTS "${AFTERMATH_LIB_DIR}")
set(AFTERMATH_REQUIRED_VARS Aftermath_INCLUDE_DIR Aftermath_LIBRARY)

if(WIN32)
    file(GLOB Aftermath_DLLS "${AFTERMATH_LIB_DIR}/*.dll")
    list(APPEND AFTERMATH_REQUIRED_VARS Aftermath_DLLS)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Aftermath DEFAULT_MSG ${AFTERMATH_REQUIRED_VARS})
mark_as_advanced(${AFTERMATH_REQUIRED_VARS})