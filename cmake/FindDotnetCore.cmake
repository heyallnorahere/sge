# searches for the .NET Core executable

find_program(DOTNET_EXE "dotnet"
    PATHS "/usr/share/" "C:\\Program Files\\" "C:\\Program Files (x86)\\"
    PATH_SUFFIXES "dotnet")

set(DOTNET_REQUIRED_VARS DOTNET_EXE)
mark_as_advanced(${DOTNET_REQUIRED_VARS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DotnetCore DEFAULT_MSG ${DOTNET_REQUIRED_VARS})