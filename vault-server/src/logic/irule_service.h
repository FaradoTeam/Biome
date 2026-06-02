#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка правил.
 */
struct RulesPage
{
    std::vector<dto::Rule> rules;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления правилами.
 */
class IRuleService
{
public:
    virtual ~IRuleService() = default;

    /**
     * @brief Получает список правил с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param roleId Фильтр по идентификатору роли (опционально)
     * @return Страница с правилами
     */
    virtual RulesPage getRules(
        int page,
        int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    /**
     * @brief Получает правило по ID.
     * @param id Идентификатор правила
     * @return DTO правила или std::nullopt
     */
    virtual std::optional<dto::Rule> getRule(int64_t id) = 0;

    /**
     * @brief Получает правило по ID роли.
     * @param roleId Идентификатор роли
     * @return DTO правила или std::nullopt
     */
    virtual std::optional<dto::Rule> getRuleByRoleId(int64_t roleId) = 0;

    /**
     * @brief Создает новое правило.
     * @param rule DTO правила
     * @param userId ID пользователя для проверки прав
     * @return Созданное правило или std::nullopt при ошибке
     */
    virtual std::optional<dto::Rule> createRule(
        const dto::Rule& rule,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее правило.
     * @param rule DTO правила с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное правило или std::nullopt при ошибке
     */
    virtual std::optional<dto::Rule> updateRule(
        const dto::Rule& rule,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет правило.
     * @param id Идентификатор правила
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteRule(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
