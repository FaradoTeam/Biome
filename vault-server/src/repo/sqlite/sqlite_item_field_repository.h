#pragma once

#include <memory>

#include "../item_field_repository.h"

namespace db
{
class IDatabase;
class IConnection;
class IResultSet;
}

namespace server
{
namespace repositories
{

/**
 * @brief SQLite-реализация репозитория значений полей элементов.
 */
class SqliteItemFieldRepository final : public IItemFieldRepository
{
public:
    explicit SqliteItemFieldRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteItemFieldRepository() override = default;

    SqliteItemFieldRepository(const SqliteItemFieldRepository&) = delete;
    SqliteItemFieldRepository& operator=(const SqliteItemFieldRepository&) = delete;

    // IItemFieldRepository
    ItemFieldsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) override;

    std::optional<dto::ItemField> findById(int64_t id) override;
    std::optional<dto::ItemField> findByItemAndFieldType(
        int64_t itemId,
        int64_t fieldTypeId
    ) override;
    std::vector<dto::ItemField> findByItemId(int64_t itemId) override;
    int64_t create(const dto::ItemField& field) override;
    bool update(const dto::ItemField& field) override;
    bool remove(int64_t id) override;
    int64_t removeByItemId(int64_t itemId) override;
    bool exists(int64_t id) override;
    bool existsByItemAndFieldType(int64_t itemId, int64_t fieldTypeId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
