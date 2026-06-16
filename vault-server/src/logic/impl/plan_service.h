#pragma once

#include <memory>
#include <optional>

#include "logic/iauthorization_service.h"
#include "logic/iplan_service.h"

#include "repo/item_repository.h"
#include "repo/phase_repository.h"
#include "repo/plan_item_repository.h"
#include "repo/plan_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с планами.
 */
class PlanService final : public IPlanService
{
public:
    PlanService(
        std::shared_ptr<repositories::IPlanRepository> planRepo,
        std::shared_ptr<repositories::IPlanItemRepository> planItemRepo,
        std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
        std::shared_ptr<repositories::IItemRepository> itemRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IPlanService
    PlansPage plans(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<bool> isActive = std::nullopt
    ) override;

    std::optional<dto::Plan> plan(int64_t id, int64_t userId) override;

    std::optional<dto::Plan> createPlan(
        const dto::Plan& plan,
        int64_t userId
    ) override;

    std::optional<dto::Plan> forkPlan(
        int64_t planId,
        const std::string& caption,
        const std::string& description,
        int64_t userId
    ) override;

    PlanResult activatePlan(
        int64_t id,
        int64_t activatedByUserId
    ) override;

    PlanResult deletePlan(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::PlanItem> getPlanItems(
        int64_t planId,
        int64_t userId
    ) override;

    PlanItemsPage getPlanItemsWithPagination(
        int page,
        int pageSize,
        int64_t planId,
        std::optional<int64_t> userIdFilter,
        int64_t userId
    ) override;

    std::optional<dto::PlanItem> getPlanItem(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::PlanItem> addPlanItem(
        const dto::PlanItem& planItem,
        int64_t userId
    ) override;

    std::optional<dto::PlanItem> updatePlanItem(
        const dto::PlanItem& planItem,
        int64_t userId
    ) override;

    PlanResult removePlanItem(
        int64_t planItemId,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к плану.
     * @param planId ID плана
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return DTO плана или std::nullopt при ошибке
     */
    std::optional<dto::Plan> checkPlanAccess(
        int64_t planId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Проверяет доступ к фазе для работы с планами.
     * @param phaseId ID фазы
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return true если доступ разрешён
     */
    bool checkPhaseAccess(
        int64_t phaseId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Валидирует DTO плана.
     */
    bool validatePlan(const dto::Plan& plan, std::string& errorMessage);

    /**
     * @brief Валидирует DTO элемента плана.
     */
    bool validatePlanItem(
        const dto::PlanItem& planItem,
        std::string& errorMessage
    );

    /**
     * @brief Проверяет, что даты находятся в пределах дат фазы.
     */
    bool validateDatesWithinPhase(
        int64_t phaseId,
        const common::DateTime& startDate,
        const common::DateTime& endDate,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IPlanRepository> m_planRepo;
    std::shared_ptr<repositories::IPlanItemRepository> m_planItemRepo;
    std::shared_ptr<repositories::IPhaseRepository> m_phaseRepo;
    std::shared_ptr<repositories::IItemRepository> m_itemRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
