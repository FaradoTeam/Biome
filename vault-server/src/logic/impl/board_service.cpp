#include "common/log/log.h"

#include "board_service.h"

namespace server
{
namespace services
{

BoardService::BoardService(
    std::shared_ptr<repositories::IBoardRepository> boardRepo,
    std::shared_ptr<IProjectService> projectService,
    std::shared_ptr<IPhaseService> phaseService,
    std::shared_ptr<IWorkflowService> workflowService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_boardRepo(std::move(boardRepo))
    , m_projectService(std::move(projectService))
    , m_phaseService(std::move(phaseService))
    , m_workflowService(std::move(workflowService))
    , m_authzService(std::move(authzService))
{
    if (!m_boardRepo)
    {
        throw std::runtime_error("BoardService: репозиторий досок не инициализирован");
    }
    if (!m_projectService)
    {
        throw std::runtime_error("BoardService: сервис проектов не инициализирован");
    }
    if (!m_phaseService)
    {
        throw std::runtime_error("BoardService: сервис фаз не инициализирован");
    }
    if (!m_workflowService)
    {
        throw std::runtime_error("BoardService: сервис рабочих процессов не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("BoardService: сервис авторизации не инициализирован");
    }
}

BoardsPage BoardService::getBoards(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> projectId,
    std::optional<int64_t> phaseId,
    std::optional<int64_t> workflowId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан projectId, проверяем доступ
    if (projectId.has_value() && !checkProjectAccess(*projectId, userId, false))
    {
        LOG_WARN
            << "getBoards: пользователь " << userId
            << " не имеет доступа к проекту " << *projectId;
        return { {}, 0 };
    }

    // Если указан phaseId, проверяем доступ через фазу
    if (phaseId.has_value())
    {
        auto phase = m_phaseService->phase(*phaseId, userId);
        if (!phase.has_value() || !phase->projectId.has_value())
        {
            LOG_WARN
                << "getBoards: фаза " << *phaseId
                << " не найдена или недоступна";
            return { {}, 0 };
        }
        if (!checkProjectAccess(*phase->projectId, userId, false))
        {
            LOG_WARN
                << "getBoards: пользователь " << userId
                << " не имеет доступа к проекту фазы " << *phase->projectId;
            return { {}, 0 };
        }
    }

    // Если указан workflowId, проверяем его существование
    if (workflowId.has_value())
    {
        auto workflow = m_workflowService->workflow(*workflowId);
        if (!workflow.has_value())
        {
            LOG_WARN
                << "getBoards: рабочий процесс " << *workflowId
                << " не найден";
            return { {}, 0 };
        }
    }

    auto [boards, total] = m_boardRepo->findAll(
        page, pageSize, projectId, phaseId, workflowId
    );

    // Фильтруем по правам доступа
    auto filteredBoards = filterBoardsByAccess(boards, userId);

    return { filteredBoards, static_cast<int64_t>(filteredBoards.size()) };
}

std::optional<dto::Board> BoardService::getBoard(
    int64_t id,
    int64_t userId
)
{
    return checkBoardAccess(id, userId, false);
}

std::optional<dto::Board> BoardService::createBoard(
    const dto::Board& board,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateBoard(board, errorMessage))
    {
        LOG_WARN << "createBoard: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем существование проекта и права на редактирование досок
    if (!checkProjectAccess(*board.projectId, userId, true))
    {
        LOG_WARN
            << "createBoard: пользователь " << userId
            << " не имеет прав на редактирование досок в проекте "
            << *board.projectId;
        return std::nullopt;
    }

    // 3. Проверяем существование рабочего процесса
    auto workflow = m_workflowService->workflow(*board.workflowId);
    if (!workflow.has_value())
    {
        LOG_WARN
            << "createBoard: рабочий процесс " << *board.workflowId
            << " не найден";
        return std::nullopt;
    }

    // 4. Если указана фаза, проверяем её существование и принадлежность проекту
    if (board.phaseId.has_value())
    {
        auto phase = m_phaseService->phase(*board.phaseId, userId);
        if (!phase.has_value())
        {
            LOG_WARN
                << "createBoard: фаза " << *board.phaseId
                << " не найдена";
            return std::nullopt;
        }
        if (!phase->projectId.has_value() || *phase->projectId != *board.projectId)
        {
            LOG_WARN
                << "createBoard: фаза " << *board.phaseId
                << " не принадлежит проекту " << *board.projectId;
            return std::nullopt;
        }
    }

    // 5. Создаём доску
    const int64_t newId = m_boardRepo->create(board);
    if (newId <= 0)
    {
        LOG_ERROR << "createBoard: не удалось создать доску";
        return std::nullopt;
    }

    LOG_INFO
        << "Доска создана: id=" << newId
        << ", проект=" << *board.projectId
        << ", пользователь=" << userId;

    return m_boardRepo->findById(newId);
}

std::optional<dto::Board> BoardService::updateBoard(
    const dto::Board& board,
    int64_t userId
)
{
    if (!board.id.has_value())
    {
        LOG_WARN << "updateBoard: отсутствует ID доски";
        return std::nullopt;
    }

    // 1. Проверяем существование и доступ к доске
    auto existing = checkBoardAccess(*board.id, userId, true);
    if (!existing.has_value())
    {
        return std::nullopt;
    }

    // 2. Если меняется проект, проверяем права на новый проект
    if (board.projectId.has_value() && *board.projectId != *existing->projectId)
    {
        if (!checkProjectAccess(*board.projectId, userId, true))
        {
            LOG_WARN
                << "updateBoard: пользователь " << userId
                << " не имеет прав на редактирование досок в проекте "
                << *board.projectId;
            return std::nullopt;
        }
    }

    // 3. Если меняется рабочая процесса, проверяем его существование
    if (board.workflowId.has_value())
    {
        auto workflow = m_workflowService->workflow(*board.workflowId);
        if (!workflow.has_value())
        {
            LOG_WARN
                << "updateBoard: рабочий процесс " << *board.workflowId
                << " не найден";
            return std::nullopt;
        }
    }

    // 4. Если меняется фаза, проверяем её существование и принадлежность проекту
    if (board.phaseId.has_value())
    {
        int64_t projectId = board.projectId.has_value()
            ? *board.projectId
            : *existing->projectId;

        auto phase = m_phaseService->phase(*board.phaseId, userId);
        if (!phase.has_value())
        {
            LOG_WARN
                << "updateBoard: фаза " << *board.phaseId
                << " не найдена";
            return std::nullopt;
        }
        if (!phase->projectId.has_value() || *phase->projectId != projectId)
        {
            LOG_WARN
                << "updateBoard: фаза " << *board.phaseId
                << " не принадлежит проекту " << projectId;
            return std::nullopt;
        }
    }

    // 5. Обновляем доску
    if (!m_boardRepo->update(board))
    {
        LOG_ERROR << "updateBoard: не удалось обновить доску id=" << *board.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Доска обновлена: id=" << *board.id
        << ", пользователь=" << userId;

    return m_boardRepo->findById(*board.id);
}

BoardResult BoardService::deleteBoard(
    int64_t id,
    int64_t userId
)
{
    BoardResult result;

    // 1. Проверяем существование и доступ к доске
    auto existing = checkBoardAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Доска не найдена или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Удаляем доску (колонки будут удалены каскадно)
    if (!m_boardRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить доску";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Доска удалена: id=" << id
        << ", пользователь=" << userId;

    return result;
}

std::vector<dto::Board> BoardService::getBoardsByProject(
    int64_t projectId,
    int64_t userId
)
{
    if (!checkProjectAccess(projectId, userId, false))
    {
        LOG_WARN
            << "getBoardsByProject: пользователь " << userId
            << " не имеет доступа к проекту " << projectId;
        return {};
    }

    return m_boardRepo->findByProject(projectId);
}

std::vector<dto::Board> BoardService::getBoardsByPhase(
    int64_t phaseId,
    int64_t userId
)
{
    auto phase = m_phaseService->phase(phaseId, userId);
    if (!phase.has_value() || !phase->projectId.has_value())
    {
        LOG_WARN
            << "getBoardsByPhase: фаза " << phaseId
            << " не найдена или недоступна";
        return {};
    }

    if (!checkProjectAccess(*phase->projectId, userId, false))
    {
        LOG_WARN
            << "getBoardsByPhase: пользователь " << userId
            << " не имеет доступа к проекту фазы";
        return {};
    }

    return m_boardRepo->findByPhase(phaseId);
}

// ============================================================
// Приватные методы
// ============================================================

bool BoardService::checkProjectAccess(
    int64_t projectId,
    int64_t userId,
    bool needEdit
)
{
    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    // Проверяем через сервис проектов
    auto project = m_projectService->project(projectId, userId);
    if (!project.has_value())
    {
        LOG_DEBUG
            << "checkProjectAccess: проект " << projectId
            << " не найден или недоступен для пользователя " << userId;
        return false;
    }

    if (needEdit)
    {
        // Для редактирования досок нужно право BoardEditor
        auto authz = m_authzService->canEditBoards(userId, projectId);
        if (!authz.granted)
        {
            LOG_DEBUG
                << "checkProjectAccess: пользователь " << userId
                << " не имеет права BoardEditor в проекте " << projectId;
            return false;
        }
    }

    return true;
}

std::optional<dto::Board> BoardService::checkBoardAccess(
    int64_t boardId,
    int64_t userId,
    bool needEdit
)
{
    auto board = m_boardRepo->findById(boardId);
    if (!board.has_value())
    {
        LOG_DEBUG << "checkBoardAccess: доска " << boardId << " не найдена";
        return std::nullopt;
    }

    if (!board->projectId.has_value())
    {
        LOG_WARN
            << "checkBoardAccess: доска " << boardId
            << " не имеет projectId";
        return std::nullopt;
    }

    if (!checkProjectAccess(*board->projectId, userId, needEdit))
    {
        LOG_WARN
            << "checkBoardAccess: пользователь " << userId
            << " не имеет доступа к проекту " << *board->projectId;
        return std::nullopt;
    }

    return board;
}

bool BoardService::validateBoard(
    const dto::Board& board,
    std::string& errorMessage
)
{
    if (!board.caption.has_value() || board.caption->empty())
    {
        errorMessage = "Название доски обязательно для заполнения";
        return false;
    }

    if (board.caption->length() > 255)
    {
        errorMessage = "Название доски не может превышать 255 символов";
        return false;
    }

    if (!board.workflowId.has_value())
    {
        errorMessage = "Идентификатор рабочего процесса обязателен";
        return false;
    }

    if (!board.projectId.has_value())
    {
        errorMessage = "Идентификатор проекта обязателен";
        return false;
    }

    if (board.description.has_value() && board.description->length() > 1000)
    {
        errorMessage = "Описание доски не может превышать 1000 символов";
        return false;
    }

    return true;
}

std::vector<dto::Board> BoardService::filterBoardsByAccess(
    const std::vector<dto::Board>& boards,
    int64_t userId
)
{
    // Супер-админ видит все доски
    if (m_authzService->isSuperAdmin(userId))
    {
        return boards;
    }

    std::vector<dto::Board> filtered;
    for (const auto& board : boards)
    {
        if (!board.projectId.has_value())
            continue;

        if (checkProjectAccess(*board.projectId, userId, false))
        {
            filtered.push_back(board);
        }
    }

    return filtered;
}

} // namespace services
} // namespace server
