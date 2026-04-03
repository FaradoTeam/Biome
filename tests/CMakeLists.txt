cmake_minimum_required(VERSION 3.15.7)

project(farado_tests)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
option(BUILD_DTO_TESTS "Build tests for data transfer objects" ON)
option(BUILD_ALL_TESTS "Build all test" OFF)
# ---------------------------------------------------------------------------- #

enable_testing()

if(BUILD_DTO_TESTS OR BUILD_ALL)
    add_subdirectory(dto)
endif()
