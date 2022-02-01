# https://gist.github.com/likema/f5c04dad837d2f5068ae7a8860c180e7

# - Check glibc version
# CHECK_GLIBC_VERSION()
# 
# Once done this will define
#
#   GLIBC_VERSION - glibc version
#
MACRO (CHECK_GLIBC_VERSION)
    EXECUTE_PROCESS (
        COMMAND ${CMAKE_C_COMPILER} -print-file-name=libc.so.6
	OUTPUT_VARIABLE GLIBC
	OUTPUT_STRIP_TRAILING_WHITESPACE)

    GET_FILENAME_COMPONENT (GLIBC ${GLIBC} REALPATH)
    GET_FILENAME_COMPONENT (GLIBC_VERSION ${GLIBC} NAME)
    STRING (REPLACE "libc-" "" GLIBC_VERSION ${GLIBC_VERSION})
    STRING (REPLACE ".so" "" GLIBC_VERSION ${GLIBC_VERSION})
    IF (NOT GLIBC_VERSION MATCHES "^[0-9.]+$")
        MESSAGE (FATAL_ERROR "Unknown glibc version: ${GLIBC_VERSION}")
    ENDIF (NOT GLIBC_VERSION MATCHES "^[0-9.]+$")
ENDMACRO (CHECK_GLIBC_VERSION)