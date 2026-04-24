project(farado_repo_tests)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

set(SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/database_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_result_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_statement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_transaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_user_repository.cpp
)

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_user_repository.cpp
)

add_executable(
    ${PROJECT_NAME}
    ${SERVER_SRC}
    ${TEST_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/
    ${Boost_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    boost::boost
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME RepoTests COMMAND ${PROJECT_NAME})
