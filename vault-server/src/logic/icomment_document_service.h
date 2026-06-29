#pragma once

#include <optional>
#include <vector>

#include "common/dto/comment_document.h"

namespace server
{
namespace services
{

/**
 * @brief Страница со связями комментариев и документов.
 */
struct CommentDocumentsPage
{
    std::vector<dto::CommentDocument> items;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции со связью комментария и документа.
 */
struct CommentDocumentResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для управления связями комментариев и документов.
 */
class ICommentDocumentService
{
public:
    virtual ~ICommentDocumentService() = default;

    /**
     * @brief Получает список связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param commentId Фильтр по комментарию (опционально)
     * @param documentId Фильтр по документу (опционально)
     * @return Страница со связями
     */
    virtual CommentDocumentsPage getCommentDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> commentId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) = 0;

    /**
     * @brief Получает связь по ID.
     * @param id Идентификатор связи
     * @param userId ID пользователя для проверки прав
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::CommentDocument> getCommentDocument(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все документы, привязанные к комментарию.
     * @param commentId Идентификатор комментария
     * @param userId ID пользователя для проверки прав
     * @return Вектор связей
     */
    virtual std::vector<dto::CommentDocument> getDocumentsByComment(
        int64_t commentId,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все комментарии, к которым привязан документ.
     * @param documentId Идентификатор документа
     * @param userId ID пользователя для проверки прав
     * @return Вектор связей
     */
    virtual std::vector<dto::CommentDocument> getCommentsByDocument(
        int64_t documentId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую связь комментария с документом.
     * @param commentDocument DTO связи
     * @param userId ID пользователя для проверки прав
     * @return Созданная связь или std::nullopt при ошибке
     */
    virtual std::optional<dto::CommentDocument> createCommentDocument(
        const dto::CommentDocument& commentDocument,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет связь комментария с документом.
     * @param id Идентификатор связи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual CommentDocumentResult deleteCommentDocument(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет все связи комментария с документами.
     * @param commentId Идентификатор комментария
     * @param userId ID пользователя для проверки прав
     * @return Количество удалённых связей
     */
    virtual int64_t deleteCommentDocumentsByComment(
        int64_t commentId,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет все связи документа с комментариями.
     * @param documentId Идентификатор документа
     * @param userId ID пользователя для проверки прав
     * @return Количество удалённых связей
     */
    virtual int64_t deleteCommentDocumentsByDocument(
        int64_t documentId,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
