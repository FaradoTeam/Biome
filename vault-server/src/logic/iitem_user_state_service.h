#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/item_user_state.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с историей состояний.
 */
struct ItemUserStateResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с историей состояний.
 */
struct ItemUserStatesPage
{
    std::vector<dto::ItemUserState> states;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с историей состояний элементов.
 */
class IItemUserStateService
{
public:
    virtual ~IItemUserStateService() = default;

    /**
     * @brief Получает список записей истории состояний с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param itemId Фильтр по элементу (опционально)
     * @param filterUserId Фильтр по пользователю (опционально)
     * @return Страница с записями истории
     */
    virtual ItemUserStatesPage getItemUserStates(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt
    ) = 0;

    /**
     * @brief Получает запись истории по ID.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return DTO записи или std::nullopt
     */
    virtual std::optional<dto::ItemUserState> getItemUserState(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает последнюю запись истории для элемента.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return DTO записи или std::nullopt
     */
    virtual std::optional<dto::ItemUserState> getLastItemUserState(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую запись истории состояния.
     * @param state DTO записи истории
     * @param userId ID пользователя для проверки прав
     * @return Созданная запись или std::nullopt при ошибке
     */
    virtual std::optional<dto::ItemUserState> createItemUserState(
        const dto::ItemUserState& state,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет запись истории.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ItemUserStateResult deleteItemUserState(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
