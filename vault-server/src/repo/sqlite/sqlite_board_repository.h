#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../board_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с досками.
 */
class SqliteBoardRepository final : public IBoardRepository
{
public:
    explicit SqliteBoardRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteBoardRepository() override = default;

    SqliteBoardRepository(const SqliteBoardRepository&) = delete;
    SqliteBoardRepository& operator=(const SqliteBoardRepository&) = delete;

    std::pair<std::vector<dto::Board>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> workflowId = std::nullopt
    ) override;

    std::optional<dto::Board> findById(int64_t id) override;
    int64_t create(const dto::Board& board) override;
    bool update(const dto::Board& board) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    std::vector<dto::Board> findByProject(int64_t projectId) override;
    std::vector<dto::Board> findByPhase(int64_t phaseId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::Board mapRowToBoard(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
