#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_item_type.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка прав на типы элементов.
 */
struct RuleItemTypesPage
{
    std::vector<dto::RuleItemType> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления правами на типы элементов (RuleItemType).
 */
class IRuleItemTypeService
{
public:
    virtual ~IRuleItemTypeService() = default;

    /**
     * @brief Получает список прав на типы элементов с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param ruleId Фильтр по идентификатору правила (опционально)
     * @param itemTypeId Фильтр по идентификатору типа элемента (опционально)
     * @return Страница с правами на типы элементов
     */
    virtual RuleItemTypesPage getRuleItemTypes(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) = 0;

    /**
     * @brief Получает право на тип элемента по ID.
     * @param id Идентификатор записи
     * @return DTO права или std::nullopt
     */
    virtual std::optional<dto::RuleItemType> getRuleItemType(int64_t id) = 0;

    /**
     * @brief Создает новое право на тип элемента.
     * @param ruleItemType DTO права
     * @param userId ID пользователя для проверки прав
     * @return Созданное право или std::nullopt при ошибке
     */
    virtual std::optional<dto::RuleItemType> createRuleItemType(
        const dto::RuleItemType& ruleItemType,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее право на тип элемента.
     * @param ruleItemType DTO права с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное право или std::nullopt при ошибке
     */
    virtual std::optional<dto::RuleItemType> updateRuleItemType(
        const dto::RuleItemType& ruleItemType,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет право на тип элемента.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteRuleItemType(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
