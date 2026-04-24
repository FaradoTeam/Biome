project(farado_rest_server_tests)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(cpprestsdk REQUIRED)
find_package(jwt-cpp REQUIRED)

set(SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/api/rest_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/api/middleware/impl/auth_middleware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/api/handlers/auth_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src/api/handlers/items_handler.cpp
)

set(COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/auth_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/auth_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/board_column.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/change_password_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/comment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/comment_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/edge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/error_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/field_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/field_type_possible_value.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item_field.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item_history.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item_link.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/item_user_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/link_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/page_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/phase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/private_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/project_team.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/role.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/role_menu_item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/rule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/rule_item_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/rule_project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/rule_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/special_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/standard_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/team.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/team_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/user.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/user_action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/user_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/user_notification.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/user_team_role.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/user_todo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/dto/workflow.cpp
)

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rest_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_token_expiration.cpp
)

add_executable(
    ${PROJECT_NAME}
    ${SERVER_SOURCES}
    ${COMMON_SOURCES}
    ${TEST_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vault-server/src
    ${Boost_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    boost::boost
    nlohmann_json::nlohmann_json
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    cpprestsdk::cpprest
    jwt-cpp::jwt-cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME RestServerTests COMMAND ${PROJECT_NAME})
