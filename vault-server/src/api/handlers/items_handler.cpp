#include <iostream>
#include <regex>

#include <httplib.h>

#include <nlohmann/json.hpp>

#include "common/log/log.h"

#include "items_handler.h"

namespace farado
{
namespace server
{
namespace handlers
{

ItemsHandler::ItemsHandler()
{
    LOG_INFO << "ItemsHandler created";
}

void ItemsHandler::handleGetItems(const httplib::Request& req, httplib::Response& res, const std::string& userId)
{
    LOG_INFO << "GET /api/items from user " << userId;
    
    // Получаем параметры запроса
    int page = 1;
    int pageSize = 20;
    
    if (req.has_param("page"))
    {
        page = std::stoi(req.get_param_value("page"));
    }
    if (req.has_param("pageSize"))
    {
        pageSize = std::stoi(req.get_param_value("pageSize"));
    }
    
    // TODO: Здесь должна быть логика получения элементов из БД с учетом прав доступа
    
    nlohmann::json response;
    response["items"] = nlohmann::json::array();
    
    // Пример данных
    for (int i = (page - 1) * pageSize + 1; i <= page * pageSize && i <= 100; ++i)
    {
        nlohmann::json item;
        item["id"] = i;
        item["caption"] = "Item " + std::to_string(i);
        item["content"] = "Content of item " + std::to_string(i);
        response["items"].push_back(item);
    }
    response["totalCount"] = 100;
    response["page"] = page;
    response["pageSize"] = pageSize;
    
    res.set_content(response.dump(), "application/json");
}

void ItemsHandler::handleGetItem(const httplib::Request& req, httplib::Response& res, const std::string& userId)
{
    int64_t itemId = extractItemId(req);
    if (itemId <= 0)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid item ID";
        res.status = 400;
        res.set_content(error.dump(), "application/json");
        return;
    }
    
    LOG_INFO << "GET /api/items/" << itemId << " from user " << userId;
    
    // TODO: Здесь должна быть логика получения элемента из БД с проверкой прав
    
    nlohmann::json response;
    response["id"] = itemId;
    response["caption"] = "Item " + std::to_string(itemId);
    response["content"] = "Content of item " + std::to_string(itemId);
    response["stateId"] = 1;
    response["phaseId"] = 1;
    
    res.set_content(response.dump(), "application/json");
}

void ItemsHandler::handleCreateItem(const httplib::Request& req, httplib::Response& res, const std::string& userId)
{
    LOG_INFO << "POST /api/items from user " << userId;
    
    try
    {
        nlohmann::json request = nlohmann::json::parse(req.body);
        
        // Валидация обязательных полей
        if (!request.contains("caption") || request["caption"].get<std::string>().empty())
        {
            nlohmann::json error;
            error["code"] = 400;
            error["message"] = "Field 'caption' is required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        // TODO: Здесь должна быть логика создания элемента в БД
        
        nlohmann::json response;
        response["id"] = 100;  // Сгенерированный ID
        response["caption"] = request["caption"];
        response["success"] = true;
        
        res.status = 201;
        res.set_content(response.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid JSON: " + std::string(e.what());
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

void ItemsHandler::handleUpdateItem(const httplib::Request& req, httplib::Response& res, const std::string& userId)
{
    int64_t itemId = extractItemId(req);
    if (itemId <= 0)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid item ID";
        res.status = 400;
        res.set_content(error.dump(), "application/json");
        return;
    }
    
    LOG_INFO << "PUT /api/items/" << itemId << " from user " << userId;
    
    try
    {
        nlohmann::json request = nlohmann::json::parse(req.body);
        
        // TODO: Здесь должна быть логика обновления элемента в БД
        
        nlohmann::json response;
        response["id"] = itemId;
        response["caption"] = request.value("caption", "");
        response["success"] = true;
        
        res.set_content(response.dump(), "application/json");
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid JSON: " + std::string(e.what());
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

void ItemsHandler::handleDeleteItem(const httplib::Request& req, httplib::Response& res, const std::string& userId)
{
    int64_t itemId = extractItemId(req);
    if (itemId <= 0)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid item ID";
        res.status = 400;
        res.set_content(error.dump(), "application/json");
        return;
    }
    
    LOG_INFO << "DELETE /api/items/" << itemId << " from user " << userId;
    
    // TODO: Здесь должна быть логика удаления элемента из БД
    
    res.status = 204;
}

int64_t ItemsHandler::extractItemId(const httplib::Request& req)
{
    // Путь имеет формат /api/items/123
    // Используем matches из regex, который мы установили в роутере
    if (req.matches.size() > 1)
    {
        try
        {
            return std::stoll(req.matches[1]);
        }
        catch (const std::exception&)
        {
            return -1;
        }
    }
    
    return -1;
}

} // namespace handlers
} // namespace server
} // namespace farado
