#pragma once

#include <memory>

#include "../team_message_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server
{
namespace repositories
{

class SqliteTeamMessageRepository final : public ITeamMessageRepository
{
public:
    explicit SqliteTeamMessageRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteTeamMessageRepository() override = default;

    TeamMessagesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> senderUserId = std::nullopt
    ) override;

    std::optional<dto::TeamMessage> findById(int64_t id) override;
    std::vector<dto::TeamMessage> findByTeamId(int64_t teamId) override;
    std::vector<dto::TeamMessage> findBySenderAndTeam(
        int64_t senderUserId,
        int64_t teamId
    ) override;

    int64_t create(const dto::TeamMessage& message) override;
    bool update(const dto::TeamMessage& message) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    int64_t removeByTeamId(int64_t teamId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
