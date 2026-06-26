#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_document_repository.h"

namespace server
{
namespace repositories
{

SqliteDocumentRepository::SqliteDocumentRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteDocumentRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteDocumentRepository::connection() const
{
    return m_database->connection();
}

dto::Document SqliteDocumentRepository::mapRowToDocument(db::IResultSet& rs) const
{
    dto::Document doc;
    doc.id = rs.valueInt64("id");
    doc.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        doc.description = rs.valueString("description");

    doc.path = rs.valueString("path");
    doc.filename = rs.valueString("filename");
    doc.size = rs.valueInt64("size");

    if (!rs.isNull("mimeType"))
        doc.mimeType = rs.valueString("mimeType");

    if (!rs.isNull("uploadedAt"))
    {
        int64_t timestamp = rs.valueInt64("uploadedAt");
        doc.uploadedAt = common::secondsToTimePoint(timestamp);
    }

    doc.uploadedByUserId = rs.valueInt64("uploadedByUserId");

    return doc;
}

std::pair<std::vector<dto::Document>, int64_t>
SqliteDocumentRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> uploadedByUserId,
    const std::string& searchCaption
)
{
    std::vector<dto::Document> documents;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (uploadedByUserId.has_value())
            whereClauses.push_back("uploadedByUserId = :uploadedByUserId");

        if (!searchCaption.empty())
            whereClauses.push_back("searchCaption LIKE '%' || :searchCaption || '%'");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Document" + whereClause
        );

        if (uploadedByUserId.has_value())
            countStmt->bindInt64("uploadedByUserId", *uploadedByUserId);

        if (!searchCaption.empty())
            countStmt->bindString("searchCaption", common::toLowerCase(searchCaption));

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { documents, totalCount };
        }

        // Получаем страницу с документами
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, caption, description, path, filename, size, mimeType, "
            "uploadedAt, uploadedByUserId "
            "FROM Document"
            + whereClause + " ORDER BY uploadedAt DESC LIMIT :limit OFFSET :offset"
        );

        if (uploadedByUserId.has_value())
            stmt->bindInt64("uploadedByUserId", *uploadedByUserId);

        if (!searchCaption.empty())
            stmt->bindString("searchCaption", common::toLowerCase(searchCaption));

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            documents.push_back(mapRowToDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка документов: " << e.what();
        throw;
    }

    return { documents, totalCount };
}

std::optional<dto::Document> SqliteDocumentRepository::findById(int64_t id)
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
            "SELECT id, caption, description, path, filename, size, mimeType, "
            "uploadedAt, uploadedByUserId "
            "FROM Document WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToDocument(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска документа по id: " << e.what();
        throw;
    }
}

int64_t SqliteDocumentRepository::create(const dto::Document& document)
{
    // Проверка обязательных полей
    if (!document.caption.has_value() || document.caption->empty())
    {
        LOG_WARN << "create: отсутствует caption";
        return 0;
    }

    if (!document.path.has_value() || document.path->empty())
    {
        LOG_WARN << "create: отсутствует path";
        return 0;
    }

    if (!document.filename.has_value() || document.filename->empty())
    {
        LOG_WARN << "create: отсутствует filename";
        return 0;
    }

    if (!document.size.has_value())
    {
        LOG_WARN << "create: отсутствует size";
        return 0;
    }

    if (!document.uploadedByUserId.has_value())
    {
        LOG_WARN << "create: отсутствует uploadedByUserId";
        return 0;
    }

    // Проверяем уникальность пути
    if (!isPathUnique(*document.path))
    {
        LOG_WARN << "create: путь '" << *document.path << "' уже используется";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO Document (caption, description, path, filename, size, "
            "mimeType, uploadedAt, uploadedByUserId, searchCaption, searchDescription) "
            "VALUES (:caption, :description, :path, :filename, :size, "
            ":mimeType, :uploadedAt, :uploadedByUserId, :searchCaption, :searchDescription)"
        );

        stmt->bindString("caption", *document.caption);

        if (document.description.has_value())
            stmt->bindString("description", *document.description);
        else
            stmt->bindNull("description");

        stmt->bindString("path", *document.path);
        stmt->bindString("filename", *document.filename);
        stmt->bindInt64("size", *document.size);

        if (document.mimeType.has_value())
            stmt->bindString("mimeType", *document.mimeType);
        else
            stmt->bindNull("mimeType");

        int64_t nowSeconds = common::timePointToSeconds(std::chrono::system_clock::now());
        stmt->bindInt64("uploadedAt", nowSeconds);
        stmt->bindInt64("uploadedByUserId", *document.uploadedByUserId);

        stmt->bindString("searchCaption", common::toLowerCase(*document.caption));

        if (document.description.has_value())
            stmt->bindString("searchDescription", common::toLowerCase(*document.description));
        else
            stmt->bindNull("searchDescription");

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания документа: " << e.what();
        throw;
    }
}

bool SqliteDocumentRepository::update(const dto::Document& document)
{
    if (!document.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID документа";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Document SET ";

        if (document.caption.has_value())
        {
            setClauses.push_back("caption = :caption");
            setClauses.push_back("searchCaption = :searchCaption");
        }

        if (document.description.has_value())
        {
            setClauses.push_back("description = :description");
            setClauses.push_back("searchDescription = :searchDescription");
        }

        if (document.path.has_value())
        {
            // Проверяем уникальность нового пути
            if (!isPathUnique(*document.path, *document.id))
            {
                LOG_WARN << "update: путь '" << *document.path << "' уже используется";
                return false;
            }
            setClauses.push_back("path = :path");
        }

        if (document.filename.has_value())
            setClauses.push_back("filename = :filename");

        if (document.size.has_value())
            setClauses.push_back("size = :size");

        if (document.mimeType.has_value())
            setClauses.push_back("mimeType = :mimeType");

        if (document.uploadedByUserId.has_value())
            setClauses.push_back("uploadedByUserId = :uploadedByUserId");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (document.caption.has_value())
        {
            stmt->bindString("caption", *document.caption);
            stmt->bindString("searchCaption", common::toLowerCase(*document.caption));
        }

        if (document.description.has_value())
        {
            stmt->bindString("description", *document.description);
            stmt->bindString("searchDescription", common::toLowerCase(*document.description));
        }

        if (document.path.has_value())
            stmt->bindString("path", *document.path);

        if (document.filename.has_value())
            stmt->bindString("filename", *document.filename);

        if (document.size.has_value())
            stmt->bindInt64("size", *document.size);

        if (document.mimeType.has_value())
            stmt->bindString("mimeType", *document.mimeType);

        if (document.uploadedByUserId.has_value())
            stmt->bindInt64("uploadedByUserId", *document.uploadedByUserId);

        stmt->bindInt64("id", *document.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления документа: " << e.what();
        return false;
    }
}

bool SqliteDocumentRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Document WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления документа: " << e.what();
        return false;
    }
}

bool SqliteDocumentRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Document WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования документа: " << e.what();
        return false;
    }
}

bool SqliteDocumentRepository::isPathUnique(
    const std::string& path,
    std::optional<int64_t> excludeId
)
{
    if (path.empty())
        return false;

    try
    {
        auto conn = connection();

        std::string sql = "SELECT 1 FROM Document WHERE path = :path";
        if (excludeId.has_value())
        {
            sql += " AND id != :excludeId";
        }
        sql += " LIMIT 1";

        auto stmt = conn->prepareStatement(sql);
        stmt->bindString("path", path);

        if (excludeId.has_value())
            stmt->bindInt64("excludeId", *excludeId);

        auto rs = stmt->executeQuery();
        return !rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки уникальности пути: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
