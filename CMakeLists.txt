cmake_minimum_required(VERSION 2.8)
project( ParkoPlan )
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

set( SRC
    cpp/ConfigLoad.cpp
    cpp/Parking.cpp
    cpp/utils.cpp
    cpp/main.cpp
)

set( HEADERS
  cpp/ConfigLoad.h
  cpp/Parking.h
  cpp/utils.h
)

find_package(Threads REQUIRED)

add_executable( detect-parking ${SRC} ${HEADERS} )
target_link_libraries( detect-parking ${OpenCV_LIBS} Threads::Threads )
