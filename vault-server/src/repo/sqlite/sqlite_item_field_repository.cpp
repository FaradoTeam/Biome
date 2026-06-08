#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_item_field_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект ItemField.
 */
dto::ItemField mapRowToItemField(db::IResultSet& rs)
{
    dto::ItemField field;
    field.id = rs.valueInt64("id");
    field.itemId = rs.valueInt64("itemId");
    field.fieldTypeId = rs.valueInt64("fieldTypeId");

    if (!rs.isNull("value"))
        field.value = rs.valueString("value");

    return field;
}

} // namespace

namespace server
{
namespace repositories
{

SqliteItemFieldRepository::SqliteItemFieldRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteItemFieldRepository: database is null");
    }
}

ItemFieldsPage SqliteItemFieldRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> itemId,
    std::optional<int64_t> fieldTypeId
)
{
    std::vector<dto::ItemField> fields;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем WHERE clause
        std::vector<std::string> whereConditions;
        if (itemId.has_value())
            whereConditions.push_back("itemId = :itemId");
        if (fieldTypeId.has_value())
            whereConditions.push_back("fieldTypeId = :fieldTypeId");

        std::string whereClause;
        if (!whereConditions.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereConditions, " AND ");
        }

        // 1. Получаем общее количество
        std::string countSql = "SELECT COUNT(*) FROM ItemField" + whereClause;
        auto countStmt = conn->prepareStatement(countSql);

        if (itemId.has_value())
            countStmt->bindInt64("itemId", *itemId);
        if (fieldTypeId.has_value())
            countStmt->bindInt64("fieldTypeId", *fieldTypeId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { fields, totalCount };
        }

        // 2. Получаем страницу со значениями полей
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, itemId, fieldTypeId, value "
                                "FROM ItemField"
            + whereClause + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);

        if (itemId.has_value())
            stmt->bindInt64("itemId", *itemId);
        if (fieldTypeId.has_value())
            stmt->bindInt64("fieldTypeId", *fieldTypeId);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            fields.push_back(mapRowToItemField(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка значений полей: " << e.what();
        throw;
    }

    return { fields, totalCount };
}

std::optional<dto::ItemField> SqliteItemFieldRepository::findById(int64_t id)
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
            "SELECT id, itemId, fieldTypeId, value "
            "FROM ItemField WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemField(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска значения поля по id: " << e.what();
        throw;
    }
}

std::optional<dto::ItemField> SqliteItemFieldRepository::findByItemAndFieldType(
    int64_t itemId,
    int64_t fieldTypeId
)
{
    if (itemId <= 0 || fieldTypeId <= 0)
    {
        LOG_WARN << "findByItemAndFieldType: неверные параметры";
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, fieldTypeId, value "
            "FROM ItemField WHERE itemId = :itemId AND fieldTypeId = :fieldTypeId"
        );

        stmt->bindInt64("itemId", itemId);
        stmt->bindInt64("fieldTypeId", fieldTypeId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemField(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска значения поля по itemId и fieldTypeId: " << e.what();
        throw;
    }
}

std::vector<dto::ItemField> SqliteItemFieldRepository::findByItemId(int64_t itemId)
{
    std::vector<dto::ItemField> fields;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return fields;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, fieldTypeId, value "
            "FROM ItemField WHERE itemId = :itemId ORDER BY fieldTypeId"
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            fields.push_back(mapRowToItemField(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения значений полей элемента: " << e.what();
        throw;
    }

    return fields;
}

int64_t SqliteItemFieldRepository::create(const dto::ItemField& field)
{
    // Проверка обязательных полей
    if (!field.itemId.has_value() || !field.fieldTypeId.has_value())
    {
        LOG_WARN << "Создание значения поля: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO ItemField (itemId, fieldTypeId, value, searchValue) "
            "VALUES (:itemId, :fieldTypeId, :value, :searchValue)"
        );

        stmt->bindInt64("itemId", *field.itemId);
        stmt->bindInt64("fieldTypeId", *field.fieldTypeId);

        if (field.value.has_value())
        {
            stmt->bindString("value", *field.value);
            stmt->bindString("searchValue", common::toLowerCase(*field.value));
        }
        else
        {
            stmt->bindNull("value");
            stmt->bindNull("searchValue");
        }

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания значения поля: " << e.what();
        throw;
    }
}

bool SqliteItemFieldRepository::update(const dto::ItemField& field)
{
    if (!field.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID значения поля";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE ItemField SET ";

        if (field.itemId.has_value())
            setClauses.push_back("itemId = :itemId");
        if (field.fieldTypeId.has_value())
            setClauses.push_back("fieldTypeId = :fieldTypeId");
        if (field.value.has_value())
        {
            setClauses.push_back("value = :value");
            setClauses.push_back("searchValue = :searchValue");
        }

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (field.itemId.has_value())
            stmt->bindInt64("itemId", *field.itemId);
        if (field.fieldTypeId.has_value())
            stmt->bindInt64("fieldTypeId", *field.fieldTypeId);
        if (field.value.has_value())
        {
            stmt->bindString("value", *field.value);
            stmt->bindString("searchValue", common::toLowerCase(*field.value));
        }

        stmt->bindInt64("id", *field.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления значения поля: " << e.what();
        return false;
    }
}

bool SqliteItemFieldRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM ItemField WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления значения поля: " << e.what();
        return false;
    }
}

int64_t SqliteItemFieldRepository::removeByItemId(int64_t itemId)
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
            "DELETE FROM ItemField WHERE itemId = :itemId"
        );

        stmt->bindInt64("itemId", itemId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления значений полей элемента: " << e.what();
        return 0;
    }
}

bool SqliteItemFieldRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemField WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования значения поля: " << e.what();
        return false;
    }
}

bool SqliteItemFieldRepository::existsByItemAndFieldType(
    int64_t itemId,
    int64_t fieldTypeId
)
{
    if (itemId <= 0 || fieldTypeId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemField WHERE itemId = :itemId AND fieldTypeId = :fieldTypeId LIMIT 1"
        );

        stmt->bindInt64("itemId", itemId);
        stmt->bindInt64("fieldTypeId", fieldTypeId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования значения поля для элемента и типа: " << e.what();
        return false;
    }
}

std::shared_ptr<db::IConnection> SqliteItemFieldRepository::connection() const
{
    return m_database->connection();
}

} // namespace repositories
} // namespace server
