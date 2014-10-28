INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_ANALYSIS analysis)

FIND_PATH(
    ANALYSIS_INCLUDE_DIRS
    NAMES analysis/api.h
    HINTS $ENV{ANALYSIS_DIR}/include
        ${PC_ANALYSIS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    ANALYSIS_LIBRARIES
    NAMES gnuradio-analysis
    HINTS $ENV{ANALYSIS_DIR}/lib
        ${PC_ANALYSIS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ANALYSIS DEFAULT_MSG ANALYSIS_LIBRARIES ANALYSIS_INCLUDE_DIRS)
MARK_AS_ADVANCED(ANALYSIS_LIBRARIES ANALYSIS_INCLUDE_DIRS)

