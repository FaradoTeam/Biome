#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_user_notification_repository.h"

namespace
{

dto::UserNotification mapRowToUserNotification(db::IResultSet& rs)
{
    dto::UserNotification notification;
    notification.id = rs.valueInt64("id");
    notification.userId = rs.valueInt64("userId");
    notification.itemId = rs.valueInt64("itemId");
    return notification;
}

} // namespace

namespace server
{
namespace repositories
{

SqliteUserNotificationRepository::SqliteUserNotificationRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteUserNotificationRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteUserNotificationRepository::connection() const
{
    return m_database->connection();
}

UserNotificationsPage SqliteUserNotificationRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> userId,
    std::optional<int64_t> itemId
)
{
    std::vector<dto::UserNotification> notifications;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");
        if (itemId.has_value())
            whereClauses.push_back("itemId = :itemId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM UserNotification" + whereStr
        );
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);
        if (itemId.has_value())
            countStmt->bindInt64("itemId", *itemId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { notifications, totalCount };
        }

        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, userId, itemId FROM UserNotification" + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        if (itemId.has_value())
            stmt->bindInt64("itemId", *itemId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            notifications.push_back(mapRowToUserNotification(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка UserNotification: " << e.what();
        throw;
    }

    return { notifications, totalCount };
}

std::optional<dto::UserNotification> SqliteUserNotificationRepository::findById(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "findById: неверный идентификатор " << id;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId FROM UserNotification WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUserNotification(*rs);
        }
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска UserNotification по id: " << e.what();
        throw;
    }
}

std::optional<dto::UserNotification> SqliteUserNotificationRepository::findByUserAndItem(
    int64_t userId,
    int64_t itemId
)
{
    if (userId <= 0 || itemId <= 0)
    {
        LOG_WARN << "findByUserAndItem: неверные параметры";
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId FROM UserNotification "
            "WHERE userId = :userId AND itemId = :itemId LIMIT 1"
        );
        stmt->bindInt64("userId", userId);
        stmt->bindInt64("itemId", itemId);

        auto rs = stmt->executeQuery();
        if (rs->next())
        {
            return mapRowToUserNotification(*rs);
        }
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска подписки пользователя: " << e.what();
        throw;
    }
}

std::vector<dto::UserNotification> SqliteUserNotificationRepository::findByUserId(int64_t userId)
{
    std::vector<dto::UserNotification> notifications;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return notifications;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId FROM UserNotification "
            "WHERE userId = :userId ORDER BY itemId"
        );
        stmt->bindInt64("userId", userId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            notifications.push_back(mapRowToUserNotification(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения подписок пользователя: " << e.what();
        throw;
    }

    return notifications;
}

std::vector<dto::UserNotification> SqliteUserNotificationRepository::findByItemId(int64_t itemId)
{
    std::vector<dto::UserNotification> notifications;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return notifications;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId FROM UserNotification "
            "WHERE itemId = :itemId ORDER BY userId"
        );
        stmt->bindInt64("itemId", itemId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            notifications.push_back(mapRowToUserNotification(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения подписчиков элемента: " << e.what();
        throw;
    }

    return notifications;
}

bool SqliteUserNotificationRepository::exists(int64_t userId, int64_t itemId)
{
    if (userId <= 0 || itemId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM UserNotification "
            "WHERE userId = :userId AND itemId = :itemId LIMIT 1"
        );
        stmt->bindInt64("userId", userId);
        stmt->bindInt64("itemId", itemId);

        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования подписки: " << e.what();
        return false;
    }
}

int64_t SqliteUserNotificationRepository::create(const dto::UserNotification& notification)
{
    if (!notification.userId.has_value() || !notification.itemId.has_value())
    {
        LOG_WARN << "createUserNotification: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO UserNotification (userId, itemId) VALUES (:userId, :itemId)"
        );
        stmt->bindInt64("userId", *notification.userId);
        stmt->bindInt64("itemId", *notification.itemId);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания UserNotification: " << e.what();
        throw;
    }
}

bool SqliteUserNotificationRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "removeUserNotification: неверный ID " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM UserNotification WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления UserNotification: " << e.what();
        return false;
    }
}

bool SqliteUserNotificationRepository::removeByUserAndItem(int64_t userId, int64_t itemId)
{
    if (userId <= 0 || itemId <= 0)
    {
        LOG_WARN << "removeByUserAndItem: неверные параметры";
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM UserNotification WHERE userId = :userId AND itemId = :itemId"
        );
        stmt->bindInt64("userId", userId);
        stmt->bindInt64("itemId", itemId);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления подписки пользователя: " << e.what();
        return false;
    }
}

int64_t SqliteUserNotificationRepository::removeByUserId(int64_t userId)
{
    if (userId <= 0)
    {
        LOG_WARN << "removeByUserId: неверный userId " << userId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM UserNotification WHERE userId = :userId"
        );
        stmt->bindInt64("userId", userId);

        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления подписок пользователя: " << e.what();
        return 0;
    }
}

int64_t SqliteUserNotificationRepository::removeByItemId(int64_t itemId)
{
    if (itemId <= 0)
    {
        LOG_WARN << "removeByItemId: неверный itemId " << itemId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM UserNotification WHERE itemId = :itemId"
        );
        stmt->bindInt64("itemId", itemId);

        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления подписчиков элемента: " << e.what();
        return 0;
    }
}

std::vector<int64_t> SqliteUserNotificationRepository::getSubscriberIds(int64_t itemId)
{
    std::vector<int64_t> userIds;

    if (itemId <= 0)
    {
        LOG_WARN << "getSubscriberIds: неверный itemId " << itemId;
        return userIds;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT userId FROM UserNotification WHERE itemId = :itemId"
        );
        stmt->bindInt64("itemId", itemId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            userIds.push_back(rs->valueInt64(0));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения подписчиков элемента: " << e.what();
        throw;
    }

    return userIds;
}

} // namespace repositories
} // namespace server
