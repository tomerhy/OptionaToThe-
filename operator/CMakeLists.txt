cmake_minimum_required(VERSION 2.8)
project(operator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

INCLUDE_DIRECTORIES(
)

set(SOURCE_FILES
	operator.cpp
	config.h
)

add_executable(operator ${SOURCE_FILES})

target_link_libraries( operator
	log4cpp
	event
)
