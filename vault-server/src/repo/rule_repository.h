#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для работы с правилами ролей.
 */
class IRuleRepository
{
public:
    virtual ~IRuleRepository() = default;

    /**
     * @brief Получает список правил с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param roleId Фильтр по идентификатору роли (std::nullopt – все)
     * @return Пара: вектор DTO правил и общее количество
     */
    virtual std::pair<std::vector<dto::Rule>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    /**
     * @brief Находит правило по ID.
     * @param id Идентификатор правила
     * @return DTO правила или std::nullopt, если не найдено
     */
    virtual std::optional<dto::Rule> findById(int64_t id) = 0;

    /**
     * @brief Находит правило по идентификатору роли.
     * @param roleId Идентификатор роли
     * @return DTO правила или std::nullopt (у роли может быть только одно правило)
     */
    virtual std::optional<dto::Rule> findByRoleId(int64_t roleId) = 0;

    /**
     * @brief Создаёт новое правило.
     * @param rule DTO правила
     * @return ID созданного правила или 0 при ошибке
     */
    virtual int64_t create(const dto::Rule& rule) = 0;

    /**
     * @brief Обновляет существующее правило.
     * @param rule DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Rule& rule) = 0;

    /**
     * @brief Удаляет правило.
     * @param id Идентификатор правила
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование правила с указанным ID.
     * @param id Идентификатор правила
     * @return true если правило существует
     */
    virtual bool exists(int64_t id) = 0;
};

} // namespace server::repositories
