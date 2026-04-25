cmake_minimum_required(VERSION 3.10)
project(Survivor VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SURVIVOR_VERSION ${PROJECT_VERSION})
set(SURVIVOR_DESCRIPTION "Self-preserving autonomous CLI tool")

add_executable(survivor survivor.cpp)

target_link_libraries(survivor PRIVATE shell32 advapi32 iphlpapi winmm kernel32)

target_compile_options(survivor PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

if(WIN32)
    target_compile_definitions(survivor PRIVATE _WIN32_WINNT=0x0600)
    set_target_properties(survivor PROPERTIES
        OUTPUT_NAME "survivor"
        DEBUG_POSTFIX ""
        RELEASE_POSTFIX ""
    )
endif()

install(TARGETS survivor DESTINATION bin)