#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/role.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для работы с ролями.
 */
class IRoleRepository
{
public:
    virtual ~IRoleRepository() = default;

    /**
     * @brief Получает список ролей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param searchCaption Поиск по названию
     * @return Пара: вектор DTO ролей и общее количество
     */
    virtual std::pair<std::vector<dto::Role>, int64_t> findAll(
        int page,
        int pageSize,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Находит роль по ID.
     * @param id Идентификатор роли
     * @return DTO роли или std::nullopt, если не найдена
     */
    virtual std::optional<dto::Role> findById(int64_t id) = 0;

    /**
     * @brief Находит роль по названию.
     * @param caption Название роли
     * @return DTO роли или std::nullopt
     */
    virtual std::optional<dto::Role> findByCaption(const std::string& caption) = 0;

    /**
     * @brief Создаёт новую роль.
     * @param role DTO роли
     * @return ID созданной роли или 0 при ошибке
     */
    virtual int64_t create(const dto::Role& role) = 0;

    /**
     * @brief Обновляет существующую роль.
     * @param role DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Role& role) = 0;

    /**
     * @brief Удаляет роль.
     * @param id Идентификатор роли
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование роли с указанным ID.
     * @param id Идентификатор роли
     * @return true если роль существует
     */
    virtual bool exists(int64_t id) = 0;
};

} // namespace server::repositories
