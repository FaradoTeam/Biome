#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/board.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с досками (Kanban Boards).
 */
class IBoardRepository
{
public:
    virtual ~IBoardRepository() = default;

    /**
     * @brief Получает список досок с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param projectId Фильтр по проекту (std::nullopt - все)
     * @param phaseId Фильтр по фазе (std::nullopt - все)
     * @param workflowId Фильтр по рабочему процессу (std::nullopt - все)
     * @return Пара: вектор DTO досок и общее количество
     */
    virtual std::pair<std::vector<dto::Board>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> workflowId = std::nullopt
    ) = 0;

    /**
     * @brief Находит доску по ID.
     * @param id Идентификатор доски
     * @return DTO доски или std::nullopt, если не найдена
     */
    virtual std::optional<dto::Board> findById(int64_t id) = 0;

    /**
     * @brief Создает новую доску.
     * @param board DTO доски
     * @return ID созданной доски или 0 при ошибке
     */
    virtual int64_t create(const dto::Board& board) = 0;

    /**
     * @brief Обновляет существующую доску.
     * @param board DTO доски с новыми данными. Поле id обязательно.
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Board& board) = 0;

    /**
     * @brief Удаляет доску по ID.
     * @param id Идентификатор доски
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование доски с указанным ID.
     * @param id Идентификатор доски
     * @return true если доска существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Получает доски, связанные с проектом.
     * @param projectId Идентификатор проекта
     * @return Вектор DTO досок
     */
    virtual std::vector<dto::Board> findByProject(int64_t projectId) = 0;

    /**
     * @brief Получает доски, связанные с фазой.
     * @param phaseId Идентификатор фазы
     * @return Вектор DTO досок
     */
    virtual std::vector<dto::Board> findByPhase(int64_t phaseId) = 0;
};

} // namespace repositories
} // namespace server
