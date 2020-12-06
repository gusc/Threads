cmake_minimum_required(VERSION 3.8)
project(ThreadSafeSignals VERSION 1.0.0 LANGUAGES CXX)

option(ThreadSafeSignals_BuildTests "Build the unit tests when BUILD_TESTING is enabled." ON)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

include(CTest)
if(BUILD_TESTING AND ThreadSafeSignals_BuildTests)
    add_subdirectory(Tests)
endif()