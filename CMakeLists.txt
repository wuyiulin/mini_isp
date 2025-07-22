cmake_minimum_required(VERSION 3.10)
project(mini_ISP)

set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

file(GLOB SOURCES src/*.cpp)

add_executable(mini_isp main.cpp ${SOURCES})
target_link_libraries(mini_isp ${OpenCV_LIBS})

find_package(OpenCV REQUIRED)
find_library(LIBRAW_LIB NAMES raw PATH_SUFFIXES lib)
include_directories(${OpenCV_INCLUDE_DIRS} /usr/include/libraw)
target_link_libraries(mini_isp ${OpenCV_LIBS} ${LIBRAW_LIB})
