#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/document.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы с документами.
 */
class IDocumentRepository
{
public:
    virtual ~IDocumentRepository() = default;

    /**
     * @brief Получает список документов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param uploadedByUserId Фильтр по загрузившему пользователю (std::nullopt - все)
     * @param searchCaption Поиск по названию (пустая строка - без поиска)
     * @return Пара: вектор DTO документов и общее количество
     */
    virtual std::pair<std::vector<dto::Document>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> uploadedByUserId = std::nullopt,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Находит документ по ID.
     * @param id Идентификатор документа
     * @return DTO документа или std::nullopt
     */
    virtual std::optional<dto::Document> findById(int64_t id) = 0;

    /**
     * @brief Создаёт новый документ.
     * @param document DTO документа
     * @return ID созданного документа или 0 при ошибке
     */
    virtual int64_t create(const dto::Document& document) = 0;

    /**
     * @brief Обновляет существующий документ.
     * @param document DTO документа с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Document& document) = 0;

    /**
     * @brief Удаляет документ.
     * @param id Идентификатор документа
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование документа с указанным ID.
     * @param id Идентификатор документа
     * @return true если документ существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Проверяет, что путь к файлу уникален.
     * @param path Путь к файлу
     * @param excludeId ID документа для исключения (при обновлении)
     * @return true если путь уникален
     */
    virtual bool isPathUnique(
        const std::string& path,
        std::optional<int64_t> excludeId = std::nullopt
    ) = 0;
};

} // namespace repositories
} // namespace server
