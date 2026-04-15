cmake_minimum_required(VERSION 3.15.7)

project(farado_tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

if(BUILD_DTO_TESTS OR BUILD_ALL)
    add_subdirectory(dto)
endif()

if(BUILD_STORAGE_TESTS OR BUILD_ALL)
    add_subdirectory(storage)
endif()

if(BUILD_API_TESTS OR BUILD_ALL)
    add_subdirectory(rest_server)
endif()
