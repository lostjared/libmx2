cmake_minimum_required(VERSION 3.10)
project(TestConsumer)

find_package(SDL2)
find_package(SDL2_ttf)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the libmx2 package
find_package(libmx2 REQUIRED)

# Create an executable
add_executable(test_app main.cpp)

# Link the libmx2 library
target_link_libraries(test_app PRIVATE libmx2::mx)
