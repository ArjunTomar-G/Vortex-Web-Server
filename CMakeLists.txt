# Require a relatively modern version of CMake
cmake_minimum_required(VERSION 3.10)

# Define your project name and version
project(VortexServer VERSION 1.0 DESCRIPTION "High-Performance Asynchronous HTTP Server")

# Force C++17 Standard (Gives you modern features like std::string_view for faster parsing)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable strict compiler warnings to catch memory leaks and uninitialized variables early
add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)

# Explicitly find the system Threading library (Required for Teammate B's std::thread)
find_package(Threads REQUIRED)

# Tell the compiler where to look for your .hpp header files
include_directories(${CMAKE_SOURCE_DIR}/include)

# Map all of your implementation files exactly as they appear in your structure
set(SOURCES
    src/main.cpp
    src/network/EpollServer.cpp
    src/network/Connection.cpp
    src/concurrency/ThreadPool.cpp
    src/http/Request.cpp
    src/http/Response.cpp
    src/utils/Logger.cpp
)

# Command to build the final executable named 'vortex_server'
add_executable(vortex_server ${SOURCES})

# Link the Linux threading library to your executable
target_link_libraries(vortex_server PRIVATE Threads::Threads)