cmake_minimum_required(VERSION 3.18)
project(ThreadsTests VERSION 1.0.0 LANGUAGES CXX)

include(GoogleTest)

set(SOURCES
	"main.cpp"
	"SignalMocks.hpp"
	"SignalTests.cpp"
    "TaskQueueMocks.hpp"
    "TaskQueueTests.cpp"
	"ThreadMocks.hpp"
	"ThreadTests.cpp"
	"Utilities.hpp"
)
list(SORT SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${SOURCES})

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/../include/)
target_link_libraries(${PROJECT_NAME} PRIVATE gtest gmock)

gtest_discover_tests(${PROJECT_NAME})
