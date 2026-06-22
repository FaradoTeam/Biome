#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_board_column_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект BoardColumn.
 */
dto::BoardColumn mapRowToBoardColumn(db::IResultSet& rs)
{
    dto::BoardColumn column;
    column.id = rs.valueInt64("id");
    column.boardId = rs.valueInt64("boardId");
    column.stateId = rs.valueInt64("stateId");
    column.orderNumber = rs.valueInt64("orderNumber");

    if (!rs.isNull("settings"))
        column.settings = rs.valueString("settings");

    return column;
}

} // namespace

namespace server
{
namespace repositories
{

SqliteBoardColumnRepository::SqliteBoardColumnRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteBoardColumnRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteBoardColumnRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::BoardColumn>, int64_t>
SqliteBoardColumnRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> boardId,
    std::optional<int64_t> stateId
)
{
    std::vector<dto::BoardColumn> columns;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (boardId.has_value())
            whereClauses.push_back("boardId = :boardId");
        if (stateId.has_value())
            whereClauses.push_back("stateId = :stateId");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM BoardColumn" + whereClause
        );

        if (boardId.has_value())
            countStmt->bindInt64("boardId", *boardId);
        if (stateId.has_value())
            countStmt->bindInt64("stateId", *stateId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { columns, totalCount };
        }

        // Получаем страницу с колонками
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, boardId, stateId, orderNumber, settings "
            "FROM BoardColumn"
            + whereClause + " ORDER BY boardId, orderNumber LIMIT :limit OFFSET :offset"
        );

        if (boardId.has_value())
            stmt->bindInt64("boardId", *boardId);
        if (stateId.has_value())
            stmt->bindInt64("stateId", *stateId);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            columns.push_back(mapRowToBoardColumn(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка колонок досок: " << e.what();
        throw;
    }

    return { columns, totalCount };
}

std::optional<dto::BoardColumn> SqliteBoardColumnRepository::findById(int64_t id)
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
            "SELECT id, boardId, stateId, orderNumber, settings "
            "FROM BoardColumn WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToBoardColumn(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска колонки доски по id: " << e.what();
        throw;
    }
}

int64_t SqliteBoardColumnRepository::create(const dto::BoardColumn& column)
{
    // Проверка обязательных полей
    if (!column.boardId.has_value())
    {
        LOG_WARN << "Создание колонки: отсутствует boardId";
        return 0;
    }

    if (!column.stateId.has_value())
    {
        LOG_WARN << "Создание колонки: отсутствует stateId";
        return 0;
    }

    if (!column.orderNumber.has_value())
    {
        LOG_WARN << "Создание колонки: отсутствует orderNumber";
        return 0;
    }

    // Проверка уникальности (boardId, stateId)
    if (existsByBoardAndState(*column.boardId, *column.stateId))
    {
        LOG_WARN
            << "Создание колонки: состояние " << *column.stateId
            << " уже используется на доске " << *column.boardId;
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO BoardColumn (boardId, stateId, orderNumber, settings) "
            "VALUES (:boardId, :stateId, :orderNumber, :settings)"
        );

        stmt->bindInt64("boardId", *column.boardId);
        stmt->bindInt64("stateId", *column.stateId);
        stmt->bindInt64("orderNumber", *column.orderNumber);

        if (column.settings.has_value())
            stmt->bindString("settings", *column.settings);
        else
            stmt->bindNull("settings");

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания колонки доски: " << e.what();
        throw;
    }
}

bool SqliteBoardColumnRepository::update(const dto::BoardColumn& column)
{
    if (!column.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID колонки";
        return false;
    }

    try
    {
        auto conn = connection();

        // Если меняется boardId или stateId, проверяем уникальность
        auto existing = findById(*column.id);
        if (existing)
        {
            int64_t newBoardId = column.boardId.has_value()
                ? *column.boardId
                : *existing->boardId;
            int64_t newStateId = column.stateId.has_value()
                ? *column.stateId
                : *existing->stateId;

            if ((column.boardId.has_value() || column.stateId.has_value()) && existsByBoardAndState(newBoardId, newStateId) && (newBoardId != *existing->boardId || newStateId != *existing->stateId))
            {
                LOG_WARN
                    << "update: состояние " << newStateId
                    << " уже используется на доске " << newBoardId;
                return false;
            }
        }

        std::vector<std::string> setClauses;
        std::string sql = "UPDATE BoardColumn SET ";

        if (column.boardId.has_value())
            setClauses.push_back("boardId = :boardId");
        if (column.stateId.has_value())
            setClauses.push_back("stateId = :stateId");
        if (column.orderNumber.has_value())
            setClauses.push_back("orderNumber = :orderNumber");
        if (column.settings.has_value())
            setClauses.push_back("settings = :settings");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (column.boardId.has_value())
            stmt->bindInt64("boardId", *column.boardId);
        if (column.stateId.has_value())
            stmt->bindInt64("stateId", *column.stateId);
        if (column.orderNumber.has_value())
            stmt->bindInt64("orderNumber", *column.orderNumber);
        if (column.settings.has_value())
            stmt->bindString("settings", *column.settings);

        stmt->bindInt64("id", *column.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления колонки доски: " << e.what();
        return false;
    }
}

bool SqliteBoardColumnRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM BoardColumn WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления колонки доски: " << e.what();
        return false;
    }
}

int64_t SqliteBoardColumnRepository::removeByBoardId(int64_t boardId)
{
    if (boardId <= 0)
    {
        LOG_WARN << "removeByBoardId: неверный boardId " << boardId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM BoardColumn WHERE boardId = :boardId"
        );

        stmt->bindInt64("boardId", boardId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления колонок доски: " << e.what();
        return 0;
    }
}

bool SqliteBoardColumnRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM BoardColumn WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования колонки доски: " << e.what();
        return false;
    }
}

bool SqliteBoardColumnRepository::existsByBoardAndState(
    int64_t boardId,
    int64_t stateId
)
{
    if (boardId <= 0 || stateId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM BoardColumn WHERE boardId = :boardId AND stateId = :stateId LIMIT 1"
        );

        stmt->bindInt64("boardId", boardId);
        stmt->bindInt64("stateId", stateId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования колонки доски: " << e.what();
        return false;
    }
}

std::vector<dto::BoardColumn> SqliteBoardColumnRepository::findByBoardId(
    int64_t boardId,
    bool orderByOrderNumber
)
{
    std::vector<dto::BoardColumn> columns;

    if (boardId <= 0)
    {
        LOG_WARN << "findByBoardId: неверный boardId " << boardId;
        return columns;
    }

    try
    {
        auto conn = connection();

        std::string sql = "SELECT id, boardId, stateId, orderNumber, settings "
                          "FROM BoardColumn WHERE boardId = :boardId";

        if (orderByOrderNumber)
        {
            sql += " ORDER BY orderNumber";
        }
        else
        {
            sql += " ORDER BY id";
        }

        auto stmt = conn->prepareStatement(sql);
        stmt->bindInt64("boardId", boardId);

        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            columns.push_back(mapRowToBoardColumn(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения колонок доски по boardId: " << e.what();
        throw;
    }

    return columns;
}

std::vector<dto::BoardColumn> SqliteBoardColumnRepository::findByStateId(
    int64_t stateId
)
{
    std::vector<dto::BoardColumn> columns;

    if (stateId <= 0)
    {
        LOG_WARN << "findByStateId: неверный stateId " << stateId;
        return columns;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, boardId, stateId, orderNumber, settings "
            "FROM BoardColumn WHERE stateId = :stateId ORDER BY boardId"
        );

        stmt->bindInt64("stateId", stateId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            columns.push_back(mapRowToBoardColumn(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения колонок доски по stateId: " << e.what();
        throw;
    }

    return columns;
}

dto::BoardColumn SqliteBoardColumnRepository::mapRowToBoardColumn(db::IResultSet& rs) const
{
    dto::BoardColumn column;
    column.id = rs.valueInt64("id");
    column.boardId = rs.valueInt64("boardId");
    column.stateId = rs.valueInt64("stateId");
    column.orderNumber = rs.valueInt64("orderNumber");

    if (!rs.isNull("settings"))
        column.settings = rs.valueString("settings");

    return column;
}

} // namespace repositories
} // namespace server
