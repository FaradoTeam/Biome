#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с элементами (CRUD операции).
 */
class ItemsHandler final : public BaseHandler
{
public:
    explicit ItemsHandler(std::shared_ptr<services::IItemService> itemService);

    /**
     * @brief Получает список элементов с пагинацией.
     */
    void handleGetItems(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает элемент по ID.
     */
    void handleGetItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новый элемент.
     */
    void handleCreateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующий элемент.
     */
    void handleUpdateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Мягкое удаление элемента.
     */
    void handleDeleteItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Восстанавливает элемент из мягкого удаления.
     */
    void handleRestoreItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает значения всех полей элемента.
     */
    void handleGetItemFields(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Устанавливает значение поля элемента.
     */
    void handleSetItemField(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет значение поля элемента.
     */
    void handleDeleteItemField(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IItemService> m_itemService;
};

} // namespace handlers
} // namespace server
