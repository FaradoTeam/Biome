#include <regex>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>

#include "common/log/log.h"

#include "items_handler.h"

namespace server
{
namespace handlers
{

// TODO: Эта сущность будет переработана, сейчас это прототип-набросок.

ItemsHandler::ItemsHandler()
{
}

std::map<std::string, std::string> ItemsHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto uri = request.relative_uri();
    auto query = uri.query(); // Получаем строку запроса (часть после '?')

    if (!query.empty())
    {
        // Разбираем query-строку вида "key1=value1&key2=value2"
        auto queryParams = web::uri::split_query(query);
        for (const auto& param : queryParams)
        {
            params[param.first] = param.second;
        }
    }

    return params;
}

void ItemsHandler::handleGetItems(
    const web::http::http_request& request,
    const std::string& userId
)
{
    LOG_INFO << "GET /api/items from user " << userId;

    // Получаем параметры пагинации из query-строки
    auto params = extractQueryParams(request);

    int page = 1; // Номер страницы (начиная с 1)
    int pageSize = 20; // Количество элементов на странице

    if (params.find("page") != params.end())
    {
        page = std::stoi(params["page"]);
    }
    if (params.find("pageSize") != params.end())
    {
        pageSize = std::stoi(params["pageSize"]);
    }

    // TODO: Здесь должна быть логика получения элементов из БД
    // с учетом прав доступа

    web::json::value response;
    response["items"] = web::json::value::array();

    // Пример данных для тестирования (в реальном коде данные приходят из БД)
    int start = (page - 1) * pageSize + 1;
    int end = std::min(page * pageSize, 100); // Всего 100 тестовых элементов

    int index = 0;
    for (int i = start; i <= end; ++i)
    {
        web::json::value item;
        item["id"] = web::json::value::number(i);
        item["caption"] = web::json::value::string("Item " + std::to_string(i));
        item["content"] = web::json::value::string("Content of item " + std::to_string(i));
        response["items"][index++] = item;
    }

    // Метаинформация для пагинации
    response["totalCount"] = web::json::value::number(100);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void ItemsHandler::handleGetItem(
    const web::http::http_request& request,
    const std::string& userId
)
{
    int64_t itemId = extractItemId(request);
    if (itemId <= 0)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(response, 400, "Invalid item ID");
        request.reply(response);
        return;
    }

    LOG_INFO << "GET /api/items/" << itemId << " from user " << userId;

    // TODO: Здесь должна быть логика получения элемента из БД с проверкой прав

    web::json::value response;
    response["id"] = web::json::value::number(itemId);
    response["caption"] = web::json::value::string("Item " + std::to_string(itemId));
    response["content"] = web::json::value::string("Content of item " + std::to_string(itemId));
    response["stateId"] = web::json::value::number(1);
    response["phaseId"] = web::json::value::number(1);

    request.reply(web::http::status_codes::OK, response);
}

void ItemsHandler::handleCreateItem(
    const web::http::http_request& request,
    const std::string& userId
)
{
    LOG_INFO << "POST /api/items from user " << userId;

    try
    {
        request
            .extract_json()
            .then(
                [this, request, userId](pplx::task<web::json::value> task)
                {
                    web::json::value jsonBody;
                    try
                    {
                        jsonBody = task.get();
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            "Invalid JSON: " + std::string(e.what())
                        );
                        request.reply(response);
                        return;
                    }

                    // Валидация: поле "caption" обязательно для заполнения
                    if (!jsonBody.has_field("caption")
                        || jsonBody.at("caption").as_string().empty())
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            "Field 'caption' is required"
                        );
                        request.reply(response);
                        return;
                    }

                    // TODO: Здесь должна быть логика создания элемента в БД

                    web::json::value response;
                    response["id"] = web::json::value::number(100); // Сгенерированный ID
                    response["caption"] = jsonBody.at("caption");
                    response["success"] = web::json::value::boolean(true);

                    request.reply(web::http::status_codes::Created, response);
                }
            )
            .wait();
    }
    catch (const std::exception& e)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(
            response,
            400,
            "Invalid request: " + std::string(e.what())
        );
        request.reply(response);
    }
}

void ItemsHandler::handleUpdateItem(const web::http::http_request& request, const std::string& userId)
{
    int64_t itemId = extractItemId(request);
    if (itemId <= 0)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(response, 400, "Invalid item ID");
        request.reply(response);
        return;
    }

    LOG_INFO << "PUT /api/items/" << itemId << " from user " << userId;

    try
    {
        request
            .extract_json()
            .then(
                [this, request, itemId](pplx::task<web::json::value> task)
                {
                    web::json::value jsonBody;
                    try
                    {
                        jsonBody = task.get();
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            "Invalid JSON: " + std::string(e.what())
                        );
                        request.reply(response);
                        return;
                    }

                    // TODO: Здесь должна быть логика обновления элемента в БД

                    web::json::value response;
                    response["id"] = web::json::value::number(itemId);
                    response["caption"] = jsonBody.has_field("caption")
                        ? jsonBody.at("caption")
                        : web::json::value::string("");
                    response["success"] = web::json::value::boolean(true);

                    request.reply(web::http::status_codes::OK, response);
                }
            )
            .wait();
    }
    catch (const std::exception& e)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(response, 400, "Invalid request: " + std::string(e.what()));
        request.reply(response);
    }
}

void ItemsHandler::handleDeleteItem(
    const web::http::http_request& request,
    const std::string& userId
)
{
    int64_t itemId = extractItemId(request);
    if (itemId <= 0)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(response, 400, "Invalid item ID");
        request.reply(response);
        return;
    }

    LOG_INFO << "DELETE /api/items/" << itemId << " from user " << userId;

    // TODO: Здесь должна быть логика удаления элемента из БД

    request.reply(web::http::status_codes::NoContent);
}

int64_t ItemsHandler::extractItemId(const web::http::http_request& request)
{
    // Извлекаем ID из пути, ожидая формат: /api/items/{id}
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/items/(\d+))"); // Регулярка для поиска числа после /api/items/
    std::smatch matches;

    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            return std::stoll(matches[1].str()); // Конвертируем строку в число
        }
        catch (const std::exception&)
        {
            return -1; // Ошибка конвертации
        }
    }

    return -1; // ID не найден
}

void ItemsHandler::sendErrorResponse(
    web::http::http_response& response,
    int code,
    const std::string& message
)
{
    // Формируем JSON с информацией об ошибке
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

} // namespace handlers
} // namespace server
