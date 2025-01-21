cmake_minimum_required(VERSION 3.10)

project(Rack VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES
    src/server.c
)

add_library(http STATIC ${SOURCES})

add_executable(tests tests/test_http.c)

target_link_libraries(tests http)

target_compile_options(http PRIVATE -Wall -Wextra -pedantic)