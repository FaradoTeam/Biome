#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/istate_service.h"

#include "repo/edge_repository.h"
#include "repo/state_repository.h"
#include "repo/workflow_repository.h"

namespace server
{
namespace services
{

class StateService final : public IStateService
{
public:
    explicit StateService(
        std::shared_ptr<repositories::IStateRepository> stateRepo,
        std::shared_ptr<repositories::IEdgeRepository> edgeRepo,
        std::shared_ptr<repositories::IWorkflowRepository> workflowRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    StatesPage states(
        int page,
        int pageSize,
        std::optional<int64_t> workflowId = std::nullopt
    ) override;

    std::optional<dto::State> state(int64_t id) override;

    std::optional<dto::State> createState(
        const dto::State& state,
        int64_t userId
    ) override;

    std::optional<dto::State> updateState(
        const dto::State& state,
        int64_t userId
    ) override;

    StateResult deleteState(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::State> getWorkflowStates(int64_t workflowId) override;
    bool canDeleteState(int64_t id) override;

private:
    bool validateState(const dto::State& state, std::string& errorMessage);

private:
    std::shared_ptr<repositories::IStateRepository> m_stateRepo;
    std::shared_ptr<repositories::IEdgeRepository> m_edgeRepo;
    std::shared_ptr<repositories::IWorkflowRepository> m_workflowRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
