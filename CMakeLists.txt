cmake_minimum_required(VERSION 3.16)
project(handmade)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(handmade code/sdl_handmade.cpp)

target_include_directories(handmade PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(handmade ${SDL2_LIBRARIES})
target_compile_options(handmade PRIVATE ${SDL2_CFLAGS_OTHER})

set_target_properties(handmade PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/target
)