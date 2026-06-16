#include "common/log/log.h"

#include "plan_service.h"

namespace server
{
namespace services
{

PlanService::PlanService(
    std::shared_ptr<repositories::IPlanRepository> planRepo,
    std::shared_ptr<repositories::IPlanItemRepository> planItemRepo,
    std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
    std::shared_ptr<repositories::IItemRepository> itemRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_planRepo(std::move(planRepo))
    , m_planItemRepo(std::move(planItemRepo))
    , m_phaseRepo(std::move(phaseRepo))
    , m_itemRepo(std::move(itemRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_planRepo || !m_planItemRepo || !m_phaseRepo || !m_itemRepo)
    {
        throw std::runtime_error(
            "PlanService: один или несколько репозиториев не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error("PlanService: сервис авторизации не инициализирован");
    }
}

// ============================================================
// Получение списка планов
// ============================================================

PlansPage PlanService::plans(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> phaseId,
    std::optional<bool> isActive
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан phaseId, проверяем доступ к фазе
    if (phaseId.has_value() && !checkPhaseAccess(*phaseId, userId, false))
    {
        LOG_WARN
            << "plans: пользователь " << userId
            << " не имеет доступа к фазе " << *phaseId;
        return { {}, 0 };
    }

    auto [plans, total] = m_planRepo->findAll(page, pageSize, phaseId, isActive);

    // Если phaseId не указан, фильтруем планы по доступным фазам
    if (!phaseId.has_value() && !m_authzService->isSuperAdmin(userId))
    {
        std::vector<dto::Plan> filteredPlans;
        for (const auto& plan : plans)
        {
            if (!plan.phaseId.has_value())
                continue;

            if (checkPhaseAccess(*plan.phaseId, userId, false))
            {
                filteredPlans.push_back(plan);
            }
        }
        return { filteredPlans, static_cast<int64_t>(filteredPlans.size()) };
    }

    return { plans, total };
}

// ============================================================
// Получение плана по ID
// ============================================================

std::optional<dto::Plan> PlanService::plan(int64_t id, int64_t userId)
{
    return checkPlanAccess(id, userId, false);
}

// ============================================================
// Создание плана
// ============================================================

std::optional<dto::Plan> PlanService::createPlan(
    const dto::Plan& plan,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validatePlan(plan, errorMessage))
    {
        LOG_WARN << "createPlan: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем доступ к фазе (требуется право на редактирование фаз)
    if (!checkPhaseAccess(*plan.phaseId, userId, true))
    {
        LOG_WARN
            << "createPlan: пользователь " << userId
            << " не имеет прав на создание планов в фазе " << *plan.phaseId;
        return std::nullopt;
    }

    // 3. Проверяем, что в фазе нет активного плана при создании активного
    if (plan.isActive.value_or(false))
    {
        auto existingActive = m_planRepo->findActiveByPhaseId(*plan.phaseId);
        if (existingActive.has_value())
        {
            LOG_WARN
                << "createPlan: в фазе " << *plan.phaseId
                << " уже есть активный план id=" << *existingActive->id;
            return std::nullopt;
        }
    }

    // 4. Создаём план
    dto::Plan newPlan = plan;
    newPlan.createdByUserId = userId;
    newPlan.createdAt = std::chrono::system_clock::now();

    const int64_t newId = m_planRepo->create(newPlan);
    if (newId <= 0)
    {
        LOG_ERROR << "createPlan: не удалось создать план";
        return std::nullopt;
    }

    LOG_INFO
        << "План создан: id=" << newId
        << ", фаза=" << *newPlan.phaseId
        << ", пользователь=" << userId;

    return m_planRepo->findById(newId);
}

// ============================================================
// Форк плана (создание новой версии)
// ============================================================

std::optional<dto::Plan> PlanService::forkPlan(
    int64_t planId,
    const std::string& caption,
    const std::string& description,
    int64_t userId
)
{
    // 1. Проверяем доступ к исходному плану
    auto sourcePlan = checkPlanAccess(planId, userId, false);
    if (!sourcePlan.has_value())
    {
        LOG_WARN << "forkPlan: исходный план не найден или нет доступа, id=" << planId;
        return std::nullopt;
    }

    // 2. Проверяем, что исходный план активен (можно форкать только активный план)
    if (!sourcePlan->isActive.value_or(false))
    {
        LOG_WARN
            << "forkPlan: можно форкать только активный план, id=" << planId;
        return std::nullopt;
    }

    // 3. Проверяем доступ к фазе для создания нового плана
    if (!checkPhaseAccess(*sourcePlan->phaseId, userId, true))
    {
        LOG_WARN
            << "forkPlan: пользователь " << userId
            << " не имеет прав на создание планов в фазе " << *sourcePlan->phaseId;
        return std::nullopt;
    }

    // 4. Проверяем, что в фазе уже есть активный план (нельзя создать новый активный)
    //    Новый план создаётся неактивным
    auto existingActive = m_planRepo->findActiveByPhaseId(*sourcePlan->phaseId);
    if (existingActive.has_value() && *existingActive->id != planId)
    {
        LOG_WARN
            << "forkPlan: в фазе " << *sourcePlan->phaseId
            << " уже есть активный план id=" << *existingActive->id;
        return std::nullopt;
    }

    // 5. Создаём новый план (неактивный)
    dto::Plan newPlan;
    newPlan.phaseId = sourcePlan->phaseId;
    newPlan.basePlanId = planId;
    newPlan.caption = caption;
    newPlan.description = description;
    newPlan.isActive = false;
    newPlan.createdByUserId = userId;
    newPlan.createdAt = std::chrono::system_clock::now();

    const int64_t newId = m_planRepo->create(newPlan);
    if (newId <= 0)
    {
        LOG_ERROR << "forkPlan: не удалось создать план-форк";
        return std::nullopt;
    }

    // 6. Копируем PlanItem из исходного плана
    const int64_t copied = m_planItemRepo->copyFromPlan(planId, newId);
    LOG_DEBUG
        << "forkPlan: скопировано " << copied
        << " элементов из плана " << planId << " в план " << newId;

    LOG_INFO
        << "План-форк создан: id=" << newId
        << ", исходный план=" << planId
        << ", пользователь=" << userId;

    return m_planRepo->findById(newId);
}

// ============================================================
// Активация плана
// ============================================================

PlanResult PlanService::activatePlan(int64_t id, int64_t activatedByUserId)
{
    PlanResult result;

    // 1. Проверяем существование плана
    auto plan = m_planRepo->findById(id);
    if (!plan.has_value())
    {
        result.errorMessage = "План не найден";
        result.errorCode = 404;
        return result;
    }

    // 2. Проверяем доступ к фазе (требуется право на редактирование фаз)
    if (!checkPhaseAccess(*plan->phaseId, activatedByUserId, true))
    {
        result.errorMessage = "Недостаточно прав для активации плана";
        result.errorCode = 403;
        LOG_WARN
            << "activatePlan: пользователь " << activatedByUserId
            << " не имеет прав на активацию плана в фазе " << *plan->phaseId;
        return result;
    }

    // 3. Проверяем, что план не активен
    if (plan->isActive.value_or(false))
    {
        result.errorMessage = "План уже активен";
        result.errorCode = 400;
        return result;
    }

    // 4. Проверяем, что план содержит элементы
    auto items = m_planItemRepo->findByPlanId(id);
    if (items.empty())
    {
        result.errorMessage = "Нельзя активировать пустой план";
        result.errorCode = 400;
        return result;
    }

    // 5. Валидируем даты элементов в пределах фазы
    auto phase = m_phaseRepo->findById(*plan->phaseId);
    if (phase.has_value())
    {
        for (const auto& item : items)
        {
            if (!validateDatesWithinPhase(
                    *plan->phaseId,
                    *item.startDate,
                    *item.endDate,
                    result.errorMessage
                ))
            {
                result.errorCode = 400;
                return result;
            }
        }
    }

    // 6. Деактивируем все планы фазы
    m_planRepo->deactivateAllByPhaseId(*plan->phaseId);

    // 7. Активируем выбранный план
    dto::Plan updateData;
    updateData.id = id;
    updateData.isActive = true;
    updateData.activatedAt = std::chrono::system_clock::now();
    updateData.activatedByUserId = activatedByUserId;

    if (!m_planRepo->update(updateData))
    {
        result.errorMessage = "Не удалось активировать план";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "План активирован: id=" << id
        << ", пользователь=" << activatedByUserId;

    return result;
}

// ============================================================
// Удаление плана
// ============================================================

PlanResult PlanService::deletePlan(int64_t id, int64_t userId)
{
    PlanResult result;

    // 1. Проверяем существование и доступ к плану
    auto plan = checkPlanAccess(id, userId, true);
    if (!plan.has_value())
    {
        result.errorMessage = "План не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Проверяем, что план не активен
    if (plan->isActive.value_or(false))
    {
        result.errorMessage = "Нельзя удалить активный план";
        result.errorCode = 400;
        return result;
    }

    // 3. Удаляем все элементы плана
    m_planItemRepo->removeByPlanId(id);

    // 4. Удаляем план
    if (!m_planRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить план";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "План удалён: id=" << id
        << ", пользователь=" << userId;

    return result;
}

// ============================================================
// Получение элементов плана (без пагинации)
// ============================================================

std::vector<dto::PlanItem> PlanService::getPlanItems(int64_t planId, int64_t userId)
{
    // Проверяем доступ к плану
    auto plan = checkPlanAccess(planId, userId, false);
    if (!plan.has_value())
    {
        LOG_WARN
            << "getPlanItems: план не найден или нет доступа, id=" << planId;
        return {};
    }

    return m_planItemRepo->findByPlanId(planId);
}

// ============================================================
// Получение элементов плана с пагинацией
// ============================================================

PlanItemsPage PlanService::getPlanItemsWithPagination(
    int page,
    int pageSize,
    int64_t planId,
    std::optional<int64_t> userIdFilter,
    int64_t userId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Проверяем доступ к плану
    auto plan = checkPlanAccess(planId, userId, false);
    if (!plan.has_value())
    {
        LOG_WARN
            << "getPlanItemsWithPagination: план не найден или нет доступа, id=" << planId;
        return { {}, 0 };
    }

    // Если указан фильтр по пользователю, проверяем права (только супер-админ или сам пользователь)
    if (userIdFilter.has_value() && *userIdFilter != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN
            << "getPlanItemsWithPagination: пользователь " << userId
            << " не имеет прав на фильтрацию по пользователю " << *userIdFilter;
        return { {}, 0 };
    }

    auto [items, total] = m_planItemRepo->findAll(page, pageSize, planId, userIdFilter);
    return { items, total };
}

// ============================================================
// Получение элемента плана по ID
// ============================================================

std::optional<dto::PlanItem> PlanService::getPlanItem(int64_t id, int64_t userId)
{
    auto planItem = m_planItemRepo->findById(id);
    if (!planItem.has_value())
    {
        LOG_DEBUG << "getPlanItem: элемент плана не найден, id=" << id;
        return std::nullopt;
    }

    // Проверяем доступ к плану
    auto plan = checkPlanAccess(*planItem->planId, userId, false);
    if (!plan.has_value())
    {
        LOG_DEBUG
            << "getPlanItem: нет доступа к плану " << *planItem->planId;
        return std::nullopt;
    }

    return planItem;
}

// ============================================================
// Добавление элемента в план
// ============================================================

std::optional<dto::PlanItem> PlanService::addPlanItem(
    const dto::PlanItem& planItem,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validatePlanItem(planItem, errorMessage))
    {
        LOG_WARN << "addPlanItem: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем доступ к плану (требуется право на запись)
    auto plan = checkPlanAccess(*planItem.planId, userId, true);
    if (!plan.has_value())
    {
        LOG_WARN
            << "addPlanItem: план не найден или нет доступа, id=" << *planItem.planId;
        return std::nullopt;
    }

    // 3. Проверяем, что план не активен (нельзя изменять активный план)
    if (plan->isActive.value_or(false))
    {
        LOG_WARN
            << "addPlanItem: нельзя изменить активный план, id=" << *planItem.planId;
        return std::nullopt;
    }

    // 4. Проверяем существование элемента
    auto item = m_itemRepo->findById(*planItem.itemId);
    if (!item.has_value())
    {
        LOG_WARN
            << "addPlanItem: элемент не найден, id=" << *planItem.itemId;
        return std::nullopt;
    }

    // 5. Проверяем, что элемент не добавлен в план повторно
    if (m_planItemRepo->existsByPlanAndItem(*planItem.planId, *planItem.itemId))
    {
        LOG_WARN
            << "addPlanItem: элемент уже в плане, itemId=" << *planItem.itemId;
        return std::nullopt;
    }

    // 6. Проверяем даты в пределах фазы
    if (!validateDatesWithinPhase(
            *plan->phaseId,
            *planItem.startDate,
            *planItem.endDate,
            errorMessage
        ))
    {
        LOG_WARN << "addPlanItem: " << errorMessage;
        return std::nullopt;
    }

    // 7. Создаём элемент плана
    const int64_t newId = m_planItemRepo->create(planItem);
    if (newId <= 0)
    {
        LOG_ERROR << "addPlanItem: не удалось добавить элемент в план";
        return std::nullopt;
    }

    LOG_INFO
        << "Элемент добавлен в план: id=" << newId
        << ", план=" << *planItem.planId
        << ", элемент=" << *planItem.itemId
        << ", пользователь=" << userId;

    return m_planItemRepo->findById(newId);
}

// ============================================================
// Обновление элемента плана
// ============================================================

std::optional<dto::PlanItem> PlanService::updatePlanItem(
    const dto::PlanItem& planItem,
    int64_t userId
)
{
    if (!planItem.id.has_value())
    {
        LOG_WARN << "updatePlanItem: отсутствует ID элемента плана";
        return std::nullopt;
    }

    // 1. Получаем существующий элемент
    auto existing = m_planItemRepo->findById(*planItem.id);
    if (!existing.has_value())
    {
        LOG_WARN
            << "updatePlanItem: элемент плана не найден, id=" << *planItem.id;
        return std::nullopt;
    }

    // 2. Проверяем доступ к плану (требуется право на запись)
    auto plan = checkPlanAccess(*existing->planId, userId, true);
    if (!plan.has_value())
    {
        LOG_WARN
            << "updatePlanItem: план не найден или нет доступа, id=" << *existing->planId;
        return std::nullopt;
    }

    // 3. Проверяем, что план не активен (нельзя изменять активный план)
    if (plan->isActive.value_or(false))
    {
        LOG_WARN
            << "updatePlanItem: нельзя изменить активный план, id=" << *existing->planId;
        return std::nullopt;
    }

    // 4. Обновляем элемент
    dto::PlanItem updateData = planItem;
    if (!updateData.startDate.has_value())
        updateData.startDate = existing->startDate;
    if (!updateData.endDate.has_value())
        updateData.endDate = existing->endDate;
    if (!updateData.planId.has_value())
        updateData.planId = existing->planId;
    if (!updateData.itemId.has_value())
        updateData.itemId = existing->itemId;

    // 5. Если меняется план, проверяем доступ к новому плану
    if (planItem.planId.has_value() && *planItem.planId != *existing->planId)
    {
        auto newPlan = checkPlanAccess(*planItem.planId, userId, true);
        if (!newPlan.has_value())
        {
            LOG_WARN
                << "updatePlanItem: новый план не найден или нет доступа, id=" << *planItem.planId;
            return std::nullopt;
        }

        // Проверяем, что новый план не активен
        if (newPlan->isActive.value_or(false))
        {
            LOG_WARN
                << "updatePlanItem: нельзя переместить элемент в активный план, id=" << *planItem.planId;
            return std::nullopt;
        }

        plan = newPlan;
    }

    // 6. Проверяем даты в пределах фазы
    std::string errorMessage;
    if (!validateDatesWithinPhase(
            *plan->phaseId,
            *updateData.startDate,
            *updateData.endDate,
            errorMessage
        ))
    {
        LOG_WARN << "updatePlanItem: " << errorMessage;
        return std::nullopt;
    }

    // 7. Проверяем уникальность при перемещении в другой план
    if (planItem.planId.has_value() && *planItem.planId != *existing->planId)
    {
        if (m_planItemRepo->existsByPlanAndItem(*planItem.planId, *updateData.itemId))
        {
            LOG_WARN
                << "updatePlanItem: элемент уже существует в целевом плане, itemId=" << *updateData.itemId;
            return std::nullopt;
        }
    }

    if (!m_planItemRepo->update(updateData))
    {
        LOG_ERROR
            << "updatePlanItem: не удалось обновить элемент плана id=" << *planItem.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Элемент плана обновлён: id=" << *planItem.id
        << ", пользователь=" << userId;

    return m_planItemRepo->findById(*planItem.id);
}

// ============================================================
// Удаление элемента из плана
// ============================================================

PlanResult PlanService::removePlanItem(int64_t planItemId, int64_t userId)
{
    PlanResult result;

    // 1. Получаем существующий элемент
    auto existing = m_planItemRepo->findById(planItemId);
    if (!existing.has_value())
    {
        result.errorMessage = "Элемент плана не найден";
        result.errorCode = 404;
        return result;
    }

    // 2. Проверяем доступ к плану (требуется право на запись)
    auto plan = checkPlanAccess(*existing->planId, userId, true);
    if (!plan.has_value())
    {
        result.errorMessage = "План не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 3. Проверяем, что план не активен (нельзя изменять активный план)
    if (plan->isActive.value_or(false))
    {
        result.errorMessage = "Нельзя удалить элемент из активного плана";
        result.errorCode = 400;
        return result;
    }

    // 4. Удаляем элемент
    if (!m_planItemRepo->remove(planItemId))
    {
        result.errorMessage = "Не удалось удалить элемент плана";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Элемент плана удалён: id=" << planItemId
        << ", пользователь=" << userId;

    return result;
}

// ============================================================
// Приватные методы
// ============================================================

std::optional<dto::Plan> PlanService::checkPlanAccess(
    int64_t planId,
    int64_t userId,
    bool needWrite
)
{
    auto plan = m_planRepo->findById(planId);
    if (!plan.has_value())
    {
        LOG_DEBUG << "checkPlanAccess: план не найден, id=" << planId;
        return std::nullopt;
    }

    if (!checkPhaseAccess(*plan->phaseId, userId, needWrite))
    {
        LOG_DEBUG
            << "checkPlanAccess: нет доступа к фазе " << *plan->phaseId
            << " для пользователя " << userId;
        return std::nullopt;
    }

    return plan;
}

bool PlanService::checkPhaseAccess(
    int64_t phaseId,
    int64_t userId,
    bool needWrite
)
{
    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    auto phase = m_phaseRepo->findById(phaseId);
    if (!phase.has_value() || !phase->projectId.has_value())
    {
        LOG_DEBUG << "checkPhaseAccess: фаза не найдена, id=" << phaseId;
        return false;
    }

    if (needWrite)
    {
        // Для записи нужно право на редактирование фаз в проекте
        auto authz = m_authzService->canEditPhases(userId, *phase->projectId);
        return authz.granted;
    }
    else
    {
        // Для чтения достаточно права на чтение проекта
        auto authz = m_authzService->canReadProject(userId, *phase->projectId);
        return authz.granted;
    }
}

bool PlanService::validatePlan(const dto::Plan& plan, std::string& errorMessage)
{
    if (!plan.phaseId.has_value())
    {
        errorMessage = "phaseId является обязательным полем";
        return false;
    }

    if (!plan.caption.has_value() || plan.caption->empty())
    {
        errorMessage = "caption является обязательным полем и не может быть пустым";
        return false;
    }

    if (plan.caption->length() > 255)
    {
        errorMessage = "caption не может превышать 255 символов";
        return false;
    }

    if (plan.description.has_value() && plan.description->length() > 1000)
    {
        errorMessage = "description не может превышать 1000 символов";
        return false;
    }

    return true;
}

bool PlanService::validatePlanItem(
    const dto::PlanItem& planItem,
    std::string& errorMessage
)
{
    if (!planItem.planId.has_value())
    {
        errorMessage = "planId является обязательным полем";
        return false;
    }

    if (!planItem.itemId.has_value())
    {
        errorMessage = "itemId является обязательным полем";
        return false;
    }

    if (!planItem.startDate.has_value())
    {
        errorMessage = "startDate является обязательным полем";
        return false;
    }

    if (!planItem.endDate.has_value())
    {
        errorMessage = "endDate является обязательным полем";
        return false;
    }

    if (*planItem.startDate > *planItem.endDate)
    {
        errorMessage = "startDate не может быть позже endDate";
        return false;
    }

    return true;
}

bool PlanService::validateDatesWithinPhase(
    int64_t phaseId,
    const common::DateTime& startDate,
    const common::DateTime& endDate,
    std::string& errorMessage
)
{
    auto phase = m_phaseRepo->findById(phaseId);
    if (!phase.has_value())
    {
        errorMessage = "Фаза не найдена";
        return false;
    }

    if (phase->beginDate.has_value() && startDate < *phase->beginDate)
    {
        errorMessage = "Дата начала не может быть раньше даты начала фазы";
        return false;
    }

    if (phase->endDate.has_value() && endDate > *phase->endDate)
    {
        errorMessage = "Дата окончания не может быть позже даты окончания фазы";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
