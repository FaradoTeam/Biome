#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_history_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с историей изменений элементов.
 */
class ItemHistoriesHandler final : public BaseHandler
{
public:
    explicit ItemHistoriesHandler(
        std::shared_ptr<services::IItemHistoryService> service
    );

    /**
     * @brief Получает список записей истории изменений.
     * GET /items/{itemId}/histories
     * GET /items/histories (с фильтрацией)
     */
    void handleGetItemHistories(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает запись истории по ID.
     * GET /items/histories/{id}
     */
    void handleGetItemHistory(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает последнюю запись истории для элемента.
     * GET /items/{itemId}/histories/last
     */
    void handleGetLastItemHistory(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую запись истории.
     * POST /items/{itemId}/histories
     */
    void handleCreateItemHistory(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет запись истории.
     * DELETE /items/histories/{id}
     */
    void handleDeleteItemHistory(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IItemHistoryService> m_service;
};

} // namespace handlers
} // namespace server
