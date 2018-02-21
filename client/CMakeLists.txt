find_package(Box2D REQUIRED)
find_package(Websocketpp REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

set(CLIENT_INCLUDE_DIRS
    ${BOX2D_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/common
    ${Websocketpp_INCLUDE_DIRS}
)

set(CLIENT_DEPEND_LIBS
    ${BOX2D_LIBRARY}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${Boost_SYSTEM_LIBRARY}
)

set(CLIENT_SOURCES client.cpp render.cpp)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CLIENT_INCLUDE_DIRS})
link_directories(${PROJECT_SOURCE_DIR}/common)
add_executable(Client ${CLIENT_SOURCES})
target_link_libraries(Client Common ${CLIENT_DEPEND_LIBS})