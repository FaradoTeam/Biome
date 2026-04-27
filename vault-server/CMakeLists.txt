project(biome-vault-server VERSION 0.0.1)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(SQLite3 REQUIRED)

set(SERVER_SOURCES
    # Application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
    # API Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/rest_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/middleware/impl/auth_middleware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/items_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/auth_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/users_handler.cpp
    # Logic Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/auth_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_service.cpp
    # Repo Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_repository.cpp
    # Storage Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/database_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/sqlite/sqlite_connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/sqlite/sqlite_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/sqlite/sqlite_result_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/sqlite/sqlite_statement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/sqlite/sqlite_transaction.cpp
)

set(COMMON_SOURCES
    # Logger
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/log/log.cpp
    # DTO
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/auth_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/auth_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/board_column.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/change_password_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/comment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/comment_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/edge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/error_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/field_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/field_type_possible_value.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item_field.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item_history.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item_link.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/item_user_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/link_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/page_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/phase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/private_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/project_team.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/role.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/role_menu_item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/rule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/rule_item_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/rule_project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/rule_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/special_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/standard_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/team.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/team_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/user.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/user_action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/user_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/user_notification.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/user_team_role.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/user_todo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/workflow.cpp
)

add_executable(
    ${PROJECT_NAME}
    ${SERVER_SOURCES}
    ${COMMON_SOURCES}
)

target_link_libraries(
    ${PROJECT_NAME}
    boost::boost
    nlohmann_json::nlohmann_json
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    cpprestsdk::cpprest
    jwt-cpp::jwt-cpp
    SQLite::SQLite3
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

set(TARGET_BINARY_DIR
    ${CMAKE_BINARY_DIR}/bin/server
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TARGET_BINARY_DIR}
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config/vault-server.ini"
        "${TARGET_BINARY_DIR}/vault-server.ini"
    COMMENT "Копирование vault-server.ini в выходную директорию"
)
