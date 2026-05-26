#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_rule_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::Rule mapRowToRule(db::IResultSet& rs)
{
    dto::Rule rule;
    rule.id = rs.valueInt64("id");
    rule.roleId = rs.valueInt64("roleId");
    rule.isRootProjectCreator = rs.valueInt64("isRootProjectCreator") != 0;
    return rule;
}
} // namespace

SqliteRuleRepository::SqliteRuleRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteRuleRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteRuleRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::Rule>, int64_t> SqliteRuleRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> roleId
)
{
    std::vector<dto::Rule> rules;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::string whereClause;
        if (roleId.has_value())
            whereClause = "WHERE roleId = :roleId";

        auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM Rule " + whereClause);
        if (roleId.has_value())
            countStmt->bindInt64("roleId", *roleId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { rules, totalCount };

        const int offset = (page - 1) * pageSize;
        const std::string sql = "SELECT id, roleId, isRootProjectCreator FROM Rule "
            + whereClause
            + " ORDER BY id LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (roleId.has_value())
            stmt->bindInt64("roleId", *roleId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            rules.push_back(mapRowToRule(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка правил: " << e.what();
        throw;
    }
    return { rules, totalCount };
}

std::optional<dto::Rule> SqliteRuleRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, roleId, isRootProjectCreator FROM Rule WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRule(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска правила по id: " << e.what();
        throw;
    }
}

std::optional<dto::Rule> SqliteRuleRepository::findByRoleId(int64_t roleId)
{
    if (roleId <= 0)
        return std::nullopt;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, roleId, isRootProjectCreator FROM Rule WHERE roleId = :roleId"
        );
        stmt->bindInt64("roleId", roleId);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRule(*rs);

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска правила по roleId: " << e.what();
        throw;
    }
}

int64_t SqliteRuleRepository::create(const dto::Rule& rule)
{
    if (!rule.roleId.has_value())
    {
        LOG_WARN << "createRule: отсутствует roleId";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO Rule (roleId, isRootProjectCreator) VALUES (:roleId, :isRootProjectCreator)"
        );
        stmt->bindInt64("roleId", *rule.roleId);
        stmt->bindInt64("isRootProjectCreator", rule.isRootProjectCreator.value_or(false) ? 1 : 0);
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания правила: " << e.what();
        throw;
    }
}

bool SqliteRuleRepository::update(const dto::Rule& rule)
{
    if (!rule.id.has_value())
        return false;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (rule.roleId.has_value())
            setClauses.push_back("roleId = :roleId");
        if (rule.isRootProjectCreator.has_value())
            setClauses.push_back("isRootProjectCreator = :isRootProjectCreator");
        if (setClauses.empty())
            return false;
        std::string sql = "UPDATE Rule SET " + boost::algorithm::join(setClauses, ", ") + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (rule.roleId.has_value())
            stmt->bindInt64("roleId", *rule.roleId);
        if (rule.isRootProjectCreator.has_value())
            stmt->bindInt64("isRootProjectCreator", *rule.isRootProjectCreator ? 1 : 0);
        stmt->bindInt64("id", *rule.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления правила: " << e.what();
        return false;
    }
}

bool SqliteRuleRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Rule WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления правила: " << e.what();
        return false;
    }
}

bool SqliteRuleRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("SELECT 1 FROM Rule WHERE id = :id LIMIT 1");
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования правила: " << e.what();
        return false;
    }
}

} // namespace server::repositories
