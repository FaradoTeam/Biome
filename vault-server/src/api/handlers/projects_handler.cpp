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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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
                pageSize = 100;
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

    LOG_DEBUG << "GET /projects: user=" << userId
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

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(projectsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка проектов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ProjectsHandler::handleGetProject(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t projectId = extractIdFromPath(request);
    if (projectId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid project ID");
        return;
    }

    LOG_DEBUG << "GET /projects/" << projectId << " from user " << userId;

    try
    {
        auto project = m_projectService->project(projectId, userId);
        if (!project)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Project not found");
            return;
        }

        sendJsonResponse(
            request,
            web::http::status_codes::OK,
            dto::toWebJson(project->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении проекта " << projectId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ProjectsHandler::handleCreateProject(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    LOG_DEBUG << "POST /projects from user " << userId;

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
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Project caption is required");
                        return;
                    }

                    auto created = m_projectService->createProject(project, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create project: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO << "Пользователь " << userId
                             << " создал проект с id=" << *created->id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании проекта: " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t projectId = extractIdFromPath(request);
    if (projectId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid project ID");
        return;
    }

    LOG_DEBUG << "PUT /projects/" << projectId << " from user " << userId;

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
                        sendErrorResponse(request, web::http::status_codes::NotFound, "Project not found");
                        return;
                    }

                    // Проект существует, пробуем обновить
                    auto updated = m_projectService->updateProject(project, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this project"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил проект с id=" << projectId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении проекта " << projectId << ": " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t projectId = extractIdFromPath(request);
    if (projectId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid project ID");
        return;
    }

    LOG_DEBUG << "DELETE /projects/" << projectId << " from user " << userId;

    try
    {
        // Сначала проверяем, существует ли проект
        auto existingProject = m_projectService->project(projectId, userId);
        if (!existingProject)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Project not found");
            return;
        }

        // Проект существует, пробуем архивировать
        const bool success = m_projectService->archiveProject(projectId, userId);
        if (!success)
        {
            sendErrorResponse(
                request,
                web::http::status_codes::Forbidden,
                "Insufficient permissions to archive this project"
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " архивировал проект с id=" << projectId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при архивации проекта " << projectId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
