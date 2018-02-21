cmake_minimum_required(VERSION 3.7)

project(Game)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-modules")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)