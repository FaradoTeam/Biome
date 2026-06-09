#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/types.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_phase_repository.h"

namespace server
{
namespace repositories
{

SqlitePhaseRepository::SqlitePhaseRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqlitePhaseRepository: database is null");
    }
}

std::pair<std::vector<dto::Phase>, int64_t> SqlitePhaseRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> projectId,
    std::optional<bool> isArchive
)
{
    std::vector<dto::Phase> phases;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Строим динамический SQL с условиями фильтрации
        std::string whereClause = "WHERE 1=1";

        // Фильтр по проекту
        if (projectId.has_value())
        {
            whereClause += " AND projectId = :projectId";
        }

        // Фильтр по статусу архивации
        if (isArchive.has_value())
        {
            whereClause += " AND isArchive = :isArchive";
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Phase " + whereClause
        );

        if (projectId.has_value())
            countStmt->bindInt64("projectId", projectId.value());

        if (isArchive.has_value())
            countStmt->bindInt64("isArchive", isArchive.value() ? 1 : 0);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { phases, totalCount };
        }

        // 2. Получаем страницу с фазами
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, projectId, caption, description, "
                                "beginDate, endDate, isArchive "
                                "FROM Phase "
            + whereClause + " ORDER BY beginDate, id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);

        if (projectId.has_value())
            stmt->bindInt64("projectId", projectId.value());

        if (isArchive.has_value())
            stmt->bindInt64("isArchive", isArchive.value() ? 1 : 0);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            phases.push_back(mapRowToPhase(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка фаз: " << e.what();
        throw;
    }

    return { phases, totalCount };
}

std::optional<dto::Phase> SqlitePhaseRepository::findById(int64_t id)
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
            "SELECT id, projectId, caption, description, "
            "beginDate, endDate, isArchive "
            "FROM Phase WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToPhase(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска фазы по id: " << e.what();
        throw;
    }
}

int64_t SqlitePhaseRepository::create(const dto::Phase& phase)
{
    if (!phase.caption.has_value()
        || phase.caption->empty()
        || !phase.projectId.has_value())
    {
        LOG_WARN << "Создание фазы: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption, description, "
            "beginDate, endDate, isArchive, searchCaption, searchDescription) "
            "VALUES (:projectId, :caption, :description, "
            ":beginDate, :endDate, :isArchive, :searchCaption, :searchDescription)"
        );

        // Поле projectId
        stmt->bindInt64("projectId", phase.projectId.value());

        // Поле caption
        stmt->bindString("caption", phase.caption.value());

        // Поле description
        if (phase.description.has_value())
            stmt->bindString("description", phase.description.value());
        else
            stmt->bindNull("description");

        // Поле beginDate
        if (phase.beginDate.has_value())
            stmt->bindInt64("beginDate", common::timePointToSeconds(phase.beginDate.value()));
        else
            stmt->bindNull("beginDate");

        // Поле endDate
        if (phase.endDate.has_value())
            stmt->bindInt64("endDate", common::timePointToSeconds(phase.endDate.value()));
        else
            stmt->bindNull("endDate");

        // Поле isArchive
        stmt->bindInt64("isArchive", phase.isArchive.value_or(false) ? 1 : 0);

        // Поле searchCaption
        stmt->bindString("searchCaption", common::toLowerCase(phase.caption.value()));

        // Поле searchDescription
        if (phase.description.has_value())
        {
            stmt->bindString(
                "searchDescription",
                common::toLowerCase(phase.description.value())
            );
        }
        else
        {
            stmt->bindNull("searchDescription");
        }

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания фазы: " << e.what();
        throw;
    }
}

bool SqlitePhaseRepository::update(const dto::Phase& phase)
{
    if (!phase.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID фазы";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Phase SET ";

        // Обновляем только переданные поля
        if (phase.projectId.has_value())
            setClauses.push_back("projectId = :projectId");

        if (phase.caption.has_value())
        {
            setClauses.push_back("caption = :caption");
            setClauses.push_back("searchCaption = :searchCaption");
        }

        if (phase.description.has_value())
        {
            setClauses.push_back("description = :description");
            setClauses.push_back("searchDescription = :searchDescription");
        }

        if (phase.beginDate.has_value())
            setClauses.push_back("beginDate = :beginDate");

        if (phase.endDate.has_value())
            setClauses.push_back("endDate = :endDate");

        if (phase.isArchive.has_value())
            setClauses.push_back("isArchive = :isArchive");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        // Биндим параметры
        if (phase.projectId.has_value())
            stmt->bindInt64("projectId", phase.projectId.value());

        if (phase.caption.has_value())
        {
            stmt->bindString("caption", phase.caption.value());
            stmt->bindString(
                "searchCaption",
                common::toLowerCase(phase.caption.value())
            );
        }

        if (phase.description.has_value())
        {
            stmt->bindString("description", phase.description.value());
            stmt->bindString(
                "searchDescription",
                common::toLowerCase(phase.description.value())
            );
        }

        if (phase.beginDate.has_value())
            stmt->bindInt64("beginDate", common::timePointToSeconds(phase.beginDate.value()));

        if (phase.endDate.has_value())
            stmt->bindInt64("endDate", common::timePointToSeconds(phase.endDate.value()));

        if (phase.isArchive.has_value())
            stmt->bindInt64("isArchive", phase.isArchive.value() ? 1 : 0);

        stmt->bindInt64("id", phase.id.value());

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления фазы: " << e.what();
        return false;
    }
}

bool SqlitePhaseRepository::archive(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "archive: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "UPDATE Phase SET isArchive = 1 WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка архивации фазы: " << e.what();
        return false;
    }
}

bool SqlitePhaseRepository::restore(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "restore: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "UPDATE Phase SET isArchive = 0 WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка восстановления фазы: " << e.what();
        return false;
    }
}

bool SqlitePhaseRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Phase WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования фазы: " << e.what();
        return false;
    }
}

std::vector<dto::Phase> SqlitePhaseRepository::findByProject(
    int64_t projectId,
    bool includeArchived
)
{
    std::vector<dto::Phase> phases;

    if (projectId <= 0)
    {
        LOG_WARN
            << "findByProject: неверный идентификатор проекта " << projectId;
        return phases;
    }

    try
    {
        auto conn = connection();
        std::string sql = "SELECT id, projectId, caption, description, "
                          "beginDate, endDate, isArchive "
                          "FROM Phase WHERE projectId = :projectId";

        if (!includeArchived)
        {
            sql += " AND isArchive = 0";
        }

        sql += " ORDER BY beginDate, id";

        auto stmt = conn->prepareStatement(sql);
        stmt->bindInt64("projectId", projectId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            phases.push_back(mapRowToPhase(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения фаз проекта: " << e.what();
    }

    return phases;
}

std::shared_ptr<db::IConnection> SqlitePhaseRepository::connection() const
{
    return m_database->connection();
}

dto::Phase SqlitePhaseRepository::mapRowToPhase(db::IResultSet& rs) const
{
    dto::Phase phase;

    phase.id = rs.valueInt64("id");
    phase.projectId = rs.valueInt64("projectId");
    phase.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        phase.description = rs.valueString("description");

    if (!rs.isNull("beginDate"))
    {
        const int64_t timestamp = rs.valueInt64("beginDate");
        phase.beginDate = common::secondsToTimePoint(timestamp);
    }

    if (!rs.isNull("endDate"))
    {
        const int64_t timestamp = rs.valueInt64("endDate");
        phase.endDate = common::secondsToTimePoint(timestamp);
    }

    phase.isArchive = rs.valueInt64("isArchive") != 0;

    return phase;
}

} // namespace repositories
} // namespace server
