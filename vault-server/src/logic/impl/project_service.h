#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iproject_service.h"

#include "repo/project_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления проектами.
 */
class ProjectService final : public IProjectService
{
public:
    /**
     * @brief Конструктор.
     * @param projectRepo Репозиторий проектов
     * @param authzService Сервис авторизации для проверки прав
     */
    ProjectService(
        std::shared_ptr<repositories::IProjectRepository> projectRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IProjectService
    ProjectsPage projects(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<bool> isArchive = std::nullopt,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::Project> project(int64_t id, int64_t userId) override;

    std::optional<dto::Project> createProject(
        const dto::Project& project,
        int64_t userId
    ) override;

    std::optional<dto::Project> updateProject(
        const dto::Project& project,
        int64_t userId
    ) override;

    bool archiveProject(int64_t id, int64_t userId) override;
    bool restoreProject(int64_t id, int64_t userId) override;

private:
    /**
     * @brief Проверяет, имеет ли пользователь доступ к проекту.
     * @param projectId ID проекта
     * @param userId ID пользователя
     * @param requiredRight Функция, проверяющая нужное право
     * @return true если доступ разрешен
     */
    bool checkProjectAccess(
        int64_t projectId,
        int64_t userId,
        std::function<AuthzResult(int64_t, int64_t)> checkFunction
    );

    /**
     * @brief Фильтрует список проектов по правам доступа.
     * @param projects Исходный список проектов
     * @param userId ID пользователя
     * @return Отфильтрованный список проектов
     */
    std::vector<dto::Project> filterProjectsByReadAccess(
        const std::vector<dto::Project>& projects,
        int64_t userId
    );

private:
    std::shared_ptr<repositories::IProjectRepository> m_projectRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
