#include "common/log/log.h"

#include "item_history_service.h"

namespace server
{
namespace services
{

ItemHistoryService::ItemHistoryService(
    std::shared_ptr<repositories::IItemHistoryRepository> historyRepo,
    std::shared_ptr<IItemService> itemService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_historyRepo(std::move(historyRepo))
    , m_itemService(std::move(itemService))
    , m_authzService(std::move(authzService))
{
    if (!m_historyRepo || !m_itemService || !m_authzService)
    {
        throw std::runtime_error(
            "ItemHistoryService: один или несколько компонентов не инициализированы"
        );
    }
}

ItemHistoriesPage ItemHistoryService::getItemHistories(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> itemId,
    std::optional<int64_t> filterUserId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
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
            << "getItemHistories: пользователь " << userId
            << " не имеет доступа к элементу " << *itemId;
        return { {}, 0 };
    }

    // Если указан filterUserId, проверяем, что это либо сам пользователь, либо супер-админ
    if (filterUserId.has_value() && *filterUserId != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN
            << "getItemHistories: пользователь " << userId
            << " не имеет прав на просмотр истории пользователя " << *filterUserId;
        return { {}, 0 };
    }

    auto [histories, total] = m_historyRepo->findAll(
        page, pageSize, itemId, filterUserId, dateFrom, dateTo
    );
    return { histories, total };
}

std::optional<dto::ItemHistory> ItemHistoryService::getItemHistory(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "getItemHistory: неверный ID " << id;
        return std::nullopt;
    }

    auto history = m_historyRepo->findById(id);
    if (!history.has_value())
    {
        LOG_DEBUG << "getItemHistory: запись с id=" << id << " не найдена";
        return std::nullopt;
    }

    // Проверяем доступ к элементу
    if (!checkItemAccess(*history->itemId, userId, false))
    {
        LOG_WARN
            << "getItemHistory: пользователь " << userId
            << " не имеет доступа к элементу " << *history->itemId;
        return std::nullopt;
    }

    return history;
}

std::optional<dto::ItemHistory> ItemHistoryService::getLastItemHistory(
    int64_t itemId,
    int64_t userId
)
{
    if (itemId <= 0)
    {
        LOG_WARN << "getLastItemHistory: неверный itemId " << itemId;
        return std::nullopt;
    }

    // Проверяем доступ к элементу
    if (!checkItemAccess(itemId, userId, false))
    {
        LOG_WARN
            << "getLastItemHistory: пользователь " << userId
            << " не имеет доступа к элементу " << itemId;
        return std::nullopt;
    }

    return m_historyRepo->findLastByItemId(itemId);
}

std::optional<dto::ItemHistory> ItemHistoryService::createItemHistory(
    const dto::ItemHistory& history,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateItemHistory(history, errorMessage))
    {
        LOG_WARN << "createItemHistory: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем доступ к элементу (требуется право на запись для создания истории)
    if (!checkItemAccess(*history.itemId, userId, true))
    {
        LOG_WARN
            << "createItemHistory: пользователь " << userId
            << " не имеет прав на запись к элементу " << *history.itemId;
        return std::nullopt;
    }

    // 3. Создаём запись
    dto::ItemHistory newHistory = history;
    if (!newHistory.timestamp.has_value())
    {
        newHistory.timestamp = std::chrono::system_clock::now();
    }

    const int64_t newId = m_historyRepo->create(newHistory);
    if (newId <= 0)
    {
        LOG_ERROR << "createItemHistory: не удалось создать запись";
        return std::nullopt;
    }

    LOG_INFO
        << "Создана запись ItemHistory: id=" << newId
        << ", itemId=" << *newHistory.itemId
        << ", userId=" << *newHistory.userId
        << ", пользователь=" << userId;

    return m_historyRepo->findById(newId);
}

ItemHistoryResult ItemHistoryService::deleteItemHistory(
    int64_t id,
    int64_t userId
)
{
    ItemHistoryResult result;

    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор";
        result.errorCode = 400;
        return result;
    }

    auto existing = m_historyRepo->findById(id);
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
            << "deleteItemHistory: пользователь " << userId
            << " не имеет прав на запись к элементу " << *existing->itemId;
        return result;
    }

    if (!m_historyRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить запись";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Удалена запись ItemHistory: id=" << id
        << ", пользователь=" << userId;
    return result;
}

bool ItemHistoryService::checkItemAccess(
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

bool ItemHistoryService::validateItemHistory(
    const dto::ItemHistory& history,
    std::string& errorMessage
)
{
    if (!history.itemId.has_value())
    {
        errorMessage = "itemId является обязательным полем";
        return false;
    }

    if (!history.userId.has_value())
    {
        errorMessage = "userId является обязательным полем";
        return false;
    }

    if (history.diff.has_value() && history.diff->length() > 10000)
    {
        errorMessage = "diff не может превышать 10000 символов";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
