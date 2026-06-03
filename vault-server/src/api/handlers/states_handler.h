#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/istate_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с состояниями.
 */
class StatesHandler final : public BaseHandler
{
public:
    explicit StatesHandler(
        std::shared_ptr<services::IStateService> stateService
    );

    void handleGetStates(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetState(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateState(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateState(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteState(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IStateService> m_stateService;
};

} // namespace handlers
} // namespace server
