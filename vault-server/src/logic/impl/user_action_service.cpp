#include "common/log/log.h"

#include "user_action_service.h"

namespace server
{
namespace services
{

UserActionService::UserActionService(
    std::shared_ptr<repositories::IUserActionRepository> actionRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_actionRepo(std::move(actionRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_actionRepo)
    {
        throw std::runtime_error("UserActionService: репозиторий действий не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("UserActionService: сервис авторизации не инициализирован");
    }
}

UserActionsPage UserActionService::getActions(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> filterUserId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    repositories::UserActionsPage repoPage;

    // Супер-админ может видеть действия всех пользователей
    if (m_authzService->isSuperAdmin(userId))
    {
        repoPage = m_actionRepo->findAll(page, pageSize, filterUserId, dateFrom, dateTo);
    }
    // Обычный пользователь может видеть только свои действия
    else if (filterUserId.has_value() && *filterUserId != userId)
    {
        LOG_WARN
            << "getActions: пользователь " << userId
            << " пытается получить действия другого пользователя " << *filterUserId;
        return { {}, 0 };
    }
    else
    {
        repoPage = m_actionRepo->findAll(page, pageSize, userId, dateFrom, dateTo);
    }

    // Конвертируем из репозиторной структуры в сервисную
    UserActionsPage result;
    result.actions = std::move(repoPage.actions);
    result.totalCount = repoPage.totalCount;
    return result;
}

std::optional<dto::UserAction> UserActionService::getAction(
    int64_t id,
    int64_t userId
)
{
    return checkActionAccess(id, userId, false);
}

std::optional<dto::UserAction> UserActionService::createAction(
    const dto::UserAction& action,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateAction(action, errorMessage))
    {
        LOG_WARN << "createAction: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем, что пользователь создаёт действие для себя
    if (!action.userId.has_value() || *action.userId != userId)
    {
        // Только супер-админ может создавать действия для других пользователей
        if (!m_authzService->isSuperAdmin(userId))
        {
            LOG_WARN
                << "createAction: пользователь " << userId
                << " пытается создать действие для другого пользователя " << *action.userId;
            return std::nullopt;
        }
    }

    // 3. Создаём действие
    const int64_t newId = m_actionRepo->create(action);
    if (newId <= 0)
    {
        LOG_ERROR << "createAction: не удалось создать действие";
        return std::nullopt;
    }

    LOG_INFO
        << "Действие создано: id=" << newId
        << ", пользователь=" << userId
        << ", caption=" << *action.caption;

    return m_actionRepo->findById(newId);
}

UserActionResult UserActionService::deleteAction(
    int64_t id,
    int64_t userId
)
{
    UserActionResult result;

    // 1. Проверяем существование и доступ к действию
    auto existing = checkActionAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Действие не найдено или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Удаляем действие
    if (!m_actionRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить действие";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Действие удалено: id=" << id
        << ", пользователь=" << userId;

    return result;
}

std::optional<dto::UserAction> UserActionService::checkActionAccess(
    int64_t actionId,
    int64_t userId,
    bool needWrite
)
{
    auto action = m_actionRepo->findById(actionId);
    if (!action.has_value())
    {
        LOG_DEBUG
            << "checkActionAccess: действие не найдено, id=" << actionId;
        return std::nullopt;
    }

    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return action;
    }

    // Обычный пользователь может видеть только свои действия
    if (!action->userId.has_value() || *action->userId != userId)
    {
        LOG_WARN
            << "checkActionAccess: пользователь " << userId
            << " не является владельцем действия " << actionId;
        return std::nullopt;
    }

    return action;
}

bool UserActionService::validateAction(
    const dto::UserAction& action,
    std::string& errorMessage
)
{
    if (!action.userId.has_value())
    {
        errorMessage = "userId является обязательным полем";
        return false;
    }

    if (!action.caption.has_value() || action.caption->empty())
    {
        errorMessage = "caption является обязательным полем и не может быть пустым";
        return false;
    }

    if (action.caption->length() > 255)
    {
        errorMessage = "caption не может превышать 255 символов";
        return false;
    }

    if (action.description.has_value() && action.description->length() > 1000)
    {
        errorMessage = "description не может превышать 1000 символов";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
