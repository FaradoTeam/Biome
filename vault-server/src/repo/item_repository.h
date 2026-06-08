#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/item.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка элементов.
 */
struct ItemsPage
{
    std::vector<dto::Item> items;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с элементами (Items).
 */
class IItemRepository
{
public:
    virtual ~IItemRepository() = default;

    /**
     * @brief Получает список элементов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param itemTypeId Фильтр по типу элемента
     * @param parentId Фильтр по родительскому элементу
     * @param phaseId Фильтр по фазе
     * @param stateId Фильтр по состоянию
     * @param isDeleted Фильтр по статусу удаления
     * @param searchCaption Поиск по названию
     * @param projectIds Список ID проектов, доступных пользователю (для фильтрации через фазы)
     * @return Страница с элементами
     */
    virtual ItemsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt,
        std::optional<bool> isDeleted = std::nullopt,
        const std::string& searchCaption = "",
        const std::vector<int64_t>& projectIds = {}
    ) = 0;

    /**
     * @brief Находит элемент по ID.
     * @param id Идентификатор элемента
     * @return DTO элемента или std::nullopt
     */
    virtual std::optional<dto::Item> findById(int64_t id) = 0;

    /**
     * @brief Создает новый элемент.
     * @param item DTO элемента
     * @return ID созданного элемента или 0 при ошибке
     */
    virtual int64_t create(const dto::Item& item) = 0;

    /**
     * @brief Обновляет существующий элемент.
     * @param item DTO элемента с новыми данными
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Item& item) = 0;

    /**
     * @brief Мягкое удаление элемента.
     * @param id Идентификатор элемента
     * @return true если удаление успешно
     */
    virtual bool softDelete(int64_t id) = 0;

    /**
     * @brief Восстановление элемента из мягкого удаления.
     * @param id Идентификатор элемента
     * @return true если восстановление успешно
     */
    virtual bool restore(int64_t id) = 0;

    /**
     * @brief Полное удаление элемента из БД.
     * @param id Идентификатор элемента
     * @return true если удаление успешно
     */
    virtual bool hardDelete(int64_t id) = 0;

    /**
     * @brief Проверяет существование элемента.
     * @param id Идентификатор элемента
     * @return true если элемент существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Получает все дочерние элементы.
     * @param parentId Идентификатор родительского элемента
     * @param includeDeleted Включать ли удаленные элементы
     * @return Вектор дочерних элементов
     */
    virtual std::vector<dto::Item> findChildren(
        int64_t parentId,
        bool includeDeleted = false
    ) = 0;

    /**
     * @brief Получает корневые элементы проекта (без родителя).
     * @param phaseId Идентификатор фазы
     * @param includeDeleted Включать ли удаленные элементы
     * @return Вектор корневых элементов
     */
    virtual std::vector<dto::Item> findRootItems(
        int64_t phaseId,
        bool includeDeleted = false
    ) = 0;
};

} // namespace repositories
} // namespace server
