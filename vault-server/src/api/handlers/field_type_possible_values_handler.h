#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/ifield_type_possible_value_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с возможными значениями полей.
 */
class FieldTypePossibleValuesHandler final : public BaseHandler
{
public:
    explicit FieldTypePossibleValuesHandler(
        std::shared_ptr<services::IFieldTypePossibleValueService> service
    );

    /**
     * @brief Получает список возможных значений с пагинацией.
     */
    void handleGetValues(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает возможное значение по ID.
     */
    void handleGetValue(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новое возможное значение.
     */
    void handleCreateValue(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующее возможное значение.
     */
    void handleUpdateValue(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет возможное значение.
     */
    void handleDeleteValue(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все возможные значения для указанного типа поля.
     */
    void handleGetValuesByFieldType(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IFieldTypePossibleValueService> m_service;
};

} // namespace handlers
} // namespace server
