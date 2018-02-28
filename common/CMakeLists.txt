find_package(Box2D REQUIRED)
find_package(Websocketpp REQUIRED)

set(COMMON_INCLUDE_DIRS
    ${BOX2D_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann
)

set(COMMON_SOURCES 
    physics.cpp
    scene_objects.cpp
    scene.cpp
    cache.cpp
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${COMMON_INCLUDE_DIRS})
add_library(Common ${COMMON_SOURCES})