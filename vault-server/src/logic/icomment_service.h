#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/comment.h"
#include "common/types.h"

namespace server
{
namespace services
{

/**
 * @brief Страница с комментариями.
 */
struct CommentsPage
{
    std::vector<dto::Comment> comments;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с комментарием.
 */
struct CommentResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для управления комментариями.
 */
class ICommentService
{
public:
    virtual ~ICommentService() = default;

    /**
     * @brief Получает список комментариев с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param itemId Фильтр по элементу (опционально)
     * @param filterUserId Фильтр по автору (опционально)
     * @param dateFrom Фильтр по дате начала (опционально)
     * @param dateTo Фильтр по дате окончания (опционально)
     * @return Страница с комментариями
     */
    virtual CommentsPage getComments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Получает комментарий по ID.
     * @param id Идентификатор комментария
     * @param userId ID пользователя для проверки прав
     * @return DTO комментария или std::nullopt
     */
    virtual std::optional<dto::Comment> getComment(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все комментарии для элемента.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return Вектор комментариев
     */
    virtual std::vector<dto::Comment> getCommentsByItem(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новый комментарий.
     * @param comment DTO комментария
     * @param userId ID пользователя для проверки прав
     * @return Созданный комментарий или std::nullopt при ошибке
     */
    virtual std::optional<dto::Comment> createComment(
        const dto::Comment& comment,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий комментарий.
     * @param comment DTO комментария с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённый комментарий или std::nullopt при ошибке
     */
    virtual std::optional<dto::Comment> updateComment(
        const dto::Comment& comment,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет комментарий.
     * @param id Идентификатор комментария
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual CommentResult deleteComment(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
