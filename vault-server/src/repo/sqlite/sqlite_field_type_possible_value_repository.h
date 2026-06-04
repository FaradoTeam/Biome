#pragma once

#include <memory>

#include "../field_type_possible_value_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server
{
namespace repositories
{

/**
 * @brief SQLite реализация репозитория для возможных значений полей.
 */
class SqliteFieldTypePossibleValueRepository final : public IFieldTypePossibleValueRepository
{
public:
    explicit SqliteFieldTypePossibleValueRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteFieldTypePossibleValueRepository() override = default;

    SqliteFieldTypePossibleValueRepository(const SqliteFieldTypePossibleValueRepository&) = delete;
    SqliteFieldTypePossibleValueRepository& operator=(const SqliteFieldTypePossibleValueRepository&) = delete;

    std::pair<std::vector<dto::FieldTypePossibleValue>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) override;

    std::optional<dto::FieldTypePossibleValue> findById(int64_t id) override;
    std::vector<dto::FieldTypePossibleValue> findByFieldTypeId(int64_t fieldTypeId) override;
    int64_t create(const dto::FieldTypePossibleValue& value) override;
    bool update(const dto::FieldTypePossibleValue& value) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    bool existsByValue(int64_t fieldTypeId, const std::string& value) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
