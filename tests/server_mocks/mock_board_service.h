#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/board.h"
#include "common/dto/board_column.h"

#include "logic/iboard_service.h"

namespace server::tests
{

class MockBoardService : public services::IBoardService
{
public:
    using BoardsPage = services::BoardsPage;
    using BoardResult = services::BoardResult;

    MockBoardService() = default;
    ~MockBoardService() override = default;

    // ============================================================
    // Настройка результатов (простой режим)
    // ============================================================

    void setGetBoardsResult(const BoardsPage& result)
    {
        m_getBoardsResult = result;
        m_getBoardsCallback = nullptr;
    }

    void setGetBoardResult(std::optional<dto::Board> board)
    {
        m_getBoardResult = std::move(board);
        m_getBoardCallback = nullptr;
    }

    void setCreateBoardResult(std::optional<dto::Board> board)
    {
        m_createBoardResult = std::move(board);
        m_createBoardCallback = nullptr;
    }

    void setUpdateBoardResult(std::optional<dto::Board> board)
    {
        m_updateBoardResult = std::move(board);
        m_updateBoardCallback = nullptr;
    }

    void setDeleteBoardResult(const BoardResult& result)
    {
        m_deleteBoardResult = result;
        m_deleteBoardCallback = nullptr;
    }

    void setGetBoardsByProjectResult(const std::vector<dto::Board>& boards)
    {
        m_getBoardsByProjectResult = boards;
        m_getBoardsByProjectCallback = nullptr;
    }

    void setGetBoardsByPhaseResult(const std::vector<dto::Board>& boards)
    {
        m_getBoardsByPhaseResult = boards;
        m_getBoardsByPhaseCallback = nullptr;
    }

    // ============================================================
    // Настройка callback'ов для кастомной логики
    // ============================================================

    void setGetBoardsCallback(
        std::function<BoardsPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>)> callback
    )
    {
        m_getBoardsCallback = std::move(callback);
    }

    void setGetBoardCallback(
        std::function<std::optional<dto::Board>(int64_t, int64_t)> callback
    )
    {
        m_getBoardCallback = std::move(callback);
    }

    void setCreateBoardCallback(
        std::function<std::optional<dto::Board>(const dto::Board&, int64_t)> callback
    )
    {
        m_createBoardCallback = std::move(callback);
    }

    void setUpdateBoardCallback(
        std::function<std::optional<dto::Board>(const dto::Board&, int64_t)> callback
    )
    {
        m_updateBoardCallback = std::move(callback);
    }

    void setDeleteBoardCallback(
        std::function<BoardResult(int64_t, int64_t)> callback
    )
    {
        m_deleteBoardCallback = std::move(callback);
    }

    void setGetBoardsByProjectCallback(
        std::function<std::vector<dto::Board>(int64_t, int64_t)> callback
    )
    {
        m_getBoardsByProjectCallback = std::move(callback);
    }

    void setGetBoardsByPhaseCallback(
        std::function<std::vector<dto::Board>(int64_t, int64_t)> callback
    )
    {
        m_getBoardsByPhaseCallback = std::move(callback);
    }

    // ============================================================
    // Реализация интерфейса IBoardService
    // ============================================================

    BoardsPage getBoards(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> workflowId = std::nullopt
    ) override
    {
        m_lastGetBoardsPage = page;
        m_lastGetBoardsPageSize = pageSize;
        m_lastGetBoardsUserId = userId;
        m_lastGetBoardsProjectId = projectId;
        m_lastGetBoardsPhaseId = phaseId;
        m_lastGetBoardsWorkflowId = workflowId;
        ++m_getBoardsCallCount;

        if (m_getBoardsCallback)
        {
            return m_getBoardsCallback(page, pageSize, userId, projectId, phaseId, workflowId);
        }

        // Фильтрация результатов если указаны фильтры
        if (projectId.has_value() || phaseId.has_value() || workflowId.has_value())
        {
            BoardsPage filtered;
            for (const auto& board : m_getBoardsResult.boards)
            {
                bool match = true;
                if (projectId.has_value() && (!board.projectId.has_value() || *board.projectId != *projectId))
                    match = false;
                if (phaseId.has_value() && (!board.phaseId.has_value() || *board.phaseId != *phaseId))
                    match = false;
                if (workflowId.has_value() && (!board.workflowId.has_value() || *board.workflowId != *workflowId))
                    match = false;
                if (match)
                    filtered.boards.push_back(board);
            }
            filtered.totalCount = filtered.boards.size();
            return filtered;
        }

        return m_getBoardsResult;
    }

    std::optional<dto::Board> getBoard(int64_t id, int64_t userId) override
    {
        m_lastGetBoardId = id;
        m_lastGetBoardUserId = userId;
        ++m_getBoardCallCount;

        if (m_getBoardCallback)
        {
            return m_getBoardCallback(id, userId);
        }

        // Симуляция проверки прав: пользователь 999 не имеет доступа
        if (userId == 999)
        {
            return std::nullopt;
        }

        if (m_getBoardResult.has_value() && m_getBoardResult->id.has_value())
        {
            if (*m_getBoardResult->id == id)
            {
                return m_getBoardResult;
            }
        }
        return std::nullopt;
    }

    std::optional<dto::Board> createBoard(
        const dto::Board& board,
        int64_t userId
    ) override
    {
        m_lastCreatedBoard = board;
        m_lastCreateBoardUserId = userId;
        ++m_createBoardCallCount;

        if (m_createBoardCallback)
        {
            return m_createBoardCallback(board, userId);
        }

        // Симуляция проверки прав: только пользователь 1 (супер-админ) или 100 (с правами)
        if (userId != 1 && userId != 100)
        {
            return std::nullopt;
        }

        if (m_createBoardResult.has_value() && !m_createBoardResult->id.has_value())
        {
            m_createBoardResult->id = m_nextBoardId++;
        }

        return m_createBoardResult;
    }

    std::optional<dto::Board> updateBoard(
        const dto::Board& board,
        int64_t userId
    ) override
    {
        m_lastUpdatedBoard = board;
        m_lastUpdateBoardUserId = userId;
        ++m_updateBoardCallCount;

        if (m_updateBoardCallback)
        {
            return m_updateBoardCallback(board, userId);
        }

        // Симуляция проверки прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        if (m_updateBoardResult.has_value() && m_updateBoardResult->id.has_value())
        {
            if (board.id.has_value() && *m_updateBoardResult->id == *board.id)
            {
                return m_updateBoardResult;
            }
        }

        return std::nullopt;
    }

    BoardResult deleteBoard(int64_t id, int64_t userId) override
    {
        m_lastDeletedBoardId = id;
        m_lastDeleteBoardUserId = userId;
        ++m_deleteBoardCallCount;

        if (m_deleteBoardCallback)
        {
            return m_deleteBoardCallback(id, userId);
        }

        // Симуляция проверки прав
        if (userId == 999)
        {
            BoardResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        return m_deleteBoardResult;
    }

    std::vector<dto::Board> getBoardsByProject(
        int64_t projectId,
        int64_t userId
    ) override
    {
        m_lastGetBoardsByProjectId = projectId;
        ++m_getBoardsByProjectCallCount;

        if (m_getBoardsByProjectCallback)
        {
            return m_getBoardsByProjectCallback(projectId, userId);
        }

        if (userId == 999)
        {
            return {};
        }

        return m_getBoardsByProjectResult;
    }

    std::vector<dto::Board> getBoardsByPhase(
        int64_t phaseId,
        int64_t userId
    ) override
    {
        m_lastGetBoardsByPhaseId = phaseId;
        ++m_getBoardsByPhaseCallCount;

        if (m_getBoardsByPhaseCallback)
        {
            return m_getBoardsByPhaseCallback(phaseId, userId);
        }

        if (userId == 999)
        {
            return {};
        }

        return m_getBoardsByPhaseResult;
    }

    // ============================================================
    // Методы для проверки вызовов
    // ============================================================

    int getGetBoardsCallCount() const { return m_getBoardsCallCount; }
    int getGetBoardCallCount() const { return m_getBoardCallCount; }
    int getCreateBoardCallCount() const { return m_createBoardCallCount; }
    int getUpdateBoardCallCount() const { return m_updateBoardCallCount; }
    int getDeleteBoardCallCount() const { return m_deleteBoardCallCount; }
    int getGetBoardsByProjectCallCount() const { return m_getBoardsByProjectCallCount; }
    int getGetBoardsByPhaseCallCount() const { return m_getBoardsByPhaseCallCount; }

    int getLastGetBoardsPage() const { return m_lastGetBoardsPage; }
    int getLastGetBoardsPageSize() const { return m_lastGetBoardsPageSize; }
    int64_t getLastGetBoardsUserId() const { return m_lastGetBoardsUserId; }
    std::optional<int64_t> getLastGetBoardsProjectId() const { return m_lastGetBoardsProjectId; }
    std::optional<int64_t> getLastGetBoardsPhaseId() const { return m_lastGetBoardsPhaseId; }
    std::optional<int64_t> getLastGetBoardsWorkflowId() const { return m_lastGetBoardsWorkflowId; }
    int64_t getLastGetBoardId() const { return m_lastGetBoardId; }
    int64_t getLastGetBoardUserId() const { return m_lastGetBoardUserId; }
    const dto::Board& getLastCreatedBoard() const { return m_lastCreatedBoard; }
    int64_t getLastCreateBoardUserId() const { return m_lastCreateBoardUserId; }
    const dto::Board& getLastUpdatedBoard() const { return m_lastUpdatedBoard; }
    int64_t getLastUpdateBoardUserId() const { return m_lastUpdateBoardUserId; }
    int64_t getLastDeletedBoardId() const { return m_lastDeletedBoardId; }
    int64_t getLastDeleteBoardUserId() const { return m_lastDeleteBoardUserId; }
    int64_t getLastGetBoardsByProjectId() const { return m_lastGetBoardsByProjectId; }
    int64_t getLastGetBoardsByPhaseId() const { return m_lastGetBoardsByPhaseId; }

    void reset()
    {
        m_getBoardsCallCount = 0;
        m_getBoardCallCount = 0;
        m_createBoardCallCount = 0;
        m_updateBoardCallCount = 0;
        m_deleteBoardCallCount = 0;
        m_getBoardsByProjectCallCount = 0;
        m_getBoardsByPhaseCallCount = 0;

        m_lastGetBoardsPage = 0;
        m_lastGetBoardsPageSize = 0;
        m_lastGetBoardsUserId = 0;
        m_lastGetBoardsProjectId.reset();
        m_lastGetBoardsPhaseId.reset();
        m_lastGetBoardsWorkflowId.reset();
        m_lastGetBoardId = 0;
        m_lastGetBoardUserId = 0;
        m_lastCreatedBoard = dto::Board {};
        m_lastCreateBoardUserId = 0;
        m_lastUpdatedBoard = dto::Board {};
        m_lastUpdateBoardUserId = 0;
        m_lastDeletedBoardId = 0;
        m_lastDeleteBoardUserId = 0;
        m_lastGetBoardsByProjectId = 0;
        m_lastGetBoardsByPhaseId = 0;

        m_getBoardsCallback = nullptr;
        m_getBoardCallback = nullptr;
        m_createBoardCallback = nullptr;
        m_updateBoardCallback = nullptr;
        m_deleteBoardCallback = nullptr;
        m_getBoardsByProjectCallback = nullptr;
        m_getBoardsByPhaseCallback = nullptr;

        m_getBoardsResult = BoardsPage {};
        m_getBoardResult = std::nullopt;
        m_createBoardResult = std::nullopt;
        m_updateBoardResult = std::nullopt;
        m_deleteBoardResult = BoardResult { false, 0, "" };
        m_getBoardsByProjectResult.clear();
        m_getBoardsByPhaseResult.clear();
        m_nextBoardId = 100;
    }

    static dto::Board createTestBoard(
        int64_t id,
        const std::string& caption,
        int64_t projectId,
        int64_t workflowId = 1,
        std::optional<int64_t> phaseId = std::nullopt
    )
    {
        dto::Board board;
        board.id = id;
        board.caption = caption;
        board.projectId = projectId;
        board.workflowId = workflowId;
        if (phaseId.has_value())
            board.phaseId = phaseId;
        board.description = "Test description for " + caption;
        return board;
    }

    static BoardsPage createTestBoardsPage(
        const std::vector<dto::Board>& boards,
        int64_t totalCount = -1
    )
    {
        BoardsPage page;
        page.boards = boards;
        page.totalCount = (totalCount >= 0) ? totalCount : static_cast<int64_t>(boards.size());
        return page;
    }

private:
    BoardsPage m_getBoardsResult;
    std::optional<dto::Board> m_getBoardResult;
    std::optional<dto::Board> m_createBoardResult;
    std::optional<dto::Board> m_updateBoardResult;
    BoardResult m_deleteBoardResult;
    std::vector<dto::Board> m_getBoardsByProjectResult;
    std::vector<dto::Board> m_getBoardsByPhaseResult;

    std::function<BoardsPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>)> m_getBoardsCallback;
    std::function<std::optional<dto::Board>(int64_t, int64_t)> m_getBoardCallback;
    std::function<std::optional<dto::Board>(const dto::Board&, int64_t)> m_createBoardCallback;
    std::function<std::optional<dto::Board>(const dto::Board&, int64_t)> m_updateBoardCallback;
    std::function<BoardResult(int64_t, int64_t)> m_deleteBoardCallback;
    std::function<std::vector<dto::Board>(int64_t, int64_t)> m_getBoardsByProjectCallback;
    std::function<std::vector<dto::Board>(int64_t, int64_t)> m_getBoardsByPhaseCallback;

    int m_getBoardsCallCount = 0;
    int m_getBoardCallCount = 0;
    int m_createBoardCallCount = 0;
    int m_updateBoardCallCount = 0;
    int m_deleteBoardCallCount = 0;
    int m_getBoardsByProjectCallCount = 0;
    int m_getBoardsByPhaseCallCount = 0;

    int m_lastGetBoardsPage = 0;
    int m_lastGetBoardsPageSize = 0;
    int64_t m_lastGetBoardsUserId = 0;
    std::optional<int64_t> m_lastGetBoardsProjectId;
    std::optional<int64_t> m_lastGetBoardsPhaseId;
    std::optional<int64_t> m_lastGetBoardsWorkflowId;
    int64_t m_lastGetBoardId = 0;
    int64_t m_lastGetBoardUserId = 0;
    dto::Board m_lastCreatedBoard;
    int64_t m_lastCreateBoardUserId = 0;
    dto::Board m_lastUpdatedBoard;
    int64_t m_lastUpdateBoardUserId = 0;
    int64_t m_lastDeletedBoardId = 0;
    int64_t m_lastDeleteBoardUserId = 0;
    int64_t m_lastGetBoardsByProjectId = 0;
    int64_t m_lastGetBoardsByPhaseId = 0;
    int64_t m_nextBoardId = 100;
};

} // namespace server::tests
