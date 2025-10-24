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
    "include/Threads/private/ThreadApple.hpp"
    "include/Threads/private/ThreadLinux.hpp"
    "include/Threads/private/ThreadStructures.hpp"
    "include/Threads/private/ThreadWindows.hpp"
)
list(SORT SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/include/Threads" FILES ${SOURCES})

if (${CMAKE_VERSION} VERSION_GREATER "3.19.0")
	add_library(${PROJECT_NAME} INTERFACE)
else()
	add_library(${PROJECT_NAME} INTERFACE)
endif()
if(CMAKE_VERSION VERSION_LESS "3.22.0")
    target_sources(${PROJECT_NAME} INTERFACE ${SOURCES})
else()
    target_sources(${PROJECT_NAME} PUBLIC ${SOURCES})
endif()
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

include(CTest)
if(Threads_BuildTests)
    if(Threads_GoogleTest_DIR)
    	get_filename_component(Threads_GoogleTest_DIR "${Threads_GoogleTest_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../")
    else()
    	get_filename_component(Threads_GoogleTest_DIR "${CMAKE_CURRENT_LIST_DIR}/../googletest" ABSOLUTE "${CMAKE_CURRENT_LIST_DIR}/../")
    endif()
    if(NOT TARGET gtest)
    	add_subdirectory(${Threads_GoogleTest_DIR} googletest)
    endif()
    if(NOT TARGET gtest)
        message(FATAL_ERROR "You must provide Google Test library to build unittests")
    endif()
    add_subdirectory(Tests)
endif()
if(Threads_BuildExamples)
    add_subdirectory(Examples)
endif()
