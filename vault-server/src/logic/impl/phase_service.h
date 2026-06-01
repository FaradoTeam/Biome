#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iphase_service.h"

#include "repo/phase_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления фазами проектов.
 */
class PhaseService final : public IPhaseService
{
public:
    PhaseService(
        std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    PhasesPage phases(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId,
        std::optional<bool> isArchive
    ) override;

    std::optional<dto::Phase> phase(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::Phase> createPhase(
        const dto::Phase& phase,
        int64_t userId
    ) override;

    std::optional<dto::Phase> updatePhase(
        const dto::Phase& phase,
        int64_t userId
    ) override;

    bool archivePhase(
        int64_t id,
        int64_t userId
    ) override;

    bool restorePhase(
        int64_t id,
        int64_t userId
    ) override;

private:
    std::shared_ptr<repositories::IPhaseRepository> m_phaseRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
