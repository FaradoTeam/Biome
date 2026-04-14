cmake_minimum_required(VERSION 3.15.7)

project(farado_tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
option(BUILD_DTO_TESTS "Build tests for data transfer objects" OFF)
option(BUILD_STORAGE_TESTS "Build tests for storage" ON)
option(BUILD_API_TESTS "Build tests for rest server" ON)
option(BUILD_ALL_TESTS "Build all test" OFF)
# ---------------------------------------------------------------------------- #

enable_testing()

if(BUILD_DTO_TESTS OR BUILD_ALL_TESTS)
    add_subdirectory(dto)
endif()

if(BUILD_STORAGE_TESTS OR BUILD_ALL_TESTS)
    add_subdirectory(storage)
endif()

if(BUILD_API_TESTS OR BUILD_ALL_TESTS)
    add_subdirectory(rest_server)
endif()
