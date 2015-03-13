## 
# Project Setup
##
cmake_minimum_required(VERSION 2.6)
project (specrec CXX C)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)

IF(CMAKE_COMPILER_IS_GNUCXX)
   ADD_DEFINITIONS(-Wall)
   ADD_DEFINITIONS(-Wextra)
   ADD_DEFINITIONS(-Wsign-compare)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

message("PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
########################################################################
# Find boost
########################################################################
if(UNIX AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND EXISTS "/usr/lib64")
set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)
find_package(Boost "1.35" COMPONENTS filesystem system)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to compile bt")
endif()

# Includes
set(UHD_INCLUDE_DIR /usr/local/include)
# Libraries
set(UHD_LIBRARY_DIRS /usr/local/lib)

include_directories(${UHD_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIRS}
                    )
                    
link_directories(${UHD_LIBRARY_DIRS}
                 ${Boost_LIBRARY_DIRS})
add_executable(specrec specrec.cpp)
target_link_libraries(specrec uhd gnuradio-pmt boost_thread 
                      boost_system boost_program_options)
                      
install(
    FILES ${CMAKE_BINARY_DIR}/apps/record/specrec
    PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION /usr/local/bin/
)
