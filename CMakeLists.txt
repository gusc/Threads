cmake_minimum_required(VERSION 3.18)
project(Threads VERSION 1.0.0 LANGUAGES CXX)

option(Threads_BuildTests "Build the unit tests." OFF)
option(Threads_BuildExamples "Build the examples." OFF)

set(SOURCES
	"include/Threads/Signal.hpp"
    "include/Threads/TaskQueue.hpp"
	"include/Threads/Thread.hpp"
    "include/Threads/ThreadPool.hpp"
    "include/Threads/private/Utilities.hpp"
    "include/Threads/private/LockedReference.hpp"
    "include/Threads/private/ThreadStructures.hpp"
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
if(Threads_BuildTests)
    if(NOT TARGET gtest)
        message(FATAL_ERROR "You must provide Google Test library to build unittests")
    endif()
    add_subdirectory(Tests)
endif()
if(Threads_BuildExamples)
    add_subdirectory(Examples)
endif()
