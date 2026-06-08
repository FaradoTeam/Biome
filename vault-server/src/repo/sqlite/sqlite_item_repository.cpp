#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_item_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект Item.
 */
dto::Item mapRowToItem(db::IResultSet& rs)
{
    dto::Item item;
    item.id = rs.valueInt64("id");
    item.itemTypeId = rs.valueInt64("itemTypeId");

    if (!rs.isNull("parentId"))
        item.parentId = rs.valueInt64("parentId");

    item.stateId = rs.valueInt64("stateId");
    item.phaseId = rs.valueInt64("phaseId");
    item.caption = rs.valueString("caption");

    if (!rs.isNull("content"))
        item.content = rs.valueString("content");

    item.isDeleted = rs.valueInt64("isDeleted") != 0;

    return item;
}

std::string buildSearchCondition(const std::string& searchCaption)
{
    if (searchCaption.empty())
    {
        return "";
    }
    return "(i.searchCaption LIKE '%' || :search || '%')";
}

} // namespace

namespace server
{
namespace repositories
{

SqliteItemRepository::SqliteItemRepository(std::shared_ptr<db::IDatabase> database)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteItemRepository: database is null");
    }
}

ItemsPage SqliteItemRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> itemTypeId,
    std::optional<int64_t> parentId,
    std::optional<int64_t> phaseId,
    std::optional<int64_t> stateId,
    std::optional<bool> isDeleted,
    const std::string& searchCaption,
    const std::vector<int64_t>& projectIds
)
{
    std::vector<dto::Item> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Базовый SQL
        std::string fromClause = "FROM Item i";
        std::vector<std::string> whereConditions;

        // Фильтр по типу элемента
        if (itemTypeId.has_value())
        {
            whereConditions.push_back("i.itemTypeId = :itemTypeId");
        }

        // Фильтр по родительскому элементу
        if (parentId.has_value())
        {
            whereConditions.push_back("i.parentId = :parentId");
        }
        else if (!parentId.has_value() && phaseId.has_value())
        {
            // Если parentId не указан, но указан phaseId - показываем корневые элементы
            whereConditions.push_back("i.parentId IS NULL");
        }

        // Фильтр по фазе
        if (phaseId.has_value())
        {
            whereConditions.push_back("i.phaseId = :phaseId");
        }

        // Фильтр по состоянию
        if (stateId.has_value())
        {
            whereConditions.push_back("i.stateId = :stateId");
        }

        // Фильтр по статусу удаления
        if (isDeleted.has_value())
        {
            whereConditions.push_back("i.isDeleted = :isDeleted");
        }
        else
        {
            // По умолчанию не показываем удаленные элементы
            whereConditions.push_back("i.isDeleted = 0");
        }

        // Фильтр по проектам (через фазы)
        if (!projectIds.empty())
        {
            fromClause += " INNER JOIN Phase p ON i.phaseId = p.id";
            whereConditions.push_back(
                "p.projectId IN ("
                + boost::algorithm::join(
                    std::vector<std::string>(projectIds.size(), "?"), ","
                )
                + ")"
            );
        }

        // Поиск по названию
        std::string searchCondition = buildSearchCondition(searchCaption);
        if (!searchCondition.empty())
        {
            whereConditions.push_back(searchCondition);
        }

        // Собираем WHERE clause
        std::string whereClause;
        if (!whereConditions.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereConditions, " AND ");
        }

        // 1. Получаем общее количество
        const std::string countSql = "SELECT COUNT(*) " + fromClause + whereClause;
        auto countStmt = conn->prepareStatement(countSql);

        int paramIndex = 1;
        if (itemTypeId.has_value())
            countStmt->bindInt64("itemTypeId", *itemTypeId);
        if (parentId.has_value())
            countStmt->bindInt64("parentId", *parentId);
        if (phaseId.has_value())
            countStmt->bindInt64("phaseId", *phaseId);
        if (stateId.has_value())
            countStmt->bindInt64("stateId", *stateId);
        if (isDeleted.has_value())
            countStmt->bindInt64("isDeleted", *isDeleted ? 1 : 0);
        if (!projectIds.empty())
        {
            for (int64_t pid : projectIds)
            {
                countStmt->bindInt64(":" + std::to_string(paramIndex++), pid);
            }
        }
        if (!searchCaption.empty())
        {
            countStmt->bindString("search", common::toLowerCase(searchCaption));
        }

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { items, totalCount };
        }

        // 2. Получаем страницу с элементами
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT i.id, i.itemTypeId, i.parentId, i.stateId, "
                                "i.phaseId, i.caption, i.content, i.isDeleted "
            + fromClause + whereClause
            + " ORDER BY i.id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);

        paramIndex = 1;
        if (itemTypeId.has_value())
            stmt->bindInt64("itemTypeId", *itemTypeId);
        if (parentId.has_value())
            stmt->bindInt64("parentId", *parentId);
        if (phaseId.has_value())
            stmt->bindInt64("phaseId", *phaseId);
        if (stateId.has_value())
            stmt->bindInt64("stateId", *stateId);
        if (isDeleted.has_value())
            stmt->bindInt64("isDeleted", *isDeleted ? 1 : 0);
        if (!projectIds.empty())
        {
            for (int64_t pid : projectIds)
            {
                stmt->bindInt64(":" + std::to_string(paramIndex++), pid);
            }
        }
        if (!searchCaption.empty())
        {
            stmt->bindString("search", common::toLowerCase(searchCaption));
        }

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка элементов: " << e.what();
        throw;
    }

    return { items, totalCount };
}

std::optional<dto::Item> SqliteItemRepository::findById(int64_t id)
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
            "SELECT id, itemTypeId, parentId, stateId, phaseId, "
            "caption, content, isDeleted "
            "FROM Item WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItem(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска элемента по id: " << e.what();
        throw;
    }
}

int64_t SqliteItemRepository::create(const dto::Item& item)
{
    // Проверка обязательных полей
    if (!item.itemTypeId.has_value()
        || !item.stateId.has_value()
        || !item.phaseId.has_value()
        || !item.caption.has_value()
        || item.caption->empty())
    {
        LOG_WARN << "Создание элемента: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO Item (itemTypeId, parentId, stateId, phaseId, "
            "caption, content, searchCaption, searchContent, isDeleted) "
            "VALUES (:itemTypeId, :parentId, :stateId, :phaseId, "
            ":caption, :content, :searchCaption, :searchContent, :isDeleted)"
        );

        stmt->bindInt64("itemTypeId", *item.itemTypeId);

        if (item.parentId.has_value())
            stmt->bindInt64("parentId", *item.parentId);
        else
            stmt->bindNull("parentId");

        stmt->bindInt64("stateId", *item.stateId);
        stmt->bindInt64("phaseId", *item.phaseId);
        stmt->bindString("caption", *item.caption);

        if (item.content.has_value())
        {
            stmt->bindString("content", *item.content);
            stmt->bindString("searchContent", common::toLowerCase(*item.content));
        }
        else
        {
            stmt->bindNull("content");
            stmt->bindNull("searchContent");
        }

        stmt->bindString("searchCaption", common::toLowerCase(*item.caption));
        stmt->bindInt64("isDeleted", item.isDeleted.value_or(false) ? 1 : 0);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания элемента: " << e.what();
        throw;
    }
}

bool SqliteItemRepository::update(const dto::Item& item)
{
    if (!item.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID элемента";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Item SET ";

        if (item.itemTypeId.has_value())
            setClauses.push_back("itemTypeId = :itemTypeId");
        if (item.parentId.has_value())
            setClauses.push_back("parentId = :parentId");
        if (item.stateId.has_value())
            setClauses.push_back("stateId = :stateId");
        if (item.phaseId.has_value())
            setClauses.push_back("phaseId = :phaseId");
        if (item.caption.has_value())
        {
            setClauses.push_back("caption = :caption");
            setClauses.push_back("searchCaption = :searchCaption");
        }
        if (item.content.has_value())
        {
            setClauses.push_back("content = :content");
            setClauses.push_back("searchContent = :searchContent");
        }
        if (item.isDeleted.has_value())
            setClauses.push_back("isDeleted = :isDeleted");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (item.itemTypeId.has_value())
            stmt->bindInt64("itemTypeId", *item.itemTypeId);
        if (item.parentId.has_value())
            stmt->bindInt64("parentId", *item.parentId);
        if (item.stateId.has_value())
            stmt->bindInt64("stateId", *item.stateId);
        if (item.phaseId.has_value())
            stmt->bindInt64("phaseId", *item.phaseId);
        if (item.caption.has_value())
        {
            stmt->bindString("caption", *item.caption);
            stmt->bindString("searchCaption", common::toLowerCase(*item.caption));
        }
        if (item.content.has_value())
        {
            stmt->bindString("content", *item.content);
            stmt->bindString("searchContent", common::toLowerCase(*item.content));
        }
        if (item.isDeleted.has_value())
            stmt->bindInt64("isDeleted", *item.isDeleted ? 1 : 0);

        stmt->bindInt64("id", *item.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления элемента: " << e.what();
        return false;
    }
}

bool SqliteItemRepository::softDelete(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "softDelete: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "UPDATE Item SET isDeleted = 1 WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка мягкого удаления элемента: " << e.what();
        return false;
    }
}

bool SqliteItemRepository::restore(int64_t id)
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
            "UPDATE Item SET isDeleted = 0 WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка восстановления элемента: " << e.what();
        return false;
    }
}

bool SqliteItemRepository::hardDelete(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "hardDelete: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Item WHERE id = :id");
        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка полного удаления элемента: " << e.what();
        return false;
    }
}

bool SqliteItemRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Item WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования элемента: " << e.what();
        return false;
    }
}

std::vector<dto::Item> SqliteItemRepository::findChildren(
    int64_t parentId,
    bool includeDeleted
)
{
    std::vector<dto::Item> items;

    if (parentId <= 0)
    {
        LOG_WARN << "findChildren: неверный parentId " << parentId;
        return items;
    }

    try
    {
        auto conn = connection();
        std::string sql = "SELECT id, itemTypeId, parentId, stateId, phaseId, "
                          "caption, content, isDeleted "
                          "FROM Item WHERE parentId = :parentId";

        if (!includeDeleted)
        {
            sql += " AND isDeleted = 0";
        }

        sql += " ORDER BY id";

        auto stmt = conn->prepareStatement(sql);
        stmt->bindInt64("parentId", parentId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения дочерних элементов: " << e.what();
        throw;
    }

    return items;
}

std::vector<dto::Item> SqliteItemRepository::findRootItems(
    int64_t phaseId,
    bool includeDeleted
)
{
    std::vector<dto::Item> items;

    if (phaseId <= 0)
    {
        LOG_WARN << "findRootItems: неверный phaseId " << phaseId;
        return items;
    }

    try
    {
        auto conn = connection();
        std::string sql = "SELECT id, itemTypeId, parentId, stateId, phaseId, "
                          "caption, content, isDeleted "
                          "FROM Item WHERE phaseId = :phaseId AND parentId IS NULL";

        if (!includeDeleted)
        {
            sql += " AND isDeleted = 0";
        }

        sql += " ORDER BY id";

        auto stmt = conn->prepareStatement(sql);
        stmt->bindInt64("phaseId", phaseId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToItem(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения корневых элементов: " << e.what();
        throw;
    }

    return items;
}

std::shared_ptr<db::IConnection> SqliteItemRepository::connection() const
{
    return m_database->connection();
}

} // namespace repositories
} // namespace server
