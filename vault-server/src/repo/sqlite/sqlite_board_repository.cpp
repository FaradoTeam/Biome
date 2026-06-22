#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_board_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект Board.
 */
dto::Board mapRowToBoard(db::IResultSet& rs)
{
    dto::Board board;
    board.id = rs.valueInt64("id");
    board.workflowId = rs.valueInt64("workflowId");
    board.projectId = rs.valueInt64("projectId");

    if (!rs.isNull("phaseId"))
        board.phaseId = rs.valueInt64("phaseId");

    board.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        board.description = rs.valueString("description");

    return board;
}

} // namespace

namespace server
{
namespace repositories
{

SqliteBoardRepository::SqliteBoardRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteBoardRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteBoardRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::Board>, int64_t>
SqliteBoardRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> projectId,
    std::optional<int64_t> phaseId,
    std::optional<int64_t> workflowId
)
{
    std::vector<dto::Board> boards;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (projectId.has_value())
            whereClauses.push_back("projectId = :projectId");
        if (phaseId.has_value())
            whereClauses.push_back("phaseId = :phaseId");
        if (workflowId.has_value())
            whereClauses.push_back("workflowId = :workflowId");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Board" + whereClause
        );

        if (projectId.has_value())
            countStmt->bindInt64("projectId", *projectId);
        if (phaseId.has_value())
            countStmt->bindInt64("phaseId", *phaseId);
        if (workflowId.has_value())
            countStmt->bindInt64("workflowId", *workflowId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { boards, totalCount };
        }

        // Получаем страницу с досками
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, workflowId, projectId, phaseId, caption, description "
            "FROM Board"
            + whereClause + " ORDER BY id LIMIT :limit OFFSET :offset"
        );

        if (projectId.has_value())
            stmt->bindInt64("projectId", *projectId);
        if (phaseId.has_value())
            stmt->bindInt64("phaseId", *phaseId);
        if (workflowId.has_value())
            stmt->bindInt64("workflowId", *workflowId);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            boards.push_back(mapRowToBoard(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка досок: " << e.what();
        throw;
    }

    return { boards, totalCount };
}

std::optional<dto::Board> SqliteBoardRepository::findById(int64_t id)
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
            "SELECT id, workflowId, projectId, phaseId, caption, description "
            "FROM Board WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToBoard(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска доски по id: " << e.what();
        throw;
    }
}

int64_t SqliteBoardRepository::create(const dto::Board& board)
{
    // Проверка обязательных полей
    if (!board.caption.has_value() || board.caption->empty())
    {
        LOG_WARN << "Создание доски: отсутствует caption";
        return 0;
    }

    if (!board.workflowId.has_value())
    {
        LOG_WARN << "Создание доски: отсутствует workflowId";
        return 0;
    }

    if (!board.projectId.has_value())
    {
        LOG_WARN << "Создание доски: отсутствует projectId";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO Board (workflowId, projectId, phaseId, caption, description) "
            "VALUES (:workflowId, :projectId, :phaseId, :caption, :description)"
        );

        stmt->bindInt64("workflowId", *board.workflowId);
        stmt->bindInt64("projectId", *board.projectId);

        if (board.phaseId.has_value())
            stmt->bindInt64("phaseId", *board.phaseId);
        else
            stmt->bindNull("phaseId");

        stmt->bindString("caption", *board.caption);

        if (board.description.has_value())
            stmt->bindString("description", *board.description);
        else
            stmt->bindNull("description");

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания доски: " << e.what();
        throw;
    }
}

bool SqliteBoardRepository::update(const dto::Board& board)
{
    if (!board.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID доски";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Board SET ";

        if (board.workflowId.has_value())
            setClauses.push_back("workflowId = :workflowId");
        if (board.projectId.has_value())
            setClauses.push_back("projectId = :projectId");
        if (board.phaseId.has_value())
            setClauses.push_back("phaseId = :phaseId");
        if (board.caption.has_value())
            setClauses.push_back("caption = :caption");
        if (board.description.has_value())
            setClauses.push_back("description = :description");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (board.workflowId.has_value())
            stmt->bindInt64("workflowId", *board.workflowId);
        if (board.projectId.has_value())
            stmt->bindInt64("projectId", *board.projectId);
        if (board.phaseId.has_value())
            stmt->bindInt64("phaseId", *board.phaseId);
        if (board.caption.has_value())
            stmt->bindString("caption", *board.caption);
        if (board.description.has_value())
            stmt->bindString("description", *board.description);

        stmt->bindInt64("id", *board.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления доски: " << e.what();
        return false;
    }
}

bool SqliteBoardRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Board WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления доски: " << e.what();
        return false;
    }
}

bool SqliteBoardRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Board WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования доски: " << e.what();
        return false;
    }
}

std::vector<dto::Board> SqliteBoardRepository::findByProject(int64_t projectId)
{
    std::vector<dto::Board> boards;

    if (projectId <= 0)
    {
        LOG_WARN << "findByProject: неверный projectId " << projectId;
        return boards;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, workflowId, projectId, phaseId, caption, description "
            "FROM Board WHERE projectId = :projectId ORDER BY caption"
        );

        stmt->bindInt64("projectId", projectId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            boards.push_back(mapRowToBoard(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения досок по projectId: " << e.what();
        throw;
    }

    return boards;
}

std::vector<dto::Board> SqliteBoardRepository::findByPhase(int64_t phaseId)
{
    std::vector<dto::Board> boards;

    if (phaseId <= 0)
    {
        LOG_WARN << "findByPhase: неверный phaseId " << phaseId;
        return boards;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, workflowId, projectId, phaseId, caption, description "
            "FROM Board WHERE phaseId = :phaseId ORDER BY caption"
        );

        stmt->bindInt64("phaseId", phaseId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            boards.push_back(mapRowToBoard(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения досок по phaseId: " << e.what();
        throw;
    }

    return boards;
}

dto::Board SqliteBoardRepository::mapRowToBoard(db::IResultSet& rs) const
{
    dto::Board board;
    board.id = rs.valueInt64("id");
    board.workflowId = rs.valueInt64("workflowId");
    board.projectId = rs.valueInt64("projectId");

    if (!rs.isNull("phaseId"))
        board.phaseId = rs.valueInt64("phaseId");

    board.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        board.description = rs.valueString("description");

    return board;
}

} // namespace repositories
} // namespace server
