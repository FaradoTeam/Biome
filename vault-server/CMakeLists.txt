project(biome-vault-server VERSION 0.0.1)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(ICU REQUIRED)

set(SERVER_SOURCES
    # Application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.cpp
    # API Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/rest_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/middleware/impl/auth_middleware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/auth_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/base_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/board_columns_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/boards_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/comment_documents_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/comments_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/documents_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/edges_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/field_type_possible_values_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/field_types_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/item_documents_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/item_histories_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/item_links_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/item_types_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/item_user_states_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/items_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/link_types_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/phases_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/plans_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/private_messages_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/project_teams_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/projects_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/role_menu_items_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/roles_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/rule_item_types_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/rule_projects_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/rule_states_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/rules_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/special_days_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/standard_days_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/states_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/team_messages_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/teams_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/user_actions_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/user_days_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/user_notifications_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/user_team_roles_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/user_todos_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/users_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/handlers/workflows_handler.cpp
    # Logic Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/auth_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/authorization_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/board_column_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/board_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/comment_document_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/comment_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/document_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/edge_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/field_type_possible_value_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/field_type_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/item_document_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/item_history_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/item_link_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/item_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/item_type_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/item_user_state_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/link_type_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/phase_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/plan_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/private_message_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/project_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/project_team_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/role_menu_item_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/role_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/rule_item_type_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/rule_project_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/rule_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/rule_state_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/special_day_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/standard_day_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/state_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/team_message_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/team_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_action_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_day_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_notification_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_team_role_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/user_todo_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic/impl/workflow_service.cpp
    # Repo Layer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_board_column_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_board_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_comment_document_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_comment_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_document_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_edge_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_field_type_possible_value_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_field_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_document_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_field_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_history_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_link_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_item_user_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_link_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_phase_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_plan_item_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_plan_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_private_message_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_project_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_project_team_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_role_menu_item_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_role_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_rule_item_type_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_rule_project_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_rule_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_rule_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_special_day_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_standard_day_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_state_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_team_message_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_team_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_action_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_day_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_notification_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_team_role_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_user_todo_repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/repo/sqlite/sqlite_workflow_repository.cpp
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
    # Helpers
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/helpers/string_helper.cpp
    # DTO
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/activate_plan_request.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/fork_plan_request.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/plan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/dto/plan_item.cpp
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
    icu::icu
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
