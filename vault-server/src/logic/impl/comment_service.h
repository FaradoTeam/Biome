#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/icomment_service.h"
#include "logic/iitem_service.h"
#include "repo/comment_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления комментариями.
 */
class CommentService final : public ICommentService
{
public:
    CommentService(
        std::shared_ptr<repositories::ICommentRepository> commentRepo,
        std::shared_ptr<IItemService> itemService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // ICommentService
    CommentsPage getComments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::Comment> getComment(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::Comment> getCommentsByItem(
        int64_t itemId,
        int64_t userId
    ) override;

    std::optional<dto::Comment> createComment(
        const dto::Comment& comment,
        int64_t userId
    ) override;

    std::optional<dto::Comment> updateComment(
        const dto::Comment& comment,
        int64_t userId
    ) override;

    CommentResult deleteComment(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к комментарию.
     * @param commentId ID комментария
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return DTO комментария или std::nullopt
     */
    std::optional<dto::Comment> checkCommentAccess(
        int64_t commentId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Проверяет доступ к элементу, связанному с комментарием.
     * @param itemId ID элемента
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return true если доступ разрешён
     */
    bool checkItemAccess(
        int64_t itemId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Валидирует DTO комментария.
     */
    bool validateComment(
        const dto::Comment& comment,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::ICommentRepository> m_commentRepo;
    std::shared_ptr<IItemService> m_itemService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
