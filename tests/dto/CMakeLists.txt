project(test_dto)

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
    ${PROJECT_NAME}
    ${DTO_SOURCES}
    ${DTO_TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    Boost::unit_test_framework
    nlohmann_json::nlohmann_json
    Threads::Threads
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME TestDto COMMAND ${PROJECT_NAME})
