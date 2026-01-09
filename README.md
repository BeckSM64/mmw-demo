# MMW Demo

This is a demo project to demonstrate the [Minimal Middleware](https://github.com/BeckSM64/Minimal-Middleware) C++ Library. It is a simple client-server example application. It uses the MMW and SFML graphics library to create playable characters that can update their positions on the other connected clients.

# Building
## Linux
The project is built using CMake. Run the following commands to build the project
```
git clone https://github.com/BeckSM64/mmw-demo.git
cd mmw-demo/
mkdir -p build/
cd build/
cmake ../ -DBUILD_BROKER=ON
make
```
This will build the project and the broker on Linux.

## Windows
Windows will require the vcpkg package manager to be installed. Then run the following build commands
```
git clone https://github.com/BeckSM64/mmw-demo.git
cd mmw-demo/
mkdir -p build/
cd build/
cmake ../ -DBUILD_BROKER=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Unix Makefiles"
make
```
NOTE: The CMAKE_TOOLCHAIN_FILE should be set to the path to where your vcpkg is installed.
This will build the project and the broker on Windows.

# Running
In order to run the demo, you'll need to run at least three applications
- Broker
- Server
- Client

The broker will be located at mmw-demo/build/deps/mmw-build/broker. So first run
```
./broker
```
Then run the server and at least one client from the build directory
```
./Server
./Client 1
./Client 2
```
The client takes an ID. This must be a unique value. If everything was done correctly, you should be able to control the character on screen using the arrow keys.
