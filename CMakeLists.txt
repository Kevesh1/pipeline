cmake_minimum_required(VERSION 3.22)
project( pipeline )

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

#find_package( AprilTags REQUIRED )
find_package(PkgConfig REQUIRED)

pkg_search_module(APRILTAGS REQUIRED apriltags)
include_directories ( ${APRILTAGS_INCLUDE_DIRS} )


#include_directories (/usr/local/include/apriltag)
add_executable( pipeline main.cpp )
target_link_libraries( pipeline ${OpenCV_LIBS} )
target_link_libraries ( pipeline ${APRILTAGS_LIBRARIES})
