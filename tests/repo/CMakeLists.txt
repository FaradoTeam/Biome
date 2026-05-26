project(test_bvs_repo)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(ICU REQUIRED)

set(SERVER_SRC
    # common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/helpers/string_helper.cpp
    # server
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/database_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_result_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_statement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/storage/sqlite/sqlite_transaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_field_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_item_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_edge_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_user_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_workflow_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_project_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_phase_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_role_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_team_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_rule_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_rule_project_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_rule_item_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_rule_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_role_menu_item_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/repo/sqlite/sqlite_user_team_role_repository.cpp
)

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_field_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_item_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_edge_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_user_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_workflow_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_project_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_phase_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_team_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_role_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_rule_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_rule_project_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_rule_item_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_rule_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_role_menu_item_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sqlite_user_team_role_repository.cpp
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
    icu::icu
)

add_test(NAME TestBiomVaultServerRepo COMMAND ${PROJECT_NAME})
