#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_plan_item_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект PlanItem.
 */
dto::PlanItem mapRowToPlanItem(db::IResultSet& rs)
{
    dto::PlanItem planItem;
    planItem.id = rs.valueInt64("id");
    planItem.itemId = rs.valueInt64("itemId");

    if (!rs.isNull("userId"))
        planItem.userId = rs.valueInt64("userId");

    planItem.planId = rs.valueInt64("planId");

    if (!rs.isNull("startDate"))
    {
        int64_t timestamp = rs.valueInt64("startDate");
        planItem.startDate = common::secondsToTimePoint(timestamp);
    }

    if (!rs.isNull("endDate"))
    {
        int64_t timestamp = rs.valueInt64("endDate");
        planItem.endDate = common::secondsToTimePoint(timestamp);
    }

    return planItem;
}

} // namespace

namespace server
{
namespace repositories
{

SqlitePlanItemRepository::SqlitePlanItemRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqlitePlanItemRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqlitePlanItemRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::PlanItem>, int64_t> SqlitePlanItemRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> planId,
    std::optional<int64_t> userId
)
{
    std::vector<dto::PlanItem> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем WHERE clause
        std::vector<std::string> whereClauses;
        if (planId.has_value())
            whereClauses.push_back("planId = :planId");
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM PlanItem" + whereStr
        );
        if (planId.has_value())
            countStmt->bindInt64("planId", *planId);
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { items, totalCount };
        }

        // 2. Получаем страницу с элементами плана
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, itemId, userId, planId, startDate, endDate "
                                "FROM PlanItem"
            + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (planId.has_value())
            stmt->bindInt64("planId", *planId);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToPlanItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка элементов плана: " << e.what();
        throw;
    }

    return { items, totalCount };
}

std::optional<dto::PlanItem> SqlitePlanItemRepository::findById(int64_t id)
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
            "SELECT id, itemId, userId, planId, startDate, endDate "
            "FROM PlanItem WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToPlanItem(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска элемента плана по id: " << e.what();
        throw;
    }
}

std::optional<dto::PlanItem> SqlitePlanItemRepository::findByPlanAndItem(
    int64_t planId,
    int64_t itemId
)
{
    if (planId <= 0 || itemId <= 0)
    {
        LOG_WARN << "findByPlanAndItem: неверные параметры";
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, planId, startDate, endDate "
            "FROM PlanItem WHERE planId = :planId AND itemId = :itemId LIMIT 1"
        );

        stmt->bindInt64("planId", planId);
        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToPlanItem(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска элемента плана по planId и itemId: " << e.what();
        throw;
    }
}

std::vector<dto::PlanItem> SqlitePlanItemRepository::findByPlanId(int64_t planId)
{
    std::vector<dto::PlanItem> items;

    if (planId <= 0)
    {
        LOG_WARN << "findByPlanId: неверный planId " << planId;
        return items;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, planId, startDate, endDate "
            "FROM PlanItem WHERE planId = :planId ORDER BY startDate"
        );

        stmt->bindInt64("planId", planId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToPlanItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения элементов плана: " << e.what();
        throw;
    }

    return items;
}

std::vector<dto::PlanItem> SqlitePlanItemRepository::findByUserId(int64_t userId)
{
    std::vector<dto::PlanItem> items;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return items;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT pi.id, pi.itemId, pi.userId, pi.planId, pi.startDate, pi.endDate "
            "FROM PlanItem pi "
            "INNER JOIN Plan p ON pi.planId = p.id "
            "WHERE pi.userId = :userId AND p.isActive = 1 "
            "ORDER BY pi.startDate"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToPlanItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения элементов по пользователю: " << e.what();
        throw;
    }

    return items;
}

std::vector<dto::PlanItem> SqlitePlanItemRepository::findByDateRange(
    const common::DateTime& dateFrom,
    const common::DateTime& dateTo
)
{
    std::vector<dto::PlanItem> items;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT pi.id, pi.itemId, pi.userId, pi.planId, pi.startDate, pi.endDate "
            "FROM PlanItem pi "
            "INNER JOIN Plan p ON pi.planId = p.id "
            "WHERE p.isActive = 1 "
            "AND ((pi.startDate BETWEEN :dateFrom AND :dateTo) "
            "OR (pi.endDate BETWEEN :dateFrom AND :dateTo) "
            "OR (pi.startDate <= :dateFrom AND pi.endDate >= :dateTo)) "
            "ORDER BY pi.startDate"
        );

        stmt->bindInt64("dateFrom", common::timePointToSeconds(dateFrom));
        stmt->bindInt64("dateTo", common::timePointToSeconds(dateTo));
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToPlanItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения элементов по диапазону дат: " << e.what();
        throw;
    }

    return items;
}

int64_t SqlitePlanItemRepository::create(const dto::PlanItem& planItem)
{
    // Проверка обязательных полей
    if (!planItem.itemId.has_value())
    {
        LOG_WARN << "createPlanItem: отсутствует itemId";
        return 0;
    }
    if (!planItem.planId.has_value())
    {
        LOG_WARN << "createPlanItem: отсутствует planId";
        return 0;
    }
    if (!planItem.startDate.has_value())
    {
        LOG_WARN << "createPlanItem: отсутствует startDate";
        return 0;
    }
    if (!planItem.endDate.has_value())
    {
        LOG_WARN << "createPlanItem: отсутствует endDate";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO PlanItem (itemId, userId, planId, startDate, endDate) "
            "VALUES (:itemId, :userId, :planId, :startDate, :endDate)"
        );

        stmt->bindInt64("itemId", *planItem.itemId);

        if (planItem.userId.has_value())
            stmt->bindInt64("userId", *planItem.userId);
        else
            stmt->bindNull("userId");

        stmt->bindInt64("planId", *planItem.planId);
        stmt->bindInt64("startDate", common::timePointToSeconds(*planItem.startDate));
        stmt->bindInt64("endDate", common::timePointToSeconds(*planItem.endDate));

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания элемента плана: " << e.what();
        throw;
    }
}

bool SqlitePlanItemRepository::update(const dto::PlanItem& planItem)
{
    if (!planItem.id.has_value())
    {
        LOG_WARN << "updatePlanItem: отсутствует ID";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE PlanItem SET ";

        if (planItem.itemId.has_value())
            setClauses.push_back("itemId = :itemId");
        if (planItem.userId.has_value())
            setClauses.push_back("userId = :userId");
        if (planItem.planId.has_value())
            setClauses.push_back("planId = :planId");
        if (planItem.startDate.has_value())
            setClauses.push_back("startDate = :startDate");
        if (planItem.endDate.has_value())
            setClauses.push_back("endDate = :endDate");

        if (setClauses.empty())
        {
            LOG_WARN << "updatePlanItem: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (planItem.itemId.has_value())
            stmt->bindInt64("itemId", *planItem.itemId);
        if (planItem.userId.has_value())
            stmt->bindInt64("userId", *planItem.userId);
        if (planItem.planId.has_value())
            stmt->bindInt64("planId", *planItem.planId);
        if (planItem.startDate.has_value())
            stmt->bindInt64("startDate", common::timePointToSeconds(*planItem.startDate));
        if (planItem.endDate.has_value())
            stmt->bindInt64("endDate", common::timePointToSeconds(*planItem.endDate));

        stmt->bindInt64("id", *planItem.id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления элемента плана: " << e.what();
        return false;
    }
}

bool SqlitePlanItemRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "removePlanItem: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM PlanItem WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления элемента плана: " << e.what();
        return false;
    }
}

int64_t SqlitePlanItemRepository::removeByPlanId(int64_t planId)
{
    if (planId <= 0)
    {
        LOG_WARN << "removeByPlanId: неверный planId " << planId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM PlanItem WHERE planId = :planId"
        );

        stmt->bindInt64("planId", planId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления элементов плана: " << e.what();
        return 0;
    }
}

bool SqlitePlanItemRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM PlanItem WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования элемента плана: " << e.what();
        return false;
    }
}

bool SqlitePlanItemRepository::existsByPlanAndItem(int64_t planId, int64_t itemId)
{
    if (planId <= 0 || itemId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM PlanItem WHERE planId = :planId AND itemId = :itemId LIMIT 1"
        );

        stmt->bindInt64("planId", planId);
        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования элемента в плане: " << e.what();
        return false;
    }
}

int64_t SqlitePlanItemRepository::copyFromPlan(int64_t sourcePlanId, int64_t targetPlanId)
{
    if (sourcePlanId <= 0 || targetPlanId <= 0)
    {
        LOG_WARN << "copyFromPlan: неверные параметры";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO PlanItem (itemId, userId, planId, startDate, endDate) "
            "SELECT itemId, userId, :targetPlanId, startDate, endDate "
            "FROM PlanItem WHERE planId = :sourcePlanId"
        );

        stmt->bindInt64("targetPlanId", targetPlanId);
        stmt->bindInt64("sourcePlanId", sourcePlanId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка копирования элементов плана: " << e.what();
        return 0;
    }
}

dto::PlanItem SqlitePlanItemRepository::mapRowToPlanItem(db::IResultSet& rs) const
{
    dto::PlanItem planItem;
    planItem.id = rs.valueInt64("id");
    planItem.itemId = rs.valueInt64("itemId");

    if (!rs.isNull("userId"))
        planItem.userId = rs.valueInt64("userId");

    planItem.planId = rs.valueInt64("planId");

    if (!rs.isNull("startDate"))
    {
        int64_t timestamp = rs.valueInt64("startDate");
        planItem.startDate = common::secondsToTimePoint(timestamp);
    }

    if (!rs.isNull("endDate"))
    {
        int64_t timestamp = rs.valueInt64("endDate");
        planItem.endDate = common::secondsToTimePoint(timestamp);
    }

    return planItem;
}

} // namespace repositories
} // namespace server
