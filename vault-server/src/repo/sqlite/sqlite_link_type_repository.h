#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "repo/link_type_repository.h"

namespace db
{
class IDatabase;
class IConnection;
} // namespace db

namespace server::repositories
{

/**
 * @brief SQLite-реализация репозитория для типов связей.
 */
class SqliteLinkTypeRepository final : public ILinkTypeRepository
{
public:
    explicit SqliteLinkTypeRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteLinkTypeRepository() override = default;

    SqliteLinkTypeRepository(const SqliteLinkTypeRepository&) = delete;
    SqliteLinkTypeRepository& operator=(const SqliteLinkTypeRepository&) = delete;

    std::pair<std::vector<dto::LinkType>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> sourceItemTypeId = std::nullopt,
        std::optional<int64_t> destinationItemTypeId = std::nullopt
    ) override;

    std::optional<dto::LinkType> findById(int64_t id) override;
    int64_t create(const dto::LinkType& linkType) override;
    bool update(const dto::LinkType& linkType) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    bool isUsed(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
