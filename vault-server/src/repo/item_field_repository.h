#pragma once

#include <optional>
#include <vector>

#include "common/dto/item_field.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка значений полей.
 */
struct ItemFieldsPage
{
    std::vector<dto::ItemField> fields;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы со значениями полей элементов.
 */
class IItemFieldRepository
{
public:
    virtual ~IItemFieldRepository() = default;

    /**
     * @brief Получает список значений полей с пагинацией.
     * @param page Номер страницы
     * @param pageSize Количество записей на странице
     * @param itemId Фильтр по элементу
     * @param fieldTypeId Фильтр по типу поля
     * @return Страница со значениями полей
     */
    virtual ItemFieldsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) = 0;

    /**
     * @brief Находит значение поля по ID.
     * @param id Идентификатор записи
     * @return DTO значения поля или std::nullopt
     */
    virtual std::optional<dto::ItemField> findById(int64_t id) = 0;

    /**
     * @brief Находит значение поля по элементу и типу поля.
     * @param itemId Идентификатор элемента
     * @param fieldTypeId Идентификатор типа поля
     * @return DTO значения поля или std::nullopt
     */
    virtual std::optional<dto::ItemField> findByItemAndFieldType(
        int64_t itemId,
        int64_t fieldTypeId
    ) = 0;

    /**
     * @brief Получает все значения полей для элемента.
     * @param itemId Идентификатор элемента
     * @return Вектор значений полей
     */
    virtual std::vector<dto::ItemField> findByItemId(int64_t itemId) = 0;

    /**
     * @brief Создает новое значение поля.
     * @param field DTO значения поля
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::ItemField& field) = 0;

    /**
     * @brief Обновляет существующее значение поля.
     * @param field DTO значения поля с новыми данными
     * @return true если обновление успешно
     */
    virtual bool update(const dto::ItemField& field) = 0;

    /**
     * @brief Удаляет значение поля.
     * @param id Идентификатор записи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все значения полей для элемента.
     * @param itemId Идентификатор элемента
     * @return Количество удаленных записей
     */
    virtual int64_t removeByItemId(int64_t itemId) = 0;

    /**
     * @brief Проверяет существование значения поля.
     * @param id Идентификатор записи
     * @return true если запись существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет существование значения поля для элемента и типа.
     * @param itemId Идентификатор элемента
     * @param fieldTypeId Идентификатор типа поля
     * @return true если значение существует
     */
    virtual bool existsByItemAndFieldType(int64_t itemId, int64_t fieldTypeId) = 0;
};

} // namespace repositories
} // namespace server
