#pragma once

#include <optional>
#include <vector>

#include "common/dto/item_document.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для связей элементов с документами.
 */
class IItemDocumentRepository
{
public:
    virtual ~IItemDocumentRepository() = default;

    /**
     * @brief Получает список связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param itemId Фильтр по элементу (std::nullopt - все)
     * @param documentId Фильтр по документу (std::nullopt - все)
     * @return Пара: вектор DTO связей и общее количество
     */
    virtual std::pair<std::vector<dto::ItemDocument>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) = 0;

    /**
     * @brief Находит связь по ID.
     * @param id Идентификатор связи
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::ItemDocument> findById(int64_t id) = 0;

    /**
     * @brief Находит все связи для элемента.
     * @param itemId Идентификатор элемента
     * @return Вектор DTO связей
     */
    virtual std::vector<dto::ItemDocument> findByItemId(int64_t itemId) = 0;

    /**
     * @brief Находит все связи для документа.
     * @param documentId Идентификатор документа
     * @return Вектор DTO связей
     */
    virtual std::vector<dto::ItemDocument> findByDocumentId(int64_t documentId) = 0;

    /**
     * @brief Находит связь по паре (itemId, documentId).
     * @param itemId Идентификатор элемента
     * @param documentId Идентификатор документа
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::ItemDocument> findByItemAndDocument(
        int64_t itemId,
        int64_t documentId
    ) = 0;

    /**
     * @brief Проверяет существование связи.
     * @param itemId Идентификатор элемента
     * @param documentId Идентификатор документа
     * @return true если связь существует
     */
    virtual bool exists(int64_t itemId, int64_t documentId) = 0;

    /**
     * @brief Создаёт новую связь элемента с документом.
     * @param itemDocument DTO связи
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::ItemDocument& itemDocument) = 0;

    /**
     * @brief Удаляет связь по ID.
     * @param id Идентификатор связи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все связи для элемента.
     * @param itemId Идентификатор элемента
     * @return Количество удалённых записей
     */
    virtual int64_t removeByItemId(int64_t itemId) = 0;

    /**
     * @brief Удаляет все связи для документа.
     * @param documentId Идентификатор документа
     * @return Количество удалённых записей
     */
    virtual int64_t removeByDocumentId(int64_t documentId) = 0;
};

} // namespace repositories
} // namespace server
