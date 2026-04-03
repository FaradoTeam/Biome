cmake_minimum_required(VERSION 3.15.7)

project(farado_dto_tests)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${nlohmann_json_INCLUDE_DIRS})

set(DTO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto)
file(GLOB DTO_SOURCES
    ${DTO_SOURCE_DIR}/*.cpp
)

file(GLOB DTO_TEST_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp
)

add_executable(
    dto_tests
    ${DTO_SOURCES}
    ${DTO_TEST_SOURCES}
)

target_link_libraries(dto_tests
    Boost::unit_test_framework
    nlohmann_json::nlohmann_json
    Threads::Threads
)

set_target_properties(dto_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME DtoTests COMMAND dto_tests)

message(STATUS "DTO tests found: ${DTO_TEST_SOURCES}")
message(STATUS "DTO test executable: dto_tests")
