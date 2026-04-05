#pragma once

#include <memory>
#include <string>

namespace httplib
{
class Request;
class Response;
}

namespace farado
{
namespace server
{
namespace handlers
{

class ItemsHandler final
{
public:
    ItemsHandler();
    ~ItemsHandler() = default;

    /**
     * Получает список элементов
     */
    void handleGetItems(
        const httplib::Request& req,
        httplib::Response& res,
        const std::string& userId
    );

    /**
     * Получает элемент по ID
     */
    void handleGetItem(
        const httplib::Request& req,
        httplib::Response& res,
        const std::string& userId
    );

    /**
     * Создает новый элемент
     */
    void handleCreateItem(
        const httplib::Request& req,
        httplib::Response& res,
        const std::string& userId
    );

    /**
     * Обновляет существующий элемент
     */
    void handleUpdateItem(
        const httplib::Request& req,
        httplib::Response& res,
        const std::string& userId
    );

    /**
     * Удаляет элемент
     */
    void handleDeleteItem(
        const httplib::Request& req,
        httplib::Response& res,
        const std::string& userId
    );

private:
    /**
     * Извлекает ID из пути запроса
     */
    int64_t extractItemId(const httplib::Request& req);
};

} // namespace handlers
} // namespace server
} // namespace farado
