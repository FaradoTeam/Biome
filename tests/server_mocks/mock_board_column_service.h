#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/dto/board_column.h"

#include "logic/iboard_column_service.h"

namespace server::tests
{

class MockBoardColumnService : public services::IBoardColumnService
{
public:
    using BoardColumnsPage = services::BoardColumnsPage;
    using BoardColumnResult = services::BoardColumnResult;

    MockBoardColumnService() = default;
    ~MockBoardColumnService() override = default;

    // ============================================================
    // Настройка результатов (простой режим)
    // ============================================================

    void setGetBoardColumnsResult(const BoardColumnsPage& result)
    {
        m_getBoardColumnsResult = result;
        // Очищаем хранилище перед добавлением новых колонок
        m_existingColumns.clear();
        // Добавляем колонки в хранилище существующих
        for (const auto& column : result.columns)
        {
            if (column.id.has_value())
            {
                m_existingColumns[*column.id] = column;
            }
        }
        m_getBoardColumnsCallback = nullptr;
    }

    void setGetBoardColumnResult(std::optional<dto::BoardColumn> column)
    {
        m_getBoardColumnResult = std::move(column);
        if (m_getBoardColumnResult.has_value() && m_getBoardColumnResult->id.has_value())
        {
            m_existingColumns[*m_getBoardColumnResult->id] = *m_getBoardColumnResult;
        }
        m_getBoardColumnCallback = nullptr;
    }

    void setGetColumnsByBoardResult(const std::vector<dto::BoardColumn>& columns)
    {
        m_getColumnsByBoardResult = columns;
        // НЕ добавляем в m_existingColumns, чтобы избежать дублирования
        // Это просто предустановленный результат для getColumnsByBoard
        m_getColumnsByBoardCallback = nullptr;
    }

    void setCreateBoardColumnResult(std::optional<dto::BoardColumn> column)
    {
        m_createBoardColumnResult = std::move(column);
        if (m_createBoardColumnResult.has_value() && m_createBoardColumnResult->id.has_value())
        {
            m_existingColumns[*m_createBoardColumnResult->id] = *m_createBoardColumnResult;
        }
        m_createBoardColumnCallback = nullptr;
    }

    void setUpdateBoardColumnResult(std::optional<dto::BoardColumn> column)
    {
        m_updateBoardResult = std::move(column);
        if (m_updateBoardResult.has_value() && m_updateBoardResult->id.has_value())
        {
            m_existingColumns[*m_updateBoardResult->id] = *m_updateBoardResult;
        }
        m_updateBoardColumnCallback = nullptr;
    }

    void setDeleteBoardColumnResult(const BoardColumnResult& result)
    {
        m_deleteBoardColumnResult = result;
        m_deleteBoardColumnCallback = nullptr;
    }

    void setDeleteColumnsByBoardResult(int64_t count)
    {
        m_deleteColumnsByBoardResult = count;
        m_deleteColumnsByBoardCallback = nullptr;
    }

    // ============================================================
    // Настройка callback'ов для кастомной логики
    // ============================================================

    void setGetBoardColumnsCallback(
        std::function<BoardColumnsPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>)> callback
    )
    {
        m_getBoardColumnsCallback = std::move(callback);
    }

    void setGetBoardColumnCallback(
        std::function<std::optional<dto::BoardColumn>(int64_t, int64_t)> callback
    )
    {
        m_getBoardColumnCallback = std::move(callback);
    }

    void setGetColumnsByBoardCallback(
        std::function<std::vector<dto::BoardColumn>(int64_t, int64_t)> callback
    )
    {
        m_getColumnsByBoardCallback = std::move(callback);
    }

    void setCreateBoardColumnCallback(
        std::function<std::optional<dto::BoardColumn>(const dto::BoardColumn&, int64_t)> callback
    )
    {
        m_createBoardColumnCallback = std::move(callback);
    }

    void setUpdateBoardColumnCallback(
        std::function<std::optional<dto::BoardColumn>(const dto::BoardColumn&, int64_t)> callback
    )
    {
        m_updateBoardColumnCallback = std::move(callback);
    }

    void setDeleteBoardColumnCallback(
        std::function<BoardColumnResult(int64_t, int64_t)> callback
    )
    {
        m_deleteBoardColumnCallback = std::move(callback);
    }

    void setDeleteColumnsByBoardCallback(
        std::function<int64_t(int64_t, int64_t)> callback
    )
    {
        m_deleteColumnsByBoardCallback = std::move(callback);
    }

    // ============================================================
    // Вспомогательные методы для управления существующими колонками
    // ============================================================

    void addExistingColumn(const dto::BoardColumn& column)
    {
        if (column.id.has_value())
        {
            m_existingColumns[*column.id] = column;
        }
    }

    void removeExistingColumn(int64_t id)
    {
        m_existingColumns.erase(id);
    }

    void clearExistingColumns()
    {
        m_existingColumns.clear();
    }

    // ============================================================
    // Реализация интерфейса IBoardColumnService
    // ============================================================

    BoardColumnsPage getBoardColumns(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> boardId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) override
    {
        m_lastGetBoardColumnsPage = page;
        m_lastGetBoardColumnsPageSize = pageSize;
        m_lastGetBoardColumnsUserId = userId;
        m_lastGetBoardColumnsBoardId = boardId;
        m_lastGetBoardColumnsStateId = stateId;
        ++m_getBoardColumnsCallCount;

        if (m_getBoardColumnsCallback)
        {
            return m_getBoardColumnsCallback(page, pageSize, userId, boardId, stateId);
        }

        // Используем предустановленный результат, а не фильтруем из m_existingColumns
        // Чтобы избежать дублирования, возвращаем m_getBoardColumnsResult
        BoardColumnsPage result = m_getBoardColumnsResult;

        // Применяем фильтры к результату
        if (boardId.has_value() || stateId.has_value())
        {
            std::vector<dto::BoardColumn> filtered;
            for (const auto& column : result.columns)
            {
                bool match = true;
                if (boardId.has_value() && (!column.boardId.has_value() || *column.boardId != *boardId))
                    match = false;
                if (stateId.has_value() && (!column.stateId.has_value() || *column.stateId != *stateId))
                    match = false;
                if (match)
                    filtered.push_back(column);
            }
            result.columns = filtered;
            result.totalCount = filtered.size();
        }

        return result;
    }

    std::optional<dto::BoardColumn> getBoardColumn(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastGetBoardColumnId = id;
        m_lastGetBoardColumnUserId = userId;
        ++m_getBoardColumnCallCount;

        if (m_getBoardColumnCallback)
        {
            return m_getBoardColumnCallback(id, userId);
        }

        // Симуляция проверки прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        // Проверяем существование колонки
        auto it = m_existingColumns.find(id);
        if (it != m_existingColumns.end())
        {
            return it->second;
        }

        return std::nullopt;
    }

    std::vector<dto::BoardColumn> getColumnsByBoard(
        int64_t boardId,
        int64_t userId
    ) override
    {
        m_lastGetColumnsByBoardId = boardId;
        ++m_getColumnsByBoardCallCount;

        if (m_getColumnsByBoardCallback)
        {
            return m_getColumnsByBoardCallback(boardId, userId);
        }

        if (userId == 999)
        {
            return {};
        }

        // Используем предустановленный результат
        return m_getColumnsByBoardResult;
    }

    std::optional<dto::BoardColumn> createBoardColumn(
        const dto::BoardColumn& column,
        int64_t userId
    ) override
    {
        m_lastCreatedBoardColumn = column;
        m_lastCreateBoardColumnUserId = userId;
        ++m_createBoardColumnCallCount;

        if (m_createBoardColumnCallback)
        {
            auto result = m_createBoardColumnCallback(column, userId);
            if (result.has_value() && result->id.has_value())
            {
                m_existingColumns[*result->id] = *result;
            }
            return result;
        }

        // Симуляция проверки прав
        if (userId != 1 && userId != 100)
        {
            return std::nullopt;
        }

        if (m_createBoardColumnResult.has_value())
        {
            dto::BoardColumn newColumn = *m_createBoardColumnResult;
            if (!newColumn.id.has_value())
            {
                newColumn.id = m_nextColumnId++;
            }
            m_existingColumns[*newColumn.id] = newColumn;
            return newColumn;
        }

        return std::nullopt;
    }

    std::optional<dto::BoardColumn> updateBoardColumn(
        const dto::BoardColumn& column,
        int64_t userId
    ) override
    {
        m_lastUpdatedBoardColumn = column;
        ++m_updateBoardColumnCallCount;

        if (m_updateBoardColumnCallback)
        {
            auto result = m_updateBoardColumnCallback(column, userId);
            if (result.has_value() && result->id.has_value())
            {
                m_existingColumns[*result->id] = *result;
            }
            return result;
        }

        // Симуляция проверки прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        // Проверяем, существует ли колонка
        if (!column.id.has_value())
        {
            return std::nullopt;
        }

        auto it = m_existingColumns.find(*column.id);
        if (it == m_existingColumns.end())
        {
            return std::nullopt;
        }

        // Обновляем существующую колонку
        dto::BoardColumn updated = it->second;
        if (column.boardId.has_value())
            updated.boardId = column.boardId;
        if (column.stateId.has_value())
            updated.stateId = column.stateId;
        if (column.orderNumber.has_value())
            updated.orderNumber = column.orderNumber;
        if (column.settings.has_value())
            updated.settings = column.settings;

        m_existingColumns[*column.id] = updated;
        return updated;
    }

    BoardColumnResult deleteBoardColumn(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedBoardColumnId = id;
        ++m_deleteBoardColumnCallCount;

        if (m_deleteBoardColumnCallback)
        {
            return m_deleteBoardColumnCallback(id, userId);
        }

        // Симуляция проверки прав
        if (userId == 999)
        {
            BoardColumnResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        // Проверяем, существует ли колонка
        auto it = m_existingColumns.find(id);
        if (it == m_existingColumns.end())
        {
            BoardColumnResult result;
            result.success = false;
            result.errorCode = 404;
            result.errorMessage = "Board column not found";
            return result;
        }

        // Удаляем колонку
        m_existingColumns.erase(it);

        BoardColumnResult result;
        result.success = true;
        return result;
    }

    int64_t deleteColumnsByBoard(
        int64_t boardId,
        int64_t userId
    ) override
    {
        m_lastDeleteColumnsByBoardId = boardId;
        ++m_deleteColumnsByBoardCallCount;

        if (m_deleteColumnsByBoardCallback)
        {
            return m_deleteColumnsByBoardCallback(boardId, userId);
        }

        if (userId == 999)
        {
            return 0;
        }

        int64_t deletedCount = 0;
        auto it = m_existingColumns.begin();
        while (it != m_existingColumns.end())
        {
            if (it->second.boardId.has_value() && *it->second.boardId == boardId)
            {
                it = m_existingColumns.erase(it);
                ++deletedCount;
            }
            else
            {
                ++it;
            }
        }

        return deletedCount;
    }

    // ============================================================
    // Методы для проверки вызовов
    // ============================================================

    int getGetBoardColumnsCallCount() const { return m_getBoardColumnsCallCount; }
    int getGetBoardColumnCallCount() const { return m_getBoardColumnCallCount; }
    int getGetColumnsByBoardCallCount() const { return m_getColumnsByBoardCallCount; }
    int getCreateBoardColumnCallCount() const { return m_createBoardColumnCallCount; }
    int getUpdateBoardColumnCallCount() const { return m_updateBoardColumnCallCount; }
    int getDeleteBoardColumnCallCount() const { return m_deleteBoardColumnCallCount; }
    int getDeleteColumnsByBoardCallCount() const { return m_deleteColumnsByBoardCallCount; }

    int getLastGetBoardColumnsPage() const { return m_lastGetBoardColumnsPage; }
    int getLastGetBoardColumnsPageSize() const { return m_lastGetBoardColumnsPageSize; }
    int64_t getLastGetBoardColumnsUserId() const { return m_lastGetBoardColumnsUserId; }
    std::optional<int64_t> getLastGetBoardColumnsBoardId() const { return m_lastGetBoardColumnsBoardId; }
    std::optional<int64_t> getLastGetBoardColumnsStateId() const { return m_lastGetBoardColumnsStateId; }
    int64_t getLastGetBoardColumnId() const { return m_lastGetBoardColumnId; }
    int64_t getLastGetBoardColumnUserId() const { return m_lastGetBoardColumnUserId; }
    int64_t getLastGetColumnsByBoardId() const { return m_lastGetColumnsByBoardId; }
    int64_t getLastCreateBoardColumnUserId() const { return m_lastCreateBoardColumnUserId; }
    const dto::BoardColumn& getLastCreatedBoardColumn() const { return m_lastCreatedBoardColumn; }
    const dto::BoardColumn& getLastUpdatedBoardColumn() const { return m_lastUpdatedBoardColumn; }
    int64_t getLastDeletedBoardColumnId() const { return m_lastDeletedBoardColumnId; }
    int64_t getLastDeleteColumnsByBoardId() const { return m_lastDeleteColumnsByBoardId; }

    void reset()
    {
        m_getBoardColumnsCallCount = 0;
        m_getBoardColumnCallCount = 0;
        m_getColumnsByBoardCallCount = 0;
        m_createBoardColumnCallCount = 0;
        m_updateBoardColumnCallCount = 0;
        m_deleteBoardColumnCallCount = 0;
        m_deleteColumnsByBoardCallCount = 0;

        m_lastGetBoardColumnsPage = 0;
        m_lastGetBoardColumnsPageSize = 0;
        m_lastGetBoardColumnsUserId = 0;
        m_lastGetBoardColumnsBoardId.reset();
        m_lastGetBoardColumnsStateId.reset();
        m_lastGetBoardColumnId = 0;
        m_lastGetBoardColumnUserId = 0;
        m_lastGetColumnsByBoardId = 0;
        m_lastCreateBoardColumnUserId = 0;
        m_lastCreatedBoardColumn = dto::BoardColumn {};
        m_lastUpdatedBoardColumn = dto::BoardColumn {};
        m_lastDeletedBoardColumnId = 0;
        m_lastDeleteColumnsByBoardId = 0;

        m_getBoardColumnsCallback = nullptr;
        m_getBoardColumnCallback = nullptr;
        m_getColumnsByBoardCallback = nullptr;
        m_createBoardColumnCallback = nullptr;
        m_updateBoardColumnCallback = nullptr;
        m_deleteBoardColumnCallback = nullptr;
        m_deleteColumnsByBoardCallback = nullptr;

        m_getBoardColumnsResult = BoardColumnsPage {};
        m_getBoardColumnResult = std::nullopt;
        m_getColumnsByBoardResult.clear();
        m_createBoardColumnResult = std::nullopt;
        m_updateBoardResult = std::nullopt;
        m_deleteBoardColumnResult = BoardColumnResult { false, 0, "" };
        m_deleteColumnsByBoardResult = 0;
        m_existingColumns.clear();
        m_nextColumnId = 100;
    }

    static dto::BoardColumn createTestColumn(
        int64_t id,
        int64_t boardId,
        int64_t stateId,
        int orderNumber = 1,
        const std::string& settings = ""
    )
    {
        dto::BoardColumn column;
        column.id = id;
        column.boardId = boardId;
        column.stateId = stateId;
        column.orderNumber = orderNumber;
        if (!settings.empty())
            column.settings = settings;
        return column;
    }

    static BoardColumnsPage createTestColumnsPage(
        const std::vector<dto::BoardColumn>& columns,
        int64_t totalCount = -1
    )
    {
        BoardColumnsPage page;
        page.columns = columns;
        page.totalCount = (totalCount >= 0) ? totalCount : static_cast<int64_t>(columns.size());
        return page;
    }

private:
    BoardColumnsPage m_getBoardColumnsResult;
    std::optional<dto::BoardColumn> m_getBoardColumnResult;
    std::vector<dto::BoardColumn> m_getColumnsByBoardResult;
    std::optional<dto::BoardColumn> m_createBoardColumnResult;
    std::optional<dto::BoardColumn> m_updateBoardResult;
    BoardColumnResult m_deleteBoardColumnResult;
    int64_t m_deleteColumnsByBoardResult = 0;
    std::unordered_map<int64_t, dto::BoardColumn> m_existingColumns;

    std::function<BoardColumnsPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>)> m_getBoardColumnsCallback;
    std::function<std::optional<dto::BoardColumn>(int64_t, int64_t)> m_getBoardColumnCallback;
    std::function<std::vector<dto::BoardColumn>(int64_t, int64_t)> m_getColumnsByBoardCallback;
    std::function<std::optional<dto::BoardColumn>(const dto::BoardColumn&, int64_t)> m_createBoardColumnCallback;
    std::function<std::optional<dto::BoardColumn>(const dto::BoardColumn&, int64_t)> m_updateBoardColumnCallback;
    std::function<BoardColumnResult(int64_t, int64_t)> m_deleteBoardColumnCallback;
    std::function<int64_t(int64_t, int64_t)> m_deleteColumnsByBoardCallback;

    int m_getBoardColumnsCallCount = 0;
    int m_getBoardColumnCallCount = 0;
    int m_getColumnsByBoardCallCount = 0;
    int m_createBoardColumnCallCount = 0;
    int m_updateBoardColumnCallCount = 0;
    int m_deleteBoardColumnCallCount = 0;
    int m_deleteColumnsByBoardCallCount = 0;

    int m_lastGetBoardColumnsPage = 0;
    int m_lastGetBoardColumnsPageSize = 0;
    int64_t m_lastGetBoardColumnsUserId = 0;
    std::optional<int64_t> m_lastGetBoardColumnsBoardId;
    std::optional<int64_t> m_lastGetBoardColumnsStateId;
    int64_t m_lastGetBoardColumnId = 0;
    int64_t m_lastGetBoardColumnUserId = 0;
    int64_t m_lastGetColumnsByBoardId = 0;
    int64_t m_lastCreateBoardColumnUserId = 0;
    dto::BoardColumn m_lastCreatedBoardColumn;
    dto::BoardColumn m_lastUpdatedBoardColumn;
    int64_t m_lastDeletedBoardColumnId = 0;
    int64_t m_lastDeleteColumnsByBoardId = 0;
    int64_t m_nextColumnId = 100;
};

} // namespace server::tests
