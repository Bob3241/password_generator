cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
project(password_generator)

add_executable(password_generator ./src/main.cpp ./src/web_server.cpp ./src/utils.cpp)
