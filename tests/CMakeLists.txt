# CMake configuration for DTO tests
# Auto-generated DTO tests will be compiled here

cmake_minimum_required(VERSION 3.15.7)

# Project for tests
project(farado_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${nlohmann_json_INCLUDE_DIRS})

# Collect all DTO test files
file(GLOB DTO_TEST_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dto/test_*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
)

# Create DTO test executable
if(DTO_TEST_SOURCES)
    add_executable(dto_tests ${DTO_TEST_SOURCES})
    
    target_link_libraries(dto_tests
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${nlohmann_json_LIBRARIES}
        Threads::Threads
    )
    
    # Set properties
    set_target_properties(dto_tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # Add test
    add_test(NAME DtoTests COMMAND dto_tests)
    
    message(STATUS "DTO tests found: ${DTO_TEST_SOURCES}")
    message(STATUS "DTO test executable: dto_tests")
else()
    message(WARNING "No DTO test files found in ${CMAKE_CURRENT_SOURCE_DIR}/dto/")
endif()

# Optionally, collect all other test files (non-DTO tests)
file(GLOB OTHER_TEST_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp
)

# Remove DTO test files from OTHER_TEST_SOURCES
foreach(DTO_TEST ${DTO_TEST_SOURCES})
    list(REMOVE_ITEM OTHER_TEST_SOURCES ${DTO_TEST})
endforeach()

# Create other tests executable if there are other tests
if(OTHER_TEST_SOURCES)
    add_executable(other_tests ${OTHER_TEST_SOURCES})
    
    target_link_libraries(other_tests
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${nlohmann_json_LIBRARIES}
        Threads::Threads
    )
    
    set_target_properties(other_tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    add_test(NAME OtherTests COMMAND other_tests)
    
    message(STATUS "Other tests found: ${OTHER_TEST_SOURCES}")
endif()
