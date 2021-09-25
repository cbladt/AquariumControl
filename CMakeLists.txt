cmake_minimum_required(VERSION 2.8)

project(AquariumControl C CXX)
add_executable(${PROJECT_NAME} main.cpp AquariumService.c)
include_directories(.)
