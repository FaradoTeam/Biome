#pragma once

#include <optional>
#include <vector>

#include "common/dto/comment_document.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для связей комментариев с документами.
 */
class ICommentDocumentRepository
{
public:
    virtual ~ICommentDocumentRepository() = default;

    /**
     * @brief Получает список связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param commentId Фильтр по комментарию (std::nullopt - все)
     * @param documentId Фильтр по документу (std::nullopt - все)
     * @return Пара: вектор DTO связей и общее количество
     */
    virtual std::pair<std::vector<dto::CommentDocument>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> commentId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) = 0;

    /**
     * @brief Находит связь по ID.
     * @param id Идентификатор связи
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::CommentDocument> findById(int64_t id) = 0;

    /**
     * @brief Находит все связи для комментария.
     * @param commentId Идентификатор комментария
     * @return Вектор DTO связей
     */
    virtual std::vector<dto::CommentDocument> findByCommentId(int64_t commentId) = 0;

    /**
     * @brief Находит все связи для документа.
     * @param documentId Идентификатор документа
     * @return Вектор DTO связей
     */
    virtual std::vector<dto::CommentDocument> findByDocumentId(int64_t documentId) = 0;

    /**
     * @brief Находит связь по паре (commentId, documentId).
     * @param commentId Идентификатор комментария
     * @param documentId Идентификатор документа
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::CommentDocument> findByCommentAndDocument(
        int64_t commentId,
        int64_t documentId
    ) = 0;

    /**
     * @brief Проверяет существование связи.
     * @param commentId Идентификатор комментария
     * @param documentId Идентификатор документа
     * @return true если связь существует
     */
    virtual bool exists(int64_t commentId, int64_t documentId) = 0;

    /**
     * @brief Создаёт новую связь комментария с документом.
     * @param commentDocument DTO связи
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::CommentDocument& commentDocument) = 0;

    /**
     * @brief Удаляет связь по ID.
     * @param id Идентификатор связи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все связи для комментария.
     * @param commentId Идентификатор комментария
     * @return Количество удалённых записей
     */
    virtual int64_t removeByCommentId(int64_t commentId) = 0;

    /**
     * @brief Удаляет все связи для документа.
     * @param documentId Идентификатор документа
     * @return Количество удалённых записей
     */
    virtual int64_t removeByDocumentId(int64_t documentId) = 0;
};

} // namespace repositories
} // namespace server
