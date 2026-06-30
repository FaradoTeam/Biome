#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_action.h"
#include "common/types.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка действий пользователя.
 */
struct UserActionsPage
{
    std::vector<dto::UserAction> actions;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с действиями пользователя.
 */
class IUserActionRepository
{
public:
    virtual ~IUserActionRepository() = default;

    /**
     * @brief Получает список действий пользователя с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId Фильтр по пользователю (std::nullopt - все)
     * @param dateFrom Фильтр по дате начала (std::nullopt - без ограничения)
     * @param dateTo Фильтр по дате окончания (std::nullopt - без ограничения)
     * @return Страница с действиями
     */
    virtual UserActionsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Находит действие по ID.
     * @param id Идентификатор действия
     * @return DTO действия или std::nullopt
     */
    virtual std::optional<dto::UserAction> findById(int64_t id) = 0;

    /**
     * @brief Находит все действия пользователя.
     * @param userId Идентификатор пользователя
     * @return Вектор действий
     */
    virtual std::vector<dto::UserAction> findByUserId(int64_t userId) = 0;

    /**
     * @brief Создаёт новое действие.
     * @param action DTO действия
     * @return ID созданного действия или 0 при ошибке
     */
    virtual int64_t create(const dto::UserAction& action) = 0;

    /**
     * @brief Удаляет действие по ID.
     * @param id Идентификатор действия
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование действия.
     * @param id Идентификатор действия
     * @return true если действие существует
     */
    virtual bool exists(int64_t id) = 0;
};

} // namespace repositories
} // namespace server
