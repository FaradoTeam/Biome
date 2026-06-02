#include "common/log/log.h"

#include "edge_service.h"

namespace server
{
namespace services
{

EdgeService::EdgeService(
    std::shared_ptr<repositories::IEdgeRepository> edgeRepo,
    std::shared_ptr<repositories::IStateRepository> stateRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_edgeRepo(std::move(edgeRepo))
    , m_stateRepo(std::move(stateRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_edgeRepo || !m_stateRepo)
    {
        throw std::runtime_error(
            "EdgeService: один или несколько репозиториев не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error("AuthorizationService не может быть пустым");
    }
}

EdgesPage EdgeService::edges(
    int page, int pageSize,
    std::optional<int64_t> beginStateId,
    std::optional<int64_t> endStateId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [edges, total] = m_edgeRepo->findAll(
        page, pageSize, beginStateId, endStateId
    );
    return { edges, total };
}

std::optional<dto::Edge> EdgeService::edge(int64_t id)
{
    return m_edgeRepo->findById(id);
}

std::optional<dto::Edge> EdgeService::createEdge(
    const dto::Edge& edge,
    int64_t userId
)
{
    // Проверяем обязательные поля
    if (!edge.beginStateId.has_value() || !edge.endStateId.has_value())
    {
        LOG_WARN << "createEdge: отсутствуют обязательные поля";
        return std::nullopt;
    }

    // Получаем состояния для проверки прав
    auto beginState = m_stateRepo->findById(*edge.beginStateId);
    if (!beginState.has_value())
    {
        LOG_WARN << "createEdge: начальное состояние не найдено";
        return std::nullopt;
    }

    auto endState = m_stateRepo->findById(*edge.endStateId);
    if (!endState.has_value())
    {
        LOG_WARN << "createEdge: конечное состояние не найдено";
        return std::nullopt;
    }

    // Проверяем, что состояния принадлежат одному рабочему процессу
    if (beginState->workflowId.value() != endState->workflowId.value())
    {
        LOG_WARN << "createEdge: состояния должны принадлежать одному рабочему процессу";
        return std::nullopt;
    }

    // Проверяем, что состояния не архивированы
    if (beginState->isArchive.value_or(false))
    {
        LOG_WARN << "createEdge: нельзя создать переход из архивированного состояния";
        return std::nullopt;
    }

    if (endState->isArchive.value_or(false))
    {
        LOG_WARN << "createEdge: нельзя создать переход в архивированное состояние";
        return std::nullopt;
    }

    // Проверяем права: супер-админ может всё
    if (!m_authzService->isSuperAdmin(userId))
    {
        // Для обычных пользователей нужна дополнительная проверка прав
        // Переходы связаны с рабочими процессами, которые требуют прав администратора
        LOG_WARN
            << "createEdge: пользователь " << userId
            << " не имеет прав на создание переходов";
        return std::nullopt;
    }

    // Проверяем, не существует ли уже такой переход
    if (m_edgeRepo->exists(*edge.beginStateId, *edge.endStateId))
    {
        LOG_WARN << "createEdge: переход уже существует";
        return std::nullopt;
    }

    const int64_t newId = m_edgeRepo->create(edge);
    if (newId <= 0)
    {
        LOG_ERROR << "createEdge: не удалось создать переход";
        return std::nullopt;
    }

    LOG_INFO
        << "Переход создан: id=" << newId
        << ", пользователь=" << userId;
    return m_edgeRepo->findById(newId);
}

EdgeResult EdgeService::deleteEdge(int64_t id, int64_t userId)
{
    EdgeResult result;

    auto existing = m_edgeRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Переход не найден";
        result.errorCode = 404;
        return result;
    }

    // Проверяем права: супер-админ может удалять любые переходы
    if (!m_authzService->isSuperAdmin(userId))
    {
        result.errorMessage = "Недостаточно прав для удаления перехода";
        result.errorCode = 403;
        LOG_WARN << "deleteEdge: пользователь " << userId << " не имеет прав";
        return result;
    }

    if (!m_edgeRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить переход";
        result.errorCode = 500;
        LOG_ERROR << "deleteEdge: ошибка удаления перехода id=" << id;
        return result;
    }

    result.success = true;
    LOG_INFO << "Переход удален: id=" << id << ", пользователь=" << userId;
    return result;
}

std::vector<dto::Edge> EdgeService::getWorkflowEdges(int64_t workflowId)
{
    return m_edgeRepo->findByWorkflowId(workflowId);
}

EdgeResult EdgeService::validateEdge(int64_t beginStateId, int64_t endStateId)
{
    EdgeResult result;
    result.success = true;

    auto beginState = m_stateRepo->findById(beginStateId);
    if (!beginState.has_value())
    {
        result.success = false;
        result.errorMessage = "Начальное состояние не найдено";
        result.errorCode = 404;
        return result;
    }

    auto endState = m_stateRepo->findById(endStateId);
    if (!endState.has_value())
    {
        result.success = false;
        result.errorMessage = "Конечное состояние не найдено";
        result.errorCode = 404;
        return result;
    }

    if (beginState->workflowId.value() != endState->workflowId.value())
    {
        result.success = false;
        result.errorMessage = "Состояния должны принадлежать одному рабочему процессу";
        result.errorCode = 400;
        return result;
    }

    return result;
}

} // namespace services
} // namespace server
