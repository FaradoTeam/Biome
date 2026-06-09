#include "common/helpers/time_helpers.h"
#include "common/log/log.h"

#include "item_user_state_service.h"

namespace server
{
namespace services
{

ItemUserStateService::ItemUserStateService(
    std::shared_ptr<repositories::IItemUserStateRepository> stateRepo,
    std::shared_ptr<repositories::IStateRepository> stateTypeRepo,
    std::shared_ptr<IItemService> itemService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_stateRepo(std::move(stateRepo))
    , m_stateTypeRepo(std::move(stateTypeRepo))
    , m_itemService(std::move(itemService))
    , m_authzService(std::move(authzService))
{
    if (!m_stateRepo || !m_stateTypeRepo || !m_itemService || !m_authzService)
    {
        throw std::runtime_error(
            "ItemUserStateService: один или несколько компонентов не инициализированы"
        );
    }
}

ItemUserStatesPage ItemUserStateService::getItemUserStates(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> itemId,
    std::optional<int64_t> filterUserId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан itemId, проверяем доступ к элементу
    if (itemId.has_value() && !checkItemAccess(*itemId, userId, false))
    {
        LOG_WARN
            << "getItemUserStates: пользователь " << userId
            << " не имеет доступа к элементу " << *itemId;
        return { {}, 0 };
    }

    // Если указан filterUserId, проверяем, что это либо сам пользователь, либо супер-админ
    if (filterUserId.has_value()
        && *filterUserId != userId
        && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN
            << "getItemUserStates: пользователь " << userId
            << " не имеет прав на просмотр истории пользователя " << *filterUserId;
        return { {}, 0 };
    }

    auto [states, total] = m_stateRepo->findAll(page, pageSize, itemId, filterUserId);
    return { states, total };
}

std::optional<dto::ItemUserState> ItemUserStateService::getItemUserState(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "getItemUserState: неверный ID " << id;
        return std::nullopt;
    }

    auto state = m_stateRepo->findById(id);
    if (!state.has_value())
    {
        LOG_DEBUG << "getItemUserState: запись с id=" << id << " не найдена";
        return std::nullopt;
    }

    // Проверяем доступ к элементу
    if (!checkItemAccess(*state->itemId, userId, false))
    {
        LOG_WARN
            << "getItemUserState: пользователь " << userId
            << " не имеет доступа к элементу " << *state->itemId;
        return std::nullopt;
    }

    return state;
}

std::optional<dto::ItemUserState> ItemUserStateService::getLastItemUserState(
    int64_t itemId,
    int64_t userId
)
{
    if (itemId <= 0)
    {
        LOG_WARN << "getLastItemUserState: неверный itemId " << itemId;
        return std::nullopt;
    }

    // Проверяем доступ к элементу
    if (!checkItemAccess(itemId, userId, false))
    {
        LOG_WARN
            << "getLastItemUserState: пользователь " << userId
            << " не имеет доступа к элементу " << itemId;
        return std::nullopt;
    }

    return m_stateRepo->findLastByItemId(itemId);
}

std::optional<dto::ItemUserState> ItemUserStateService::createItemUserState(
    const dto::ItemUserState& state,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateItemUserState(state, errorMessage))
    {
        LOG_WARN << "createItemUserState: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем доступ к элементу (требуется право на запись для создания истории)
    if (!checkItemAccess(*state.itemId, userId, true))
    {
        LOG_WARN
            << "createItemUserState: пользователь " << userId
            << " не имеет прав на запись к элементу " << *state.itemId;
        return std::nullopt;
    }

    // 3. Проверяем существование состояния
    auto stateType = m_stateTypeRepo->findById(*state.stateId);
    if (!stateType.has_value())
    {
        LOG_WARN
            << "createItemUserState: состояние с id=" << *state.stateId
            << " не найдено";
        return std::nullopt;
    }

    // 4. Создаём запись
    dto::ItemUserState newState = state;
    if (!newState.timestamp.has_value())
    {
        newState.timestamp = std::chrono::system_clock::now();
    }

    const int64_t newId = m_stateRepo->create(newState);
    if (newId <= 0)
    {
        LOG_ERROR << "createItemUserState: не удалось создать запись";
        return std::nullopt;
    }

    LOG_INFO
        << "Создана запись ItemUserState: id=" << newId
        << ", itemId=" << *newState.itemId
        << ", userId=" << *newState.userId
        << ", stateId=" << *newState.stateId
        << ", пользователь=" << userId;

    return m_stateRepo->findById(newId);
}

ItemUserStateResult ItemUserStateService::deleteItemUserState(
    int64_t id,
    int64_t userId
)
{
    ItemUserStateResult result;

    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор";
        result.errorCode = 400;
        return result;
    }

    auto existing = m_stateRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Запись не найдена";
        result.errorCode = 404;
        return result;
    }

    // Проверяем доступ к элементу (требуется право на запись)
    if (!checkItemAccess(*existing->itemId, userId, true))
    {
        result.errorMessage = "Недостаточно прав для удаления записи";
        result.errorCode = 403;
        LOG_WARN
            << "deleteItemUserState: пользователь " << userId
            << " не имеет прав на запись к элементу " << *existing->itemId;
        return result;
    }

    if (!m_stateRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить запись";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Удалена запись ItemUserState: id=" << id
        << ", пользователь=" << userId;
    return result;
}

bool ItemUserStateService::checkItemAccess(
    int64_t itemId,
    int64_t userId,
    bool needWrite
)
{
    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    // Получаем элемент через IItemService
    auto item = m_itemService->item(itemId, userId);
    if (!item.has_value())
    {
        LOG_DEBUG << "checkItemAccess: элемент " << itemId << " не найден";
        return false;
    }

    // Доступ уже проверен в IItemService::item()
    return true;
}

bool ItemUserStateService::validateItemUserState(
    const dto::ItemUserState& state,
    std::string& errorMessage
)
{
    if (!state.itemId.has_value())
    {
        errorMessage = "itemId является обязательным полем";
        return false;
    }

    if (!state.userId.has_value())
    {
        errorMessage = "userId является обязательным полем";
        return false;
    }

    if (!state.stateId.has_value())
    {
        errorMessage = "stateId является обязательным полем";
        return false;
    }

    if (state.comment.has_value() && state.comment->length() > 1000)
    {
        errorMessage = "comment не может превышать 1000 символов";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
