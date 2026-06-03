#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/user.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка пользователей.
 */
struct UsersPage
{
    std::vector<dto::User> users;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления пользователями.
 */
class IUserService
{
public:
    virtual ~IUserService() = default;

    /**
     * @brief Получает список пользователей с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @return Страница с пользователями
     */
    virtual UsersPage users(
        int page,
        int pageSize,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает пользователя по ID.
     * @param id Идентификатор пользователя
     * @param userId ID пользователя для проверки прав
     * @return DTO пользователя или std::nullopt
     */
    virtual std::optional<dto::User> user(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создает нового пользователя.
     * @param user DTO пользователя
     * @param password Пароль пользователя
     * @param userId ID пользователя для проверки прав
     * @return Созданный пользователь или std::nullopt при ошибке
     */
    virtual std::optional<dto::User> createUser(
        const dto::User& user,
        const std::string& password,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующего пользователя.
     * @param user DTO пользователя с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленный пользователь или std::nullopt при ошибке
     */
    virtual std::optional<dto::User> updateUser(
        const dto::User& user,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет пользователя.
     * @param id Идентификатор пользователя
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteUser(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
