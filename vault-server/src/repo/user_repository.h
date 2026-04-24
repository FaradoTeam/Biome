#pragma once

#include <optional>
#include <string>

#include "common/dto/user.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с пользователями.
 */
class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    /**
     * @brief Находит пользователя по логину.
     * @param login Логин пользователя
     * @return DTO пользователя или std::nullopt, если не найден
     */
    virtual std::optional<dto::User> findByLogin(const std::string& login) = 0;

    /**
     * @brief Находит пользователя по ID.
     * @param id Идентификатор пользователя
     * @return DTO пользователя или std::nullopt, если не найден
     */
    virtual std::optional<dto::User> findById(int64_t id) = 0;

    /**
     * @brief Получает хеш пароля пользователя по ID.
     * @param userId Идентификатор пользователя
     * @return Хеш пароля или пустая строка, если не найден
     */
    virtual std::string passwordHash(int64_t userId) = 0;

    /**
     * @brief Обновляет пароль пользователя.
     * @param userId Идентификатор пользователя
     * @param passwordHash Хеш нового пароля
     * @return true если обновление успешно
     */
    virtual bool updatePassword(int64_t userId, const std::string& passwordHash) = 0;

    /**
     * @brief Обновляет флаг необходимости смены пароля.
     * @param userId Идентификатор пользователя
     * @param needChange Новое значение флага
     * @return true если обновление успешно
     */
    virtual bool updateNeedChangePassword(int64_t userId, bool needChange) = 0;

    /**
     * @brief Создает нового пользователя (для начальной инициализации).
     * @param user DTO пользователя
     * @param passwordHash Хеш пароля
     * @return ID созданного пользователя или 0 при ошибке
     */
    virtual int64_t create(const dto::User& user, const std::string& passwordHash) = 0;

    /**
     * @brief Проверяет существование пользователя с указанным логином.
     * @param login Логин для проверки
     * @return true если пользователь существует
     */
    virtual bool existsByLogin(const std::string& login) = 0;
};

} // namespace repositories
} // namespace server
