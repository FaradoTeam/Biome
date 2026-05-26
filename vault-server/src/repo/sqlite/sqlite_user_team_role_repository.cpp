#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_user_team_role_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::UserTeamRole mapRowToUserTeamRole(db::IResultSet& rs)
{
    dto::UserTeamRole utr;
    utr.id = rs.valueInt64("id");
    utr.userId = rs.valueInt64("userId");
    utr.teamId = rs.valueInt64("teamId");
    utr.roleId = rs.valueInt64("roleId");
    return utr;
}
} // namespace

SqliteUserTeamRoleRepository::SqliteUserTeamRoleRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteUserTeamRoleRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteUserTeamRoleRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::UserTeamRole>, int64_t> SqliteUserTeamRoleRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> userId,
    std::optional<int64_t> teamId,
    std::optional<int64_t> roleId
)
{
    std::vector<dto::UserTeamRole> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::vector<std::string> whereClauses;
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");
        if (teamId.has_value())
            whereClauses.push_back("teamId = :teamId");
        if (roleId.has_value())
            whereClauses.push_back("roleId = :roleId");
        const std::string whereStr = whereClauses.empty()
            ? ""
            : "WHERE " + boost::algorithm::join(whereClauses, " AND ");

        auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM UserTeamRole " + whereStr);
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);
        if (teamId.has_value())
            countStmt->bindInt64("teamId", *teamId);
        if (roleId.has_value())
            countStmt->bindInt64("roleId", *roleId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { items, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, userId, teamId, roleId FROM UserTeamRole "
            + whereStr
            + " ORDER BY id LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        if (teamId.has_value())
            stmt->bindInt64("teamId", *teamId);
        if (roleId.has_value())
            stmt->bindInt64("roleId", *roleId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            items.push_back(mapRowToUserTeamRole(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка UserTeamRole: " << e.what();
        throw;
    }
    return { items, totalCount };
}

std::optional<dto::UserTeamRole> SqliteUserTeamRoleRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, teamId, roleId FROM UserTeamRole WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToUserTeamRole(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска UserTeamRole по id: " << e.what();
        throw;
    }
}

std::vector<dto::UserTeamRole> SqliteUserTeamRoleRepository::findByUserId(int64_t userId)
{
    std::vector<dto::UserTeamRole> result;
    if (userId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, teamId, roleId FROM UserTeamRole WHERE userId = :userId"
        );
        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToUserTeamRole(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения UserTeamRole по userId: " << e.what();
        throw;
    }
    return result;
}

std::vector<dto::UserTeamRole> SqliteUserTeamRoleRepository::findByTeamId(int64_t teamId)
{
    std::vector<dto::UserTeamRole> result;
    if (teamId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, teamId, roleId FROM UserTeamRole WHERE teamId = :teamId"
        );
        stmt->bindInt64("teamId", teamId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToUserTeamRole(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения UserTeamRole по teamId: " << e.what();
        throw;
    }
    return result;
}

std::optional<dto::UserTeamRole> SqliteUserTeamRoleRepository::findByUserAndTeam(int64_t userId, int64_t teamId)
{
    if (userId <= 0 || teamId <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, teamId, roleId FROM UserTeamRole WHERE userId = :userId AND teamId = :teamId LIMIT 1"
        );
        stmt->bindInt64("userId", userId);
        stmt->bindInt64("teamId", teamId);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToUserTeamRole(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска UserTeamRole по userId и teamId: " << e.what();
        throw;
    }
}

bool SqliteUserTeamRoleRepository::exists(int64_t userId, int64_t teamId)
{
    if (userId <= 0 || teamId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM UserTeamRole WHERE userId = :userId AND teamId = :teamId LIMIT 1"
        );
        stmt->bindInt64("userId", userId);
        stmt->bindInt64("teamId", teamId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования UserTeamRole: " << e.what();
        return false;
    }
}

int64_t SqliteUserTeamRoleRepository::create(const dto::UserTeamRole& utr)
{
    if (!utr.userId.has_value() || !utr.teamId.has_value() || !utr.roleId.has_value())
    {
        LOG_WARN << "createUserTeamRole: отсутствуют обязательные поля (userId, teamId, roleId)";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO UserTeamRole (userId, teamId, roleId) VALUES (:userId, :teamId, :roleId)"
        );
        stmt->bindInt64("userId", *utr.userId);
        stmt->bindInt64("teamId", *utr.teamId);
        stmt->bindInt64("roleId", *utr.roleId);
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания UserTeamRole: " << e.what();
        throw;
    }
}

bool SqliteUserTeamRoleRepository::update(const dto::UserTeamRole& utr)
{
    if (!utr.id.has_value())
        return false;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (utr.userId.has_value())
            setClauses.push_back("userId = :userId");
        if (utr.teamId.has_value())
            setClauses.push_back("teamId = :teamId");
        if (utr.roleId.has_value())
            setClauses.push_back("roleId = :roleId");
        if (setClauses.empty())
            return false;

        const std::string sql = "UPDATE UserTeamRole SET "
            + boost::algorithm::join(setClauses, ", ")
            + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (utr.userId.has_value())
            stmt->bindInt64("userId", *utr.userId);
        if (utr.teamId.has_value())
            stmt->bindInt64("teamId", *utr.teamId);
        if (utr.roleId.has_value())
            stmt->bindInt64("roleId", *utr.roleId);
        stmt->bindInt64("id", *utr.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления UserTeamRole: " << e.what();
        return false;
    }
}

bool SqliteUserTeamRoleRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM UserTeamRole WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления UserTeamRole: " << e.what();
        return false;
    }
}

bool SqliteUserTeamRoleRepository::removeByUserAndTeam(int64_t userId, int64_t teamId)
{
    if (userId <= 0 || teamId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM UserTeamRole WHERE userId = :userId AND teamId = :teamId"
        );
        stmt->bindInt64("userId", userId);
        stmt->bindInt64("teamId", teamId);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления UserTeamRole по userId и teamId: " << e.what();
        return false;
    }
}

} // namespace server::repositories
