#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/board_column.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с колонкой доски.
 */
struct BoardColumnResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с колонками досок.
 */
struct BoardColumnsPage
{
    std::vector<dto::BoardColumn> columns;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с колонками досок.
 */
class IBoardColumnService
{
public:
    virtual ~IBoardColumnService() = default;

    /**
     * @brief Получает список колонок с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param boardId Фильтр по доске (опционально)
     * @param stateId Фильтр по состоянию (опционально)
     * @return Страница с колонками
     */
    virtual BoardColumnsPage getBoardColumns(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> boardId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) = 0;

    /**
     * @brief Получает колонку по ID.
     * @param id Идентификатор колонки
     * @param userId ID пользователя для проверки прав
     * @return DTO колонки или std::nullopt
     */
    virtual std::optional<dto::BoardColumn> getBoardColumn(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все колонки для доски.
     * @param boardId Идентификатор доски
     * @param userId ID пользователя для проверки прав
     * @return Вектор колонок
     */
    virtual std::vector<dto::BoardColumn> getColumnsByBoard(
        int64_t boardId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую колонку доски.
     * @param column DTO колонки
     * @param userId ID пользователя для проверки прав
     * @return Созданная колонка или std::nullopt при ошибке
     */
    virtual std::optional<dto::BoardColumn> createBoardColumn(
        const dto::BoardColumn& column,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующую колонку.
     * @param column DTO колонки с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённая колонка или std::nullopt при ошибке
     */
    virtual std::optional<dto::BoardColumn> updateBoardColumn(
        const dto::BoardColumn& column,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет колонку доски.
     * @param id Идентификатор колонки
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual BoardColumnResult deleteBoardColumn(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет все колонки доски.
     * @param boardId Идентификатор доски
     * @param userId ID пользователя для проверки прав
     * @return Количество удалённых колонок
     */
    virtual int64_t deleteColumnsByBoard(int64_t boardId, int64_t userId) = 0;
};

} // namespace services
} // namespace server
