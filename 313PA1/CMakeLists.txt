cmake_minimum_required(VERSION 3.12)
project(PA1)

set(CMAKE_CXX_STANDARD 14)


set(Source BuddyAllocator.cpp)
set(Header BuddyAllocator.h)

add_executable(PA1 main.cpp BuddyAllocator.cpp)
