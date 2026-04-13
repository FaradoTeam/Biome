#pragma once

#include "idatabase.h"

namespace db
{

class SqliteConnection;

class SqliteTransaction : public ITransaction
{
public:
    explicit SqliteTransaction(SqliteConnection& conn);
    ~SqliteTransaction() override;

    SqliteTransaction(const SqliteTransaction&) = delete;
    SqliteTransaction& operator=(const SqliteTransaction&) = delete;
    SqliteTransaction(SqliteTransaction&&) = delete;
    SqliteTransaction& operator=(SqliteTransaction&&) = delete;

    void commit() override;
    void rollback() override;
    bool isActive() const override;

private:
    SqliteConnection& m_connection;
    bool m_active = true;
};

} // namespace db
