#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/item_type.h"

namespace server
{
namespace services
{

/**
 * @brief Структура для возврата результатов пагинированного списка типов элементов.
 */
struct ItemTypesPage
{
    std::vector<dto::ItemType> itemTypes;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления типами элементов.
 */
class IItemTypeService
{
public:
    virtual ~IItemTypeService() = default;

    /**
     * @brief Получает список типов элементов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param workflowId Фильтр по рабочему процессу
     * @param kind Фильтр по виду элемента
     * @param searchCaption Поиск по названию
     * @return Страница с типами элементов
     */
    virtual ItemTypesPage itemTypes(
        int page,
        int pageSize,
        std::optional<int64_t> workflowId = std::nullopt,
        std::optional<std::string> kind = std::nullopt,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает тип элемента по ID.
     * @param id Идентификатор типа элемента
     * @return DTO типа элемента или std::nullopt
     */
    virtual std::optional<dto::ItemType> itemType(int64_t id) = 0;

    /**
     * @brief Создает новый тип элемента.
     * @param itemType DTO типа элемента
     * @param userId ID пользователя для проверки прав
     * @return Созданный тип элемента или std::nullopt при ошибке
     */
    virtual std::optional<dto::ItemType> createItemType(
        const dto::ItemType& itemType,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий тип элемента.
     * @param itemType DTO типа элемента с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленный тип элемента или std::nullopt при ошибке
     */
    virtual std::optional<dto::ItemType> updateItemType(
        const dto::ItemType& itemType,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет тип элемента.
     * @param id Идентификатор типа элемента
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteItemType(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает типы элементов для рабочего процесса.
     * @param workflowId Идентификатор рабочего процесса
     * @return Вектор типов элементов
     */
    virtual std::vector<dto::ItemType> itemTypesByWorkflow(int64_t workflowId) = 0;
};

} // namespace services
} // namespace server
