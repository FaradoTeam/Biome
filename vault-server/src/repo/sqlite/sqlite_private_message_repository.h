#pragma once

#include <memory>

#include "../private_message_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server
{
namespace repositories
{

class SqlitePrivateMessageRepository final : public IPrivateMessageRepository
{
public:
    explicit SqlitePrivateMessageRepository(std::shared_ptr<db::IDatabase> database);
    ~SqlitePrivateMessageRepository() override = default;

    PrivateMessagesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<bool> isViewed = std::nullopt
    ) override;

    std::optional<dto::PrivateMessage> findById(int64_t id) override;
    std::vector<dto::PrivateMessage> findConversation(int64_t userId1, int64_t userId2) override;
    std::vector<dto::PrivateMessage> findBySender(int64_t senderUserId) override;
    std::vector<dto::PrivateMessage> findByReceiver(
        int64_t receiverUserId,
        bool onlyUnviewed = false
    ) override;

    int64_t create(const dto::PrivateMessage& message) override;
    bool update(const dto::PrivateMessage& message) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    int64_t markAllAsViewed(int64_t senderUserId, int64_t receiverUserId) override;
    int64_t countUnviewed(int64_t userId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
