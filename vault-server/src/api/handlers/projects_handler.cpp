#include <cctype>

#include <cpprest/uri.h>

#include "common/dto/project.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "projects_handler.h"

namespace server
{
namespace handlers
{

ProjectsHandler::ProjectsHandler(
    std::shared_ptr<services::IProjectService> projectService
)
    : m_projectService(std::move(projectService))
{
    if (!m_projectService)
    {
        LOG_WARN << "ProjectsHandler инициализирован без ProjectService";
    }
}

void ProjectsHandler::handleGetProjects(
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
        catch (...)
        {
            page = 1;
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 20;
            if (pageSize > 100)
                pageSize = 100; // Максимум 100 записей
        }
        catch (...)
        {
            pageSize = 20;
        }
    }

    // Фильтры
    std::optional<int64_t> parentId = std::nullopt;
    if (params.count("parentId"))
    {
        try
        {
            parentId = std::stoll(params["parentId"]);
        }
        catch (...)
        {
            parentId = std::nullopt;
        }
    }

    std::optional<bool> isArchive = std::nullopt;
    if (params.count("isArchive"))
    {
        std::string val = params["isArchive"];
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        isArchive = (val == "true" || val == "1");
    }

    std::string searchCaption = "";
    if (params.count("searchCaption"))
    {
        searchCaption = params["searchCaption"];
    }

    LOG_DEBUG << "GET /api/projects: user=" << userId
              << ", page=" << page << ", pageSize=" << pageSize
              << ", parentId=" << (parentId.has_value() ? std::to_string(*parentId) : "none")
              << ", isArchive=" << (isArchive.has_value() ? (*isArchive ? "true" : "false") : "none");

    try
    {
        auto projectsPage = m_projectService->projects(
            page, pageSize, userId, parentId, isArchive, searchCaption
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < projectsPage.projects.size(); ++i)
        {
            items[i] = dto::toWebJson(projectsPage.projects[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(projectsPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка проектов: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ProjectsHandler::handleGetProject(
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

    const int64_t projectId = extractIdFromPath(request);
    if (projectId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid project ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /api/projects/" << projectId << " from user " << userId;

    try
    {
        auto project = m_projectService->project(projectId, userId);
        if (!project)
        {
            // Возвращаем 404, чтобы не раскрывать существование проекта
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Project not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(project->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении проекта " << projectId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ProjectsHandler::handleCreateProject(
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

    LOG_DEBUG << "POST /api/projects from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Project project(nlohmannJson);

                    // Валидация обязательных полей
                    if (!project.caption.has_value() || project.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Project caption is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_projectService->createProject(project, userId);
                    if (!created)
                    {
                        // Не уточняем причину (может быть 403 или 400)
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Cannot create project: insufficient permissions or invalid data");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO << "Пользователь " << userId
                             << " создал проект с id=" << *created->id;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании проекта: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void ProjectsHandler::handleUpdateProject(
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

    const int64_t projectId = extractIdFromPath(request);
    if (projectId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid project ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /api/projects/" << projectId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, projectId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = projectId;
                    dto::Project project(nlohmannJson);

                    // Сначала проверяем, существует ли проект
                    auto existingProject = m_projectService->project(projectId, userId);
                    if (!existingProject)
                    {
                        // Проект не существует или нет прав на чтение
                        // Не раскрываем причину
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "Project not found");
                        request.reply(resp);
                        return;
                    }

                    // Проект существует, пробуем обновить
                    auto updated = m_projectService->updateProject(project, userId);
                    if (!updated)
                    {
                        // Проект существует, но нет прав на обновление
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this project");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил проект с id=" << projectId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении проекта " << projectId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void ProjectsHandler::handleDeleteProject(
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

    const int64_t projectId = extractIdFromPath(request);
    if (projectId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid project ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /api/projects/" << projectId << " from user " << userId;

    try
    {
        // Сначала проверяем, существует ли проект
        auto existingProject = m_projectService->project(projectId, userId);
        if (!existingProject)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Project not found");
            request.reply(resp);
            return;
        }

        // Проект существует, пробуем архивировать
        const bool success = m_projectService->archiveProject(projectId, userId);
        if (!success)
        {
            web::http::http_response resp(web::http::status_codes::Forbidden);
            sendErrorResponse(resp, 403, "Insufficient permissions to archive this project");
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " архивировал проект с id=" << projectId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при архивации проекта " << projectId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
