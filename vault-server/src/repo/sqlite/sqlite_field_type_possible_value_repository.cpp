#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_field_type_possible_value_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект FieldTypePossibleValue.
 */
dto::FieldTypePossibleValue mapRowToFieldTypePossibleValue(db::IResultSet& rs)
{
    dto::FieldTypePossibleValue value;
    value.id = rs.valueInt64("id");
    value.fieldTypeId = rs.valueInt64("fieldTypeId");
    value.value = rs.valueString("value");
    return value;
}

} // namespace

namespace server
{
namespace repositories
{

SqliteFieldTypePossibleValueRepository::SqliteFieldTypePossibleValueRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteFieldTypePossibleValueRepository: database is null");
    }
}

std::pair<std::vector<dto::FieldTypePossibleValue>, int64_t>
SqliteFieldTypePossibleValueRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> fieldTypeId
)
{
    std::vector<dto::FieldTypePossibleValue> values;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем SQL с учётом фильтра
        std::string whereClause;
        if (fieldTypeId.has_value())
        {
            whereClause = " WHERE fieldTypeId = :fieldTypeId";
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM FieldTypePossibleValue" + whereClause
        );
        if (fieldTypeId.has_value())
        {
            countStmt->bindInt64("fieldTypeId", *fieldTypeId);
        }
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { values, totalCount };
        }

        // 2. Получаем страницу с возможными значениями
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, fieldTypeId, value FROM FieldTypePossibleValue" + whereClause + " ORDER BY value LIMIT :limit OFFSET :offset"
        );

        if (fieldTypeId.has_value())
        {
            stmt->bindInt64("fieldTypeId", *fieldTypeId);
        }
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            values.push_back(mapRowToFieldTypePossibleValue(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка возможных значений полей: " << e.what();
        throw;
    }

    return { values, totalCount };
}

std::optional<dto::FieldTypePossibleValue>
SqliteFieldTypePossibleValueRepository::findById(int64_t id)
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
            "SELECT id, fieldTypeId, value FROM FieldTypePossibleValue WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToFieldTypePossibleValue(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска возможного значения по id: " << e.what();
        throw;
    }
}

std::vector<dto::FieldTypePossibleValue>
SqliteFieldTypePossibleValueRepository::findByFieldTypeId(int64_t fieldTypeId)
{
    std::vector<dto::FieldTypePossibleValue> values;

    if (fieldTypeId <= 0)
    {
        LOG_WARN << "findByFieldTypeId: неверный идентификатор " << fieldTypeId;
        return values;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, fieldTypeId, value FROM FieldTypePossibleValue "
            "WHERE fieldTypeId = :fieldTypeId ORDER BY value"
        );

        stmt->bindInt64("fieldTypeId", fieldTypeId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            values.push_back(mapRowToFieldTypePossibleValue(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения возможных значений для типа поля: " << e.what();
        throw;
    }

    return values;
}

int64_t SqliteFieldTypePossibleValueRepository::create(
    const dto::FieldTypePossibleValue& value
)
{
    // Проверка обязательных полей
    if (!value.fieldTypeId.has_value())
    {
        LOG_WARN << "createFieldTypePossibleValue: отсутствует fieldTypeId";
        return 0;
    }

    if (!value.value.has_value() || value.value->empty())
    {
        LOG_WARN << "createFieldTypePossibleValue: отсутствует value";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO FieldTypePossibleValue (fieldTypeId, value) "
            "VALUES (:fieldTypeId, :value)"
        );

        stmt->bindInt64("fieldTypeId", *value.fieldTypeId);
        stmt->bindString("value", *value.value);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания возможного значения поля: " << e.what();
        throw;
    }
}

bool SqliteFieldTypePossibleValueRepository::update(
    const dto::FieldTypePossibleValue& value
)
{
    if (!value.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID возможного значения";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE FieldTypePossibleValue SET ";

        if (value.fieldTypeId.has_value())
        {
            setClauses.push_back("fieldTypeId = :fieldTypeId");
        }
        if (value.value.has_value())
        {
            setClauses.push_back("value = :value");
        }

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (value.fieldTypeId.has_value())
        {
            stmt->bindInt64("fieldTypeId", *value.fieldTypeId);
        }
        if (value.value.has_value())
        {
            stmt->bindString("value", *value.value);
        }

        stmt->bindInt64("id", *value.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления возможного значения поля: " << e.what();
        return false;
    }
}

bool SqliteFieldTypePossibleValueRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM FieldTypePossibleValue WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления возможного значения поля: " << e.what();
        return false;
    }
}

bool SqliteFieldTypePossibleValueRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM FieldTypePossibleValue WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования возможного значения: " << e.what();
        return false;
    }
}

bool SqliteFieldTypePossibleValueRepository::existsByValue(
    int64_t fieldTypeId,
    const std::string& value
)
{
    if (fieldTypeId <= 0 || value.empty())
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM FieldTypePossibleValue "
            "WHERE fieldTypeId = :fieldTypeId AND value = :value LIMIT 1"
        );

        stmt->bindInt64("fieldTypeId", fieldTypeId);
        stmt->bindString("value", value);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования значения для типа поля: " << e.what();
        return false;
    }
}

std::shared_ptr<db::IConnection> SqliteFieldTypePossibleValueRepository::connection() const
{
    return m_database->connection();
}

} // namespace repositories
} // namespace server
