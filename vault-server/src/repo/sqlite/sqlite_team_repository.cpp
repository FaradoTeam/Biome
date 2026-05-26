#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "sqlite_team_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::Team mapRowToTeam(db::IResultSet& rs)
{
    dto::Team team;
    team.id = rs.valueInt64("id");
    team.caption = rs.valueString("caption");
    if (!rs.isNull("description"))
        team.description = rs.valueString("description");
    return team;
}
} // namespace

SqliteTeamRepository::SqliteTeamRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteTeamRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteTeamRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::Team>, int64_t> SqliteTeamRepository::findAll(
    int page,
    int pageSize,
    const std::string& searchCaption
)
{
    std::vector<dto::Team> teams;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::string whereClause;
        if (!searchCaption.empty())
        {
            whereClause = "WHERE LOWER(caption) LIKE '%' || LOWER(:search) || '%'";
        }

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Team " + whereClause
        );
        if (!searchCaption.empty())
            countStmt->bindString("search", searchCaption);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { teams, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, caption, description FROM Team "
            + whereClause
            + " ORDER BY caption LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (!searchCaption.empty())
            stmt->bindString("search", searchCaption);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            teams.push_back(mapRowToTeam(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка команд: " << e.what();
        throw;
    }
    return { teams, totalCount };
}

std::optional<dto::Team> SqliteTeamRepository::findById(int64_t id)
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
            "SELECT id, caption, description FROM Team WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToTeam(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска команды по id: " << e.what();
        throw;
    }
}

int64_t SqliteTeamRepository::create(const dto::Team& team)
{
    if (!team.caption.has_value() || team.caption->empty())
    {
        LOG_WARN << "createTeam: отсутствует название команды";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO Team (caption, description) VALUES (:caption, :description)"
        );
        stmt->bindString("caption", *team.caption);
        if (team.description.has_value())
            stmt->bindString("description", *team.description);
        else
            stmt->bindNull("description");
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания команды: " << e.what();
        throw;
    }
}

bool SqliteTeamRepository::update(const dto::Team& team)
{
    if (!team.id.has_value())
    {
        LOG_WARN << "updateTeam: отсутствует ID";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (team.caption.has_value())
            setClauses.push_back("caption = :caption");
        if (team.description.has_value())
            setClauses.push_back("description = :description");
        if (setClauses.empty())
        {
            LOG_WARN << "updateTeam: нет полей для обновления";
            return false;
        }
        std::string sql = "UPDATE Team SET " + boost::algorithm::join(setClauses, ", ") + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (team.caption.has_value())
            stmt->bindString("caption", *team.caption);
        if (team.description.has_value())
            stmt->bindString("description", *team.description);
        stmt->bindInt64("id", *team.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления команды: " << e.what();
        return false;
    }
}

bool SqliteTeamRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "removeTeam: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Team WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления команды: " << e.what();
        return false;
    }
}

bool SqliteTeamRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("SELECT 1 FROM Team WHERE id = :id LIMIT 1");
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования команды: " << e.what();
        return false;
    }
}

} // namespace server::repositories
