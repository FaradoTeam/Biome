#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_item_link_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект ItemLink.
 */
dto::ItemLink mapRowToItemLink(db::IResultSet& rs)
{
    dto::ItemLink itemLink;
    itemLink.id = rs.valueInt64("id");
    itemLink.linkTypeId = rs.valueInt64("linkTypeId");
    itemLink.sourceItemId = rs.valueInt64("sourceItemId");
    itemLink.destinationItemId = rs.valueInt64("destinationItemId");
    return itemLink;
}

} // namespace

namespace server::repositories
{

SqliteItemLinkRepository::SqliteItemLinkRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteItemLinkRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteItemLinkRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::ItemLink>, int64_t>
SqliteItemLinkRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> linkTypeId,
    std::optional<int64_t> sourceItemId,
    std::optional<int64_t> destinationItemId
)
{
    std::vector<dto::ItemLink> links;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем WHERE clause
        std::vector<std::string> whereClauses;
        if (linkTypeId.has_value())
            whereClauses.push_back("linkTypeId = :linkTypeId");
        if (sourceItemId.has_value())
            whereClauses.push_back("sourceItemId = :sourceItemId");
        if (destinationItemId.has_value())
            whereClauses.push_back("destinationItemId = :destinationItemId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // 1. Общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM ItemLink" + whereStr
        );
        if (linkTypeId.has_value())
            countStmt->bindInt64("linkTypeId", *linkTypeId);
        if (sourceItemId.has_value())
            countStmt->bindInt64("sourceItemId", *sourceItemId);
        if (destinationItemId.has_value())
            countStmt->bindInt64("destinationItemId", *destinationItemId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { links, totalCount };
        }

        // 2. Страница
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, linkTypeId, sourceItemId, destinationItemId "
                                "FROM ItemLink"
            + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (linkTypeId.has_value())
            stmt->bindInt64("linkTypeId", *linkTypeId);
        if (sourceItemId.has_value())
            stmt->bindInt64("sourceItemId", *sourceItemId);
        if (destinationItemId.has_value())
            stmt->bindInt64("destinationItemId", *destinationItemId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            links.push_back(mapRowToItemLink(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка связей элементов: " << e.what();
        throw;
    }

    return { links, totalCount };
}

std::optional<dto::ItemLink> SqliteItemLinkRepository::findById(int64_t id)
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
            "SELECT id, linkTypeId, sourceItemId, destinationItemId "
            "FROM ItemLink WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemLink(*rs);
        }
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска связи элемента по id: " << e.what();
        throw;
    }
}

std::vector<dto::ItemLink> SqliteItemLinkRepository::findByItemId(int64_t itemId)
{
    std::vector<dto::ItemLink> links;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return links;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, linkTypeId, sourceItemId, destinationItemId "
            "FROM ItemLink WHERE sourceItemId = :itemId OR destinationItemId = :itemId2 "
            "ORDER BY id"
        );
        stmt->bindInt64("itemId", itemId);
        stmt->bindInt64("itemId2", itemId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            links.push_back(mapRowToItemLink(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения связей по элементу: " << e.what();
        throw;
    }

    return links;
}

std::vector<dto::ItemLink> SqliteItemLinkRepository::findByLinkTypeId(int64_t linkTypeId)
{
    std::vector<dto::ItemLink> links;

    if (linkTypeId <= 0)
    {
        LOG_WARN << "findByLinkTypeId: неверный linkTypeId " << linkTypeId;
        return links;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, linkTypeId, sourceItemId, destinationItemId "
            "FROM ItemLink WHERE linkTypeId = :linkTypeId ORDER BY id"
        );
        stmt->bindInt64("linkTypeId", linkTypeId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            links.push_back(mapRowToItemLink(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения связей по типу связи: " << e.what();
        throw;
    }

    return links;
}

int64_t SqliteItemLinkRepository::create(const dto::ItemLink& itemLink)
{
    if (!itemLink.linkTypeId.has_value()
        || !itemLink.sourceItemId.has_value()
        || !itemLink.destinationItemId.has_value())
    {
        LOG_WARN << "Создание связи: отсутствуют обязательные поля";
        return 0;
    }

    // Проверка уникальности тройки
    if (existsByTriple(*itemLink.linkTypeId, *itemLink.sourceItemId, *itemLink.destinationItemId))
    {
        LOG_WARN << "Создание связи: такая связь уже существует";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO ItemLink (linkTypeId, sourceItemId, destinationItemId) "
            "VALUES (:linkTypeId, :sourceItemId, :destinationItemId)"
        );
        stmt->bindInt64("linkTypeId", *itemLink.linkTypeId);
        stmt->bindInt64("sourceItemId", *itemLink.sourceItemId);
        stmt->bindInt64("destinationItemId", *itemLink.destinationItemId);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания связи элемента: " << e.what();
        throw;
    }
}

bool SqliteItemLinkRepository::update(const dto::ItemLink& itemLink)
{
    if (!itemLink.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID связи";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE ItemLink SET ";

        if (itemLink.linkTypeId.has_value())
            setClauses.push_back("linkTypeId = :linkTypeId");
        if (itemLink.sourceItemId.has_value())
            setClauses.push_back("sourceItemId = :sourceItemId");
        if (itemLink.destinationItemId.has_value())
            setClauses.push_back("destinationItemId = :destinationItemId");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (itemLink.linkTypeId.has_value())
            stmt->bindInt64("linkTypeId", *itemLink.linkTypeId);
        if (itemLink.sourceItemId.has_value())
            stmt->bindInt64("sourceItemId", *itemLink.sourceItemId);
        if (itemLink.destinationItemId.has_value())
            stmt->bindInt64("destinationItemId", *itemLink.destinationItemId);

        stmt->bindInt64("id", *itemLink.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления связи элемента: " << e.what();
        return false;
    }
}

bool SqliteItemLinkRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM ItemLink WHERE id = :id");
        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления связи элемента: " << e.what();
        return false;
    }
}

bool SqliteItemLinkRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemLink WHERE id = :id LIMIT 1"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования связи элемента: " << e.what();
        return false;
    }
}

bool SqliteItemLinkRepository::existsByTriple(
    int64_t linkTypeId,
    int64_t sourceItemId,
    int64_t destinationItemId
)
{
    if (linkTypeId <= 0 || sourceItemId <= 0 || destinationItemId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemLink WHERE linkTypeId = :linkTypeId "
            "AND sourceItemId = :sourceItemId AND destinationItemId = :destinationItemId LIMIT 1"
        );
        stmt->bindInt64("linkTypeId", linkTypeId);
        stmt->bindInt64("sourceItemId", sourceItemId);
        stmt->bindInt64("destinationItemId", destinationItemId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки уникальности связи: " << e.what();
        return false;
    }
}

int64_t SqliteItemLinkRepository::removeByItemId(int64_t itemId)
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
            "DELETE FROM ItemLink WHERE sourceItemId = :itemId OR destinationItemId = :itemId2"
        );
        stmt->bindInt64("itemId", itemId);
        stmt->bindInt64("itemId2", itemId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления связей по элементу: " << e.what();
        return 0;
    }
}

} // namespace server::repositories
