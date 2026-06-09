#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iitem_service.h"
#include "logic/iitem_user_state_service.h"
#include "repo/item_user_state_repository.h"
#include "repo/state_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с историей состояний элементов.
 */
class ItemUserStateService final : public IItemUserStateService
{
public:
    ItemUserStateService(
        std::shared_ptr<repositories::IItemUserStateRepository> stateRepo,
        std::shared_ptr<repositories::IStateRepository> stateTypeRepo,
        std::shared_ptr<IItemService> itemService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IItemUserStateService
    ItemUserStatesPage getItemUserStates(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt
    ) override;

    std::optional<dto::ItemUserState> getItemUserState(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::ItemUserState> getLastItemUserState(
        int64_t itemId,
        int64_t userId
    ) override;

    std::optional<dto::ItemUserState> createItemUserState(
        const dto::ItemUserState& state,
        int64_t userId
    ) override;

    ItemUserStateResult deleteItemUserState(
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
    bool validateItemUserState(
        const dto::ItemUserState& state,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IItemUserStateRepository> m_stateRepo;
    std::shared_ptr<repositories::IStateRepository> m_stateTypeRepo;
    std::shared_ptr<IItemService> m_itemService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
