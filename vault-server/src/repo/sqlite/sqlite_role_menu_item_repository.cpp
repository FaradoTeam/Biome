#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_role_menu_item_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::RoleMenuItem mapRowToRoleMenuItem(db::IResultSet& rs)
{
    dto::RoleMenuItem item;
    item.id = rs.valueInt64("id");
    item.roleId = rs.valueInt64("roleId");
    item.caption = rs.valueString("caption");
    if (!rs.isNull("link"))
        item.link = rs.valueString("link");
    if (!rs.isNull("icon"))
        item.icon = rs.valueString("icon");
    return item;
}
} // namespace

SqliteRoleMenuItemRepository::SqliteRoleMenuItemRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteRoleMenuItemRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteRoleMenuItemRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::RoleMenuItem>, int64_t> SqliteRoleMenuItemRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> roleId
)
{
    std::vector<dto::RoleMenuItem> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::string whereClause;
        if (roleId.has_value())
            whereClause = "WHERE roleId = :roleId";

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM RoleMenuItem " + whereClause
        );
        if (roleId.has_value())
            countStmt->bindInt64("roleId", *roleId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { items, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, roleId, caption, link, icon FROM RoleMenuItem "
            + whereClause
            + " ORDER BY caption LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (roleId.has_value())
            stmt->bindInt64("roleId", *roleId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            items.push_back(mapRowToRoleMenuItem(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка RoleMenuItem: " << e.what();
        throw;
    }
    return { items, totalCount };
}

std::optional<dto::RoleMenuItem> SqliteRoleMenuItemRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, roleId, caption, link, icon FROM RoleMenuItem WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRoleMenuItem(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска RoleMenuItem по id: " << e.what();
        throw;
    }
}

std::vector<dto::RoleMenuItem> SqliteRoleMenuItemRepository::findByRoleId(int64_t roleId)
{
    std::vector<dto::RoleMenuItem> result;
    if (roleId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, roleId, caption, link, icon FROM RoleMenuItem "
            "WHERE roleId = :roleId ORDER BY caption"
        );
        stmt->bindInt64("roleId", roleId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRoleMenuItem(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RoleMenuItem по roleId: " << e.what();
        throw;
    }
    return result;
}

int64_t SqliteRoleMenuItemRepository::create(const dto::RoleMenuItem& item)
{
    if (!item.roleId.has_value()
        || !item.caption.has_value()
        || item.caption->empty()
        || !item.link.has_value()
        || item.link->empty())
    {
        LOG_WARN
            << "createRoleMenuItem: отсутствуют обязательные поля (roleId, caption, link)";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO RoleMenuItem (roleId, caption, link, icon) "
            "VALUES (:roleId, :caption, :link, :icon)"
        );
        stmt->bindInt64("roleId", *item.roleId);
        stmt->bindString("caption", *item.caption);
        stmt->bindString("link", *item.link);
        if (item.icon.has_value())
            stmt->bindString("icon", *item.icon);
        else
            stmt->bindNull("icon");
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания RoleMenuItem: " << e.what();
        throw;
    }
}

bool SqliteRoleMenuItemRepository::update(const dto::RoleMenuItem& item)
{
    if (!item.id.has_value())
        return false;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (item.roleId.has_value())
            setClauses.push_back("roleId = :roleId");
        if (item.caption.has_value())
            setClauses.push_back("caption = :caption");
        if (item.link.has_value())
            setClauses.push_back("link = :link");
        if (item.icon.has_value())
            setClauses.push_back("icon = :icon");
        if (setClauses.empty())
            return false;

        const std::string sql = "UPDATE RoleMenuItem SET "
            + boost::algorithm::join(setClauses, ", ")
            + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (item.roleId.has_value())
            stmt->bindInt64("roleId", *item.roleId);
        if (item.caption.has_value())
            stmt->bindString("caption", *item.caption);
        if (item.link.has_value())
            stmt->bindString("link", *item.link);
        if (item.icon.has_value())
            stmt->bindString("icon", *item.icon);
        stmt->bindInt64("id", *item.id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления RoleMenuItem: " << e.what();
        return false;
    }
}

bool SqliteRoleMenuItemRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM RoleMenuItem WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления RoleMenuItem: " << e.what();
        return false;
    }
}

bool SqliteRoleMenuItemRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM RoleMenuItem WHERE id = :id LIMIT 1"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования RoleMenuItem: " << e.what();
        return false;
    }
}

} // namespace server::repositories
