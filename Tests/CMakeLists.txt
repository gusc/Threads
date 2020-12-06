cmake_minimum_required(VERSION 3.8)
project(ThreadsTests VERSION 1.0.0 LANGUAGES CXX)

set(SOURCES
	"main.cpp"
	"SignalTests.hpp"
	"SignalTests.cpp"
	"ThreadTests.hpp"
	"ThreadTests.cpp"
	"Utilities.hpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/../include/)
