#pragma once

#include <optional>
#include <vector>

#include "common/dto/item_link.h"

namespace server::repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы со связями элементов (ItemLink).
 */
class IItemLinkRepository
{
public:
    virtual ~IItemLinkRepository() = default;

    /**
     * @brief Получает список связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param linkTypeId Фильтр по типу связи (опционально)
     * @param sourceItemId Фильтр по исходному элементу (опционально)
     * @param destinationItemId Фильтр по целевому элементу (опционально)
     * @return Пара: вектор DTO связей и общее количество
     */
    virtual std::pair<std::vector<dto::ItemLink>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> linkTypeId = std::nullopt,
        std::optional<int64_t> sourceItemId = std::nullopt,
        std::optional<int64_t> destinationItemId = std::nullopt
    ) = 0;

    /**
     * @brief Находит связь по ID.
     * @param id Идентификатор связи
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::ItemLink> findById(int64_t id) = 0;

    /**
     * @brief Находит все связи, в которых участвует указанный элемент (как источник или цель).
     * @param itemId Идентификатор элемента
     * @return Вектор DTO связей
     */
    virtual std::vector<dto::ItemLink> findByItemId(int64_t itemId) = 0;

    /**
     * @brief Находит связи по идентификатору типа связи.
     * @param linkTypeId Идентификатор типа связи
     * @return Вектор DTO связей
     */
    virtual std::vector<dto::ItemLink> findByLinkTypeId(int64_t linkTypeId) = 0;

    /**
     * @brief Создаёт новую связь между элементами.
     * @param itemLink DTO связи
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::ItemLink& itemLink) = 0;

    /**
     * @brief Обновляет существующую связь.
     * @param itemLink DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::ItemLink& itemLink) = 0;

    /**
     * @brief Удаляет связь по ID.
     * @param id Идентификатор связи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование связи с указанным ID.
     * @param id Идентификатор связи
     * @return true если запись существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет существование связи по трём полям (уникальность).
     * @param linkTypeId Тип связи
     * @param sourceItemId Исходный элемент
     * @param destinationItemId Целевой элемент
     * @return true если такая связь уже существует
     */
    virtual bool existsByTriple(
        int64_t linkTypeId,
        int64_t sourceItemId,
        int64_t destinationItemId
    ) = 0;

    /**
     * @brief Удаляет все связи, в которых участвует указанный элемент.
     * @param itemId Идентификатор элемента
     * @return Количество удалённых записей
     */
    virtual int64_t removeByItemId(int64_t itemId) = 0;
};

} // namespace server::repositories
