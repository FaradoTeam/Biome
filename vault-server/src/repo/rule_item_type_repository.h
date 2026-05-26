#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_item_type.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для прав на типы элементов (RuleItemType).
 */
class IRuleItemTypeRepository
{
public:
    virtual ~IRuleItemTypeRepository() = default;

    virtual std::pair<std::vector<dto::RuleItemType>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RuleItemType> findById(int64_t id) = 0;
    virtual std::vector<dto::RuleItemType> findByRuleId(int64_t ruleId) = 0;
    virtual std::vector<dto::RuleItemType> findByItemTypeId(int64_t itemTypeId) = 0;
    virtual bool exists(int64_t ruleId, int64_t itemTypeId) = 0;
    virtual int64_t create(const dto::RuleItemType& ruleItemType) = 0;
    virtual bool update(const dto::RuleItemType& ruleItemType) = 0;
    virtual bool remove(int64_t id) = 0;
};

} // namespace server::repositories
