#pragma once

#include <memory>

#include "../team_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteTeamRepository final : public ITeamRepository
{
public:
    explicit SqliteTeamRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteTeamRepository() override = default;

    std::pair<std::vector<dto::Team>, int64_t> findAll(
        int page, int pageSize, const std::string& searchCaption = ""
    ) override;
    std::optional<dto::Team> findById(int64_t id) override;
    int64_t create(const dto::Team& team) override;
    bool update(const dto::Team& team) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
