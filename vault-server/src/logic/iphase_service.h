#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/phase.h"

namespace server
{
namespace services
{

/**
 * @brief Структура для возврата результатов пагинированного списка фаз.
 */
struct PhasesPage
{
    std::vector<dto::Phase> phases;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления фазами проектов.
 */
class IPhaseService
{
public:
    virtual ~IPhaseService() = default;

    /**
     * @brief Получает список фаз с пагинацией и фильтрацией.
     */
    virtual PhasesPage phases(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<bool> isArchive = std::nullopt
    ) = 0;

    /**
     * @brief Получает фазу по ID.
     */
    virtual std::optional<dto::Phase> phase(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создает новую фазу.
     */
    virtual std::optional<dto::Phase> createPhase(
        const dto::Phase& phase,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующую фазу.
     */
    virtual std::optional<dto::Phase> updatePhase(
        const dto::Phase& phase,
        int64_t userId
    ) = 0;

    /**
     * @brief Архивирует фазу.
     */
    virtual bool archivePhase(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Восстанавливает фазу из архива.
     */
    virtual bool restorePhase(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
