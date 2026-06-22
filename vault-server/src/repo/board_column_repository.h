#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/board_column.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с колонками досок.
 */
class IBoardColumnRepository
{
public:
    virtual ~IBoardColumnRepository() = default;

    /**
     * @brief Получает список колонок с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param boardId Фильтр по доске (std::nullopt - все)
     * @param stateId Фильтр по состоянию (std::nullopt - все)
     * @return Пара: вектор DTO колонок и общее количество
     */
    virtual std::pair<std::vector<dto::BoardColumn>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> boardId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) = 0;

    /**
     * @brief Находит колонку по ID.
     * @param id Идентификатор колонки
     * @return DTO колонки или std::nullopt, если не найдена
     */
    virtual std::optional<dto::BoardColumn> findById(int64_t id) = 0;

    /**
     * @brief Создает новую колонку доски.
     * @param column DTO колонки
     * @return ID созданной колонки или 0 при ошибке
     */
    virtual int64_t create(const dto::BoardColumn& column) = 0;

    /**
     * @brief Обновляет существующую колонку.
     * @param column DTO колонки с новыми данными. Поле id обязательно.
     * @return true если обновление успешно
     */
    virtual bool update(const dto::BoardColumn& column) = 0;

    /**
     * @brief Удаляет колонку по ID.
     * @param id Идентификатор колонки
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все колонки доски.
     * @param boardId Идентификатор доски
     * @return Количество удалённых колонок
     */
    virtual int64_t removeByBoardId(int64_t boardId) = 0;

    /**
     * @brief Проверяет существование колонки с указанным ID.
     * @param id Идентификатор колонки
     * @return true если колонка существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет существование колонки на доске с указанным состоянием.
     * @param boardId Идентификатор доски
     * @param stateId Идентификатор состояния
     * @return true если колонка существует
     */
    virtual bool existsByBoardAndState(int64_t boardId, int64_t stateId) = 0;

    /**
     * @brief Получает колонки, связанные с доской.
     * @param boardId Идентификатор доски
     * @param orderByOrderNumber Сортировать по orderNumber (по умолчанию true)
     * @return Вектор DTO колонок
     */
    virtual std::vector<dto::BoardColumn> findByBoardId(
        int64_t boardId,
        bool orderByOrderNumber = true
    ) = 0;

    /**
     * @brief Получает колонки, связанные с состоянием.
     * @param stateId Идентификатор состояния
     * @return Вектор DTO колонок
     */
    virtual std::vector<dto::BoardColumn> findByStateId(int64_t stateId) = 0;
};

} // namespace repositories
} // namespace server
