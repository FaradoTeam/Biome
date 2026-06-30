#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../special_day_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с особыми днями.
 */
class SqliteSpecialDayRepository final : public ISpecialDayRepository
{
public:
    explicit SqliteSpecialDayRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteSpecialDayRepository() override = default;

    SqliteSpecialDayRepository(const SqliteSpecialDayRepository&) = delete;
    SqliteSpecialDayRepository& operator=(const SqliteSpecialDayRepository&) = delete;

    SpecialDaysPage findAll(
        int page,
        int pageSize,
        std::optional<int> year = std::nullopt,
        std::optional<int> month = std::nullopt
    ) override;

    std::optional<dto::SpecialDay> findById(int64_t id) override;
    std::optional<dto::SpecialDay> findByDate(const common::DateTime& date) override;
    int64_t create(const dto::SpecialDay& specialDay) override;
    bool update(const dto::SpecialDay& specialDay) override;
    bool remove(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::SpecialDay mapRowToSpecialDay(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
