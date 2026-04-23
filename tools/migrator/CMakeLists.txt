cmake_minimum_required(VERSION 3.14)
project(migrator)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/log/log.cpp
)

set(STORAGE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/database_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_result_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_statement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_transaction.cpp
)

set(TOOL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/migration_manager.cpp
)

add_executable(${PROJECT_NAME}
    ${STORAGE_SOURCES}
    ${TOOL_SOURCES}
    ${COMMON_SOURCES}    
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/
    ${Boost_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    boost::boost
    SQLite::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
