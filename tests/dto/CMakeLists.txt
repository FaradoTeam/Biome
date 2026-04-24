project(test_dto)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${nlohmann_json_INCLUDE_DIRS})

set(DTO_SOURCES
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

set(DTO_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_auth_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_board.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_board_column.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_change_password_request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_comment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_comment_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_edge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_error_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_field_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_field_type_possible_value.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item_document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item_field.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item_history.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item_link.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_item_user_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_link_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_page_response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_phase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_private_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_project_team.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_role.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_role_menu_item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rule_item_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rule_project.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rule_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_special_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_standard_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_team.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_team_message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_day.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_notification.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_team_role.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_todo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_workflow.cpp
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

add_test(NAME TestDto COMMAND ${PROJECT_NAME})
