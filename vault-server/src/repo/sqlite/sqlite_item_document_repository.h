#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../item_document_repository.h"

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
 * @brief SQLite-реализация репозитория для связей элементов с документами.
 */
class SqliteItemDocumentRepository final : public IItemDocumentRepository
{
public:
    explicit SqliteItemDocumentRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteItemDocumentRepository() override = default;

    SqliteItemDocumentRepository(const SqliteItemDocumentRepository&) = delete;
    SqliteItemDocumentRepository& operator=(const SqliteItemDocumentRepository&) = delete;

    std::pair<std::vector<dto::ItemDocument>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) override;

    std::optional<dto::ItemDocument> findById(int64_t id) override;
    std::vector<dto::ItemDocument> findByItemId(int64_t itemId) override;
    std::vector<dto::ItemDocument> findByDocumentId(int64_t documentId) override;
    std::optional<dto::ItemDocument> findByItemAndDocument(
        int64_t itemId,
        int64_t documentId
    ) override;
    bool exists(int64_t itemId, int64_t documentId) override;
    int64_t create(const dto::ItemDocument& itemDocument) override;
    bool remove(int64_t id) override;
    int64_t removeByItemId(int64_t itemId) override;
    int64_t removeByDocumentId(int64_t documentId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::ItemDocument mapRowToItemDocument(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
