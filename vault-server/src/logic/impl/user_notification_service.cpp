#include "user_notification_service.h"
#include "common/log/log.h"

namespace server::services
{

UserNotificationService::UserNotificationService(
    std::shared_ptr<repositories::IUserNotificationRepository> notificationRepo,
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<repositories::IItemRepository> itemRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_notificationRepo(std::move(notificationRepo))
    , m_userRepo(std::move(userRepo))
    , m_itemRepo(std::move(itemRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_notificationRepo || !m_userRepo || !m_itemRepo)
    {
        throw std::runtime_error("UserNotificationService: один или несколько репозиториев не инициализированы");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("UserNotificationService: сервис авторизации не инициализирован");
    }
}

UserNotificationsPage UserNotificationService::getNotifications(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> filterUserId,
    std::optional<int64_t> itemId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    // Если указан filterUserId, проверяем, что это либо сам пользователь, либо супер-админ
    if (filterUserId.has_value() && *filterUserId != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "getNotifications: пользователь " << userId << " не имеет прав на просмотр подписок пользователя " << *filterUserId;
        return { {}, 0 };
    }

    // Если указан itemId, проверяем его существование
    if (itemId.has_value() && !itemExists(*itemId))
    {
        LOG_WARN << "getNotifications: элемент " << *itemId << " не найден";
        return { {}, 0 };
    }

    repositories::UserNotificationsPage repoPage;

    // Супер-админ видит все подписки
    if (m_authzService->isSuperAdmin(userId))
    {
        repoPage = m_notificationRepo->findAll(page, pageSize, filterUserId, itemId);
    }
    else
    {
        // Обычный пользователь видит только свои подписки
        repoPage = m_notificationRepo->findAll(page, pageSize, userId, itemId);
    }

    UserNotificationsPage result;
    result.notifications = std::move(repoPage.notifications);
    result.totalCount = repoPage.totalCount;
    return result;
}

std::optional<dto::UserNotification> UserNotificationService::getNotification(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "getNotification: неверный идентификатор " << id;
        return std::nullopt;
    }

    auto notification = m_notificationRepo->findById(id);
    if (!notification.has_value())
    {
        LOG_DEBUG << "getNotification: подписка " << id << " не найдена";
        return std::nullopt;
    }

    // Супер-админ или владелец подписки
    if (!m_authzService->isSuperAdmin(userId) && (!notification->userId.has_value() || *notification->userId != userId))
    {
        LOG_WARN << "getNotification: пользователь " << userId << " не имеет доступа к подписке " << id;
        return std::nullopt;
    }

    return notification;
}

std::vector<dto::UserNotification> UserNotificationService::getUserNotifications(
    int64_t userId
)
{
    if (userId <= 0)
    {
        LOG_WARN << "getUserNotifications: неверный userId " << userId;
        return {};
    }

    return m_notificationRepo->findByUserId(userId);
}

std::vector<int64_t> UserNotificationService::getSubscriberIds(
    int64_t itemId,
    int64_t userId
)
{
    if (itemId <= 0)
    {
        LOG_WARN << "getSubscriberIds: неверный itemId " << itemId;
        return {};
    }

    if (!itemExists(itemId))
    {
        LOG_WARN << "getSubscriberIds: элемент " << itemId << " не найден";
        return {};
    }

    // Проверяем, имеет ли пользователь доступ к элементу
    // TODO: Здесь нужна проверка прав через IItemService
    // Для MVP: супер-админ может видеть подписчиков любого элемента

    return m_notificationRepo->getSubscriberIds(itemId);
}

bool UserNotificationService::isSubscribed(int64_t userId, int64_t itemId)
{
    if (userId <= 0 || itemId <= 0)
    {
        return false;
    }

    return m_notificationRepo->exists(userId, itemId);
}

std::optional<dto::UserNotification> UserNotificationService::subscribe(
    const dto::UserNotification& notification,
    int64_t userId
)
{
    // 1. Валидация
    if (!notification.userId.has_value() || notification.userId <= 0)
    {
        LOG_WARN << "subscribe: неверный userId";
        return std::nullopt;
    }

    if (!notification.itemId.has_value() || notification.itemId <= 0)
    {
        LOG_WARN << "subscribe: неверный itemId";
        return std::nullopt;
    }

    // 2. Проверяем, что пользователь подписывает себя
    if (*notification.userId != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "subscribe: пользователь " << userId << " пытается подписать другого пользователя " << *notification.userId;
        return std::nullopt;
    }

    // 3. Проверяем существование пользователя
    if (!userExists(*notification.userId))
    {
        LOG_WARN << "subscribe: пользователь " << *notification.userId << " не найден";
        return std::nullopt;
    }

    // 4. Проверяем существование элемента
    if (!itemExists(*notification.itemId))
    {
        LOG_WARN << "subscribe: элемент " << *notification.itemId << " не найден";
        return std::nullopt;
    }

    // 5. Проверяем, не подписан ли уже
    if (m_notificationRepo->exists(*notification.userId, *notification.itemId))
    {
        LOG_WARN << "subscribe: пользователь " << *notification.userId << " уже подписан на элемент " << *notification.itemId;
        return std::nullopt;
    }

    // 6. Создаём подписку
    int64_t notifId = m_notificationRepo->create(notification);
    if (notifId <= 0)
    {
        LOG_ERROR << "subscribe: не удалось создать подписку";
        return std::nullopt;
    }

    LOG_INFO << "Пользователь " << userId << " подписался на элемент " << *notification.itemId;

    return m_notificationRepo->findById(notifId);
}

UserNotificationResult UserNotificationService::unsubscribe(
    int64_t id,
    int64_t userId
)
{
    UserNotificationResult result;

    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор подписки";
        result.errorCode = 400;
        return result;
    }

    auto notification = m_notificationRepo->findById(id);
    if (!notification.has_value())
    {
        result.errorMessage = "Подписка не найдена";
        result.errorCode = 404;
        return result;
    }

    // Супер-админ может отписать любого
    if (m_authzService->isSuperAdmin(userId))
    {
        if (m_notificationRepo->remove(id))
        {
            result.success = true;
            LOG_INFO << "Супер-админ " << userId << " удалил подписку " << id;
            return result;
        }
        result.errorMessage = "Не удалось удалить подписку";
        result.errorCode = 500;
        return result;
    }

    // Обычный пользователь может отписать только себя
    if (!notification->userId.has_value() || *notification->userId != userId)
    {
        result.errorMessage = "Нет прав на удаление этой подписки";
        result.errorCode = 403;
        LOG_WARN << "unsubscribe: пользователь " << userId << " не является владельцем подписки " << id;
        return result;
    }

    if (m_notificationRepo->remove(id))
    {
        result.success = true;
        LOG_INFO << "Пользователь " << userId << " отписался от элемента";
        return result;
    }

    result.errorMessage = "Не удалось удалить подписку";
    result.errorCode = 500;
    return result;
}

UserNotificationResult UserNotificationService::unsubscribeByUserAndItem(
    int64_t userId,
    int64_t itemId
)
{
    UserNotificationResult result;

    if (userId <= 0 || itemId <= 0)
    {
        result.errorMessage = "Неверные параметры";
        result.errorCode = 400;
        return result;
    }

    // Проверяем существование подписки
    if (!m_notificationRepo->exists(userId, itemId))
    {
        result.errorMessage = "Подписка не найдена";
        result.errorCode = 404;
        return result;
    }

    if (m_notificationRepo->removeByUserAndItem(userId, itemId))
    {
        result.success = true;
        LOG_INFO << "Пользователь " << userId << " отписался от элемента " << itemId;
        return result;
    }

    result.errorMessage = "Не удалось удалить подписку";
    result.errorCode = 500;
    return result;
}

bool UserNotificationService::userExists(int64_t userId)
{
    return m_userRepo->findById(userId).has_value();
}

bool UserNotificationService::itemExists(int64_t itemId)
{
    return m_itemRepo->findById(itemId).has_value();
}

} // namespace server::services
