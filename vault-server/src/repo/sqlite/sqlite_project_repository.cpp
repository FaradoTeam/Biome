#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/types.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_project_repository.h"

namespace server
{
namespace repositories
{

SqliteProjectRepository::SqliteProjectRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteProjectRepository: database is null");
    }
}

std::pair<std::vector<dto::Project>, int64_t> SqliteProjectRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> parentId,
    std::optional<bool> isArchive,
    const std::string& searchCaption
)
{
    std::vector<dto::Project> projects;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Строим динамический SQL с условиями фильтрации
        std::string whereClause = "WHERE 1=1";
        std::string countSql = "SELECT COUNT(*) FROM Project";
        std::string selectSql = "SELECT id, parentId, caption, description, "
                                "createdAt, updatedAt, isArchive "
                                "FROM Project";

        // Фильтр по родительскому проекту
        if (parentId.has_value())
        {
            if (parentId.value() == 0)
                whereClause += " AND parentId IS NULL";
            else
                whereClause += " AND parentId = :parentId";
        }

        // Фильтр по статусу архивации
        if (isArchive.has_value())
        {
            whereClause += " AND isArchive = :isArchive";
        }

        // Поиск по названию
        if (!searchCaption.empty())
        {
            whereClause += " AND searchCaption LIKE '%' || :search || '%'";
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(countSql + " " + whereClause);

        if (parentId.has_value() && parentId.value() != 0)
            countStmt->bindInt64("parentId", parentId.value());

        if (isArchive.has_value())
            countStmt->bindInt64("isArchive", isArchive.value() ? 1 : 0);

        if (!searchCaption.empty())
            countStmt->bindString("search", common::toLowerCase(searchCaption));

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { projects, totalCount };
        }

        // 2. Получаем страницу с проектами
        const int offset = (page - 1) * pageSize;
        selectSql += " " + whereClause + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);

        if (parentId.has_value() && parentId.value() != 0)
            stmt->bindInt64("parentId", parentId.value());

        if (isArchive.has_value())
            stmt->bindInt64("isArchive", isArchive.value() ? 1 : 0);

        if (!searchCaption.empty())
            stmt->bindString("search", common::toLowerCase(searchCaption));

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            projects.push_back(mapRowToProject(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка проектов: " << e.what();
        throw;
    }

    return { projects, totalCount };
}

std::optional<dto::Project> SqliteProjectRepository::findById(int64_t id)
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
            "SELECT id, parentId, caption, description, "
            "createdAt, updatedAt, isArchive "
            "FROM Project WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToProject(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска проекта по id: " << e.what();
        throw;
    }
}

int64_t SqliteProjectRepository::create(const dto::Project& project)
{
    if (!project.caption.has_value() || project.caption->empty())
    {
        LOG_WARN << "Создание проекта: отсутствует название";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto now = std::chrono::system_clock::now();
        auto nowSeconds = common::timePointToSeconds(now);

        auto stmt = conn->prepareStatement(
            "INSERT INTO Project (parentId, caption, description, "
            "searchCaption, searchDescription, createdAt, updatedAt, isArchive) "
            "VALUES (:parentId, :caption, :description, "
            ":searchCaption, :searchDescription, :createdAt, :updatedAt, :isArchive)"
        );

        // parentId (может быть NULL)
        if (project.parentId.has_value())
            stmt->bindInt64("parentId", project.parentId.value());
        else
            stmt->bindNull("parentId");

        // caption - сохраняем оригинал
        stmt->bindString("caption", project.caption.value());

        // description - сохраняем оригинал
        if (project.description.has_value())
            stmt->bindString("description", project.description.value());
        else
            stmt->bindNull("description");

        stmt->bindString(
            "searchCaption",
            common::toLowerCase(project.caption.value())
        );

        if (project.description.has_value())
        {
            stmt->bindString(
                "searchDescription",
                common::toLowerCase(project.description.value())
            );
        }
        else
        {
            stmt->bindNull("searchDescription");
        }

        // Временные метки
        stmt->bindInt64("createdAt", nowSeconds);
        stmt->bindInt64("updatedAt", nowSeconds);

        // isArchive
        stmt->bindInt64("isArchive", project.isArchive.value_or(false) ? 1 : 0);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания проекта: " << e.what();
        throw;
    }
}

bool SqliteProjectRepository::update(const dto::Project& project)
{
    if (!project.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID проекта";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Project SET ";

        // Обновляем только переданные поля
        if (project.parentId.has_value())
            setClauses.push_back("parentId = :parentId");

        if (project.caption.has_value())
        {
            setClauses.push_back("caption = :caption");
            setClauses.push_back("searchCaption = :searchCaption");
        }

        if (project.description.has_value())
        {
            setClauses.push_back("description = :description");
            setClauses.push_back("searchDescription = :searchDescription");
        }

        if (project.isArchive.has_value())
            setClauses.push_back("isArchive = :isArchive");

        // Всегда обновляем updatedAt
        setClauses.push_back("updatedAt = :updatedAt");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        // Биндим параметры
        if (project.parentId.has_value())
            stmt->bindInt64("parentId", project.parentId.value());

        if (project.caption.has_value())
        {
            stmt->bindString("caption", project.caption.value());
            stmt->bindString(
                "searchCaption",
                common::toLowerCase(project.caption.value())
            );
        }

        if (project.description.has_value())
        {
            stmt->bindString("description", project.description.value());
            stmt->bindString(
                "searchDescription",
                common::toLowerCase(project.description.value())
            );
        }

        if (project.isArchive.has_value())
            stmt->bindInt64("isArchive", project.isArchive.value() ? 1 : 0);

        auto nowSeconds = common::timePointToSeconds(
            std::chrono::system_clock::now()
        );
        stmt->bindInt64("updatedAt", nowSeconds);
        stmt->bindInt64("id", project.id.value());

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления проекта: " << e.what();
        return false;
    }
}

bool SqliteProjectRepository::archive(int64_t id)
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
            "UPDATE Project SET isArchive = 1, updatedAt = :updatedAt "
            "WHERE id = :id"
        );

        stmt->bindInt64("updatedAt", common::timePointToSeconds(std::chrono::system_clock::now()));
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка архивации проекта: " << e.what();
        return false;
    }
}

bool SqliteProjectRepository::restore(int64_t id)
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
            "UPDATE Project SET isArchive = 0, updatedAt = :updatedAt "
            "WHERE id = :id"
        );

        stmt->bindInt64("updatedAt", common::timePointToSeconds(std::chrono::system_clock::now()));
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка восстановления проекта: " << e.what();
        return false;
    }
}

bool SqliteProjectRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Project WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования проекта: " << e.what();
        return false;
    }
}

int64_t SqliteProjectRepository::childCount(int64_t parentId)
{
    if (parentId <= 0)
        return 0;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Project WHERE parentId = :parentId AND isArchive = 0"
        );

        stmt->bindInt64("parentId", parentId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return rs->valueInt64(0);
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка подсчета дочерних проектов: " << e.what();
        return 0;
    }
}

std::shared_ptr<db::IConnection> SqliteProjectRepository::connection() const
{
    return m_database->connection();
}

dto::Project SqliteProjectRepository::mapRowToProject(db::IResultSet& rs) const
{
    dto::Project project;

    project.id = rs.valueInt64("id");

    if (!rs.isNull("parentId"))
        project.parentId = rs.valueInt64("parentId");

    project.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        project.description = rs.valueString("description");

    if (!rs.isNull("createdAt"))
    {
        const int64_t timestamp = rs.valueInt64("createdAt");
        project.createdAt = common::secondsToTimePoint(timestamp);
    }

    if (!rs.isNull("updatedAt"))
    {
        const int64_t timestamp = rs.valueInt64("updatedAt");
        project.updatedAt = common::secondsToTimePoint(timestamp);
    }

    project.isArchive = rs.valueInt64("isArchive") != 0;

    return project;
}

} // namespace repositories
} // namespace server
