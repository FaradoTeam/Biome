#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_role_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::Role mapRowToRole(db::IResultSet& rs)
{
    dto::Role role;
    role.id = rs.valueInt64("id");
    role.caption = rs.valueString("caption");
    if (!rs.isNull("description"))
        role.description = rs.valueString("description");
    return role;
}
} // namespace

SqliteRoleRepository::SqliteRoleRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteRoleRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteRoleRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::Role>, int64_t> SqliteRoleRepository::findAll(
    int page,
    int pageSize,
    const std::string& searchCaption
)
{
    std::vector<dto::Role> roles;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::string whereClause;
        if (!searchCaption.empty())
        {
            whereClause = "WHERE LOWER(caption) LIKE '%' || LOWER(:search) || '%'";
        }
        auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM Role " + whereClause);
        if (!searchCaption.empty())
            countStmt->bindString("search", searchCaption);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { roles, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, caption, description FROM Role "
            + whereClause
            + " ORDER BY caption LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (!searchCaption.empty())
            stmt->bindString("search", searchCaption);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            roles.push_back(mapRowToRole(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка ролей: " << e.what();
        throw;
    }
    return { roles, totalCount };
}

std::optional<dto::Role> SqliteRoleRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, caption, description FROM Role WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRole(*rs);
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска роли по id: " << e.what();
        throw;
    }
}

std::optional<dto::Role> SqliteRoleRepository::findByCaption(const std::string& caption)
{
    if (caption.empty())
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, caption, description FROM Role WHERE caption = :caption"
        );
        stmt->bindString("caption", caption);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRole(*rs);
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска роли по названию: " << e.what();
        throw;
    }
}

int64_t SqliteRoleRepository::create(const dto::Role& role)
{
    if (!role.caption.has_value() || role.caption->empty())
    {
        LOG_WARN << "createRole: отсутствует название роли";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO Role (caption, description) VALUES (:caption, :description)"
        );
        stmt->bindString("caption", *role.caption);
        if (role.description.has_value())
            stmt->bindString("description", *role.description);
        else
            stmt->bindNull("description");
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания роли: " << e.what();
        throw;
    }
}

bool SqliteRoleRepository::update(const dto::Role& role)
{
    if (!role.id.has_value())
        return false;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (role.caption.has_value())
            setClauses.push_back("caption = :caption");
        if (role.description.has_value())
            setClauses.push_back("description = :description");
        if (setClauses.empty())
            return false;
        const std::string sql = "UPDATE Role SET "
            + boost::algorithm::join(setClauses, ", ")
            + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (role.caption.has_value())
            stmt->bindString("caption", *role.caption);
        if (role.description.has_value())
            stmt->bindString("description", *role.description);
        stmt->bindInt64("id", *role.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления роли: " << e.what();
        return false;
    }
}

bool SqliteRoleRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Role WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления роли: " << e.what();
        return false;
    }
}

bool SqliteRoleRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("SELECT 1 FROM Role WHERE id = :id LIMIT 1");
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования роли: " << e.what();
        return false;
    }
}

} // namespace server::repositories
