cmake_minimum_required(VERSION 3.15.7)

project(farado-vault-server VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/rest_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/middleware/auth_middleware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/items_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/auth_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

set(SERVER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/rest_server.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/middleware/auth_middleware.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/items_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/auth_handler.h
)

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log/log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/config/config.cpp
)

set(COMMON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log/log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/config/config.h
)

add_executable(
    ${PROJECT_NAME}
    ${SERVER_SOURCES}
    ${SERVER_HEADERS}
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(
    ${PROJECT_NAME}
    boost::boost
    nlohmann_json::nlohmann_json
    Threads::Threads
    openssl::openssl
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/cpp-httplib/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/jwt-cpp/include/
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
