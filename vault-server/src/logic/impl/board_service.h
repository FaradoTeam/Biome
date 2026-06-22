#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iboard_service.h"
#include "logic/iphase_service.h"
#include "logic/iproject_service.h"
#include "logic/iworkflow_service.h"

#include "repo/board_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с досками.
 */
class BoardService final : public IBoardService
{
public:
    BoardService(
        std::shared_ptr<repositories::IBoardRepository> boardRepo,
        std::shared_ptr<IProjectService> projectService,
        std::shared_ptr<IPhaseService> phaseService,
        std::shared_ptr<IWorkflowService> workflowService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IBoardService
    BoardsPage getBoards(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> workflowId = std::nullopt
    ) override;

    std::optional<dto::Board> getBoard(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::Board> createBoard(
        const dto::Board& board,
        int64_t userId
    ) override;

    std::optional<dto::Board> updateBoard(
        const dto::Board& board,
        int64_t userId
    ) override;

    BoardResult deleteBoard(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::Board> getBoardsByProject(
        int64_t projectId,
        int64_t userId
    ) override;

    std::vector<dto::Board> getBoardsByPhase(
        int64_t phaseId,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к проекту.
     * @param projectId ID проекта
     * @param userId ID пользователя
     * @param needEdit Требуется ли право на редактирование
     * @return true если доступ разрешён
     */
    bool checkProjectAccess(
        int64_t projectId,
        int64_t userId,
        bool needEdit = false
    );

    /**
     * @brief Проверяет существование и доступ к доске.
     * @param boardId ID доски
     * @param userId ID пользователя
     * @param needEdit Требуется ли право на редактирование
     * @return DTO доски или std::nullopt
     */
    std::optional<dto::Board> checkBoardAccess(
        int64_t boardId,
        int64_t userId,
        bool needEdit = false
    );

    /**
     * @brief Валидирует DTO доски.
     * @param board DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateBoard(
        const dto::Board& board,
        std::string& errorMessage
    );

    /**
     * @brief Фильтрует доски по правам доступа.
     * @param boards Исходный список досок
     * @param userId ID пользователя
     * @return Отфильтрованный список досок
     */
    std::vector<dto::Board> filterBoardsByAccess(
        const std::vector<dto::Board>& boards,
        int64_t userId
    );

private:
    std::shared_ptr<repositories::IBoardRepository> m_boardRepo;
    std::shared_ptr<IProjectService> m_projectService;
    std::shared_ptr<IPhaseService> m_phaseService;
    std::shared_ptr<IWorkflowService> m_workflowService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
