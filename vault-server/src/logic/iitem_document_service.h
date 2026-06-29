#pragma once

#include <optional>
#include <vector>

#include "common/dto/item_document.h"

namespace server
{
namespace services
{

/**
 * @brief Страница со связями элементов и документов.
 */
struct ItemDocumentsPage
{
    std::vector<dto::ItemDocument> items;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции со связью элемента и документа.
 */
struct ItemDocumentResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для управления связями элементов и документов.
 */
class IItemDocumentService
{
public:
    virtual ~IItemDocumentService() = default;

    /**
     * @brief Получает список связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param itemId Фильтр по элементу (опционально)
     * @param documentId Фильтр по документу (опционально)
     * @return Страница со связями
     */
    virtual ItemDocumentsPage getItemDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) = 0;

    /**
     * @brief Получает связь по ID.
     * @param id Идентификатор связи
     * @param userId ID пользователя для проверки прав
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::ItemDocument> getItemDocument(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все документы, привязанные к элементу.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return Вектор связей
     */
    virtual std::vector<dto::ItemDocument> getDocumentsByItem(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все элементы, к которым привязан документ.
     * @param documentId Идентификатор документа
     * @param userId ID пользователя для проверки прав
     * @return Вектор связей
     */
    virtual std::vector<dto::ItemDocument> getItemsByDocument(
        int64_t documentId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую связь элемента с документом.
     * @param itemDocument DTO связи
     * @param userId ID пользователя для проверки прав
     * @return Созданная связь или std::nullopt при ошибке
     */
    virtual std::optional<dto::ItemDocument> createItemDocument(
        const dto::ItemDocument& itemDocument,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет связь элемента с документом.
     * @param id Идентификатор связи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ItemDocumentResult deleteItemDocument(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет все связи элемента с документами.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return Количество удалённых связей
     */
    virtual int64_t deleteItemDocumentsByItem(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет все связи документа с элементами.
     * @param documentId Идентификатор документа
     * @param userId ID пользователя для проверки прав
     * @return Количество удалённых связей
     */
    virtual int64_t deleteItemDocumentsByDocument(
        int64_t documentId,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
