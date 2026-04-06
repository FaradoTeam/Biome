#pragma once
#include <functional>
#include <memory>
#include <string>

#include "types.h"

namespace db
{

class IConnection;
class IStatement;
class IResultSet;
class ITransaction;

class IDatabase
{
public:
    virtual ~IDatabase() = default;

    virtual void initialize(const DatabaseConfig& config) = 0;
    virtual void shutdown() = 0;

    virtual std::unique_ptr<IConnection> getConnection() = 0;

    virtual std::unique_ptr<IResultSet> query(const std::string& sql) = 0;
    virtual int64_t execute(const std::string& sql) = 0;

    virtual void transaction(std::function<void()> callback) = 0;
};

//-----------------------------------------------------------------------------

class IConnection
{
public:
    virtual ~IConnection() = default;

    virtual void open(const std::string& connectionString) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual std::unique_ptr<IStatement> prepareStatement(const std::string& sql) = 0;
    virtual int64_t execute(const std::string& sql) = 0;

    virtual std::unique_ptr<ITransaction> beginTransaction() = 0;

    virtual int64_t getLastInsertId() = 0;
    virtual std::string escapeString(const std::string& value) = 0;
};

//-----------------------------------------------------------------------------

class IStatement
{
public:
    virtual ~IStatement() = default;

    virtual int64_t execute() = 0;
    virtual std::unique_ptr<IResultSet> executeQuery() = 0;

    virtual void reset() = 0;

    virtual void bindNull(const std::string& name) = 0;
    virtual void bindInt64(const std::string& name, int64_t value) = 0;
    virtual void bindDouble(const std::string& name, double value) = 0;
    virtual void bindString(const std::string& name, const std::string& value) = 0;
    virtual void bindBlob(const std::string& name, const Blob& value) = 0;
    virtual void bindDateTime(const std::string& name, const DateTime& value) = 0;
};

//-----------------------------------------------------------------------------

class IResultSet
{
public:
    virtual ~IResultSet() = default;

    virtual bool next() = 0;

    virtual int getColumnCount() const = 0;
    virtual std::string getColumnName(int index) const = 0;
    virtual int getColumnIndex(const std::string& name) const = 0;

    virtual bool isNull(int index) const = 0;
    virtual bool isNull(const std::string& name) const
    {
        return isNull(getColumnIndex(name));
    }

    virtual FieldValue getValue(int index) const = 0;
    virtual FieldValue getValue(const std::string& name) const
    {
        return getValue(getColumnIndex(name));
    }

    virtual int64_t getInt64(int index) const
    {
        return getValue(index).asInt64();
    }

    virtual double getDouble(int index) const
    {
        return getValue(index).asDouble();
    }

    virtual std::string getString(int index) const
    {
        return getValue(index).asString();
    }

    virtual Blob getBlob(int index) const
    {
        return getValue(index).asBlob();
    }

    virtual DateTime getDateTime(int index) const
    {
        return getValue(index).asDateTime();
    }

    virtual int64_t getInt64(const std::string& name) const
    {
        return getInt64(getColumnIndex(name));
    }

    virtual double getDouble(const std::string& name) const
    {
        return getDouble(getColumnIndex(name));
    }

    virtual std::string getString(const std::string& name) const
    {
        return getString(getColumnIndex(name));
    }

    virtual Blob getBlob(const std::string& name) const
    {
        return getBlob(getColumnIndex(name));
    }

    virtual DateTime getDateTime(const std::string& name) const
    {
        return getDateTime(getColumnIndex(name));
    }
};

//-----------------------------------------------------------------------------

class ITransaction
{
public:
    virtual ~ITransaction() = default;

    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual bool isActive() const = 0;
};

} // namespace db
