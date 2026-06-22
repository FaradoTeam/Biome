#include "common/log/log.h"

#include "board_column_service.h"

namespace server
{
namespace services
{

BoardColumnService::BoardColumnService(
    std::shared_ptr<repositories::IBoardColumnRepository> columnRepo,
    std::shared_ptr<IBoardService> boardService,
    std::shared_ptr<IStateService> stateService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_columnRepo(std::move(columnRepo))
    , m_boardService(std::move(boardService))
    , m_stateService(std::move(stateService))
    , m_authzService(std::move(authzService))
{
    if (!m_columnRepo)
    {
        throw std::runtime_error("BoardColumnService: репозиторий колонок не инициализирован");
    }
    if (!m_boardService)
    {
        throw std::runtime_error("BoardColumnService: сервис досок не инициализирован");
    }
    if (!m_stateService)
    {
        throw std::runtime_error("BoardColumnService: сервис состояний не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("BoardColumnService: сервис авторизации не инициализирован");
    }
}

BoardColumnsPage BoardColumnService::getBoardColumns(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> boardId,
    std::optional<int64_t> stateId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан boardId, проверяем доступ к доске
    if (boardId.has_value())
    {
        auto board = m_boardService->getBoard(*boardId, userId);
        if (!board.has_value())
        {
            LOG_WARN
                << "getBoardColumns: доска " << *boardId
                << " не найдена или недоступна";
            return { {}, 0 };
        }
    }

    // Если указан stateId, проверяем его существование
    if (stateId.has_value())
    {
        auto state = m_stateService->state(*stateId);
        if (!state.has_value())
        {
            LOG_WARN
                << "getBoardColumns: состояние " << *stateId
                << " не найдено";
            return { {}, 0 };
        }
    }

    auto [columns, total] = m_columnRepo->findAll(page, pageSize, boardId, stateId);

    // Фильтруем по правам доступа к доскам
    std::vector<dto::BoardColumn> filtered;
    for (const auto& column : columns)
    {
        if (!column.boardId.has_value())
            continue;

        auto board = m_boardService->getBoard(*column.boardId, userId);
        if (board.has_value())
        {
            filtered.push_back(column);
        }
    }

    return { filtered, static_cast<int64_t>(filtered.size()) };
}

std::optional<dto::BoardColumn> BoardColumnService::getBoardColumn(
    int64_t id,
    int64_t userId
)
{
    return checkColumnAccess(id, userId, false);
}

std::vector<dto::BoardColumn> BoardColumnService::getColumnsByBoard(
    int64_t boardId,
    int64_t userId
)
{
    // Проверяем доступ к доске
    auto board = m_boardService->getBoard(boardId, userId);
    if (!board.has_value())
    {
        LOG_WARN
            << "getColumnsByBoard: доска " << boardId
            << " не найдена или недоступна";
        return {};
    }

    return m_columnRepo->findByBoardId(boardId);
}

std::optional<dto::BoardColumn> BoardColumnService::createBoardColumn(
    const dto::BoardColumn& column,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateColumn(column, errorMessage))
    {
        LOG_WARN << "createBoardColumn: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем доступ к доске (требуется право на редактирование досок)
    auto board = checkBoardAccess(*column.boardId, userId, true);
    if (!board.has_value())
    {
        return std::nullopt;
    }

    // 3. Проверяем существование состояния
    auto state = m_stateService->state(*column.stateId);
    if (!state.has_value())
    {
        LOG_WARN
            << "createBoardColumn: состояние " << *column.stateId
            << " не найдено";
        return std::nullopt;
    }

    // 4. Проверяем, что состояние принадлежит тому же рабочему процессу, что и доска
    if (!board->workflowId.has_value() || !state->workflowId.has_value() || *board->workflowId != *state->workflowId)
    {
        LOG_WARN
            << "createBoardColumn: состояние " << *column.stateId
            << " не принадлежит рабочему процессу доски " << *board->workflowId;
        return std::nullopt;
    }

    // 5. Проверяем уникальность колонки на доске
    if (!isColumnUnique(*column.boardId, *column.stateId))
    {
        LOG_WARN
            << "createBoardColumn: колонка с состоянием " << *column.stateId
            << " уже существует на доске " << *column.boardId;
        return std::nullopt;
    }

    // 6. Создаём колонку
    const int64_t newId = m_columnRepo->create(column);
    if (newId <= 0)
    {
        LOG_ERROR << "createBoardColumn: не удалось создать колонку";
        return std::nullopt;
    }

    LOG_INFO
        << "Колонка доски создана: id=" << newId
        << ", доска=" << *column.boardId
        << ", состояние=" << *column.stateId
        << ", пользователь=" << userId;

    return m_columnRepo->findById(newId);
}

std::optional<dto::BoardColumn> BoardColumnService::updateBoardColumn(
    const dto::BoardColumn& column,
    int64_t userId
)
{
    if (!column.id.has_value())
    {
        LOG_WARN << "updateBoardColumn: отсутствует ID колонки";
        return std::nullopt;
    }

    // 1. Проверяем существование и доступ к колонке
    auto existing = checkColumnAccess(*column.id, userId, true);
    if (!existing.has_value())
    {
        return std::nullopt;
    }

    // 2. Проверяем уникальность если меняется boardId или stateId
    int64_t boardId = column.boardId.has_value()
        ? *column.boardId
        : *existing->boardId;
    int64_t stateId = column.stateId.has_value()
        ? *column.stateId
        : *existing->stateId;

    if (!isColumnUnique(boardId, stateId, *column.id))
    {
        LOG_WARN
            << "updateBoardColumn: колонка с состоянием " << stateId
            << " уже существует на доске " << boardId;
        return std::nullopt;
    }

    // 3. Если меняется доска, проверяем доступ к новой доске
    if (column.boardId.has_value() && *column.boardId != *existing->boardId)
    {
        auto newBoard = checkBoardAccess(*column.boardId, userId, true);
        if (!newBoard.has_value())
        {
            return std::nullopt;
        }

        // Проверяем, что состояние принадлежит рабочему процессу новой доски
        auto state = m_stateService->state(
            column.stateId.has_value() ? *column.stateId : *existing->stateId
        );
        if (state.has_value() && newBoard->workflowId.has_value() && state->workflowId.has_value() && *newBoard->workflowId != *state->workflowId)
        {
            LOG_WARN
                << "updateBoardColumn: состояние не принадлежит рабочему процессу новой доски";
            return std::nullopt;
        }
    }

    // 4. Если меняется состояние, проверяем его существование и принадлежность рабочему процессу
    if (column.stateId.has_value() && *column.stateId != *existing->stateId)
    {
        auto state = m_stateService->state(*column.stateId);
        if (!state.has_value())
        {
            LOG_WARN
                << "updateBoardColumn: состояние " << *column.stateId
                << " не найдено";
            return std::nullopt;
        }

        // Получаем доску
        auto board = m_boardService->getBoard(boardId, userId);
        if (board.has_value() && board->workflowId.has_value() && state->workflowId.has_value() && *board->workflowId != *state->workflowId)
        {
            LOG_WARN
                << "updateBoardColumn: состояние " << *column.stateId
                << " не принадлежит рабочему процессу доски";
            return std::nullopt;
        }
    }

    // 5. Обновляем колонку
    if (!m_columnRepo->update(column))
    {
        LOG_ERROR
            << "updateBoardColumn: не удалось обновить колонку id="
            << *column.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Колонка доски обновлена: id=" << *column.id
        << ", пользователь=" << userId;

    return m_columnRepo->findById(*column.id);
}

BoardColumnResult BoardColumnService::deleteBoardColumn(
    int64_t id,
    int64_t userId
)
{
    BoardColumnResult result;

    // 1. Проверяем существование и доступ к колонке
    auto existing = checkColumnAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Колонка не найдена или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Удаляем колонку
    if (!m_columnRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить колонку";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Колонка доски удалена: id=" << id
        << ", пользователь=" << userId;

    return result;
}

int64_t BoardColumnService::deleteColumnsByBoard(
    int64_t boardId,
    int64_t userId
)
{
    // Проверяем доступ к доске
    auto board = m_boardService->getBoard(boardId, userId);
    if (!board.has_value())
    {
        LOG_WARN
            << "deleteColumnsByBoard: доска " << boardId
            << " не найдена или недоступна";
        return 0;
    }

    // Проверяем право на редактирование досок
    auto authz = m_authzService->canEditBoards(userId, *board->projectId);
    if (!authz.granted)
    {
        LOG_WARN
            << "deleteColumnsByBoard: пользователь " << userId
            << " не имеет права на редактирование досок в проекте "
            << *board->projectId;
        return 0;
    }

    return m_columnRepo->removeByBoardId(boardId);
}

// ============================================================
// Приватные методы
// ============================================================

std::optional<dto::Board> BoardColumnService::checkBoardAccess(
    int64_t boardId,
    int64_t userId,
    bool needEdit
)
{
    auto board = m_boardService->getBoard(boardId, userId);
    if (!board.has_value())
    {
        LOG_DEBUG
            << "checkBoardAccess: доска " << boardId
            << " не найдена или недоступна";
        return std::nullopt;
    }

    if (needEdit)
    {
        if (!board->projectId.has_value())
        {
            LOG_WARN
                << "checkBoardAccess: доска " << boardId
                << " не имеет projectId";
            return std::nullopt;
        }

        auto authz = m_authzService->canEditBoards(userId, *board->projectId);
        if (!authz.granted)
        {
            LOG_WARN
                << "checkBoardAccess: пользователь " << userId
                << " не имеет права на редактирование досок в проекте "
                << *board->projectId;
            return std::nullopt;
        }
    }

    return board;
}

std::optional<dto::BoardColumn> BoardColumnService::checkColumnAccess(
    int64_t columnId,
    int64_t userId,
    bool needEdit
)
{
    auto column = m_columnRepo->findById(columnId);
    if (!column.has_value())
    {
        LOG_DEBUG << "checkColumnAccess: колонка " << columnId << " не найдена";
        return std::nullopt;
    }

    if (!column->boardId.has_value())
    {
        LOG_WARN
            << "checkColumnAccess: колонка " << columnId
            << " не имеет boardId";
        return std::nullopt;
    }

    auto board = checkBoardAccess(*column->boardId, userId, needEdit);
    if (!board.has_value())
    {
        return std::nullopt;
    }

    return column;
}

bool BoardColumnService::validateColumn(
    const dto::BoardColumn& column,
    std::string& errorMessage
)
{
    if (!column.boardId.has_value())
    {
        errorMessage = "Идентификатор доски обязателен";
        return false;
    }

    if (!column.stateId.has_value())
    {
        errorMessage = "Идентификатор состояния обязателен";
        return false;
    }

    if (!column.orderNumber.has_value())
    {
        errorMessage = "Порядковый номер обязателен";
        return false;
    }

    if (column.orderNumber < 0)
    {
        errorMessage = "Порядковый номер не может быть отрицательным";
        return false;
    }

    if (column.settings.has_value() && column.settings->length() > 4096)
    {
        errorMessage = "Настройки не могут превышать 4096 символов";
        return false;
    }

    return true;
}

bool BoardColumnService::isColumnUnique(
    int64_t boardId,
    int64_t stateId,
    std::optional<int64_t> excludeColumnId
)
{
    // Проверяем, существует ли колонка с такой парой (boardId, stateId)
    if (m_columnRepo->existsByBoardAndState(boardId, stateId))
    {
        // Если есть ID для исключения, проверяем, не является ли найденная колонка той же
        if (excludeColumnId.has_value())
        {
            auto existing = m_columnRepo->findByBoardId(boardId);
            for (const auto& col : existing)
            {
                if (*col.stateId == stateId && *col.id != *excludeColumnId)
                {
                    return false; // Найдена другая колонка с таким же состоянием
                }
            }
            return true; // Найдена только исключаемая колонка
        }
        return false; // Колонка существует и не должна быть исключена
    }
    return true; // Колонка уникальна
}

} // namespace services
} // namespace server
