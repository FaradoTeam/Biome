#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "sqlite_rule_item_type_repository.h"
#include "storage/idatabase.h"

namespace server::repositories
{

namespace
{
dto::RuleItemType mapRowToRuleItemType(db::IResultSet& rs)
{
    dto::RuleItemType rit;
    rit.id = rs.valueInt64("id");
    rit.ruleId = rs.valueInt64("ruleId");
    rit.itemTypeId = rs.valueInt64("itemTypeId");
    rit.isReader = rs.valueInt64("isReader") != 0;
    rit.isWriter = rs.valueInt64("isWriter") != 0;
    return rit;
}
} // namespace

SqliteRuleItemTypeRepository::SqliteRuleItemTypeRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
        throw std::runtime_error("SqliteRuleItemTypeRepository: database is null");
}

std::shared_ptr<db::IConnection> SqliteRuleItemTypeRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::RuleItemType>, int64_t> SqliteRuleItemTypeRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> ruleId,
    std::optional<int64_t> itemTypeId
)
{
    std::vector<dto::RuleItemType> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();
        std::vector<std::string> whereClauses;
        if (ruleId.has_value())
            whereClauses.push_back("ruleId = :ruleId");
        if (itemTypeId.has_value())
            whereClauses.push_back("itemTypeId = :itemTypeId");
        std::string whereStr = whereClauses.empty() ? "" : "WHERE " + boost::algorithm::join(whereClauses, " AND ");

        auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM RuleItemType " + whereStr);
        if (ruleId.has_value())
            countStmt->bindInt64("ruleId", *ruleId);
        if (itemTypeId.has_value())
            countStmt->bindInt64("itemTypeId", *itemTypeId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
            totalCount = countRs->valueInt64(0);

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
            return { items, totalCount };

        const int offset = (page - 1) * pageSize;
        std::string sql = "SELECT id, ruleId, itemTypeId, isReader, isWriter FROM RuleItemType " + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset";
        auto stmt = conn->prepareStatement(sql);
        if (ruleId.has_value())
            stmt->bindInt64("ruleId", *ruleId);
        if (itemTypeId.has_value())
            stmt->bindInt64("itemTypeId", *itemTypeId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
            items.push_back(mapRowToRuleItemType(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка RuleItemType: " << e.what();
        throw;
    }
    return { items, totalCount };
}

std::optional<dto::RuleItemType> SqliteRuleItemTypeRepository::findById(int64_t id)
{
    if (id <= 0)
        return std::nullopt;
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, itemTypeId, isReader, isWriter FROM RuleItemType WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        if (rs->next())
            return mapRowToRuleItemType(*rs);
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска RuleItemType по id: " << e.what();
        throw;
    }
}

std::vector<dto::RuleItemType> SqliteRuleItemTypeRepository::findByRuleId(int64_t ruleId)
{
    std::vector<dto::RuleItemType> result;
    if (ruleId <= 0)
        return result;
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, itemTypeId, isReader, isWriter FROM RuleItemType "
            "WHERE ruleId = :ruleId ORDER BY itemTypeId"
        );
        stmt->bindInt64("ruleId", ruleId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRuleItemType(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RuleItemType по ruleId: " << e.what();
        throw;
    }
    return result;
}

std::vector<dto::RuleItemType> SqliteRuleItemTypeRepository::findByItemTypeId(int64_t itemTypeId)
{
    std::vector<dto::RuleItemType> result;
    if (itemTypeId <= 0)
        return result;
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, ruleId, itemTypeId, isReader, isWriter FROM RuleItemType "
            "WHERE itemTypeId = :itemTypeId ORDER BY ruleId"
        );
        stmt->bindInt64("itemTypeId", itemTypeId);
        auto rs = stmt->executeQuery();
        while (rs->next())
            result.push_back(mapRowToRuleItemType(*rs));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения RuleItemType по itemTypeId: " << e.what();
        throw;
    }
    return result;
}

bool SqliteRuleItemTypeRepository::exists(int64_t ruleId, int64_t itemTypeId)
{
    if (ruleId <= 0 || itemTypeId <= 0)
        return false;
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM RuleItemType WHERE ruleId = :ruleId AND itemTypeId = :itemTypeId LIMIT 1"
        );
        stmt->bindInt64("ruleId", ruleId);
        stmt->bindInt64("itemTypeId", itemTypeId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования RuleItemType: " << e.what();
        return false;
    }
}

int64_t SqliteRuleItemTypeRepository::create(const dto::RuleItemType& rit)
{
    if (!rit.ruleId.has_value() || !rit.itemTypeId.has_value())
    {
        LOG_WARN << "createRuleItemType: отсутствуют ruleId или itemTypeId";
        return 0;
    }
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO RuleItemType (ruleId, itemTypeId, isReader, isWriter) "
            "VALUES (:ruleId, :itemTypeId, :isReader, :isWriter)"
        );
        stmt->bindInt64("ruleId", *rit.ruleId);
        stmt->bindInt64("itemTypeId", *rit.itemTypeId);
        stmt->bindInt64("isReader", rit.isReader.value_or(false) ? 1 : 0);
        stmt->bindInt64("isWriter", rit.isWriter.value_or(false) ? 1 : 0);
        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания RuleItemType: " << e.what();
        throw;
    }
}

bool SqliteRuleItemTypeRepository::update(const dto::RuleItemType& rit)
{
    if (!rit.id.has_value())
        return false;
    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        if (rit.ruleId.has_value())
            setClauses.push_back("ruleId = :ruleId");
        if (rit.itemTypeId.has_value())
            setClauses.push_back("itemTypeId = :itemTypeId");
        if (rit.isReader.has_value())
            setClauses.push_back("isReader = :isReader");
        if (rit.isWriter.has_value())
            setClauses.push_back("isWriter = :isWriter");
        if (setClauses.empty())
            return false;

        std::string sql = "UPDATE RuleItemType SET " + boost::algorithm::join(setClauses, ", ") + " WHERE id = :id";
        auto stmt = conn->prepareStatement(sql);
        if (rit.ruleId.has_value())
            stmt->bindInt64("ruleId", *rit.ruleId);
        if (rit.itemTypeId.has_value())
            stmt->bindInt64("itemTypeId", *rit.itemTypeId);
        if (rit.isReader.has_value())
            stmt->bindInt64("isReader", *rit.isReader ? 1 : 0);
        if (rit.isWriter.has_value())
            stmt->bindInt64("isWriter", *rit.isWriter ? 1 : 0);
        stmt->bindInt64("id", *rit.id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления RuleItemType: " << e.what();
        return false;
    }
}

bool SqliteRuleItemTypeRepository::remove(int64_t id)
{
    if (id <= 0)
        return false;
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM RuleItemType WHERE id = :id");
        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления RuleItemType: " << e.what();
        return false;
    }
}

} // namespace server::repositories
