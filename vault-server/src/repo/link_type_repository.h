#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/link_type.h"

namespace server::repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с типами связей (LinkType).
 */
class ILinkTypeRepository
{
public:
    virtual ~ILinkTypeRepository() = default;

    /**
     * @brief Получает список типов связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param sourceItemTypeId Фильтр по исходному типу элемента (опционально)
     * @param destinationItemTypeId Фильтр по целевому типу элемента (опционально)
     * @return Пара: вектор DTO типов связей и общее количество
     */
    virtual std::pair<std::vector<dto::LinkType>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> sourceItemTypeId = std::nullopt,
        std::optional<int64_t> destinationItemTypeId = std::nullopt
    ) = 0;

    /**
     * @brief Находит тип связи по ID.
     * @param id Идентификатор типа связи
     * @return DTO типа связи или std::nullopt
     */
    virtual std::optional<dto::LinkType> findById(int64_t id) = 0;

    /**
     * @brief Создаёт новый тип связи.
     * @param linkType DTO типа связи
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::LinkType& linkType) = 0;

    /**
     * @brief Обновляет существующий тип связи.
     * @param linkType DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::LinkType& linkType) = 0;

    /**
     * @brief Удаляет тип связи по ID.
     * @param id Идентификатор типа связи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование типа связи с указанным ID.
     * @param id Идентификатор типа связи
     * @return true если запись существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет, используется ли тип связи в каких-либо связях элементов.
     * @param id Идентификатор типа связи
     * @return true если есть хотя бы одна связь ItemLink с этим типом
     */
    virtual bool isUsed(int64_t id) = 0;
};

} // namespace server::repositories
