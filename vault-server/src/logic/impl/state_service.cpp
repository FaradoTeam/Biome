#include "common/log/log.h"

#include "state_service.h"

namespace server
{
namespace services
{

StateService::StateService(
    std::shared_ptr<repositories::IStateRepository> stateRepo,
    std::shared_ptr<repositories::IEdgeRepository> edgeRepo,
    std::shared_ptr<repositories::IWorkflowRepository> workflowRepo
)
    : m_stateRepo(std::move(stateRepo))
    , m_edgeRepo(std::move(edgeRepo))
    , m_workflowRepo(std::move(workflowRepo))
{
    if (!m_stateRepo)
    {
        throw std::runtime_error("StateRepository не может быть пустым");
    }
    if (!m_edgeRepo)
    {
        throw std::runtime_error("EdgeRepository не может быть пустым");
    }
    if (!m_workflowRepo)
    {
        throw std::runtime_error("WorkflowRepository не может быть пустым");
    }
}

StatesPage StateService::states(
    int page, int pageSize,
    std::optional<int64_t> workflowId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [states, total] = m_stateRepo->findAll(page, pageSize, workflowId);
    return { states, total };
}

std::optional<dto::State> StateService::state(int64_t id)
{
    return m_stateRepo->findById(id);
}

std::optional<dto::State> StateService::createState(const dto::State& state)
{
    std::string errorMessage;
    if (!validateState(state, errorMessage))
    {
        LOG_WARN << "createState: проверка не пройдена - " << errorMessage;
        return std::nullopt;
    }

    // Проверяем существование workflow
    auto workflow = m_workflowRepo->findById(*state.workflowId);
    if (!workflow.has_value())
    {
        LOG_WARN
            << "createState: рабочий процесс с id="
            << *state.workflowId << " не найден";
        return std::nullopt;
    }

    const int64_t newId = m_stateRepo->create(state);
    if (newId <= 0)
    {
        LOG_ERROR << "createState: не удалось создать состояние";
        return std::nullopt;
    }

    LOG_INFO
        << "Состояние создано: id=" << newId
        << ", название='" << *state.caption << "'"
        << ", workflowId=" << *state.workflowId;

    return m_stateRepo->findById(newId);
}

std::optional<dto::State> StateService::updateState(const dto::State& state)
{
    if (!state.id.has_value())
    {
        LOG_WARN << "updateState: отсутствует ID";
        return std::nullopt;
    }

    // Проверяем существование
    auto existing = m_stateRepo->findById(*state.id);
    if (!existing.has_value())
    {
        LOG_WARN
            << "updateState: состояние с id="
            << *state.id << " не найдено";
        return std::nullopt;
    }

    // Если состояние используется элементами, запрещаем перевод в архив
    if (state.isArchive.has_value() && *state.isArchive)
    {
        if (m_stateRepo->isUsedByItems(*state.id))
        {
            LOG_WARN
                << "updateState: невозможно архивировать состояние id="
                << *state.id << ", так как оно используется элементами";
            return std::nullopt;
        }
    }

    if (!m_stateRepo->update(state))
    {
        LOG_ERROR
            << "updateState: не удалось обновить состояние id="
            << *state.id;
        return std::nullopt;
    }

    LOG_INFO << "Состояние обновлено: id=" << *state.id;

    return m_stateRepo->findById(*state.id);
}

StateResult StateService::deleteState(int64_t id)
{
    StateResult result;

    // Проверяем существование
    auto existing = m_stateRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Состояние не найдено";
        result.errorCode = 404;
        return result;
    }

    // Проверяем, можно ли удалить
    if (!canDeleteState(id))
    {
        result.errorMessage = "Невозможно удалить состояние: "
                              "имеются связанные элементы или переходы";
        result.errorCode = 409;
        LOG_WARN
            << "deleteState: состояние id=" << id
            << " не может быть удалено из-за зависимостей";
        return result;
    }

    if (!m_stateRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить состояние";
        result.errorCode = 500;
        LOG_ERROR << "deleteState: ошибка удаления состояния id=" << id;
        return result;
    }

    result.success = true;
    LOG_INFO << "Состояние удалено: id=" << id;

    return result;
}

std::vector<dto::State> StateService::getWorkflowStates(int64_t workflowId)
{
    return m_stateRepo->findByWorkflowId(workflowId);
}

bool StateService::canDeleteState(int64_t id)
{
    // Проверяем, используется ли состояние элементами
    if (m_stateRepo->isUsedByItems(id))
    {
        return false;
    }

    // Проверяем, есть ли связанные переходы
    auto edges = m_edgeRepo->findByStateId(id);
    if (!edges.empty())
    {
        return false;
    }

    return true;
}

bool StateService::validateState(
    const dto::State& state,
    std::string& errorMessage
)
{
    if (!state.workflowId.has_value())
    {
        errorMessage = "Идентификатор рабочего процесса обязателен";
        return false;
    }

    if (!state.caption.has_value() || state.caption->empty())
    {
        errorMessage = "Название состояния обязательно для заполнения";
        return false;
    }

    if (state.caption->length() > 255)
    {
        errorMessage = "Название состояния не может превышать 255 символов";
        return false;
    }

    if (state.description.has_value() && state.description->length() > 1000)
    {
        errorMessage = "Описание состояния не может превышать 1000 символов";
        return false;
    }

    if (state.weight.has_value() && (*state.weight < 0 || *state.weight > 100))
    {
        errorMessage = "Вес состояния должен быть в диапазоне от 0 до 100";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
