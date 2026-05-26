#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_rule_project_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::RuleProject mapRowToRuleProject(db::IResultSet& rs)
{
    dto::RuleProject rp;
    rp.id = rs.valueInt64("id");
    rp.ruleId = rs.valueInt64("ruleId");
    rp.projectId = rs.valueInt64("projectId");
    rp.isReader = rs.valueInt64("isReader") != 0;
    rp.isWriter = rs.valueInt64("isWriter") != 0;
    rp.isProjectEditor = rs.valueInt64("isProjectEditor") != 0;
    rp.isPhaseEditor = rs.valueInt64("isPhaseEditor") != 0;
    rp.isBoardEditor = rs.valueInt64("isBoardEditor") != 0;
    return rp;
}
} // namespace

SqliteRuleProjectRepository::SqliteRuleProjectRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteRuleProjectRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteRuleProjectRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::RuleProject>, int64_t> SqliteRuleProjectRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> ruleId,
    std::optional<int64_t> projectId
)
{
    std::vector<dto::RuleProject> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::vector<std::string> whereClauses;
        if (ruleId.has_value())
            whereClauses.push_back("ruleId = :ruleId");
        if (projectId.has_value())
            whereClauses.push_back("projectId = :projectId");
        const std::string whereStr = whereClauses.empty()
            ? ""
            : "WHERE " + boost::algorithm::join(whereClauses, " AND ");

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM RuleProject " + whereStr
        );
        if (ruleId.has_value())
            countStmt->bindInt64("ruleId", *ruleId);
        if (projectId.has_value())
            countStmt->bindInt64("projectId", *projectId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { items, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, ruleId, projectId, isReader, isWriter, isProjectEditor, "
                                "isPhaseEditor, isBoardEditor FROM RuleProject "
            + whereStr
            + " ORDER BY id LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (ruleId.has_value())
            stmt->bindInt64("ruleId", *ruleId);
        if (projectId.has_value())
            stmt->bindInt64("projectId", *projectId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            items.push_back(mapRowToRuleProject(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка RuleProject: " << e.what();
        throw;
    }
    return { items, totalCount };
}

std::optional<dto::RuleProject> SqliteRuleProjectRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, projectId, isReader, isWriter, isProjectEditor, isPhaseEditor, isBoardEditor "
            "FROM RuleProject WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRuleProject(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска RuleProject по id: " << e.what();
        throw;
    }
}

std::vector<dto::RuleProject> SqliteRuleProjectRepository::findByRuleId(int64_t ruleId)
{
    std::vector<dto::RuleProject> result;
    if (ruleId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, projectId, isReader, isWriter, isProjectEditor, isPhaseEditor, isBoardEditor "
            "FROM RuleProject WHERE ruleId = :ruleId ORDER BY projectId"
        );
        stmt->bindInt64("ruleId", ruleId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRuleProject(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RuleProject по ruleId: " << e.what();
        throw;
    }
    return result;
}

std::vector<dto::RuleProject> SqliteRuleProjectRepository::findByProjectId(int64_t projectId)
{
    std::vector<dto::RuleProject> result;
    if (projectId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, projectId, isReader, isWriter, isProjectEditor, isPhaseEditor, isBoardEditor "
            "FROM RuleProject WHERE projectId = :projectId ORDER BY ruleId"
        );
        stmt->bindInt64("projectId", projectId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRuleProject(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RuleProject по projectId: " << e.what();
        throw;
    }
    return result;
}

bool SqliteRuleProjectRepository::exists(int64_t ruleId, int64_t projectId)
{
    if (ruleId <= 0 || projectId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM RuleProject WHERE ruleId = :ruleId AND projectId = :projectId LIMIT 1"
        );
        stmt->bindInt64("ruleId", ruleId);
        stmt->bindInt64("projectId", projectId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования RuleProject: " << e.what();
        return false;
    }
}

int64_t SqliteRuleProjectRepository::create(const dto::RuleProject& rp)
{
    if (!rp.ruleId.has_value() || !rp.projectId.has_value())
    {
        LOG_WARN << "createRuleProject: отсутствуют ruleId или projectId";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO RuleProject (ruleId, projectId, isReader, isWriter, isProjectEditor, "
            "isPhaseEditor, isBoardEditor) VALUES (:ruleId, :projectId, :isReader, :isWriter, "
            ":isProjectEditor, :isPhaseEditor, :isBoardEditor)"
        );
        stmt->bindInt64("ruleId", *rp.ruleId);
        stmt->bindInt64("projectId", *rp.projectId);
        stmt->bindInt64("isReader", rp.isReader.value_or(false) ? 1 : 0);
        stmt->bindInt64("isWriter", rp.isWriter.value_or(false) ? 1 : 0);
        stmt->bindInt64("isProjectEditor", rp.isProjectEditor.value_or(false) ? 1 : 0);
        stmt->bindInt64("isPhaseEditor", rp.isPhaseEditor.value_or(false) ? 1 : 0);
        stmt->bindInt64("isBoardEditor", rp.isBoardEditor.value_or(false) ? 1 : 0);
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания RuleProject: " << e.what();
        throw;
    }
}

bool SqliteRuleProjectRepository::update(const dto::RuleProject& rp)
{
    if (!rp.id.has_value())
        return false;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (rp.ruleId.has_value())
            setClauses.push_back("ruleId = :ruleId");
        if (rp.projectId.has_value())
            setClauses.push_back("projectId = :projectId");
        if (rp.isReader.has_value())
            setClauses.push_back("isReader = :isReader");
        if (rp.isWriter.has_value())
            setClauses.push_back("isWriter = :isWriter");
        if (rp.isProjectEditor.has_value())
            setClauses.push_back("isProjectEditor = :isProjectEditor");
        if (rp.isPhaseEditor.has_value())
            setClauses.push_back("isPhaseEditor = :isPhaseEditor");
        if (rp.isBoardEditor.has_value())
            setClauses.push_back("isBoardEditor = :isBoardEditor");
        if (setClauses.empty())
            return false;

        std::string sql = "UPDATE RuleProject SET " + boost::algorithm::join(setClauses, ", ") + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (rp.ruleId.has_value())
            stmt->bindInt64("ruleId", *rp.ruleId);
        if (rp.projectId.has_value())
            stmt->bindInt64("projectId", *rp.projectId);
        if (rp.isReader.has_value())
            stmt->bindInt64("isReader", *rp.isReader ? 1 : 0);
        if (rp.isWriter.has_value())
            stmt->bindInt64("isWriter", *rp.isWriter ? 1 : 0);
        if (rp.isProjectEditor.has_value())
            stmt->bindInt64("isProjectEditor", *rp.isProjectEditor ? 1 : 0);
        if (rp.isPhaseEditor.has_value())
            stmt->bindInt64("isPhaseEditor", *rp.isPhaseEditor ? 1 : 0);
        if (rp.isBoardEditor.has_value())
            stmt->bindInt64("isBoardEditor", *rp.isBoardEditor ? 1 : 0);
        stmt->bindInt64("id", *rp.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления RuleProject: " << e.what();
        return false;
    }
}

bool SqliteRuleProjectRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM RuleProject WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления RuleProject: " << e.what();
        return false;
    }
}

} // namespace server::repositories
