#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iboard_column_service.h"
#include "logic/iboard_service.h"
#include "logic/istate_service.h"

#include "repo/board_column_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с колонками досок.
 */
class BoardColumnService final : public IBoardColumnService
{
public:
    BoardColumnService(
        std::shared_ptr<repositories::IBoardColumnRepository> columnRepo,
        std::shared_ptr<IBoardService> boardService,
        std::shared_ptr<IStateService> stateService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IBoardColumnService
    BoardColumnsPage getBoardColumns(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> boardId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) override;

    std::optional<dto::BoardColumn> getBoardColumn(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::BoardColumn> getColumnsByBoard(
        int64_t boardId,
        int64_t userId
    ) override;

    std::optional<dto::BoardColumn> createBoardColumn(
        const dto::BoardColumn& column,
        int64_t userId
    ) override;

    std::optional<dto::BoardColumn> updateBoardColumn(
        const dto::BoardColumn& column,
        int64_t userId
    ) override;

    BoardColumnResult deleteBoardColumn(
        int64_t id,
        int64_t userId
    ) override;

    int64_t deleteColumnsByBoard(
        int64_t boardId,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к доске.
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
     * @brief Проверяет существование и доступ к колонке.
     * @param columnId ID колонки
     * @param userId ID пользователя
     * @param needEdit Требуется ли право на редактирование
     * @return DTO колонки или std::nullopt
     */
    std::optional<dto::BoardColumn> checkColumnAccess(
        int64_t columnId,
        int64_t userId,
        bool needEdit = false
    );

    /**
     * @brief Валидирует DTO колонки.
     * @param column DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateColumn(
        const dto::BoardColumn& column,
        std::string& errorMessage
    );

    /**
     * @brief Проверяет уникальность колонки на доске.
     * @param boardId ID доски
     * @param stateId ID состояния
     * @param excludeColumnId ID колонки для исключения (при обновлении)
     * @return true если колонка уникальна
     */
    bool isColumnUnique(
        int64_t boardId,
        int64_t stateId,
        std::optional<int64_t> excludeColumnId = std::nullopt
    );

private:
    std::shared_ptr<repositories::IBoardColumnRepository> m_columnRepo;
    std::shared_ptr<IBoardService> m_boardService;
    std::shared_ptr<IStateService> m_stateService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
