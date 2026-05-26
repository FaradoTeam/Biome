#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_rule_state_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::RuleState mapRowToRuleState(db::IResultSet& rs)
{
    dto::RuleState rsState;
    rsState.id = rs.valueInt64("id");
    rsState.ruleId = rs.valueInt64("ruleId");
    rsState.stateId = rs.valueInt64("stateId");
    rsState.isStateAllowed = rs.valueInt64("isStateAllowed") != 0;
    return rsState;
}
} // namespace

SqliteRuleStateRepository::SqliteRuleStateRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteRuleStateRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteRuleStateRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::RuleState>, int64_t> SqliteRuleStateRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> ruleId,
    std::optional<int64_t> stateId
)
{
    std::vector<dto::RuleState> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::vector<std::string> whereClauses;
        if (ruleId.has_value())
            whereClauses.push_back("ruleId = :ruleId");
        if (stateId.has_value())
            whereClauses.push_back("stateId = :stateId");
        const std::string whereStr = whereClauses.empty()
            ? ""
            : "WHERE " + boost::algorithm::join(whereClauses, " AND ");

        auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM RuleState " + whereStr);
        if (ruleId.has_value())
            countStmt->bindInt64("ruleId", *ruleId);
        if (stateId.has_value())
            countStmt->bindInt64("stateId", *stateId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { items, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, ruleId, stateId, isStateAllowed FROM RuleState "
            + whereStr
            + " ORDER BY id LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (ruleId.has_value())
            stmt->bindInt64("ruleId", *ruleId);
        if (stateId.has_value())
            stmt->bindInt64("stateId", *stateId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            items.push_back(mapRowToRuleState(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка RuleState: " << e.what();
        throw;
    }
    return { items, totalCount };
}

std::optional<dto::RuleState> SqliteRuleStateRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, stateId, isStateAllowed FROM RuleState WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRuleState(*rs);
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска RuleState по id: " << e.what();
        throw;
    }
}

std::vector<dto::RuleState> SqliteRuleStateRepository::findByRuleId(int64_t ruleId)
{
    std::vector<dto::RuleState> result;
    if (ruleId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, stateId, isStateAllowed FROM RuleState "
            "WHERE ruleId = :ruleId ORDER BY stateId"
        );
        stmt->bindInt64("ruleId", ruleId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRuleState(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RuleState по ruleId: " << e.what();
        throw;
    }
    return result;
}

std::vector<dto::RuleState> SqliteRuleStateRepository::findByStateId(int64_t stateId)
{
    std::vector<dto::RuleState> result;
    if (stateId <= 0)
        return result;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, stateId, isStateAllowed FROM RuleState "
            "WHERE stateId = :stateId ORDER BY ruleId"
        );
        stmt->bindInt64("stateId", stateId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRuleState(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RuleState по stateId: " << e.what();
        throw;
    }
    return result;
}

bool SqliteRuleStateRepository::exists(int64_t ruleId, int64_t stateId)
{
    if (ruleId <= 0 || stateId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM RuleState WHERE ruleId = :ruleId AND stateId = :stateId LIMIT 1"
        );
        stmt->bindInt64("ruleId", ruleId);
        stmt->bindInt64("stateId", stateId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования RuleState: " << e.what();
        return false;
    }
}

int64_t SqliteRuleStateRepository::create(const dto::RuleState& rs)
{
    if (!rs.ruleId.has_value() || !rs.stateId.has_value())
    {
        LOG_WARN << "createRuleState: отсутствуют ruleId или stateId";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO RuleState (ruleId, stateId, isStateAllowed) "
            "VALUES (:ruleId, :stateId, :isStateAllowed)"
        );
        stmt->bindInt64("ruleId", *rs.ruleId);
        stmt->bindInt64("stateId", *rs.stateId);
        stmt->bindInt64("isStateAllowed", rs.isStateAllowed.value_or(false) ? 1 : 0);
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания RuleState: " << e.what();
        throw;
    }
}

bool SqliteRuleStateRepository::update(const dto::RuleState& rs)
{
    if (!rs.id.has_value())
        return false;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (rs.ruleId.has_value())
            setClauses.push_back("ruleId = :ruleId");
        if (rs.stateId.has_value())
            setClauses.push_back("stateId = :stateId");
        if (rs.isStateAllowed.has_value())
            setClauses.push_back("isStateAllowed = :isStateAllowed");
        if (setClauses.empty())
            return false;

        std::string sql = "UPDATE RuleState SET " + boost::algorithm::join(setClauses, ", ") + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (rs.ruleId.has_value())
            stmt->bindInt64("ruleId", *rs.ruleId);
        if (rs.stateId.has_value())
            stmt->bindInt64("stateId", *rs.stateId);
        if (rs.isStateAllowed.has_value())
            stmt->bindInt64("isStateAllowed", *rs.isStateAllowed ? 1 : 0);
        stmt->bindInt64("id", *rs.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления RuleState: " << e.what();
        return false;
    }
}

bool SqliteRuleStateRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM RuleState WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления RuleState: " << e.what();
        return false;
    }
}

} // namespace server::repositories
