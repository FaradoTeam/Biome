#include "common/log/log.h"

#include "workflow_service.h"

namespace server
{
namespace services
{

WorkflowService::WorkflowService(
    std::shared_ptr<repositories::IWorkflowRepository> workflowRepo,
    std::shared_ptr<repositories::IStateRepository> stateRepo,
    std::shared_ptr<repositories::IEdgeRepository> edgeRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_workflowRepo(std::move(workflowRepo))
    , m_stateRepo(std::move(stateRepo))
    , m_edgeRepo(std::move(edgeRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_workflowRepo || !m_stateRepo || !m_edgeRepo)
    {
        throw std::runtime_error(
            "WorkflowService: один или несколько репозиториев не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error("AuthorizationService не может быть пустым");
    }
}

WorkflowsPage WorkflowService::workflows(int page, int pageSize)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [workflows, total] = m_workflowRepo->findAll(page, pageSize);
    return { workflows, total };
}

std::optional<dto::Workflow> WorkflowService::workflow(int64_t id)
{
    return m_workflowRepo->findById(id);
}

std::optional<dto::Workflow> WorkflowService::createWorkflow(
    const dto::Workflow& workflow,
    int64_t userId
)
{
    // Только супер-админ может создавать рабочие процессы
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createWorkflow: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    std::string errorMessage;
    if (!validateWorkflow(workflow, errorMessage))
    {
        LOG_WARN << "createWorkflow: " << errorMessage;
        return std::nullopt;
    }

    if (m_workflowRepo->existsByCaption(*workflow.caption))
    {
        LOG_WARN
            << "createWorkflow: рабочий процесс с названием '" << *workflow.caption
            << "' уже существует";
        return std::nullopt;
    }

    const int64_t newId = m_workflowRepo->create(workflow);
    if (newId <= 0)
    {
        LOG_ERROR << "createWorkflow: не удалось создать рабочий процесс";
        return std::nullopt;
    }

    LOG_INFO << "Рабочий процесс создан: id=" << newId << ", пользователь=" << userId;
    return m_workflowRepo->findById(newId);
}

std::optional<dto::Workflow> WorkflowService::updateWorkflow(
    const dto::Workflow& workflow,
    int64_t userId
)
{
    // Только супер-админ может обновлять рабочие процессы
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateWorkflow: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!workflow.id.has_value())
    {
        LOG_WARN << "updateWorkflow: отсутствует ID";
        return std::nullopt;
    }

    auto existing = m_workflowRepo->findById(*workflow.id);
    if (!existing.has_value())
    {
        LOG_WARN << "updateWorkflow: рабочий процесс с id=" << *workflow.id << " не найден";
        return std::nullopt;
    }

    if (workflow.caption.has_value() && *workflow.caption != existing->caption.value_or(""))
    {
        if (m_workflowRepo->existsByCaption(*workflow.caption))
        {
            LOG_WARN
                << "updateWorkflow: название '" << *workflow.caption
                << "' уже используется";
            return std::nullopt;
        }
    }

    if (!m_workflowRepo->update(workflow))
    {
        LOG_ERROR
            << "updateWorkflow: не удалось обновить рабочий процесс id="
            << *workflow.id;
        return std::nullopt;
    }

    LOG_INFO << "Рабочий процесс обновлен: id=" << *workflow.id << ", пользователь=" << userId;
    return m_workflowRepo->findById(*workflow.id);
}

WorkflowResult WorkflowService::deleteWorkflow(int64_t id, int64_t userId)
{
    WorkflowResult result;

    // Только супер-админ может удалять рабочие процессы
    if (!m_authzService->isSuperAdmin(userId))
    {
        result.errorMessage = "Недостаточно прав для удаления рабочего процесса";
        result.errorCode = 403;
        LOG_WARN << "deleteWorkflow: пользователь " << userId << " не имеет прав";
        return result;
    }

    auto existing = m_workflowRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Рабочий процесс не найден";
        result.errorCode = 404;
        return result;
    }

    if (!canDeleteWorkflow(id))
    {
        result.errorMessage = "Невозможно удалить рабочий процесс: имеются связанные состояния или элементы";
        result.errorCode = 409;
        return result;
    }

    if (!m_workflowRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить рабочий процесс";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO << "Рабочий процесс удален: id=" << id << ", пользователь=" << userId;
    return result;
}

bool WorkflowService::canDeleteWorkflow(int64_t id)
{
    auto states = m_stateRepo->findByWorkflowId(id);
    for (const auto& state : states)
    {
        if (m_stateRepo->isUsedByItems(*state.id))
        {
            return false;
        }
    }
    return true;
}

bool WorkflowService::validateWorkflow(const dto::Workflow& workflow, std::string& errorMessage)
{
    if (!workflow.caption.has_value() || workflow.caption->empty())
    {
        errorMessage = "Название рабочего процесса обязательно для заполнения";
        return false;
    }

    if (workflow.caption->length() > 255)
    {
        errorMessage = "Название рабочего процесса не может превышать 255 символов";
        return false;
    }

    if (workflow.description.has_value() && workflow.description->length() > 1000)
    {
        errorMessage = "Описание рабочего процесса не может превышать 1000 символов";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
