#pragma once

#include <optional>
#include <vector>

#include "common/dto/plan.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с планами.
 */
class IPlanRepository
{
public:
    virtual ~IPlanRepository() = default;

    /**
     * @brief Получает список планов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param phaseId Фильтр по фазе (std::nullopt - все)
     * @param isActive Фильтр по статусу активности (std::nullopt - все)
     * @return Пара: вектор DTO планов и общее количество
     */
    virtual std::pair<std::vector<dto::Plan>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<bool> isActive = std::nullopt
    ) = 0;

    /**
     * @brief Находит план по ID.
     * @param id Идентификатор плана
     * @return DTO плана или std::nullopt, если не найден
     */
    virtual std::optional<dto::Plan> findById(int64_t id) = 0;

    /**
     * @brief Находит активный план для фазы.
     * @param phaseId Идентификатор фазы
     * @return DTO активного плана или std::nullopt
     */
    virtual std::optional<dto::Plan> findActiveByPhaseId(int64_t phaseId) = 0;

    /**
     * @brief Создаёт новый план.
     * @param plan DTO плана
     * @return ID созданного плана или 0 при ошибке
     */
    virtual int64_t create(const dto::Plan& plan) = 0;

    /**
     * @brief Обновляет существующий план.
     * @param plan DTO плана с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Plan& plan) = 0;

    /**
     * @brief Удаляет план по ID.
     * @param id Идентификатор плана
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование плана с указанным ID.
     * @param id Идентификатор плана
     * @return true если план существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Деактивирует все планы фазы.
     * @param phaseId Идентификатор фазы
     * @return Количество деактивированных планов
     */
    virtual int64_t deactivateAllByPhaseId(int64_t phaseId) = 0;
};

} // namespace repositories
} // namespace server
