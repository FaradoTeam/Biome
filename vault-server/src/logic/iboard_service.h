#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/board.h"
#include "common/dto/board_column.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с доской.
 */
struct BoardResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с досками.
 */
struct BoardsPage
{
    std::vector<dto::Board> boards;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с досками.
 */
class IBoardService
{
public:
    virtual ~IBoardService() = default;

    /**
     * @brief Получает список досок с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param projectId Фильтр по проекту (опционально)
     * @param phaseId Фильтр по фазе (опционально)
     * @param workflowId Фильтр по рабочему процессу (опционально)
     * @return Страница с досками
     */
    virtual BoardsPage getBoards(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> workflowId = std::nullopt
    ) = 0;

    /**
     * @brief Получает доску по ID.
     * @param id Идентификатор доски
     * @param userId ID пользователя для проверки прав
     * @return DTO доски или std::nullopt
     */
    virtual std::optional<dto::Board> getBoard(int64_t id, int64_t userId) = 0;

    /**
     * @brief Создаёт новую доску.
     * @param board DTO доски
     * @param userId ID пользователя для проверки прав
     * @return Созданная доска или std::nullopt при ошибке
     */
    virtual std::optional<dto::Board> createBoard(
        const dto::Board& board,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующую доску.
     * @param board DTO доски с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённая доска или std::nullopt при ошибке
     */
    virtual std::optional<dto::Board> updateBoard(
        const dto::Board& board,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет доску.
     * @param id Идентификатор доски
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual BoardResult deleteBoard(int64_t id, int64_t userId) = 0;

    /**
     * @brief Получает доски для проекта.
     * @param projectId Идентификатор проекта
     * @param userId ID пользователя для проверки прав
     * @return Вектор досок
     */
    virtual std::vector<dto::Board> getBoardsByProject(
        int64_t projectId,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает доски для фазы.
     * @param phaseId Идентификатор фазы
     * @param userId ID пользователя для проверки прав
     * @return Вектор досок
     */
    virtual std::vector<dto::Board> getBoardsByPhase(
        int64_t phaseId,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
