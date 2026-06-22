#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/project.h"
#include "logic/iproject_service.h"

namespace server
{
namespace tests
{

class MockProjectService : public services::IProjectService
{
public:
    using ProjectsPage = services::ProjectsPage;

    MockProjectService() = default;
    ~MockProjectService() override = default;

    // ============================================================
    // Настройка результатов
    // ============================================================

    void setGetProjectsResult(const ProjectsPage& result)
    {
        m_getProjectsResult = result;
        // Сбрасываем callback, чтобы использовать фиксированный результат
        m_getProjectsCallback = nullptr;
    }

    void setGetProjectResult(std::optional<dto::Project> project)
    {
        m_getProjectResult = std::move(project);
    }

    void setCreateProjectResult(std::optional<dto::Project> project)
    {
        m_createProjectResult = std::move(project);
    }

    void setUpdateProjectResult(std::optional<dto::Project> project)
    {
        m_updateProjectResult = std::move(project);
    }

    void setArchiveProjectResult(bool result)
    {
        m_archiveProjectResult = result;
    }

    void setRestoreProjectResult(bool result)
    {
        m_restoreProjectResult = result;
    }

    // ============================================================
    // Настройка всех проектов (для пагинации и фильтрации)
    // ============================================================

    void setAllProjects(const std::vector<dto::Project>& projects)
    {
        m_allProjects = projects;

        // Настраиваем callback для автоматической фильтрации и пагинации
        m_getProjectsCallback = [this](
                                    int page,
                                    int pageSize,
                                    int64_t userId,
                                    std::optional<int64_t> parentId,
                                    std::optional<bool> isArchive,
                                    const std::string& searchCaption
                                ) -> ProjectsPage
        {
            // Для супер-админа показываем все проекты
            bool isSuperAdmin = (userId == 1); // ID 1 - супер-админ

            // Копируем проекты
            std::vector<dto::Project> filtered = m_allProjects;

            // Если не супер-админ, фильтруем по доступным проектам
            if (!isSuperAdmin)
            {
                auto it = m_userAccessibleProjects.find(userId);
                if (it != m_userAccessibleProjects.end())
                {
                    const auto& accessibleIds = it->second;
                    filtered.erase(
                        std::remove_if(
                            filtered.begin(),
                            filtered.end(),
                            [&accessibleIds](const dto::Project& p)
                            {
                                return !p.id.has_value()
                                    || std::find(accessibleIds.begin(), accessibleIds.end(), *p.id) == accessibleIds.end();
                            }
                        ),
                        filtered.end()
                    );
                }
                else
                {
                    // Если нет явно заданных доступных проектов, по умолчанию пусто
                    filtered.clear();
                }
            }

            // Фильтр по родительскому проекту
            if (parentId.has_value())
            {
                if (*parentId == 0)
                {
                    // parentId=0 означает корневые проекты (без родителя)
                    filtered.erase(
                        std::remove_if(
                            filtered.begin(),
                            filtered.end(),
                            [](const dto::Project& p)
                            {
                                return p.parentId.has_value();
                            }
                        ),
                        filtered.end()
                    );
                }
                else
                {
                    filtered.erase(
                        std::remove_if(filtered.begin(), filtered.end(), [&parentId](const dto::Project& p)
                                       { return !p.parentId.has_value() || *p.parentId != *parentId; }),
                        filtered.end()
                    );
                }
            }

            // Фильтр по статусу архива
            if (isArchive.has_value())
            {
                filtered.erase(
                    std::remove_if(filtered.begin(), filtered.end(), [&isArchive](const dto::Project& p)
                                   { return p.isArchive.value_or(false) != *isArchive; }),
                    filtered.end()
                );
            }

            // Поиск по названию
            if (!searchCaption.empty())
            {
                std::string searchLower = searchCaption;
                std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                filtered.erase(
                    std::remove_if(
                        filtered.begin(),
                        filtered.end(),
                        [&searchLower](const dto::Project& p)
                        {
                            std::string caption = p.caption.value_or("");
                            std::transform(caption.begin(), caption.end(), caption.begin(), ::tolower);
                            return caption.find(searchLower) == std::string::npos;
                        }
                    ),
                    filtered.end()
                );
            }

            int64_t totalCount = filtered.size();

            // Пагинация
            int startIndex = (page - 1) * pageSize;
            int endIndex = std::min(startIndex + pageSize, static_cast<int>(totalCount));

            std::vector<dto::Project> pagedProjects;
            if (startIndex < totalCount)
            {
                pagedProjects.assign(
                    filtered.begin() + startIndex,
                    filtered.begin() + endIndex
                );
            }

            ProjectsPage result;
            result.projects = pagedProjects;
            result.totalCount = totalCount;
            return result;
        };
    }

    // Установка доступных проектов для пользователя
    void setUserAccessibleProjects(int64_t userId, const std::vector<int64_t>& projectIds)
    {
        m_userAccessibleProjects[userId] = projectIds;
    }

    // ============================================================
    // Настройка callback'ов для гибкого поведения
    // ============================================================

    void setGetProjectsCallback(
        std::function<ProjectsPage(int, int, int64_t, std::optional<int64_t>, std::optional<bool>, const std::string&)> callback
    )
    {
        m_getProjectsCallback = std::move(callback);
    }

    void setGetProjectCallback(
        std::function<std::optional<dto::Project>(int64_t, int64_t)> callback
    )
    {
        m_getProjectCallback = std::move(callback);
    }

    void setCreateProjectCallback(
        std::function<std::optional<dto::Project>(const dto::Project&, int64_t)> callback
    )
    {
        m_createProjectCallback = std::move(callback);
    }

    void setUpdateProjectCallback(
        std::function<std::optional<dto::Project>(const dto::Project&, int64_t)> callback
    )
    {
        m_updateProjectCallback = std::move(callback);
    }

    void setArchiveProjectCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_archiveProjectCallback = std::move(callback);
    }

    void setRestoreProjectCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_restoreProjectCallback = std::move(callback);
    }

    // ============================================================
    // Реализация интерфейса IProjectService
    // ============================================================

    ProjectsPage projects(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<bool> isArchive = std::nullopt,
        const std::string& searchCaption = ""
    ) override
    {
        m_lastGetProjectsPage = page;
        m_lastGetProjectsPageSize = pageSize;
        m_lastGetProjectsUserId = userId;
        m_lastGetProjectsParentId = parentId;
        m_lastGetProjectsIsArchive = isArchive;
        m_lastGetProjectsSearch = searchCaption;
        m_getProjectsCallCount++;

        if (m_getProjectsCallback)
        {
            return m_getProjectsCallback(page, pageSize, userId, parentId, isArchive, searchCaption);
        }

        // Фильтрация фиксированного результата (упрощенная)
        ProjectsPage result = m_getProjectsResult;
        if (parentId.has_value())
        {
            result.projects.erase(
                std::remove_if(result.projects.begin(), result.projects.end(), [&parentId](const dto::Project& p)
                               { return !p.parentId.has_value() || *p.parentId != *parentId; }),
                result.projects.end()
            );
        }

        return result;
    }

    std::optional<dto::Project> project(int64_t id, int64_t userId) override
    {
        m_lastGetProjectId = id;
        m_lastGetProjectUserId = userId;
        m_getProjectCallCount++;

        if (m_getProjectCallback)
        {
            return m_getProjectCallback(id, userId);
        }

        // Базовая проверка: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        if (m_getProjectResult.has_value() && m_getProjectResult->id.has_value())
        {
            if (*m_getProjectResult->id == id)
            {
                return m_getProjectResult;
            }
        }
        return std::nullopt;
    }

    std::optional<dto::Project> createProject(
        const dto::Project& project,
        int64_t userId
    ) override
    {
        m_lastCreatedProject = project;
        m_lastCreateProjectUserId = userId;
        m_createProjectCallCount++;

        if (m_createProjectCallback)
        {
            return m_createProjectCallback(project, userId);
        }

        // Базовая проверка: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        if (m_createProjectResult.has_value())
        {
            // Генерируем новый ID, если его нет
            if (!m_createProjectResult->id.has_value())
            {
                m_createProjectResult->id = m_nextId++;
            }
        }

        return m_createProjectResult;
    }

    std::optional<dto::Project> updateProject(
        const dto::Project& project,
        int64_t userId
    ) override
    {
        m_lastUpdatedProject = project;
        m_lastUpdateProjectUserId = userId;
        m_updateProjectCallCount++;

        if (m_updateProjectCallback)
        {
            return m_updateProjectCallback(project, userId);
        }

        // Базовая проверка: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        // Проверяем, существует ли проект
        if (m_updateProjectResult.has_value() && m_updateProjectResult->id.has_value())
        {
            if (project.id.has_value() && *m_updateProjectResult->id == *project.id)
            {
                return m_updateProjectResult;
            }
        }

        // Если проект не найден, но есть результат по умолчанию
        if (m_updateProjectResult.has_value())
        {
            return m_updateProjectResult;
        }

        return std::nullopt;
    }

    bool archiveProject(int64_t id, int64_t userId) override
    {
        m_lastArchivedProjectId = id;
        m_lastArchiveProjectUserId = userId;
        m_archiveProjectCallCount++;

        if (m_archiveProjectCallback)
        {
            return m_archiveProjectCallback(id, userId);
        }

        // Базовая проверка: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return false;
        }

        return m_archiveProjectResult;
    }

    bool restoreProject(int64_t id, int64_t userId) override
    {
        m_lastRestoredProjectId = id;
        m_lastRestoreProjectUserId = userId;
        m_restoreProjectCallCount++;

        if (m_restoreProjectCallback)
        {
            return m_restoreProjectCallback(id, userId);
        }

        // Базовая проверка: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return false;
        }

        return m_restoreProjectResult;
    }

    // ============================================================
    // Методы для проверки вызовов
    // ============================================================

    int getProjectsCallCount() const { return m_getProjectsCallCount; }
    int getProjectCallCount() const { return m_getProjectCallCount; }
    int createProjectCallCount() const { return m_createProjectCallCount; }
    int updateProjectCallCount() const { return m_updateProjectCallCount; }
    int archiveProjectCallCount() const { return m_archiveProjectCallCount; }
    int restoreProjectCallCount() const { return m_restoreProjectCallCount; }

    // ============================================================
    // Геттеры для последних параметров
    // ============================================================

    int getLastGetProjectsPage() const { return m_lastGetProjectsPage; }
    int getLastGetProjectsPageSize() const { return m_lastGetProjectsPageSize; }
    int64_t getLastGetProjectsUserId() const { return m_lastGetProjectsUserId; }
    std::optional<int64_t> getLastGetProjectsParentId() const { return m_lastGetProjectsParentId; }
    std::optional<bool> getLastGetProjectsIsArchive() const { return m_lastGetProjectsIsArchive; }
    const std::string& getLastGetProjectsSearch() const { return m_lastGetProjectsSearch; }

    int64_t getLastGetProjectId() const { return m_lastGetProjectId; }
    int64_t getLastGetProjectUserId() const { return m_lastGetProjectUserId; }

    const dto::Project& getLastCreatedProject() const { return m_lastCreatedProject; }
    int64_t getLastCreateProjectUserId() const { return m_lastCreateProjectUserId; }

    const dto::Project& getLastUpdatedProject() const { return m_lastUpdatedProject; }
    int64_t getLastUpdateProjectUserId() const { return m_lastUpdateProjectUserId; }

    int64_t getLastArchivedProjectId() const { return m_lastArchivedProjectId; }
    int64_t getLastArchiveProjectUserId() const { return m_lastArchiveProjectUserId; }

    int64_t getLastRestoredProjectId() const { return m_lastRestoredProjectId; }
    int64_t getLastRestoreProjectUserId() const { return m_lastRestoreProjectUserId; }

    // ============================================================
    // Утилиты для создания тестовых проектов
    // ============================================================

    static dto::Project createTestProject(
        int64_t id,
        const std::string& caption,
        std::optional<int64_t> parentId = std::nullopt,
        bool isArchive = false
    )
    {
        dto::Project project;
        project.id = id;
        project.caption = caption;
        if (parentId.has_value())
        {
            project.parentId = parentId;
        }
        project.isArchive = isArchive;
        project.description = "Test description for " + caption;
        return project;
    }

    static ProjectsPage createTestProjectsPage(
        const std::vector<dto::Project>& projects,
        int64_t totalCount = -1
    )
    {
        ProjectsPage page;
        page.projects = projects;
        page.totalCount = (totalCount >= 0) ? totalCount : static_cast<int64_t>(projects.size());
        return page;
    }

    // ============================================================
    // Сброс состояния
    // ============================================================

    void reset()
    {
        m_getProjectsCallCount = 0;
        m_getProjectCallCount = 0;
        m_createProjectCallCount = 0;
        m_updateProjectCallCount = 0;
        m_archiveProjectCallCount = 0;
        m_restoreProjectCallCount = 0;

        m_lastGetProjectsPage = 0;
        m_lastGetProjectsPageSize = 0;
        m_lastGetProjectsUserId = 0;
        m_lastGetProjectsParentId = std::nullopt;
        m_lastGetProjectsIsArchive = std::nullopt;
        m_lastGetProjectsSearch.clear();

        m_lastGetProjectId = 0;
        m_lastGetProjectUserId = 0;

        m_lastCreatedProject = dto::Project {};
        m_lastCreateProjectUserId = 0;

        m_lastUpdatedProject = dto::Project {};
        m_lastUpdateProjectUserId = 0;

        m_lastArchivedProjectId = 0;
        m_lastArchiveProjectUserId = 0;

        m_lastRestoredProjectId = 0;
        m_lastRestoreProjectUserId = 0;

        m_getProjectsCallback = nullptr;
        m_getProjectCallback = nullptr;
        m_createProjectCallback = nullptr;
        m_updateProjectCallback = nullptr;
        m_archiveProjectCallback = nullptr;
        m_restoreProjectCallback = nullptr;

        m_allProjects.clear();
        m_userAccessibleProjects.clear();
        m_getProjectsResult = ProjectsPage {};
        m_getProjectResult = std::nullopt;
        m_createProjectResult = std::nullopt;
        m_updateProjectResult = std::nullopt;
        m_archiveProjectResult = false;
        m_restoreProjectResult = false;
        m_nextId = 100;
    }

    void setProjectNotFound(bool notFound) { m_projectNotFound = notFound; }
    void setProjectForbidden(bool forbidden) { m_projectForbidden = forbidden; }
    void setUpdateForbidden(bool forbidden) { m_updateForbidden = forbidden; }
    void setArchiveForbidden(bool forbidden) { m_archiveForbidden = forbidden; }

private:
    // Результаты по умолчанию
    ProjectsPage m_getProjectsResult;
    std::optional<dto::Project> m_getProjectResult;
    std::optional<dto::Project> m_createProjectResult;
    std::optional<dto::Project> m_updateProjectResult;
    bool m_archiveProjectResult = false;
    bool m_restoreProjectResult = false;

    // Данные для пагинации и фильтрации
    std::vector<dto::Project> m_allProjects;
    std::map<int64_t, std::vector<int64_t>> m_userAccessibleProjects;
    int64_t m_nextId = 100;

    // Callback'и для кастомной логики
    std::function<ProjectsPage(int, int, int64_t, std::optional<int64_t>, std::optional<bool>, const std::string&)> m_getProjectsCallback;
    std::function<std::optional<dto::Project>(int64_t, int64_t)> m_getProjectCallback;
    std::function<std::optional<dto::Project>(const dto::Project&, int64_t)> m_createProjectCallback;
    std::function<std::optional<dto::Project>(const dto::Project&, int64_t)> m_updateProjectCallback;
    std::function<bool(int64_t, int64_t)> m_archiveProjectCallback;
    std::function<bool(int64_t, int64_t)> m_restoreProjectCallback;

    // Счетчики вызовов
    int m_getProjectsCallCount = 0;
    int m_getProjectCallCount = 0;
    int m_createProjectCallCount = 0;
    int m_updateProjectCallCount = 0;
    int m_archiveProjectCallCount = 0;
    int m_restoreProjectCallCount = 0;

    // Последние параметры вызовов - GET /api/v1/projects
    int m_lastGetProjectsPage = 0;
    int m_lastGetProjectsPageSize = 0;
    int64_t m_lastGetProjectsUserId = 0;
    std::optional<int64_t> m_lastGetProjectsParentId;
    std::optional<bool> m_lastGetProjectsIsArchive;
    std::string m_lastGetProjectsSearch;

    // Последние параметры вызовов - GET /api/v1/projects/{id}
    int64_t m_lastGetProjectId = 0;
    int64_t m_lastGetProjectUserId = 0;

    // Последние параметры вызовов - POST /api/v1/projects
    dto::Project m_lastCreatedProject;
    int64_t m_lastCreateProjectUserId = 0;

    // Последние параметры вызовов - PUT /api/v1/projects/{id}
    dto::Project m_lastUpdatedProject;
    int64_t m_lastUpdateProjectUserId = 0;

    // Последние параметры вызовов - DELETE /api/v1/projects/{id} (archive)
    int64_t m_lastArchivedProjectId = 0;
    int64_t m_lastArchiveProjectUserId = 0;

    // Последние параметры вызовов - восстановление проекта
    int64_t m_lastRestoredProjectId = 0;
    int64_t m_lastRestoreProjectUserId = 0;

    bool m_projectNotFound = false;
    bool m_projectForbidden = false;
    bool m_updateForbidden = false;
    bool m_archiveForbidden = false;
};

} // namespace tests
} // namespace server
