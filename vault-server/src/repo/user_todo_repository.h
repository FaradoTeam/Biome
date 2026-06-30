#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_todo.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка задач пользователя.
 */
struct UserTodosPage
{
    std::vector<dto::UserTodo> todos;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с задачами пользователя.
 */
class IUserTodoRepository
{
public:
    virtual ~IUserTodoRepository() = default;

    /**
     * @brief Получает список задач пользователя с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId Фильтр по пользователю (std::nullopt - все)
     * @param isDone Фильтр по статусу выполнения (std::nullopt - все)
     * @return Страница с задачами
     */
    virtual UserTodosPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<bool> isDone = std::nullopt
    ) = 0;

    /**
     * @brief Находит задачу по ID.
     * @param id Идентификатор задачи
     * @return DTO задачи или std::nullopt
     */
    virtual std::optional<dto::UserTodo> findById(int64_t id) = 0;

    /**
     * @brief Находит все задачи пользователя.
     * @param userId Идентификатор пользователя
     * @return Вектор задач
     */
    virtual std::vector<dto::UserTodo> findByUserId(int64_t userId) = 0;

    /**
     * @brief Создаёт новую задачу.
     * @param todo DTO задачи
     * @return ID созданной задачи или 0 при ошибке
     */
    virtual int64_t create(const dto::UserTodo& todo) = 0;

    /**
     * @brief Обновляет существующую задачу.
     * @param todo DTO задачи с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::UserTodo& todo) = 0;

    /**
     * @brief Удаляет задачу по ID.
     * @param id Идентификатор задачи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование задачи.
     * @param id Идентификатор задачи
     * @return true если задача существует
     */
    virtual bool exists(int64_t id) = 0;
};

} // namespace repositories
} // namespace server
