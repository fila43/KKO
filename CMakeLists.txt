cmake_minimum_required(VERSION 3.8)
project(KKO)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_COMPILER /opt/rh/devtoolset-7/root/usr/bin/g++)
set(SOURCE_FILES main.cpp kko.cc kko.h)

add_executable(KKO ${SOURCE_FILES})