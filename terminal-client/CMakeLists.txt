cmake_minimum_required(VERSION 3.14)
project(biome-terminal-client)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(ftxui REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log/log.cpp
)

set(TERMINAL_CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/api_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/navigation_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/auth_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/screens/login_screen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/screens/main_menu_screen.cpp
)

add_executable(${PROJECT_NAME}
    ${TERMINAL_CLIENT_SOURCES}
    ${COMMON_SOURCES}    
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    boost::boost
    ftxui::ftxui
    cpprestsdk::cpprestsdk
    Threads::Threads
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/client
)
