#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_link_type_repository.h"

namespace
{

/**
 * @brief Преобразует строку результата в объект LinkType.
 */
dto::LinkType mapRowToLinkType(db::IResultSet& rs)
{
    dto::LinkType linkType;
    linkType.id = rs.valueInt64("id");
    linkType.sourceItemTypeId = rs.valueInt64("sourceItemTypeId");
    linkType.destinationItemTypeId = rs.valueInt64("destinationItemTypeId");
    linkType.isBidirectional = rs.valueInt64("isBidirectional") != 0;
    linkType.caption = rs.valueString("caption");
    return linkType;
}

} // namespace

namespace server::repositories
{

SqliteLinkTypeRepository::SqliteLinkTypeRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteLinkTypeRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteLinkTypeRepository::connection() const
{
    return m_database->connection();
}

std::pair<std::vector<dto::LinkType>, int64_t>
SqliteLinkTypeRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> sourceItemTypeId,
    std::optional<int64_t> destinationItemTypeId
)
{
    std::vector<dto::LinkType> linkTypes;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем WHERE clause
        std::vector<std::string> whereClauses;
        if (sourceItemTypeId.has_value())
            whereClauses.push_back("sourceItemTypeId = :sourceItemTypeId");
        if (destinationItemTypeId.has_value())
            whereClauses.push_back("destinationItemTypeId = :destinationItemTypeId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM LinkType" + whereStr
        );
        if (sourceItemTypeId.has_value())
            countStmt->bindInt64("sourceItemTypeId", *sourceItemTypeId);
        if (destinationItemTypeId.has_value())
            countStmt->bindInt64("destinationItemTypeId", *destinationItemTypeId);
        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { linkTypes, totalCount };
        }

        // 2. Получаем страницу с типами связей
        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, sourceItemTypeId, destinationItemTypeId, "
                                "isBidirectional, caption FROM LinkType"
            + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (sourceItemTypeId.has_value())
            stmt->bindInt64("sourceItemTypeId", *sourceItemTypeId);
        if (destinationItemTypeId.has_value())
            stmt->bindInt64("destinationItemTypeId", *destinationItemTypeId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            linkTypes.push_back(mapRowToLinkType(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка типов связей: " << e.what();
        throw;
    }

    return { linkTypes, totalCount };
}

std::optional<dto::LinkType> SqliteLinkTypeRepository::findById(int64_t id)
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
            "SELECT id, sourceItemTypeId, destinationItemTypeId, "
            "isBidirectional, caption FROM LinkType WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToLinkType(*rs);
        }
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска типа связи по id: " << e.what();
        throw;
    }
}

int64_t SqliteLinkTypeRepository::create(const dto::LinkType& linkType)
{
    // Проверка обязательных полей
    if (!linkType.sourceItemTypeId.has_value()
        || !linkType.destinationItemTypeId.has_value()
        || !linkType.caption.has_value()
        || linkType.caption->empty())
    {
        LOG_WARN << "Создание типа связи: отсутствуют обязательные поля";
        return 0;
    }

    // Проверка допустимых значений isBidirectional (по умолчанию false)
    const bool isBidirectional = linkType.isBidirectional.value_or(false);

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO LinkType (sourceItemTypeId, destinationItemTypeId, "
            "isBidirectional, caption) VALUES (:sourceItemTypeId, :destinationItemTypeId, "
            ":isBidirectional, :caption)"
        );
        stmt->bindInt64("sourceItemTypeId", *linkType.sourceItemTypeId);
        stmt->bindInt64("destinationItemTypeId", *linkType.destinationItemTypeId);
        stmt->bindInt64("isBidirectional", isBidirectional ? 1 : 0);
        stmt->bindString("caption", *linkType.caption);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания типа связи: " << e.what();
        throw;
    }
}

bool SqliteLinkTypeRepository::update(const dto::LinkType& linkType)
{
    if (!linkType.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID типа связи";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE LinkType SET ";

        if (linkType.sourceItemTypeId.has_value())
            setClauses.push_back("sourceItemTypeId = :sourceItemTypeId");
        if (linkType.destinationItemTypeId.has_value())
            setClauses.push_back("destinationItemTypeId = :destinationItemTypeId");
        if (linkType.isBidirectional.has_value())
            setClauses.push_back("isBidirectional = :isBidirectional");
        if (linkType.caption.has_value())
            setClauses.push_back("caption = :caption");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (linkType.sourceItemTypeId.has_value())
            stmt->bindInt64("sourceItemTypeId", *linkType.sourceItemTypeId);
        if (linkType.destinationItemTypeId.has_value())
            stmt->bindInt64("destinationItemTypeId", *linkType.destinationItemTypeId);
        if (linkType.isBidirectional.has_value())
            stmt->bindInt64("isBidirectional", *linkType.isBidirectional ? 1 : 0);
        if (linkType.caption.has_value())
            stmt->bindString("caption", *linkType.caption);

        stmt->bindInt64("id", *linkType.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления типа связи: " << e.what();
        return false;
    }
}

bool SqliteLinkTypeRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    // Проверяем, используется ли тип связи
    if (isUsed(id))
    {
        LOG_WARN << "remove: тип связи с id=" << id << " используется в ItemLink";
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM LinkType WHERE id = :id");
        stmt->bindInt64("id", id);
        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления типа связи: " << e.what();
        return false;
    }
}

bool SqliteLinkTypeRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM LinkType WHERE id = :id LIMIT 1"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования типа связи: " << e.what();
        return false;
    }
}

bool SqliteLinkTypeRepository::isUsed(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemLink WHERE linkTypeId = :linkTypeId LIMIT 1"
        );
        stmt->bindInt64("linkTypeId", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки использования типа связи: " << e.what();
        return false;
    }
}

} // namespace server::repositories
