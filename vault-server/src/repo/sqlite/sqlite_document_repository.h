#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../document_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с документами.
 */
class SqliteDocumentRepository final : public IDocumentRepository
{
public:
    explicit SqliteDocumentRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteDocumentRepository() override = default;

    SqliteDocumentRepository(const SqliteDocumentRepository&) = delete;
    SqliteDocumentRepository& operator=(const SqliteDocumentRepository&) = delete;

    std::pair<std::vector<dto::Document>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> uploadedByUserId = std::nullopt,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::Document> findById(int64_t id) override;
    int64_t create(const dto::Document& document) override;
    bool update(const dto::Document& document) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    bool isPathUnique(
        const std::string& path,
        std::optional<int64_t> excludeId = std::nullopt
    ) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::Document mapRowToDocument(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
