#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/item_history.h"
#include "common/types.h"

// Добавляем using для DateTime
using DateTime = std::chrono::system_clock::time_point;

namespace server
{
namespace services
{

/**
 * @brief Результат операции с историей изменений.
 */
struct ItemHistoryResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с историей изменений.
 */
struct ItemHistoriesPage
{
    std::vector<dto::ItemHistory> histories;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с историей изменений элементов.
 */
class IItemHistoryService
{
public:
    virtual ~IItemHistoryService() = default;

    /**
     * @brief Получает список записей истории изменений с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param itemId Фильтр по элементу (опционально)
     * @param filterUserId Фильтр по пользователю (опционально)
     * @param dateFrom Фильтр по дате начала (опционально)
     * @param dateTo Фильтр по дате окончания (опционально)
     * @return Страница с записями истории
     */
    virtual ItemHistoriesPage getItemHistories(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Получает запись истории по ID.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return DTO записи или std::nullopt
     */
    virtual std::optional<dto::ItemHistory> getItemHistory(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает последнюю запись истории для элемента.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return DTO записи или std::nullopt
     */
    virtual std::optional<dto::ItemHistory> getLastItemHistory(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую запись истории.
     * @param history DTO записи истории
     * @param userId ID пользователя для проверки прав
     * @return Созданная запись или std::nullopt при ошибке
     */
    virtual std::optional<dto::ItemHistory> createItemHistory(
        const dto::ItemHistory& history,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет запись истории.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ItemHistoryResult deleteItemHistory(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
