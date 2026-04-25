cmake_minimum_required(VERSION 3.10)
project(Survivor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(survivor survivor.cpp)

target_compile_options(survivor PRIVATE
    -Wall
    -Wextra
)

if(WIN32)
    target_compile_definitions(survivor PRIVATE _WIN32_WINNT=0x0600)
endif()