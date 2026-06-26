#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_item_document_repository.h"

namespace server
{
namespace repositories
{

SqliteItemDocumentRepository::SqliteItemDocumentRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteItemDocumentRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteItemDocumentRepository::connection() const
{
    return m_database->connection();
}

dto::ItemDocument SqliteItemDocumentRepository::mapRowToItemDocument(
    db::IResultSet& rs
) const
{
    dto::ItemDocument itemDocument;
    itemDocument.id = rs.valueInt64("id");
    itemDocument.itemId = rs.valueInt64("itemId");
    itemDocument.documentId = rs.valueInt64("documentId");
    return itemDocument;
}

std::pair<std::vector<dto::ItemDocument>, int64_t>
SqliteItemDocumentRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> itemId,
    std::optional<int64_t> documentId
)
{
    std::vector<dto::ItemDocument> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (itemId.has_value())
            whereClauses.push_back("itemId = :itemId");
        if (documentId.has_value())
            whereClauses.push_back("documentId = :documentId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM ItemDocument" + whereStr
        );

        if (itemId.has_value())
            countStmt->bindInt64("itemId", *itemId);
        if (documentId.has_value())
            countStmt->bindInt64("documentId", *documentId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { items, totalCount };
        }

        // Получаем страницу
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, documentId FROM ItemDocument"
            + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset"
        );

        if (itemId.has_value())
            stmt->bindInt64("itemId", *itemId);
        if (documentId.has_value())
            stmt->bindInt64("documentId", *documentId);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToItemDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка ItemDocument: " << e.what();
        throw;
    }

    return { items, totalCount };
}

std::optional<dto::ItemDocument> SqliteItemDocumentRepository::findById(int64_t id)
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
            "SELECT id, itemId, documentId FROM ItemDocument WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemDocument(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска ItemDocument по id: " << e.what();
        throw;
    }
}

std::vector<dto::ItemDocument> SqliteItemDocumentRepository::findByItemId(
    int64_t itemId
)
{
    std::vector<dto::ItemDocument> items;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return items;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, documentId FROM ItemDocument "
            "WHERE itemId = :itemId ORDER BY documentId"
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToItemDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения ItemDocument по itemId: " << e.what();
        throw;
    }

    return items;
}

std::vector<dto::ItemDocument> SqliteItemDocumentRepository::findByDocumentId(
    int64_t documentId
)
{
    std::vector<dto::ItemDocument> items;

    if (documentId <= 0)
    {
        LOG_WARN << "findByDocumentId: неверный documentId " << documentId;
        return items;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, documentId FROM ItemDocument "
            "WHERE documentId = :documentId ORDER BY itemId"
        );

        stmt->bindInt64("documentId", documentId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToItemDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения ItemDocument по documentId: " << e.what();
        throw;
    }

    return items;
}

std::optional<dto::ItemDocument> SqliteItemDocumentRepository::findByItemAndDocument(
    int64_t itemId,
    int64_t documentId
)
{
    if (itemId <= 0 || documentId <= 0)
    {
        LOG_WARN << "findByItemAndDocument: неверные параметры";
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, documentId FROM ItemDocument "
            "WHERE itemId = :itemId AND documentId = :documentId LIMIT 1"
        );

        stmt->bindInt64("itemId", itemId);
        stmt->bindInt64("documentId", documentId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemDocument(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска ItemDocument по itemId и documentId: " << e.what();
        throw;
    }
}

bool SqliteItemDocumentRepository::exists(int64_t itemId, int64_t documentId)
{
    if (itemId <= 0 || documentId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemDocument "
            "WHERE itemId = :itemId AND documentId = :documentId LIMIT 1"
        );

        stmt->bindInt64("itemId", itemId);
        stmt->bindInt64("documentId", documentId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования ItemDocument: " << e.what();
        return false;
    }
}

int64_t SqliteItemDocumentRepository::create(const dto::ItemDocument& itemDocument)
{
    if (!itemDocument.itemId.has_value() || !itemDocument.documentId.has_value())
    {
        LOG_WARN << "create: отсутствуют обязательные поля";
        return 0;
    }

    // Проверяем уникальность пары
    if (exists(*itemDocument.itemId, *itemDocument.documentId))
    {
        LOG_WARN
            << "create: связь itemId=" << *itemDocument.itemId
            << ", documentId=" << *itemDocument.documentId << " уже существует";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO ItemDocument (itemId, documentId) "
            "VALUES (:itemId, :documentId)"
        );

        stmt->bindInt64("itemId", *itemDocument.itemId);
        stmt->bindInt64("documentId", *itemDocument.documentId);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания ItemDocument: " << e.what();
        throw;
    }
}

bool SqliteItemDocumentRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM ItemDocument WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemDocument: " << e.what();
        return false;
    }
}

int64_t SqliteItemDocumentRepository::removeByItemId(int64_t itemId)
{
    if (itemId <= 0)
    {
        LOG_WARN << "removeByItemId: неверный itemId " << itemId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM ItemDocument WHERE itemId = :itemId"
        );

        stmt->bindInt64("itemId", itemId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemDocument по itemId: " << e.what();
        return 0;
    }
}

int64_t SqliteItemDocumentRepository::removeByDocumentId(int64_t documentId)
{
    if (documentId <= 0)
    {
        LOG_WARN << "removeByDocumentId: неверный documentId " << documentId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM ItemDocument WHERE documentId = :documentId"
        );

        stmt->bindInt64("documentId", documentId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemDocument по documentId: " << e.what();
        return 0;
    }
}

} // namespace repositories
} // namespace server
