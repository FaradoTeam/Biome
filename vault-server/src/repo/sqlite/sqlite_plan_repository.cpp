#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_plan_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект Plan.
 */
dto::Plan mapRowToPlan(db::IResultSet& rs)
{
    dto::Plan plan;
    plan.id = rs.valueInt64("id");
    plan.phaseId = rs.valueInt64("phaseId");

    if (!rs.isNull("basePlanId"))
        plan.basePlanId = rs.valueInt64("basePlanId");

    plan.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        plan.description = rs.valueString("description");

    plan.isActive = rs.valueInt64("isActive") != 0;

    if (!rs.isNull("createdAt"))
    {
        int64_t timestamp = rs.valueInt64("createdAt");
        plan.createdAt = common::secondsToTimePoint(timestamp);
    }

    plan.createdByUserId = rs.valueInt64("createdByUserId");

    if (!rs.isNull("activatedAt"))
    {
        int64_t timestamp = rs.valueInt64("activatedAt");
        plan.activatedAt = common::secondsToTimePoint(timestamp);
    }

    if (!rs.isNull("activatedByUserId"))
        plan.activatedByUserId = rs.valueInt64("activatedByUserId");

    return plan;
}

} // namespace

namespace server
{
namespace repositories
{

SqlitePlanRepository::SqlitePlanRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqlitePlanRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqlitePlanRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::Plan>, int64_t> SqlitePlanRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> phaseId,
    std::optional<bool> isActive
)
{
    std::vector<dto::Plan> plans;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем WHERE clause
        std::vector<std::string> whereClauses;
        if (phaseId.has_value())
            whereClauses.push_back("phaseId = :phaseId");
        if (isActive.has_value())
            whereClauses.push_back("isActive = :isActive");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Plan" + whereStr
        );
        if (phaseId.has_value())
            countStmt->bindInt64("phaseId", *phaseId);
        if (isActive.has_value())
            countStmt->bindInt64("isActive", *isActive ? 1 : 0);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { plans, totalCount };
        }

        // 2. Получаем страницу с планами
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, phaseId, basePlanId, caption, description, "
                                "isActive, createdAt, createdByUserId, activatedAt, activatedByUserId "
                                "FROM Plan"
            + whereStr + " ORDER BY createdAt DESC LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (phaseId.has_value())
            stmt->bindInt64("phaseId", *phaseId);
        if (isActive.has_value())
            stmt->bindInt64("isActive", *isActive ? 1 : 0);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            plans.push_back(mapRowToPlan(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка планов: " << e.what();
        throw;
    }

    return { plans, totalCount };
}

std::optional<dto::Plan> SqlitePlanRepository::findById(int64_t id)
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
            "SELECT id, phaseId, basePlanId, caption, description, "
            "isActive, createdAt, createdByUserId, activatedAt, activatedByUserId "
            "FROM Plan WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToPlan(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска плана по id: " << e.what();
        throw;
    }
}

std::optional<dto::Plan> SqlitePlanRepository::findActiveByPhaseId(int64_t phaseId)
{
    if (phaseId <= 0)
    {
        LOG_WARN << "findActiveByPhaseId: неверный phaseId " << phaseId;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, phaseId, basePlanId, caption, description, "
            "isActive, createdAt, createdByUserId, activatedAt, activatedByUserId "
            "FROM Plan WHERE phaseId = :phaseId AND isActive = 1 LIMIT 1"
        );

        stmt->bindInt64("phaseId", phaseId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToPlan(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска активного плана: " << e.what();
        throw;
    }
}

int64_t SqlitePlanRepository::create(const dto::Plan& plan)
{
    // Проверка обязательных полей
    if (!plan.phaseId.has_value())
    {
        LOG_WARN << "createPlan: отсутствует phaseId";
        return 0;
    }
    if (!plan.caption.has_value() || plan.caption->empty())
    {
        LOG_WARN << "createPlan: отсутствует caption";
        return 0;
    }
    if (!plan.createdByUserId.has_value())
    {
        LOG_WARN << "createPlan: отсутствует createdByUserId";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO Plan (phaseId, basePlanId, caption, description, "
            "isActive, createdAt, createdByUserId, activatedAt, activatedByUserId) "
            "VALUES (:phaseId, :basePlanId, :caption, :description, "
            ":isActive, :createdAt, :createdByUserId, :activatedAt, :activatedByUserId)"
        );

        stmt->bindInt64("phaseId", *plan.phaseId);

        if (plan.basePlanId.has_value())
            stmt->bindInt64("basePlanId", *plan.basePlanId);
        else
            stmt->bindNull("basePlanId");

        stmt->bindString("caption", *plan.caption);

        if (plan.description.has_value())
            stmt->bindString("description", *plan.description);
        else
            stmt->bindNull("description");

        stmt->bindInt64("isActive", plan.isActive.value_or(false) ? 1 : 0);

        int64_t nowSeconds = common::timePointToSeconds(std::chrono::system_clock::now());
        stmt->bindInt64("createdAt", nowSeconds);
        stmt->bindInt64("createdByUserId", *plan.createdByUserId);

        if (plan.activatedAt.has_value())
            stmt->bindInt64("activatedAt", common::timePointToSeconds(*plan.activatedAt));
        else
            stmt->bindNull("activatedAt");

        if (plan.activatedByUserId.has_value())
            stmt->bindInt64("activatedByUserId", *plan.activatedByUserId);
        else
            stmt->bindNull("activatedByUserId");

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания плана: " << e.what();
        throw;
    }
}

bool SqlitePlanRepository::update(const dto::Plan& plan)
{
    if (!plan.id.has_value())
    {
        LOG_WARN << "updatePlan: отсутствует ID";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Plan SET ";

        if (plan.phaseId.has_value())
            setClauses.push_back("phaseId = :phaseId");
        if (plan.basePlanId.has_value())
            setClauses.push_back("basePlanId = :basePlanId");
        if (plan.caption.has_value())
            setClauses.push_back("caption = :caption");
        if (plan.description.has_value())
            setClauses.push_back("description = :description");
        if (plan.isActive.has_value())
            setClauses.push_back("isActive = :isActive");
        if (plan.activatedAt.has_value())
            setClauses.push_back("activatedAt = :activatedAt");
        if (plan.activatedByUserId.has_value())
            setClauses.push_back("activatedByUserId = :activatedByUserId");

        if (setClauses.empty())
        {
            LOG_WARN << "updatePlan: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (plan.phaseId.has_value())
            stmt->bindInt64("phaseId", *plan.phaseId);
        if (plan.basePlanId.has_value())
            stmt->bindInt64("basePlanId", *plan.basePlanId);
        if (plan.caption.has_value())
            stmt->bindString("caption", *plan.caption);
        if (plan.description.has_value())
            stmt->bindString("description", *plan.description);
        if (plan.isActive.has_value())
            stmt->bindInt64("isActive", *plan.isActive ? 1 : 0);
        if (plan.activatedAt.has_value())
            stmt->bindInt64("activatedAt", common::timePointToSeconds(*plan.activatedAt));
        if (plan.activatedByUserId.has_value())
            stmt->bindInt64("activatedByUserId", *plan.activatedByUserId);

        stmt->bindInt64("id", *plan.id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления плана: " << e.what();
        return false;
    }
}

bool SqlitePlanRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "removePlan: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Plan WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления плана: " << e.what();
        return false;
    }
}

bool SqlitePlanRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Plan WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования плана: " << e.what();
        return false;
    }
}

int64_t SqlitePlanRepository::deactivateAllByPhaseId(int64_t phaseId)
{
    if (phaseId <= 0)
    {
        LOG_WARN << "deactivateAllByPhaseId: неверный phaseId " << phaseId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "UPDATE Plan SET isActive = 0 WHERE phaseId = :phaseId AND isActive = 1"
        );

        stmt->bindInt64("phaseId", phaseId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка деактивации планов: " << e.what();
        return 0;
    }
}

dto::Plan SqlitePlanRepository::mapRowToPlan(db::IResultSet& rs) const
{
    dto::Plan plan;
    plan.id = rs.valueInt64("id");
    plan.phaseId = rs.valueInt64("phaseId");

    if (!rs.isNull("basePlanId"))
        plan.basePlanId = rs.valueInt64("basePlanId");

    plan.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        plan.description = rs.valueString("description");

    plan.isActive = rs.valueInt64("isActive") != 0;

    if (!rs.isNull("createdAt"))
    {
        int64_t timestamp = rs.valueInt64("createdAt");
        plan.createdAt = common::secondsToTimePoint(timestamp);
    }

    plan.createdByUserId = rs.valueInt64("createdByUserId");

    if (!rs.isNull("activatedAt"))
    {
        int64_t timestamp = rs.valueInt64("activatedAt");
        plan.activatedAt = common::secondsToTimePoint(timestamp);
    }

    if (!rs.isNull("activatedByUserId"))
        plan.activatedByUserId = rs.valueInt64("activatedByUserId");

    return plan;
}

} // namespace repositories
} // namespace server
