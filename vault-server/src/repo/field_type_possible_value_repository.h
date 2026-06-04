#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/field_type_possible_value.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для возможных значений полей.
 */
class IFieldTypePossibleValueRepository
{
public:
    virtual ~IFieldTypePossibleValueRepository() = default;

    /**
     * @brief Получает список возможных значений с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param fieldTypeId Фильтр по типу поля (std::nullopt - все)
     * @return Пара: вектор DTO возможных значений и общее количество
     */
    virtual std::pair<std::vector<dto::FieldTypePossibleValue>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) = 0;

    /**
     * @brief Находит возможное значение по ID.
     * @param id Идентификатор записи
     * @return DTO возможного значения или std::nullopt
     */
    virtual std::optional<dto::FieldTypePossibleValue> findById(int64_t id) = 0;

    /**
     * @brief Находит все возможные значения для указанного типа поля.
     * @param fieldTypeId Идентификатор типа поля
     * @return Вектор DTO возможных значений
     */
    virtual std::vector<dto::FieldTypePossibleValue> findByFieldTypeId(int64_t fieldTypeId) = 0;

    /**
     * @brief Создаёт новое возможное значение.
     * @param value DTO возможного значения
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::FieldTypePossibleValue& value) = 0;

    /**
     * @brief Обновляет существующее возможное значение.
     * @param value DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::FieldTypePossibleValue& value) = 0;

    /**
     * @brief Удаляет возможное значение.
     * @param id Идентификатор записи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование возможного значения с указанным ID.
     * @param id Идентификатор записи
     * @return true если запись существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет существование значения для указанного типа поля.
     * @param fieldTypeId Идентификатор типа поля
     * @param value Значение для проверки
     * @return true если такое значение уже существует
     */
    virtual bool existsByValue(int64_t fieldTypeId, const std::string& value) = 0;
};

} // namespace repositories
} // namespace server
