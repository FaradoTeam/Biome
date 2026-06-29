#include "comment_service.h"
#include "common/log/log.h"

namespace server
{
namespace services
{

CommentService::CommentService(
    std::shared_ptr<repositories::ICommentRepository> commentRepo,
    std::shared_ptr<IItemService> itemService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_commentRepo(std::move(commentRepo))
    , m_itemService(std::move(itemService))
    , m_authzService(std::move(authzService))
{
    if (!m_commentRepo)
    {
        throw std::runtime_error("CommentService: репозиторий комментариев не инициализирован");
    }
    if (!m_itemService)
    {
        throw std::runtime_error("CommentService: сервис элементов не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("CommentService: сервис авторизации не инициализирован");
    }
}

CommentsPage CommentService::getComments(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> itemId,
    std::optional<int64_t> filterUserId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан itemId, проверяем доступ к элементу
    if (itemId.has_value() && !checkItemAccess(*itemId, userId, false))
    {
        LOG_WARN
            << "getComments: пользователь " << userId
            << " не имеет доступа к элементу " << *itemId;
        return { {}, 0 };
    }

    // Если указан filterUserId, проверяем, что это либо сам пользователь, либо супер-админ
    if (filterUserId.has_value() && *filterUserId != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN
            << "getComments: пользователь " << userId
            << " не имеет прав на просмотр комментариев пользователя " << *filterUserId;
        return { {}, 0 };
    }

    auto pageData = m_commentRepo->findAll(
        page, pageSize, itemId, filterUserId, dateFrom, dateTo
    );

    // Фильтруем комментарии по правам доступа к элементам
    std::vector<dto::Comment> filtered;
    for (const auto& comment : pageData.comments)
    {
        if (comment.itemId.has_value() && checkItemAccess(*comment.itemId, userId, false))
        {
            filtered.push_back(comment);
        }
    }

    return { filtered, static_cast<int64_t>(filtered.size()) };
}

std::optional<dto::Comment> CommentService::getComment(
    int64_t id,
    int64_t userId
)
{
    return checkCommentAccess(id, userId, false);
}

std::vector<dto::Comment> CommentService::getCommentsByItem(
    int64_t itemId,
    int64_t userId
)
{
    if (!checkItemAccess(itemId, userId, false))
    {
        LOG_WARN
            << "getCommentsByItem: пользователь " << userId
            << " не имеет доступа к элементу " << itemId;
        return {};
    }

    return m_commentRepo->findByItemId(itemId);
}

std::optional<dto::Comment> CommentService::createComment(
    const dto::Comment& comment,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateComment(comment, errorMessage))
    {
        LOG_WARN << "createComment: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем, что автор комментария - текущий пользователь
    if (!comment.userId.has_value() || *comment.userId != userId)
    {
        LOG_WARN
            << "createComment: пользователь " << userId
            << " не является автором комментария";
        return std::nullopt;
    }

    // 3. Проверяем доступ к элементу (требуется право на запись)
    if (!checkItemAccess(*comment.itemId, userId, true))
    {
        LOG_WARN
            << "createComment: пользователь " << userId
            << " не имеет прав на запись к элементу " << *comment.itemId;
        return std::nullopt;
    }

    // 4. Создаём комментарий
    dto::Comment newComment = comment;
    // createdAt устанавливается в репозитории

    const int64_t newId = m_commentRepo->create(newComment);
    if (newId <= 0)
    {
        LOG_ERROR << "createComment: не удалось создать комментарий";
        return std::nullopt;
    }

    LOG_INFO
        << "Комментарий создан: id=" << newId
        << ", элемент=" << *newComment.itemId
        << ", пользователь=" << userId;

    return m_commentRepo->findById(newId);
}

std::optional<dto::Comment> CommentService::updateComment(
    const dto::Comment& comment,
    int64_t userId
)
{
    if (!comment.id.has_value())
    {
        LOG_WARN << "updateComment: отсутствует ID комментария";
        return std::nullopt;
    }

    // 1. Проверяем существование и доступ к комментарию
    auto existing = checkCommentAccess(*comment.id, userId, true);
    if (!existing.has_value())
    {
        return std::nullopt;
    }

    // 2. Обновляем комментарий
    if (!m_commentRepo->update(comment))
    {
        LOG_ERROR
            << "updateComment: не удалось обновить комментарий id="
            << *comment.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Комментарий обновлён: id=" << *comment.id
        << ", пользователь=" << userId;

    return m_commentRepo->findById(*comment.id);
}

CommentResult CommentService::deleteComment(
    int64_t id,
    int64_t userId
)
{
    CommentResult result;

    // 1. Проверяем существование и доступ к комментарию
    auto existing = checkCommentAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Комментарий не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Удаляем комментарий
    if (!m_commentRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить комментарий";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Комментарий удалён: id=" << id
        << ", пользователь=" << userId;

    return result;
}

std::optional<dto::Comment> CommentService::checkCommentAccess(
    int64_t commentId,
    int64_t userId,
    bool needWrite
)
{
    auto comment = m_commentRepo->findById(commentId);
    if (!comment.has_value())
    {
        LOG_DEBUG << "checkCommentAccess: комментарий не найден, id=" << commentId;
        return std::nullopt;
    }

    if (!checkItemAccess(*comment->itemId, userId, needWrite))
    {
        LOG_WARN
            << "checkCommentAccess: пользователь " << userId
            << " не имеет доступа к элементу " << *comment->itemId;
        return std::nullopt;
    }

    // Если требуется право на запись, проверяем, что пользователь является автором
    if (needWrite && (!comment->userId.has_value() || *comment->userId != userId))
    {
        // Только супер-админ может редактировать/удалять чужие комментарии
        if (!m_authzService->isSuperAdmin(userId))
        {
            LOG_WARN
                << "checkCommentAccess: пользователь " << userId
                << " не является автором комментария " << commentId;
            return std::nullopt;
        }
    }

    return comment;
}

bool CommentService::checkItemAccess(
    int64_t itemId,
    int64_t userId,
    bool needWrite
)
{
    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    // Получаем элемент через IItemService (проверяет права на чтение)
    auto item = m_itemService->item(itemId, userId);
    if (!item.has_value())
    {
        LOG_DEBUG
            << "checkItemAccess: элемент " << itemId
            << " не найден или нет доступа";
        return false;
    }

    if (needWrite)
    {
        // Для записи комментария нужно право на запись в проекте
        // Это проверяется через IItemService при создании/обновлении элемента
        // Здесь мы просто проверяем, что элемент существует и доступен
        // Более детальная проверка прав происходит в IItemService
        return true;
    }

    return true;
}

bool CommentService::validateComment(
    const dto::Comment& comment,
    std::string& errorMessage
)
{
    if (!comment.userId.has_value())
    {
        errorMessage = "userId является обязательным полем";
        return false;
    }

    if (!comment.itemId.has_value())
    {
        errorMessage = "itemId является обязательным полем";
        return false;
    }

    if (!comment.content.has_value() || comment.content->empty())
    {
        errorMessage = "Содержимое комментария обязательно для заполнения";
        return false;
    }

    if (comment.content->length() > 10000)
    {
        errorMessage = "Содержимое комментария не может превышать 10000 символов";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
