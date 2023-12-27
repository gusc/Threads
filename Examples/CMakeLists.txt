cmake_minimum_required(VERSION 3.8)
project(ThreadsExamples VERSION 1.0.0 LANGUAGES CXX)

set(SOURCES
	"main.cpp"
	"SignalExamples.hpp"
	"SignalExamples.cpp"
	"TaskQueueExamples.hpp"
	"TaskQueueExamples.cpp"
	"ThreadExamples.hpp"
	"ThreadExamples.cpp"
)
list(SORT SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${SOURCES})

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/../include/)
