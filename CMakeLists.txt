cmake_minimum_required(VERSION 3.2)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x  -g -DLOG_USE_COLOR") 
PROJECT(test_demo)

INCLUDE_DIRECTORIES("client")
INCLUDE_DIRECTORIES("log")
INCLUDE_DIRECTORIES("Base")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB_RECURSE source_file Base/*.cpp Base/*.hpp Base/*.h)

message("${source_file}")



AUX_SOURCE_DIRECTORY("client" DIR_SRCS_CLIENT)
AUX_SOURCE_DIRECTORY("log" DIR_SRCS_LOG)
message("${DIR_SRCS_LOG}")

#ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS_CLIENT} ${DIR_SRCS_LOG} ${source_file} "./testdemo/ThreadPoolTest.cpp")
ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS_CLIENT} ${DIR_SRCS_LOG} ${source_file} "./testdemo/SSClientTest.cpp")

TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread event )
