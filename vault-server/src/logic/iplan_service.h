#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/plan.h"
#include "common/dto/plan_item.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с планом.
 */
struct PlanResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с планами.
 */
struct PlansPage
{
    std::vector<dto::Plan> plans;
    int64_t totalCount = 0;
};

/**
 * @brief Страница с элементами плана.
 */
struct PlanItemsPage
{
    std::vector<dto::PlanItem> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с планами.
 */
class IPlanService
{
public:
    virtual ~IPlanService() = default;

    /**
     * @brief Получает список планов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param phaseId Фильтр по фазе (опционально)
     * @param isActive Фильтр по статусу активности (опционально)
     * @return Страница с планами
     */
    virtual PlansPage plans(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<bool> isActive = std::nullopt
    ) = 0;

    /**
     * @brief Получает план по ID.
     * @param id Идентификатор плана
     * @param userId ID пользователя для проверки прав
     * @return DTO плана или std::nullopt
     */
    virtual std::optional<dto::Plan> plan(int64_t id, int64_t userId) = 0;

    /**
     * @brief Создаёт новый план.
     * @param plan DTO плана
     * @param userId ID пользователя для проверки прав
     * @return Созданный план или std::nullopt при ошибке
     */
    virtual std::optional<dto::Plan> createPlan(
        const dto::Plan& plan,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую версию плана (форк).
     * @param planId ID исходного плана
     * @param caption Название нового плана
     * @param description Описание нового плана
     * @param userId ID пользователя для проверки прав
     * @return Созданный план или std::nullopt при ошибке
     */
    virtual std::optional<dto::Plan> forkPlan(
        int64_t planId,
        const std::string& caption,
        const std::string& description,
        int64_t userId
    ) = 0;

    /**
     * @brief Активирует план (деактивирует все остальные планы фазы).
     * @param id Идентификатор плана
     * @param activatedByUserId ID пользователя, активирующего план
     * @return Результат операции
     */
    virtual PlanResult activatePlan(
        int64_t id,
        int64_t activatedByUserId
    ) = 0;

    /**
     * @brief Удаляет план (только неактивный).
     * @param id Идентификатор плана
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual PlanResult deletePlan(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все элементы плана.
     * @param planId Идентификатор плана
     * @param userId ID пользователя для проверки прав
     * @return Вектор элементов плана
     */
    virtual std::vector<dto::PlanItem> getPlanItems(
        int64_t planId,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает список элементов плана с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param planId ID плана
     * @param userIdFilter Фильтр по пользователю (опционально)
     * @param userId ID пользователя для проверки прав
     * @return Страница с элементами плана
     */
    virtual PlanItemsPage getPlanItemsWithPagination(
        int page,
        int pageSize,
        int64_t planId,
        std::optional<int64_t> userIdFilter,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает элемент плана по ID.
     * @param id Идентификатор элемента плана
     * @param userId ID пользователя для проверки прав
     * @return DTO элемента плана или std::nullopt
     */
    virtual std::optional<dto::PlanItem> getPlanItem(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Добавляет элемент в план.
     * @param planItem DTO элемента плана
     * @param userId ID пользователя для проверки прав
     * @return Созданный элемент или std::nullopt при ошибке
     */
    virtual std::optional<dto::PlanItem> addPlanItem(
        const dto::PlanItem& planItem,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет элемент плана.
     * @param planItem DTO элемента плана с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённый элемент или std::nullopt при ошибке
     */
    virtual std::optional<dto::PlanItem> updatePlanItem(
        const dto::PlanItem& planItem,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет элемент из плана.
     * @param planItemId Идентификатор элемента плана
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual PlanResult removePlanItem(
        int64_t planItemId,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
