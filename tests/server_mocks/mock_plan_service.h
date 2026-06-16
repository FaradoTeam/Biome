#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/dto/plan.h"
#include "common/dto/plan_item.h"

#include "logic/iplan_service.h"

namespace server
{
namespace tests
{

class MockPlanService : public services::IPlanService
{
public:
    using PlansPage = services::PlansPage;
    using PlanItemsPage = services::PlanItemsPage;
    using PlanResult = services::PlanResult;

    MockPlanService() = default;

    // ============================================================
    // Настройка результатов
    // ============================================================

    void setGetPlansResult(const PlansPage& result)
    {
        m_getPlansResult = result;
        m_getPlansCallback = nullptr;
    }

    void setGetPlansResultForPhase(int64_t phaseId, const PlansPage& result)
    {
        m_getPlansResultForPhase[phaseId] = result;
    }

    void setGetPlanResult(std::optional<dto::Plan> plan)
    {
        m_getPlanResult = std::move(plan);
        m_getPlanCallback = nullptr;
    }

    void setCreateFirstPlanResult(std::optional<dto::Plan> plan)
    {
        m_createFirstPlanResult = std::move(plan);
        m_createFirstPlanCallback = nullptr;
    }

    void setForkPlanResult(std::optional<dto::Plan> plan)
    {
        m_forkPlanResult = std::move(plan);
        m_forkPlanCallback = nullptr;
    }

    void setActivatePlanResult(const PlanResult& result)
    {
        m_activatePlanResult = result;
        m_activatePlanCallback = nullptr;
    }

    void setDeletePlanResult(const PlanResult& result)
    {
        m_deletePlanResult = result;
        m_deletePlanCallback = nullptr;
    }

    void setGetPlanItemsResult(const PlanItemsPage& result)
    {
        m_getPlanItemsResult = result;
        m_getPlanItemsCallback = nullptr;
    }

    void setGetPlanItemResult(std::optional<dto::PlanItem> item)
    {
        m_getPlanItemResult = std::move(item);
        m_getPlanItemCallback = nullptr;
    }

    void setAddPlanItemResult(std::optional<dto::PlanItem> item)
    {
        m_addPlanItemResult = std::move(item);
        m_addPlanItemCallback = nullptr;
    }

    void setUpdatePlanItemResult(std::optional<dto::PlanItem> item)
    {
        m_updatePlanItemResult = std::move(item);
        m_updatePlanItemCallback = nullptr;
    }

    void setRemovePlanItemResult(const PlanResult& result)
    {
        m_removePlanItemResult = result;
        m_removePlanItemCallback = nullptr;
    }

    // ============================================================
    // Callback-и для кастомной логики
    // ============================================================

    void setGetPlansCallback(
        std::function<PlansPage(int, int, int64_t, std::optional<int64_t>, std::optional<bool>)> callback
    )
    {
        m_getPlansCallback = std::move(callback);
    }

    void setGetPlanCallback(
        std::function<std::optional<dto::Plan>(int64_t, int64_t)> callback
    )
    {
        m_getPlanCallback = std::move(callback);
    }

    void setCreateFirstPlanCallback(
        std::function<std::optional<dto::Plan>(int64_t, const std::string&, const std::string&, int64_t)> callback
    )
    {
        m_createFirstPlanCallback = std::move(callback);
    }

    void setForkPlanCallback(
        std::function<std::optional<dto::Plan>(int64_t, const std::string&, const std::string&, int64_t)> callback
    )
    {
        m_forkPlanCallback = std::move(callback);
    }

    void setActivatePlanCallback(
        std::function<PlanResult(int64_t, int64_t)> callback
    )
    {
        m_activatePlanCallback = std::move(callback);
    }

    void setDeletePlanCallback(
        std::function<PlanResult(int64_t, int64_t)> callback
    )
    {
        m_deletePlanCallback = std::move(callback);
    }

    void setGetPlanItemsCallback(
        std::function<PlanItemsPage(int, int, int64_t, std::optional<int64_t>, int64_t)> callback
    )
    {
        m_getPlanItemsCallback = std::move(callback);
    }

    void setGetPlanItemCallback(
        std::function<std::optional<dto::PlanItem>(int64_t, int64_t)> callback
    )
    {
        m_getPlanItemCallback = std::move(callback);
    }

    void setAddPlanItemCallback(
        std::function<std::optional<dto::PlanItem>(const dto::PlanItem&, int64_t)> callback
    )
    {
        m_addPlanItemCallback = std::move(callback);
    }

    void setUpdatePlanItemCallback(
        std::function<std::optional<dto::PlanItem>(const dto::PlanItem&, int64_t)> callback
    )
    {
        m_updatePlanItemCallback = std::move(callback);
    }

    void setRemovePlanItemCallback(
        std::function<PlanResult(int64_t, int64_t)> callback
    )
    {
        m_removePlanItemCallback = std::move(callback);
    }

    // ============================================================
    // Реализация интерфейса IPlanService
    // ============================================================

    PlansPage plans(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<bool> isActive = std::nullopt
    ) override
    {
        m_lastGetPlansPage = page;
        m_lastGetPlansPageSize = pageSize;
        m_lastGetPlansUserId = userId;
        m_lastGetPlansPhaseId = phaseId;
        m_lastGetPlansIsActive = isActive;
        ++m_getPlansCallCount;

        // Если есть callback, используем его
        if (m_getPlansCallback)
        {
            return m_getPlansCallback(page, pageSize, userId, phaseId, isActive);
        }

        // Проверяем, есть ли специальный результат для этой фазы
        if (phaseId.has_value())
        {
            auto it = m_getPlansResultForPhase.find(*phaseId);
            if (it != m_getPlansResultForPhase.end())
            {
                return it->second;
            }
        }

        // Фильтрация результата, если нужно
        PlansPage result = m_getPlansResult;

        // Если фильтр по фазе, но нет специального результата, фильтруем общий результат
        if (phaseId.has_value())
        {
            std::vector<dto::Plan> filtered;
            for (const auto& plan : result.plans)
            {
                if (plan.phaseId == phaseId)
                    filtered.push_back(plan);
            }
            result.plans = filtered;
            result.totalCount = filtered.size();
        }
        if (isActive.has_value())
        {
            std::vector<dto::Plan> filtered;
            for (const auto& plan : result.plans)
            {
                if (plan.isActive == isActive)
                    filtered.push_back(plan);
            }
            result.plans = filtered;
            result.totalCount = filtered.size();
        }

        return result;
    }

    std::optional<dto::Plan> plan(int64_t id, int64_t userId) override
    {
        m_lastGetPlanId = id;
        m_lastGetPlanUserId = userId;
        ++m_getPlanCallCount;

        if (m_getPlanCallback)
        {
            return m_getPlanCallback(id, userId);
        }

        // Поиск в предустановленных результатах
        for (const auto& plan : m_getPlansResult.plans)
        {
            if (plan.id == id)
                return plan;
        }

        return m_getPlanResult;
    }

    std::optional<dto::Plan> createPlan(
        const dto::Plan& plan,
        int64_t userId
    ) override
    {
        // Если есть basePlanId - это форк
        if (plan.basePlanId.has_value())
        {
            return forkPlan(*plan.basePlanId, *plan.caption, plan.description.value_or(""), userId);
        }

        // Иначе это создание первого плана
        if (plan.phaseId.has_value() && plan.caption.has_value())
        {
            return createFirstPlan(*plan.phaseId, *plan.caption, plan.description.value_or(""), userId);
        }

        return std::nullopt;
    }

    std::optional<dto::Plan> forkPlan(
        int64_t planId,
        const std::string& caption,
        const std::string& description,
        int64_t userId
    ) override
    {
        m_lastForkPlanId = planId;
        m_lastForkPlanCaption = caption;
        m_lastForkPlanDescription = description;
        m_lastForkPlanUserId = userId;
        ++m_forkPlanCallCount;

        if (m_forkPlanCallback)
        {
            return m_forkPlanCallback(planId, caption, description, userId);
        }
        return m_forkPlanResult;
    }

    PlanResult activatePlan(int64_t id, int64_t activatedByUserId) override
    {
        m_lastActivatePlanId = id;
        m_lastActivatePlanUserId = activatedByUserId;
        ++m_activatePlanCallCount;

        if (m_activatePlanCallback)
        {
            return m_activatePlanCallback(id, activatedByUserId);
        }
        return m_activatePlanResult;
    }

    PlanResult deletePlan(int64_t id, int64_t userId) override
    {
        m_lastDeletedPlanId = id;
        m_lastDeletePlanUserId = userId;
        ++m_deletePlanCallCount;

        if (m_deletePlanCallback)
        {
            return m_deletePlanCallback(id, userId);
        }
        return m_deletePlanResult;
    }

    std::vector<dto::PlanItem> getPlanItems(int64_t planId, int64_t userId) override
    {
        m_lastGetPlanItemsPlanId = planId;
        m_lastGetPlanItemsUserId = userId;
        ++m_getPlanItemsCallCount;

        if (m_getPlanItemsCallback)
        {
            auto result = m_getPlanItemsCallback(1, 1000, planId, std::nullopt, userId);
            return result.items;
        }
        return m_getPlanItemsResult.items;
    }

    PlanItemsPage getPlanItemsWithPagination(
        int page,
        int pageSize,
        int64_t planId,
        std::optional<int64_t> userIdFilter,
        int64_t userId
    ) override
    {
        m_lastGetPlanItemsPage = page;
        m_lastGetPlanItemsPageSize = pageSize;
        m_lastGetPlanItemsPlanId = planId;
        m_lastGetPlanItemsUserIdFilter = userIdFilter;
        m_lastGetPlanItemsUserId = userId;
        ++m_getPlanItemsCallCount;

        if (m_getPlanItemsCallback)
        {
            return m_getPlanItemsCallback(page, pageSize, planId, userIdFilter, userId);
        }

        // Фильтрация результата
        PlanItemsPage result = m_getPlanItemsResult;
        if (userIdFilter.has_value())
        {
            std::vector<dto::PlanItem> filtered;
            for (const auto& item : result.items)
            {
                if (item.userId == userIdFilter)
                    filtered.push_back(item);
            }
            result.items = filtered;
            result.totalCount = filtered.size();
        }

        return result;
    }

    std::optional<dto::PlanItem> getPlanItem(int64_t id, int64_t userId) override
    {
        m_lastGetPlanItemId = id;
        m_lastGetPlanItemUserId = userId;
        ++m_getPlanItemCallCount;

        if (m_getPlanItemCallback)
        {
            return m_getPlanItemCallback(id, userId);
        }

        // Поиск в предустановленных результатах
        for (const auto& item : m_getPlanItemsResult.items)
        {
            if (item.id == id)
                return item;
        }

        return m_getPlanItemResult;
    }

    std::optional<dto::PlanItem> addPlanItem(
        const dto::PlanItem& planItem,
        int64_t userId
    ) override
    {
        m_lastAddPlanItem = planItem;
        m_lastAddPlanItemUserId = userId;
        ++m_addPlanItemCallCount;

        if (m_addPlanItemCallback)
        {
            return m_addPlanItemCallback(planItem, userId);
        }
        return m_addPlanItemResult;
    }

    std::optional<dto::PlanItem> updatePlanItem(
        const dto::PlanItem& planItem,
        int64_t userId
    ) override
    {
        m_lastUpdatePlanItem = planItem;
        m_lastUpdatePlanItemUserId = userId;
        ++m_updatePlanItemCallCount;

        if (m_updatePlanItemCallback)
        {
            return m_updatePlanItemCallback(planItem, userId);
        }
        return m_updatePlanItemResult;
    }

    PlanResult removePlanItem(int64_t planItemId, int64_t userId) override
    {
        m_lastRemovePlanItemId = planItemId;
        m_lastRemovePlanItemUserId = userId;
        ++m_removePlanItemCallCount;

        if (m_removePlanItemCallback)
        {
            return m_removePlanItemCallback(planItemId, userId);
        }
        return m_removePlanItemResult;
    }

    // ============================================================
    // Методы для создания первого плана (специальный метод)
    // ============================================================

    std::optional<dto::Plan> createFirstPlan(
        int64_t phaseId,
        const std::string& caption,
        const std::string& description,
        int64_t userId
    )
    {
        m_lastCreateFirstPlanPhaseId = phaseId;
        m_lastCreateFirstPlanCaption = caption;
        m_lastCreateFirstPlanDescription = description;
        m_lastCreateFirstPlanUserId = userId;
        ++m_createFirstPlanCallCount;

        if (m_createFirstPlanCallback)
        {
            return m_createFirstPlanCallback(phaseId, caption, description, userId);
        }
        return m_createFirstPlanResult;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetPlansCallCount() const { return m_getPlansCallCount; }
    int getGetPlanCallCount() const { return m_getPlanCallCount; }
    int getCreateFirstPlanCallCount() const { return m_createFirstPlanCallCount; }
    int getForkPlanCallCount() const { return m_forkPlanCallCount; }
    int getActivatePlanCallCount() const { return m_activatePlanCallCount; }
    int getDeletePlanCallCount() const { return m_deletePlanCallCount; }
    int getGetPlanItemsCallCount() const { return m_getPlanItemsCallCount; }
    int getGetPlanItemCallCount() const { return m_getPlanItemCallCount; }
    int getAddPlanItemCallCount() const { return m_addPlanItemCallCount; }
    int getUpdatePlanItemCallCount() const { return m_updatePlanItemCallCount; }
    int getRemovePlanItemCallCount() const { return m_removePlanItemCallCount; }

    // ============================================================
    // Геттеры для последних параметров вызовов
    // ============================================================

    int getLastGetPlansPage() const { return m_lastGetPlansPage; }
    int getLastGetPlansPageSize() const { return m_lastGetPlansPageSize; }
    int64_t getLastGetPlansUserId() const { return m_lastGetPlansUserId; }
    std::optional<int64_t> getLastGetPlansPhaseId() const { return m_lastGetPlansPhaseId; }
    std::optional<bool> getLastGetPlansIsActive() const { return m_lastGetPlansIsActive; }

    int64_t getLastGetPlanId() const { return m_lastGetPlanId; }
    int64_t getLastGetPlanUserId() const { return m_lastGetPlanUserId; }

    int64_t getLastCreateFirstPlanPhaseId() const { return m_lastCreateFirstPlanPhaseId; }
    const std::string& getLastCreateFirstPlanCaption() const { return m_lastCreateFirstPlanCaption; }
    const std::string& getLastCreateFirstPlanDescription() const { return m_lastCreateFirstPlanDescription; }
    int64_t getLastCreateFirstPlanUserId() const { return m_lastCreateFirstPlanUserId; }

    int64_t getLastForkPlanId() const { return m_lastForkPlanId; }
    const std::string& getLastForkPlanCaption() const { return m_lastForkPlanCaption; }
    const std::string& getLastForkPlanDescription() const { return m_lastForkPlanDescription; }
    int64_t getLastForkPlanUserId() const { return m_lastForkPlanUserId; }

    int64_t getLastActivatePlanId() const { return m_lastActivatePlanId; }
    int64_t getLastActivatePlanUserId() const { return m_lastActivatePlanUserId; }

    int64_t getLastDeletedPlanId() const { return m_lastDeletedPlanId; }
    int64_t getLastDeletePlanUserId() const { return m_lastDeletePlanUserId; }

    int64_t getLastGetPlanItemsPlanId() const { return m_lastGetPlanItemsPlanId; }
    int64_t getLastGetPlanItemsUserId() const { return m_lastGetPlanItemsUserId; }
    int getLastGetPlanItemsPage() const { return m_lastGetPlanItemsPage; }
    int getLastGetPlanItemsPageSize() const { return m_lastGetPlanItemsPageSize; }
    std::optional<int64_t> getLastGetPlanItemsUserIdFilter() const { return m_lastGetPlanItemsUserIdFilter; }

    int64_t getLastGetPlanItemId() const { return m_lastGetPlanItemId; }
    int64_t getLastGetPlanItemUserId() const { return m_lastGetPlanItemUserId; }

    const dto::PlanItem& getLastAddPlanItem() const { return m_lastAddPlanItem; }
    int64_t getLastAddPlanItemUserId() const { return m_lastAddPlanItemUserId; }

    const dto::PlanItem& getLastUpdatePlanItem() const { return m_lastUpdatePlanItem; }
    int64_t getLastUpdatePlanItemUserId() const { return m_lastUpdatePlanItemUserId; }

    int64_t getLastRemovePlanItemId() const { return m_lastRemovePlanItemId; }
    int64_t getLastRemovePlanItemUserId() const { return m_lastRemovePlanItemUserId; }

    PlansPage getDefaultPlansPage() const { return m_getPlansResult; }

    // ============================================================
    // Сброс состояния
    // ============================================================

    void reset()
    {
        m_getPlansCallCount = 0;
        m_getPlanCallCount = 0;
        m_createFirstPlanCallCount = 0;
        m_forkPlanCallCount = 0;
        m_activatePlanCallCount = 0;
        m_deletePlanCallCount = 0;
        m_getPlanItemsCallCount = 0;
        m_getPlanItemCallCount = 0;
        m_addPlanItemCallCount = 0;
        m_updatePlanItemCallCount = 0;
        m_removePlanItemCallCount = 0;

        m_lastGetPlansPage = 0;
        m_lastGetPlansPageSize = 0;
        m_lastGetPlansUserId = 0;
        m_lastGetPlansPhaseId = std::nullopt;
        m_lastGetPlansIsActive = std::nullopt;

        m_lastGetPlanId = 0;
        m_lastGetPlanUserId = 0;

        m_lastCreateFirstPlanPhaseId = 0;
        m_lastCreateFirstPlanCaption.clear();
        m_lastCreateFirstPlanDescription.clear();
        m_lastCreateFirstPlanUserId = 0;

        m_lastForkPlanId = 0;
        m_lastForkPlanCaption.clear();
        m_lastForkPlanDescription.clear();
        m_lastForkPlanUserId = 0;

        m_lastActivatePlanId = 0;
        m_lastActivatePlanUserId = 0;

        m_lastDeletedPlanId = 0;
        m_lastDeletePlanUserId = 0;

        m_lastGetPlanItemsPlanId = 0;
        m_lastGetPlanItemsUserId = 0;
        m_lastGetPlanItemsPage = 0;
        m_lastGetPlanItemsPageSize = 0;
        m_lastGetPlanItemsUserIdFilter = std::nullopt;

        m_lastGetPlanItemId = 0;
        m_lastGetPlanItemUserId = 0;

        m_lastAddPlanItem = dto::PlanItem {};
        m_lastAddPlanItemUserId = 0;

        m_lastUpdatePlanItem = dto::PlanItem {};
        m_lastUpdatePlanItemUserId = 0;

        m_lastRemovePlanItemId = 0;
        m_lastRemovePlanItemUserId = 0;

        m_getPlansCallback = nullptr;
        m_getPlanCallback = nullptr;
        m_createFirstPlanCallback = nullptr;
        m_forkPlanCallback = nullptr;
        m_activatePlanCallback = nullptr;
        m_deletePlanCallback = nullptr;
        m_getPlanItemsCallback = nullptr;
        m_getPlanItemCallback = nullptr;
        m_addPlanItemCallback = nullptr;
        m_updatePlanItemCallback = nullptr;
        m_removePlanItemCallback = nullptr;

        m_getPlansResult = PlansPage {};
        m_getPlansResultForPhase.clear();
        m_getPlanResult = std::nullopt;
        m_createFirstPlanResult = std::nullopt;
        m_forkPlanResult = std::nullopt;
        m_activatePlanResult = PlanResult {};
        m_deletePlanResult = PlanResult {};
        m_getPlanItemsResult = PlanItemsPage {};
        m_getPlanItemResult = std::nullopt;
        m_addPlanItemResult = std::nullopt;
        m_updatePlanItemResult = std::nullopt;
        m_removePlanItemResult = PlanResult {};
    }

private:
    // Результаты
    PlansPage m_getPlansResult;
    std::unordered_map<int64_t, PlansPage> m_getPlansResultForPhase;
    std::optional<dto::Plan> m_getPlanResult;
    std::optional<dto::Plan> m_createFirstPlanResult;
    std::optional<dto::Plan> m_forkPlanResult;
    PlanResult m_activatePlanResult;
    PlanResult m_deletePlanResult;
    PlanItemsPage m_getPlanItemsResult;
    std::optional<dto::PlanItem> m_getPlanItemResult;
    std::optional<dto::PlanItem> m_addPlanItemResult;
    std::optional<dto::PlanItem> m_updatePlanItemResult;
    PlanResult m_removePlanItemResult;

    // Callback-и
    std::function<PlansPage(int, int, int64_t, std::optional<int64_t>, std::optional<bool>)> m_getPlansCallback;
    std::function<std::optional<dto::Plan>(int64_t, int64_t)> m_getPlanCallback;
    std::function<std::optional<dto::Plan>(int64_t, const std::string&, const std::string&, int64_t)> m_createFirstPlanCallback;
    std::function<std::optional<dto::Plan>(int64_t, const std::string&, const std::string&, int64_t)> m_forkPlanCallback;
    std::function<PlanResult(int64_t, int64_t)> m_activatePlanCallback;
    std::function<PlanResult(int64_t, int64_t)> m_deletePlanCallback;
    std::function<PlanItemsPage(int, int, int64_t, std::optional<int64_t>, int64_t)> m_getPlanItemsCallback;
    std::function<std::optional<dto::PlanItem>(int64_t, int64_t)> m_getPlanItemCallback;
    std::function<std::optional<dto::PlanItem>(const dto::PlanItem&, int64_t)> m_addPlanItemCallback;
    std::function<std::optional<dto::PlanItem>(const dto::PlanItem&, int64_t)> m_updatePlanItemCallback;
    std::function<PlanResult(int64_t, int64_t)> m_removePlanItemCallback;

    // Счётчики вызовов
    int m_getPlansCallCount = 0;
    int m_getPlanCallCount = 0;
    int m_createFirstPlanCallCount = 0;
    int m_forkPlanCallCount = 0;
    int m_activatePlanCallCount = 0;
    int m_deletePlanCallCount = 0;
    int m_getPlanItemsCallCount = 0;
    int m_getPlanItemCallCount = 0;
    int m_addPlanItemCallCount = 0;
    int m_updatePlanItemCallCount = 0;
    int m_removePlanItemCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetPlansPage = 0;
    int m_lastGetPlansPageSize = 0;
    int64_t m_lastGetPlansUserId = 0;
    std::optional<int64_t> m_lastGetPlansPhaseId;
    std::optional<bool> m_lastGetPlansIsActive;

    int64_t m_lastGetPlanId = 0;
    int64_t m_lastGetPlanUserId = 0;

    int64_t m_lastCreateFirstPlanPhaseId = 0;
    std::string m_lastCreateFirstPlanCaption;
    std::string m_lastCreateFirstPlanDescription;
    int64_t m_lastCreateFirstPlanUserId = 0;

    int64_t m_lastForkPlanId = 0;
    std::string m_lastForkPlanCaption;
    std::string m_lastForkPlanDescription;
    int64_t m_lastForkPlanUserId = 0;

    int64_t m_lastActivatePlanId = 0;
    int64_t m_lastActivatePlanUserId = 0;

    int64_t m_lastDeletedPlanId = 0;
    int64_t m_lastDeletePlanUserId = 0;

    int64_t m_lastGetPlanItemsPlanId = 0;
    int64_t m_lastGetPlanItemsUserId = 0;
    int m_lastGetPlanItemsPage = 0;
    int m_lastGetPlanItemsPageSize = 0;
    std::optional<int64_t> m_lastGetPlanItemsUserIdFilter;

    int64_t m_lastGetPlanItemId = 0;
    int64_t m_lastGetPlanItemUserId = 0;

    dto::PlanItem m_lastAddPlanItem;
    int64_t m_lastAddPlanItemUserId = 0;

    dto::PlanItem m_lastUpdatePlanItem;
    int64_t m_lastUpdatePlanItemUserId = 0;

    int64_t m_lastRemovePlanItemId = 0;
    int64_t m_lastRemovePlanItemUserId = 0;
};

} // namespace tests
} // namespace server
