#pragma once

#include <memory>

#include "common/types.h"

#include "logic/iauthorization_service.h"
#include "logic/iitem_service.h"
#include "logic/iitem_history_service.h"
#include "repo/item_history_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с историей изменений элементов.
 */
class ItemHistoryService final : public IItemHistoryService
{
public:
    ItemHistoryService(
        std::shared_ptr<repositories::IItemHistoryRepository> historyRepo,
        std::shared_ptr<IItemService> itemService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IItemHistoryService
    ItemHistoriesPage getItemHistories(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::ItemHistory> getItemHistory(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::ItemHistory> getLastItemHistory(
        int64_t itemId,
        int64_t userId
    ) override;

    std::optional<dto::ItemHistory> createItemHistory(
        const dto::ItemHistory& history,
        int64_t userId
    ) override;

    ItemHistoryResult deleteItemHistory(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к элементу.
     * @param itemId ID элемента
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return true если доступ разрешён
     */
    bool checkItemAccess(int64_t itemId, int64_t userId, bool needWrite = false);

    /**
     * @brief Валидирует DTO записи истории.
     */
    bool validateItemHistory(
        const dto::ItemHistory& history,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IItemHistoryRepository> m_historyRepo;
    std::shared_ptr<IItemService> m_itemService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
