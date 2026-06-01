#include "common/log/log.h"

#include "rule_state_service.h"

namespace server::services
{

RuleStateService::RuleStateService(
    std::shared_ptr<repositories::IRuleStateRepository> ruleStateRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IStateRepository> stateRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_ruleStateRepo(std::move(ruleStateRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_stateRepo(std::move(stateRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_ruleStateRepo || !m_ruleRepo || !m_stateRepo)
    {
        throw std::runtime_error(
            "RuleStateService: репозитории не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error(
            "RuleStateService: сервис авторизации не инициализирован"
        );
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
        LOG_WARN << "createRuleState: обязательны ruleId и stateId";
        return std::nullopt;
    }

    // Проверка существования правила
    if (!m_ruleRepo->exists(*ruleState.ruleId))
    {
        LOG_WARN << "createRuleState: правило с id=" << *ruleState.ruleId << " не найдено";
        return std::nullopt;
    }

    // Проверка существования состояния
    if (!m_stateRepo->findById(*ruleState.stateId).has_value())
    {
        LOG_WARN << "createRuleState: состояние с id=" << *ruleState.stateId << " не найдено";
        return std::nullopt;
    }

    // Проверка уникальности пары (ruleId, stateId)
    if (m_ruleStateRepo->exists(*ruleState.ruleId, *ruleState.stateId))
    {
        LOG_WARN << "createRuleState: пара (ruleId,stateId) уже существует";
        return std::nullopt;
    }

    int64_t newId = m_ruleStateRepo->create(ruleState);
    if (newId <= 0)
    {
        LOG_ERROR << "createRuleState: не удалось создать RuleState";
        return std::nullopt;
    }

    LOG_INFO
        << "RuleState создан: id=" << newId
        << ", ruleId=" << *ruleState.ruleId
        << ", stateId=" << *ruleState.stateId;

    // Инвалидируем кэш для всех пользователей с этой ролью
    invalidateUsersByRuleId(*ruleState.ruleId);

    return m_ruleStateRepo->findById(newId);
}

std::optional<dto::RuleState> RuleStateService::updateRuleState(const dto::RuleState& ruleState)
{
    if (!ruleState.id.has_value())
    {
        LOG_WARN << "updateRuleState: отсутствует id";
        return std::nullopt;
    }

    auto existing = m_ruleStateRepo->findById(*ruleState.id);
    if (!existing)
    {
        LOG_WARN << "updateRuleState: RuleState с id=" << *ruleState.id << " не найден";
        return std::nullopt;
    }

    // Проверка уникальности при изменении ruleId или stateId
    bool needUniquenessCheck = false;
    int64_t oldRuleId = *existing->ruleId;
    int64_t newRuleId = oldRuleId;
    int64_t newStateId = *existing->stateId;

    if (ruleState.ruleId.has_value() && *ruleState.ruleId != oldRuleId)
    {
        if (!m_ruleRepo->exists(*ruleState.ruleId))
        {
            LOG_WARN << "updateRuleState: новая ruleId=" << *ruleState.ruleId << " не найдена";
            return std::nullopt;
        }
        newRuleId = *ruleState.ruleId;
        needUniquenessCheck = true;
    }

    if (ruleState.stateId.has_value() && *ruleState.stateId != newStateId)
    {
        if (!m_stateRepo->findById(*ruleState.stateId).has_value())
        {
            LOG_WARN << "updateRuleState: новая stateId=" << *ruleState.stateId << " не найдена";
            return std::nullopt;
        }
        newStateId = *ruleState.stateId;
        needUniquenessCheck = true;
    }

    if (needUniquenessCheck && m_ruleStateRepo->exists(newRuleId, newStateId))
    {
        LOG_WARN << "updateRuleState: пара (ruleId,stateId) уже существует";
        return std::nullopt;
    }

    if (!m_ruleStateRepo->update(ruleState))
    {
        LOG_ERROR << "updateRuleState: не удалось обновить RuleState id=" << *ruleState.id;
        return std::nullopt;
    }

    LOG_INFO << "RuleState обновлен: id=" << *ruleState.id;

    // Инвалидируем кэш по старому и новому ruleId
    invalidateUsersByRuleId(oldRuleId);
    if (ruleState.ruleId.has_value() && *ruleState.ruleId != oldRuleId)
    {
        invalidateUsersByRuleId(*ruleState.ruleId);
    }

    return m_ruleStateRepo->findById(*ruleState.id);
}

bool RuleStateService::deleteRuleState(int64_t id)
{
    auto existing = m_ruleStateRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteRuleState: RuleState с id=" << id << " не найден";
        return false;
    }

    int64_t ruleId = *existing->ruleId;

    if (!m_ruleStateRepo->remove(id))
    {
        LOG_ERROR << "deleteRuleState: не удалось удалить RuleState id=" << id;
        return false;
    }

    LOG_INFO << "RuleState удален: id=" << id;

    // Инвалидируем кэш для всех пользователей с этой ролью
    invalidateUsersByRuleId(ruleId);

    return true;
}

void RuleStateService::invalidateUsersByRuleId(int64_t ruleId)
{
    auto rule = m_ruleRepo->findById(ruleId);
    if (!rule || !rule->roleId.has_value())
    {
        LOG_DEBUG
            << "invalidateUsersByRuleId: правило " << ruleId << " не имеет roleId";
        return;
    }

    auto users = m_authzService->getUserIdsByRoleId(*rule->roleId);
    LOG_DEBUG
        << "Инвалидация кэша для " << users.size()
        << " пользователей с ролью " << *rule->roleId;

    for (int64_t userId : users)
    {
        m_authzService->invalidateCache(userId);
        LOG_DEBUG << "Инвалидирован кэш для пользователя " << userId;
    }
}

} // namespace server::services
