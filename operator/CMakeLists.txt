cmake_minimum_required(VERSION 2.8)
project(operator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++0x")

INCLUDE_DIRECTORIES(
	/usr/include/libxml2
)

set(SOURCE_FILES
	operator.cpp
	config.h
	config.cpp
	op_manager.h
	op_manager.cpp
	record.h
	record.cpp
	informer.h
	dreamer.h
	dreamer.cpp
	threaded.h
	threaded.cpp
)

add_executable(operator ${SOURCE_FILES})

target_link_libraries( operator
	log4cpp
	event
	xml2
	pthread
)
