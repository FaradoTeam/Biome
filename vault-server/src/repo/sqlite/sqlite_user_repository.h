#pragma once

#include <memory>
#include <optional>
#include <string>

#include "../user_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server
{
namespace repositories
{

class SqliteUserRepository final : public IUserRepository
{
public:
    explicit SqliteUserRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteUserRepository() override = default;

    SqliteUserRepository(const SqliteUserRepository&) = delete;
    SqliteUserRepository& operator=(const SqliteUserRepository&) = delete;
    SqliteUserRepository(SqliteUserRepository&&) = delete;
    SqliteUserRepository& operator=(SqliteUserRepository&&) = delete;

    std::optional<dto::User> findByLogin(const std::string& login) override;
    std::optional<dto::User> findById(int64_t id) override;
    std::string passwordHash(int64_t userId) override;
    bool updatePassword(int64_t userId, const std::string& passwordHash) override;
    bool updateNeedChangePassword(int64_t userId, bool needChange) override;
    int64_t create(const dto::User& user, const std::string& passwordHash) override;
    bool existsByLogin(const std::string& login) override;
    std::pair<std::vector<dto::User>, int64_t> findAll(int page, int pageSize) override;
    bool update(const dto::User& user) override;
    bool remove(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
