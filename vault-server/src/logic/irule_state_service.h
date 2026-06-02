#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_state.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка прав на состояния.
 */
struct RuleStatesPage
{
    std::vector<dto::RuleState> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления правами на состояния (RuleState).
 */
class IRuleStateService
{
public:
    virtual ~IRuleStateService() = default;

    /**
     * @brief Получает список прав на состояния с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param ruleId Фильтр по идентификатору правила (опционально)
     * @param stateId Фильтр по идентификатору состояния (опционально)
     * @return Страница с правами на состояния
     */
    virtual RuleStatesPage getRuleStates(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) = 0;

    /**
     * @brief Получает право на состояние по ID.
     * @param id Идентификатор записи
     * @return DTO права или std::nullopt
     */
    virtual std::optional<dto::RuleState> getRuleState(int64_t id) = 0;

    /**
     * @brief Создает новое право на состояние.
     * @param ruleState DTO права
     * @param userId ID пользователя для проверки прав
     * @return Созданное право или std::nullopt при ошибке
     */
    virtual std::optional<dto::RuleState> createRuleState(
        const dto::RuleState& ruleState,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее право на состояние.
     * @param ruleState DTO права с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное право или std::nullopt при ошибке
     */
    virtual std::optional<dto::RuleState> updateRuleState(
        const dto::RuleState& ruleState,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет право на состояние.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteRuleState(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
