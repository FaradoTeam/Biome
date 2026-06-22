#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../board_column_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с колонками досок.
 */
class SqliteBoardColumnRepository final : public IBoardColumnRepository
{
public:
    explicit SqliteBoardColumnRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteBoardColumnRepository() override = default;

    SqliteBoardColumnRepository(const SqliteBoardColumnRepository&) = delete;
    SqliteBoardColumnRepository& operator=(const SqliteBoardColumnRepository&) = delete;

    std::pair<std::vector<dto::BoardColumn>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> boardId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) override;

    std::optional<dto::BoardColumn> findById(int64_t id) override;
    int64_t create(const dto::BoardColumn& column) override;
    bool update(const dto::BoardColumn& column) override;
    bool remove(int64_t id) override;
    int64_t removeByBoardId(int64_t boardId) override;
    bool exists(int64_t id) override;
    bool existsByBoardAndState(int64_t boardId, int64_t stateId) override;
    std::vector<dto::BoardColumn> findByBoardId(
        int64_t boardId,
        bool orderByOrderNumber = true
    ) override;
    std::vector<dto::BoardColumn> findByStateId(int64_t stateId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::BoardColumn mapRowToBoardColumn(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
