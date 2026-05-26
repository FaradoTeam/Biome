#pragma once

#include <memory>

#include "../role_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteRoleRepository final : public IRoleRepository
{
public:
    explicit SqliteRoleRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteRoleRepository() override = default;

    std::pair<std::vector<dto::Role>, int64_t> findAll(
        int page, int pageSize, const std::string& searchCaption = ""
    ) override;
    std::optional<dto::Role> findById(int64_t id) override;
    std::optional<dto::Role> findByCaption(const std::string& caption) override;
    int64_t create(const dto::Role& role) override;
    bool update(const dto::Role& role) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
