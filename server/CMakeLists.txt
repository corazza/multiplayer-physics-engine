set(BOX2D_STATIC_LIBRARIES TRUE)
find_package(Box2D REQUIRED)
find_package(Websocketpp REQUIRED)
set(Boost_USE_STATIC_LIBS   ON)
find_package(Boost COMPONENTS system REQUIRED)

set(SERVER_INCLUDE_DIRS
    ${BOX2D_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/common
    ${Websocketpp_INCLUDE_DIRS}
)

set(SERVER_DEPEND_LIBS
    ${BOX2D_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

set(SERVER_SOURCES server.cpp main.cpp scene_management.cpp)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${SERVER_INCLUDE_DIRS})
link_directories(${PROJECT_SOURCE_DIR}/common)
add_executable(Server ${SERVER_SOURCES})
target_link_libraries(Server Common ${SERVER_DEPEND_LIBS})