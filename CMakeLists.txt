cmake_minimum_required(VERSION 3.10)

project(Rack VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES
    src/server.c
    src/http.c
    src/list.c
)

add_library(http STATIC ${SOURCES})

add_executable(test_http tests/test_http.c)
target_link_libraries(test_http http)

add_executable(test_list tests/test_list.c)
target_link_libraries(test_list http)

target_compile_options(http PRIVATE -Wall -Wextra -pedantic)