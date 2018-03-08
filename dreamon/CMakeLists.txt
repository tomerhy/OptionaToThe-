cmake_minimum_required(VERSION 2.8)
project(dreamon)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

INCLUDE_DIRECTORIES(
)

set(SOURCE_FILES
	dreamon.cpp
)

add_executable(dreamon ${SOURCE_FILES})

target_link_libraries( dreamon
	log4cpp
	event
)
