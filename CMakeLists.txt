cmake_minimum_required(VERSION 3.15)
project(ChatApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch your middleware from Git
FetchContent_Declare(
  mmw
  GIT_REPOSITORY git@github.com:BeckSM64/Minimal-Middleware.git
  GIT_TAG v1.0.1
)
FetchContent_MakeAvailable(mmw)
set(BUILD_BROKER ON CACHE BOOL "Build broker executable")

# Fetch SFML
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.5.1 # or latest stable tag
)
FetchContent_MakeAvailable(SFML)

# Link SFML to your target
# Example: target_link_libraries(my_app PRIVATE sfml-graphics sfml-window sfml-system)


# Create chat executable
# add_executable(chat main.cpp)
add_executable(Server Server.cpp)
add_executable(Client Client.cpp)

# Link against middleware
target_link_libraries(Server PRIVATE mmw::mmw pthread sfml-graphics sfml-window sfml-system)
target_link_libraries(Client PRIVATE mmw::mmw pthread sfml-graphics sfml-window sfml-system)
