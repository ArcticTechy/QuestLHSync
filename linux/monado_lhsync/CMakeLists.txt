cmake_minimum_required(VERSION 3.16)
project(monado_lhsync CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(monado_lhsync main.cpp)
target_link_libraries(monado_lhsync pthread)
