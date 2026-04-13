cmake_minimum_required(VERSION 3.15.7)

project(farado_storage_tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(SQLite3 REQUIRED)

set(VAULT_SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/database_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_result_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_statement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_transaction.cpp
)

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_statement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_result_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_transaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_integration.cpp
)

add_executable(
    ${PROJECT_NAME}
    ${VAULT_SERVER_SRC}
    ${TEST_SOURCES}
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
    Boost::unit_test_framework
    SQLite::SQLite3
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME StorageTests COMMAND ${PROJECT_NAME})
