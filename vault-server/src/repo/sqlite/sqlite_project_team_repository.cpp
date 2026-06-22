#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "sqlite_project_team_repository.h"
#include "storage/idatabase.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект ProjectTeam.
 */
dto::ProjectTeam mapRowToProjectTeam(db::IResultSet& rs)
{
    dto::ProjectTeam projectTeam;
    projectTeam.id = rs.valueInt64("id");
    projectTeam.projectId = rs.valueInt64("projectId");
    projectTeam.teamId = rs.valueInt64("teamId");
    return projectTeam;
}

} // namespace

namespace server::repositories
{

SqliteProjectTeamRepository::SqliteProjectTeamRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error(
            "SqliteProjectTeamRepository: database is null"
        );
    }
}

std::shared_ptr<db::IConnection> SqliteProjectTeamRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::ProjectTeam>, int64_t>
SqliteProjectTeamRepository::findAll(int page, int pageSize, std::optional<int64_t> projectId, std::optional<int64_t> teamId)
{
    std::vector<dto::ProjectTeam> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (projectId.has_value())
        {
            whereClauses.push_back("projectId = :projectId");
        }
        if (teamId.has_value())
        {
            whereClauses.push_back("teamId = :teamId");
        }

        const std::string whereStr = whereClauses.empty()
            ? ""
            : "WHERE " + boost::algorithm::join(whereClauses, " AND ");

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM ProjectTeam " + whereStr
        );
        if (projectId.has_value())
        {
            countStmt->bindInt64("projectId", *projectId);
        }
        if (teamId.has_value())
        {
            countStmt->bindInt64("teamId", *teamId);
        }
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { items, totalCount };
        }

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, projectId, teamId FROM ProjectTeam " + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(sql);
        if (projectId.has_value())
        {
            stmt->bindInt64("projectId", *projectId);
        }
        if (teamId.has_value())
        {
            stmt->bindInt64("teamId", *teamId);
        }
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToProjectTeam(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка ProjectTeam: " << e.what();
        throw;
    }

    return { items, totalCount };
}

std::optional<dto::ProjectTeam> SqliteProjectTeamRepository::findById(
    int64_t id
)
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
            "SELECT id, projectId, teamId FROM ProjectTeam WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToProjectTeam(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска ProjectTeam по id: " << e.what();
        throw;
    }
}

bool SqliteProjectTeamRepository::exists(int64_t projectId, int64_t teamId)
{
    if (projectId <= 0 || teamId <= 0)
    {
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ProjectTeam WHERE projectId = :projectId AND teamId "
            "= :teamId LIMIT 1"
        );
        stmt->bindInt64("projectId", projectId);
        stmt->bindInt64("teamId", teamId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка проверки существования ProjectTeam: "
            << e.what();
        return false;
    }
}

int64_t SqliteProjectTeamRepository::create(
    const dto::ProjectTeam& projectTeam
)
{
    if (!projectTeam.projectId.has_value() || !projectTeam.teamId.has_value())
    {
        LOG_WARN
            << "createProjectTeam: отсутствуют обязательные поля (projectId, "
               "teamId)";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO ProjectTeam (projectId, teamId) VALUES (:projectId, "
            ":teamId)"
        );
        stmt->bindInt64("projectId", *projectTeam.projectId);
        stmt->bindInt64("teamId", *projectTeam.teamId);
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания ProjectTeam: " << e.what();
        throw;
    }
}

bool SqliteProjectTeamRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM ProjectTeam WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ProjectTeam: " << e.what();
        return false;
    }
}

} // namespace server::repositories
