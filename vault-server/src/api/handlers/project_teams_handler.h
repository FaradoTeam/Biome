#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "base_handler.h"
#include "logic/iproject_team_service.h"

namespace server::handlers
{

/**
 * @brief Обработчик запросов для работы со связями проектов и команд.
 */
class ProjectTeamsHandler final : public BaseHandler
{
public:
    explicit ProjectTeamsHandler(
        std::shared_ptr<services::IProjectTeamService> service
    );

    void handleGetItems(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteItem(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IProjectTeamService> m_service;
};

} // namespace server::handlers
