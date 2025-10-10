cmake_minimum_required(VERSION 3.10)

project(Engine)

find_package(SDL3 CONFIG REQUIRED)

add_executable(Engine src/main.cpp )

target_link_libraries(Engine PRIVATE SDL3::SDL3)