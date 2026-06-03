#include <algorithm>
#include <cctype>
#include <regex>

#include <cpprest/uri.h>

#include "common/dto/phase.h"
#include "common/helpers/json_helper.hpp"
#include "common/helpers/time_helpers.h"
#include "common/log/log.h"

#include "phases_handler.h"

namespace server
{
namespace handlers
{

PhasesHandler::PhasesHandler(
    std::shared_ptr<services::IPhaseService> phaseService
)
    : m_phaseService(std::move(phaseService))
{
    if (!m_phaseService)
    {
        LOG_WARN << "PhasesHandler инициализирован без PhaseService";
    }
}

void PhasesHandler::handleGetPhases(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    auto params = extractQueryParams(request);

    // Параметры пагинации
    int page = 1;
    if (params.count("page"))
    {
        try
        {
            page = std::stoi(params["page"]);
            if (page < 1)
                page = 1;
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetPhases: неверный параметр page: " << params["page"];
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 1;
            if (pageSize > 100)
                pageSize = 100; // Ограничиваем максимальный размер страницы
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetPhases: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> projectId = std::nullopt;
    if (params.count("projectId"))
    {
        try
        {
            projectId = std::stoll(params["projectId"]);
            if (projectId <= 0)
                projectId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetPhases: неверный параметр projectId: " << params["projectId"];
        }
    }

    std::optional<bool> isArchive = std::nullopt;
    if (params.count("isArchive"))
    {
        isArchive = parseBool(params["isArchive"]);
    }

    LOG_DEBUG
        << "GET /api/phases: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", projectId=" << (projectId.has_value() ? std::to_string(*projectId) : "none")
        << ", isArchive=" << (isArchive.has_value() ? (*isArchive ? "true" : "false") : "none");

    try
    {
        auto phasesPage = m_phaseService->phases(
            page,
            pageSize,
            userId,
            projectId,
            isArchive
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < phasesPage.phases.size(); ++i)
        {
            items[i] = dto::toWebJson(phasesPage.phases[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(phasesPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка фаз: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void PhasesHandler::handleGetPhase(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t phaseId = extractIdFromPath(request);
    if (phaseId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid phase ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /api/phases/" << phaseId << " from user " << userId;

    try
    {
        // Проверка прав происходит внутри phase()
        auto phase = m_phaseService->phase(phaseId, userId);
        if (!phase)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Phase not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(phase->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении фазы " << phaseId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void PhasesHandler::handleCreatePhase(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    int64_t userId = *userIdOpt;

    LOG_DEBUG << "POST /api/phases from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Phase phase(nlohmannJson);

                    // Валидация обязательных полей (простая, основная валидация в сервисе)
                    if (!phase.caption.has_value() || phase.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Phase caption is required");
                        request.reply(resp);
                        return;
                    }

                    if (!phase.projectId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Project ID is required");
                        request.reply(resp);
                        return;
                    }

                    // Проверка прав происходит внутри createPhase()
                    auto created = m_phaseService->createPhase(phase, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create phase: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал фазу id=" << *created->id
                        << " в проекте " << *created->projectId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании фазы: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void PhasesHandler::handleUpdatePhase(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t phaseId = extractIdFromPath(request);
    if (phaseId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid phase ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /api/phases/" << phaseId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, phaseId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = phaseId;
                    dto::Phase phase(nlohmannJson);

                    // Проверка прав происходит внутри updatePhase()
                    auto updated = m_phaseService->updatePhase(phase, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или фаза не найдена
                        auto existing = m_phaseService->phase(phaseId, userId);
                        if (!existing)
                        {
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Phase not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this phase");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил фазу " << phaseId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении фазы " << phaseId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void PhasesHandler::handleDeletePhase(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t phaseId = extractIdFromPath(request);
    if (phaseId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid phase ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /api/phases/" << phaseId << " from user " << userId;

    try
    {
        // Сначала проверяем существование фазы
        auto existing = m_phaseService->phase(phaseId, userId);
        if (!existing)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Phase not found");
            request.reply(resp);
            return;
        }

        // Проверка прав происходит внутри archivePhase()
        bool success = m_phaseService->archivePhase(phaseId, userId);
        if (!success)
        {
            web::http::http_response resp(web::http::status_codes::Forbidden);
            sendErrorResponse(resp, 403, "Insufficient permissions to archive this phase");
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " архивировал фазу " << phaseId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при архивации фазы " << phaseId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
