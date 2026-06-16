#pragma once

#include <optional>
#include <vector>

#include "common/dto/plan_item.h"
#include "common/types.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с элементами планов.
 */
class IPlanItemRepository
{
public:
    virtual ~IPlanItemRepository() = default;

    /**
     * @brief Получает список элементов плана с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param planId Фильтр по плану (std::nullopt - все)
     * @param userId Фильтр по пользователю (std::nullopt - все)
     * @return Пара: вектор DTO элементов плана и общее количество
     */
    virtual std::pair<std::vector<dto::PlanItem>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> planId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt
    ) = 0;

    /**
     * @brief Находит элемент плана по ID.
     * @param id Идентификатор записи
     * @return DTO элемента плана или std::nullopt
     */
    virtual std::optional<dto::PlanItem> findById(int64_t id) = 0;

    /**
     * @brief Находит элемент плана по плану и элементу.
     * @param planId Идентификатор плана
     * @param itemId Идентификатор элемента
     * @return DTO элемента плана или std::nullopt
     */
    virtual std::optional<dto::PlanItem> findByPlanAndItem(
        int64_t planId,
        int64_t itemId
    ) = 0;

    /**
     * @brief Получает все элементы плана.
     * @param planId Идентификатор плана
     * @return Вектор DTO элементов плана
     */
    virtual std::vector<dto::PlanItem> findByPlanId(int64_t planId) = 0;

    /**
     * @brief Получает элементы, назначенные на пользователя.
     * @param userId Идентификатор пользователя
     * @return Вектор DTO элементов плана
     */
    virtual std::vector<dto::PlanItem> findByUserId(int64_t userId) = 0;

    /**
     * @brief Получает элементы, запланированные на период.
     * @param dateFrom Начало периода
     * @param dateTo Конец периода
     * @return Вектор DTO элементов плана
     */
    virtual std::vector<dto::PlanItem> findByDateRange(
        const common::DateTime& dateFrom,
        const common::DateTime& dateTo
    ) = 0;

    /**
     * @brief Создаёт новый элемент плана.
     * @param planItem DTO элемента плана
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::PlanItem& planItem) = 0;

    /**
     * @brief Обновляет существующий элемент плана.
     * @param planItem DTO элемента плана с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::PlanItem& planItem) = 0;

    /**
     * @brief Удаляет элемент плана по ID.
     * @param id Идентификатор записи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все элементы плана.
     * @param planId Идентификатор плана
     * @return Количество удалённых записей
     */
    virtual int64_t removeByPlanId(int64_t planId) = 0;

    /**
     * @brief Проверяет существование элемента плана.
     * @param id Идентификатор записи
     * @return true если запись существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет существование элемента в плане.
     * @param planId Идентификатор плана
     * @param itemId Идентификатор элемента
     * @return true если элемент уже в плане
     */
    virtual bool existsByPlanAndItem(int64_t planId, int64_t itemId) = 0;

    /**
     * @brief Копирует все элементы из одного плана в другой.
     * @param sourcePlanId Исходный план
     * @param targetPlanId Целевой план
     * @return Количество скопированных записей
     */
    virtual int64_t copyFromPlan(int64_t sourcePlanId, int64_t targetPlanId) = 0;
};

} // namespace repositories
} // namespace server
