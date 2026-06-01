#include "common/log/log.h"

#include "edge_service.h"

namespace server
{
namespace services
{

EdgeService::EdgeService(
    std::shared_ptr<repositories::IEdgeRepository> edgeRepo,
    std::shared_ptr<repositories::IStateRepository> stateRepo
)
    : m_edgeRepo(std::move(edgeRepo))
    , m_stateRepo(std::move(stateRepo))
{
    if (!m_edgeRepo)
    {
        throw std::runtime_error("EdgeRepository не может быть пустым");
    }
    if (!m_stateRepo)
    {
        throw std::runtime_error("StateRepository не может быть пустым");
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

std::optional<dto::Edge> EdgeService::createEdge(const dto::Edge& edge)
{
    // Проверяем обязательные поля
    if (!edge.beginStateId.has_value() || !edge.endStateId.has_value())
    {
        LOG_WARN << "createEdge: отсутствуют обязательные поля";
        return std::nullopt;
    }

    // Валидируем переход
    auto validationResult = validateEdge(
        *edge.beginStateId, *edge.endStateId
    );
    if (!validationResult.success)
    {
        LOG_WARN
            << "createEdge: валидация не пройдена - "
            << validationResult.errorMessage;
        return std::nullopt;
    }

    // Проверяем, не существует ли уже такой переход
    if (m_edgeRepo->exists(*edge.beginStateId, *edge.endStateId))
    {
        LOG_WARN
            << "createEdge: переход из состояния "
            << *edge.beginStateId << " в состояние "
            << *edge.endStateId << " уже существует";
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
        << ", " << *edge.beginStateId << " -> " << *edge.endStateId;

    return m_edgeRepo->findById(newId);
}

EdgeResult EdgeService::deleteEdge(int64_t id)
{
    EdgeResult result;

    auto existing = m_edgeRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Переход не найден";
        result.errorCode = 404;
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
    LOG_INFO << "Переход удален: id=" << id;

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

    // Проверяем существование состояний
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

    // Проверяем, что состояния принадлежат одному рабочему процессу
    if (beginState->workflowId.value() != endState->workflowId.value())
    {
        result.success = false;
        result.errorMessage = "Состояния должны принадлежать одному рабочему процессу";
        result.errorCode = 400;
        return result;
    }

    // TODO: Решить, как будут работать архивные состояния.
    // Проверяем, что начальное и конечное состояния не архивированы
    if (beginState->isArchive.value_or(false))
    {
        result.success = false;
        result.errorMessage = "Нельзя создать переход из архивированного состояния";
        result.errorCode = 400;
        return result;
    }

    if (endState->isArchive.value_or(false))
    {
        result.success = false;
        result.errorMessage = "Нельзя создать переход в архивированное состояние";
        result.errorCode = 400;
        return result;
    }

    return result;
}

} // namespace services
} // namespace server
