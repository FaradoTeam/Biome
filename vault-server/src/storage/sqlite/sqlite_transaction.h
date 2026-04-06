#pragma once
#include <memory>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteConnection;

class SqliteTransaction : public ITransaction
{
public:
    explicit SqliteTransaction(SqliteConnection* conn);
    ~SqliteTransaction() override;

    void commit() override;
    void rollback() override;
    bool isActive() const override { return m_active; }

private:
    SqliteConnection* m_conn;
    bool m_active = true;
};

} // namespace db::sqlite
