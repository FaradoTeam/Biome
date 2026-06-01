#include "common/log/log.h"

#include "project_service.h"

namespace server
{
namespace services
{

ProjectService::ProjectService(
    std::shared_ptr<repositories::IProjectRepository> projectRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_projectRepo(std::move(projectRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_projectRepo)
    {
        throw std::runtime_error("ProjectRepository не может быть пустым");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("AuthorizationService не может быть пустым");
    }
}

ProjectsPage ProjectService::projects(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> parentId,
    std::optional<bool> isArchive,
    const std::string& searchCaption
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Супер-администратор видит все проекты
    if (m_authzService->isSuperAdmin(userId))
    {
        auto [projectList, total] = m_projectRepo->findAll(
            page,
            pageSize,
            parentId,
            isArchive,
            searchCaption
        );
        return { projectList, total };
    }

    // Для обычного пользователя:
    // 1. Получаем список проектов, к которым у него есть доступ на чтение
    auto accessibleProjectIds = m_authzService->getReadableProjectIds(userId);

    // Если доступных проектов нет, возвращаем пустой результат
    if (accessibleProjectIds.empty())
    {
        return { std::vector<dto::Project>(), 0 };
    }

    // 2. Получаем проекты с учетом пагинации и фильтров
    // TODO: Для production нужно добавить фильтрацию на уровне БД
    // Сейчас загружаем все проекты и фильтруем в памяти
    auto [allProjects, totalAll] = m_projectRepo->findAll(
        1,
        1000, // Временное ограничение, позже нужно оптимизировать
        parentId,
        isArchive,
        searchCaption
    );

    // 3. Фильтруем проекты по доступным ID
    std::vector<dto::Project> filteredProjects;
    for (const auto& project : allProjects)
    {
        if (!project.id.has_value())
            continue;

        if (std::find(
                accessibleProjectIds.begin(),
                accessibleProjectIds.end(),
                *project.id
            )
            != accessibleProjectIds.end())
        {
            filteredProjects.push_back(project);
        }
    }

    // 4. Применяем пагинацию
    const int64_t totalCount = filteredProjects.size();
    const int startIndex = (page - 1) * pageSize;
    const int endIndex = std::min(
        startIndex + pageSize,
        static_cast<int>(totalCount)
    );

    std::vector<dto::Project> pagedProjects;
    if (startIndex < totalCount)
    {
        pagedProjects.assign(
            filteredProjects.begin() + startIndex,
            filteredProjects.begin() + endIndex
        );
    }

    return { pagedProjects, totalCount };
}

std::optional<dto::Project> ProjectService::project(int64_t id, int64_t userId)
{
    if (id <= 0)
    {
        LOG_WARN << "project: неверный идентификатор " << id;
        return std::nullopt;
    }

    // Проверяем право на чтение проекта
    const auto authzResult = m_authzService->canReadProject(userId, id);
    if (!authzResult.granted)
    {
        LOG_WARN
            << "project: пользователь " << userId
            << " не имеет прав на чтение проекта " << id;
        return std::nullopt;
    }

    auto project = m_projectRepo->findById(id);
    if (!project)
    {
        LOG_WARN << "project: проект с id=" << id << " не найден";
        return std::nullopt;
    }

    return project;
}

std::optional<dto::Project> ProjectService::createProject(
    const dto::Project& project,
    int64_t userId
)
{
    // Валидация названия
    if (!project.caption.has_value() || project.caption->empty())
    {
        LOG_WARN << "createProject: название проекта не может быть пустым";
        return std::nullopt;
    }

    // Проверка прав в зависимости от типа проекта
    AuthzResult authzResult;
    if (!project.parentId.has_value())
    {
        // Создание корневого проекта
        authzResult = m_authzService->canCreateRootProject(userId);
        if (!authzResult.granted)
        {
            LOG_WARN
                << "createProject: пользователь " << userId
                << " не имеет прав на создание корневого проекта: "
                << authzResult.errorMessage;
            return std::nullopt;
        }
    }
    else
    {
        // Создание подпроекта — нужно право на редактирование родительского проекта
        authzResult = m_authzService->canEditProject(userId, *project.parentId);
        if (!authzResult.granted)
        {
            LOG_WARN
                << "createProject: пользователь " << userId
                << " не имеет прав на создание подпроекта в проекте "
                << *project.parentId << ": " << authzResult.errorMessage;
            return std::nullopt;
        }
    }

    // Создаем проект
    const int64_t newId = m_projectRepo->create(project);
    if (newId <= 0)
    {
        LOG_ERROR << "createProject: ошибка при создании проекта в репозитории";
        return std::nullopt;
    }

    LOG_INFO
        << "Пользователь " << userId
        << " создал новый проект с id=" << newId;

    return m_projectRepo->findById(newId);
}

std::optional<dto::Project> ProjectService::updateProject(
    const dto::Project& project,
    int64_t userId
)
{
    if (!project.id.has_value())
    {
        LOG_WARN << "updateProject: отсутствует ID проекта";
        return std::nullopt;
    }

    const int64_t projectId = *project.id;

    // Проверяем право на редактирование проекта
    const auto authzResult = m_authzService->canEditProject(userId, projectId);
    if (!authzResult.granted)
    {
        LOG_WARN
            << "updateProject: пользователь " << userId
            << " не имеет прав на редактирование проекта " << projectId
            << ": " << authzResult.errorMessage;
        return std::nullopt;
    }

    // Проверяем, существует ли проект
    if (!m_projectRepo->exists(projectId))
    {
        LOG_WARN << "updateProject: проект с id=" << projectId << " не найден";
        return std::nullopt;
    }

    // Если меняется родительский проект, проверяем права на нового родителя
    if (project.parentId.has_value())
    {
        auto existingProject = m_projectRepo->findById(projectId);
        if (existingProject && existingProject->parentId != project.parentId)
        {
            // Проверяем право на создание подпроекта в новом родителе
            const auto newParentAuthz = m_authzService->canEditProject(
                userId, *project.parentId
            );
            if (!newParentAuthz.granted)
            {
                LOG_WARN
                    << "updateProject: пользователь " << userId
                    << " не имеет прав на перемещение проекта в "
                    << *project.parentId;
                return std::nullopt;
            }
        }
    }

    // Выполняем обновление
    if (!m_projectRepo->update(project))
    {
        LOG_ERROR << "updateProject: ошибка при обновлении проекта в репозитории";
        return std::nullopt;
    }

    LOG_INFO
        << "Пользователь " << userId
        << " обновил проект с id=" << projectId;

    return m_projectRepo->findById(projectId);
}

bool ProjectService::archiveProject(int64_t id, int64_t userId)
{
    if (id <= 0)
    {
        LOG_WARN << "archiveProject: неверный идентификатор " << id;
        return false;
    }

    // Проверяем право на редактирование проекта (архивация — это разновидность редактирования)
    const auto authzResult = m_authzService->canEditProject(userId, id);
    if (!authzResult.granted)
    {
        LOG_WARN
            << "archiveProject: пользователь " << userId
            << " не имеет прав на архивацию проекта " << id
            << ": " << authzResult.errorMessage;
        return false;
    }

    // Проверяем, существует ли проект
    if (!m_projectRepo->exists(id))
    {
        LOG_WARN << "archiveProject: проект с id=" << id << " не найден";
        return false;
    }

    // Выполняем архивацию
    if (m_projectRepo->archive(id))
    {
        LOG_INFO
            << "Пользователь " << userId
            << " архивировал проект с id=" << id;
        return true;
    }

    LOG_ERROR << "archiveProject: ошибка при архивации проекта с id=" << id;
    return false;
}

bool ProjectService::restoreProject(int64_t id, int64_t userId)
{
    if (id <= 0)
    {
        LOG_WARN << "restoreProject: неверный идентификатор " << id;
        return false;
    }

    // Проверяем право на редактирование проекта
    const auto authzResult = m_authzService->canEditProject(userId, id);
    if (!authzResult.granted)
    {
        LOG_WARN
            << "restoreProject: пользователь " << userId
            << " не имеет прав на восстановление проекта " << id
            << ": " << authzResult.errorMessage;
        return false;
    }

    // Проверяем, существует ли проект
    if (!m_projectRepo->exists(id))
    {
        LOG_WARN << "restoreProject: проект с id=" << id << " не найден";
        return false;
    }

    // Выполняем восстановление
    if (m_projectRepo->restore(id))
    {
        LOG_INFO
            << "Пользователь " << userId
            << " восстановил проект с id=" << id;
        return true;
    }

    LOG_ERROR << "restoreProject: ошибка при восстановлении проекта с id=" << id;
    return false;
}

} // namespace services
} // namespace server
