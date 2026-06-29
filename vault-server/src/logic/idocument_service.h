#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/document.h"

namespace server
{
namespace services
{

/**
 * @brief Страница с документами.
 */
struct DocumentsPage
{
    std::vector<dto::Document> documents;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с документом.
 */
struct DocumentResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для управления документами.
 */
class IDocumentService
{
public:
    virtual ~IDocumentService() = default;

    /**
     * @brief Получает список документов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param uploadedByUserId Фильтр по загрузившему пользователю (опционально)
     * @param searchCaption Поиск по названию (опционально)
     * @return Страница с документами
     */
    virtual DocumentsPage getDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> uploadedByUserId = std::nullopt,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает документ по ID.
     * @param id Идентификатор документа
     * @param userId ID пользователя для проверки прав
     * @return DTO документа или std::nullopt
     */
    virtual std::optional<dto::Document> getDocument(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новый документ.
     * @param document DTO документа
     * @param userId ID пользователя для проверки прав
     * @return Созданный документ или std::nullopt при ошибке
     */
    virtual std::optional<dto::Document> createDocument(
        const dto::Document& document,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий документ.
     * @param document DTO документа с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённый документ или std::nullopt при ошибке
     */
    virtual std::optional<dto::Document> updateDocument(
        const dto::Document& document,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет документ.
     * @param id Идентификатор документа
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual DocumentResult deleteDocument(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Проверяет, имеет ли пользователь доступ к документу.
     * @param documentId ID документа
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return DTO документа или std::nullopt при отсутствии доступа
     */
    virtual std::optional<dto::Document> checkDocumentAccess(
        int64_t documentId,
        int64_t userId,
        bool needWrite = false
    ) = 0;
};

} // namespace services
} // namespace server
