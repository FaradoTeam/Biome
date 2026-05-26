#include "common/log/log.h"

#include "rule_state_service.h"

namespace server::services
{

RuleStateService::RuleStateService(
    std::shared_ptr<repositories::IRuleStateRepository> ruleStateRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IStateRepository> stateRepo
)
    : m_ruleStateRepo(std::move(ruleStateRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_stateRepo(std::move(stateRepo))
{
    if (!m_ruleStateRepo || !m_ruleRepo || !m_stateRepo)
    {
        throw std::runtime_error("RuleStateService: one or more repositories are null");
    }
}

RuleStatesPage RuleStateService::getRuleStates(
    int page, int pageSize,
    std::optional<int64_t> ruleId,
    std::optional<int64_t> stateId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [items, total] = m_ruleStateRepo->findAll(page, pageSize, ruleId, stateId);
    return { std::move(items), total };
}

std::optional<dto::RuleState> RuleStateService::getRuleState(int64_t id)
{
    return m_ruleStateRepo->findById(id);
}

std::optional<dto::RuleState> RuleStateService::createRuleState(const dto::RuleState& ruleState)
{
    // Проверка обязательных полей
    if (!ruleState.ruleId.has_value() || !ruleState.stateId.has_value())
    {
        LOG_WARN << "createRuleState: ruleId and stateId are required";
        return std::nullopt;
    }

    // Проверка существования правила
    if (!m_ruleRepo->exists(*ruleState.ruleId))
    {
        LOG_WARN << "createRuleState: rule with id=" << *ruleState.ruleId << " not found";
        return std::nullopt;
    }

    // Проверка существования состояния
    if (!m_stateRepo->findById(*ruleState.stateId).has_value())
    {
        LOG_WARN << "createRuleState: state with id=" << *ruleState.stateId << " not found";
        return std::nullopt;
    }

    // Проверка уникальности пары (ruleId, stateId)
    if (m_ruleStateRepo->exists(*ruleState.ruleId, *ruleState.stateId))
    {
        LOG_WARN << "createRuleState: pair (ruleId,stateId) already exists";
        return std::nullopt;
    }

    int64_t newId = m_ruleStateRepo->create(ruleState);
    if (newId <= 0)
    {
        LOG_ERROR << "createRuleState: failed to create RuleState";
        return std::nullopt;
    }

    LOG_INFO << "RuleState created: id=" << newId
             << ", ruleId=" << *ruleState.ruleId
             << ", stateId=" << *ruleState.stateId;
    return m_ruleStateRepo->findById(newId);
}

std::optional<dto::RuleState> RuleStateService::updateRuleState(const dto::RuleState& ruleState)
{
    if (!ruleState.id.has_value())
    {
        LOG_WARN << "updateRuleState: missing id";
        return std::nullopt;
    }

    auto existing = m_ruleStateRepo->findById(*ruleState.id);
    if (!existing)
    {
        LOG_WARN << "updateRuleState: RuleState with id=" << *ruleState.id << " not found";
        return std::nullopt;
    }

    // Проверка уникальности при изменении ruleId или stateId
    bool needUniquenessCheck = false;
    int64_t newRuleId = *existing->ruleId;
    int64_t newStateId = *existing->stateId;

    if (ruleState.ruleId.has_value() && *ruleState.ruleId != newRuleId)
    {
        if (!m_ruleRepo->exists(*ruleState.ruleId))
        {
            LOG_WARN << "updateRuleState: new ruleId=" << *ruleState.ruleId << " not found";
            return std::nullopt;
        }
        newRuleId = *ruleState.ruleId;
        needUniquenessCheck = true;
    }

    if (ruleState.stateId.has_value() && *ruleState.stateId != newStateId)
    {
        if (!m_stateRepo->findById(*ruleState.stateId).has_value())
        {
            LOG_WARN << "updateRuleState: new stateId=" << *ruleState.stateId << " not found";
            return std::nullopt;
        }
        newStateId = *ruleState.stateId;
        needUniquenessCheck = true;
    }

    if (needUniquenessCheck && m_ruleStateRepo->exists(newRuleId, newStateId))
    {
        LOG_WARN << "updateRuleState: pair (ruleId,stateId) already exists";
        return std::nullopt;
    }

    if (!m_ruleStateRepo->update(ruleState))
    {
        LOG_ERROR << "updateRuleState: failed to update RuleState id=" << *ruleState.id;
        return std::nullopt;
    }

    LOG_INFO << "RuleState updated: id=" << *ruleState.id;
    return m_ruleStateRepo->findById(*ruleState.id);
}

bool RuleStateService::deleteRuleState(int64_t id)
{
    if (!m_ruleStateRepo->findById(id).has_value())
    {
        LOG_WARN << "deleteRuleState: RuleState with id=" << id << " not found";
        return false;
    }

    if (!m_ruleStateRepo->remove(id))
    {
        LOG_ERROR << "deleteRuleState: failed to delete RuleState id=" << id;
        return false;
    }

    LOG_INFO << "RuleState deleted: id=" << id;
    return true;
}

} // namespace server::services
