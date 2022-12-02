cmake_minimum_required(VERSION 3.18)
project(Threads VERSION 1.0.0 LANGUAGES CXX)

option(Threads_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)

set(SOURCES
	"include/Threads/Signal.hpp"
    "include/Threads/TaskQueue.hpp"
	"include/Threads/Thread.hpp"
    "include/Threads/ThreadPool.hpp"
    "include/Threads/Utilities.hpp"
)

if (${CMAKE_VERSION} VERSION_GREATER "3.19.0")
	add_library(${PROJECT_NAME} INTERFACE ${SOURCES})
else()
	add_library(${PROJECT_NAME} INTERFACE)
endif()

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

include(CTest)
if(BUILD_TESTING AND Threads_BuildTests)
    add_subdirectory(Tests)
endif()
