cmake_minimum_required(VERSION 3.6)
project(ChatApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

FetchContent_Declare(
  mmw
  GIT_REPOSITORY git@github.com:BeckSM64/Minimal-Middleware.git
  GIT_TAG abstract-transport
)
FetchContent_MakeAvailable(mmw)

set(BUILD_BROKER ON CACHE BOOL "Build broker executable")
set(SFML_BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML as static" FORCE)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/Zombieschannel/SFML.git
    GIT_TAG 3.1.0-EMCC
)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -sUSE_WEBGL2=1 -sFULL_ES3=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -sUSE_WEBGL2=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

FetchContent_MakeAvailable(SFML)

add_executable(Server Server.cpp)
add_executable(Client Client.cpp)

target_link_libraries(
    Server PRIVATE
    mmw::mmw
    pthread
    sfml-graphics
    sfml-window
    sfml-system
)

target_link_libraries(
    Client PRIVATE
    mmw::mmw
    pthread
    sfml-graphics
    sfml-window
    sfml-system
)
