cmake_minimum_required(VERSION 3.28)
project(idk CXX)

set(CMAKE_CXX_STANDARD 23)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

# target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_20)