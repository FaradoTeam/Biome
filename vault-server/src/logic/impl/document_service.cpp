#include "document_service.h"
#include "common/log/log.h"

namespace server
{
namespace services
{

DocumentService::DocumentService(
    std::shared_ptr<repositories::IDocumentRepository> documentRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_documentRepo(std::move(documentRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_documentRepo)
    {
        throw std::runtime_error("DocumentService: репозиторий документов не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("DocumentService: сервис авторизации не инициализирован");
    }
}

DocumentsPage DocumentService::getDocuments(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> uploadedByUserId,
    const std::string& searchCaption
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан фильтр по пользователю, проверяем, что это либо сам пользователь, либо супер-админ
    if (uploadedByUserId.has_value() && *uploadedByUserId != userId && !m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN
            << "getDocuments: пользователь " << userId
            << " не имеет прав на просмотр документов пользователя " << *uploadedByUserId;
        return { {}, 0 };
    }

    auto [documents, total] = m_documentRepo->findAll(
        page, pageSize, uploadedByUserId, searchCaption
    );

    // Фильтруем документы по правам доступа
    std::vector<dto::Document> filtered;
    for (const auto& doc : documents)
    {
        if (checkDocumentPermissions(doc, userId, false))
        {
            filtered.push_back(doc);
        }
    }

    return { filtered, static_cast<int64_t>(filtered.size()) };
}

std::optional<dto::Document> DocumentService::getDocument(
    int64_t id,
    int64_t userId
)
{
    return checkDocumentAccess(id, userId, false);
}

std::optional<dto::Document> DocumentService::createDocument(
    const dto::Document& document,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateDocument(document, errorMessage))
    {
        LOG_WARN << "createDocument: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем, что пользователь является загрузившим
    if (!document.uploadedByUserId.has_value() || *document.uploadedByUserId != userId)
    {
        LOG_WARN
            << "createDocument: пользователь " << userId
            << " не является владельцем документа";
        return std::nullopt;
    }

    // 3. Создаём документ
    dto::Document newDocument = document;
    // uploadedAt устанавливается в репозитории

    const int64_t newId = m_documentRepo->create(newDocument);
    if (newId <= 0)
    {
        LOG_ERROR << "createDocument: не удалось создать документ";
        return std::nullopt;
    }

    LOG_INFO
        << "Документ создан: id=" << newId
        << ", caption=" << *newDocument.caption
        << ", пользователь=" << userId;

    return m_documentRepo->findById(newId);
}

std::optional<dto::Document> DocumentService::updateDocument(
    const dto::Document& document,
    int64_t userId
)
{
    if (!document.id.has_value())
    {
        LOG_WARN << "updateDocument: отсутствует ID документа";
        return std::nullopt;
    }

    // 1. Проверяем существование и доступ к документу
    auto existing = checkDocumentAccess(*document.id, userId, true);
    if (!existing.has_value())
    {
        return std::nullopt;
    }

    // 2. Обновляем документ
    if (!m_documentRepo->update(document))
    {
        LOG_ERROR
            << "updateDocument: не удалось обновить документ id="
            << *document.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Документ обновлён: id=" << *document.id
        << ", пользователь=" << userId;

    return m_documentRepo->findById(*document.id);
}

DocumentResult DocumentService::deleteDocument(
    int64_t id,
    int64_t userId
)
{
    DocumentResult result;

    // 1. Проверяем существование и доступ к документу
    auto existing = checkDocumentAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Документ не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Удаляем документ
    if (!m_documentRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить документ";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Документ удалён: id=" << id
        << ", пользователь=" << userId;

    return result;
}

std::optional<dto::Document> DocumentService::checkDocumentAccess(
    int64_t documentId,
    int64_t userId,
    bool needWrite
)
{
    auto document = m_documentRepo->findById(documentId);
    if (!document.has_value())
    {
        LOG_DEBUG
            << "checkDocumentAccess: документ не найден, id=" << documentId;
        return std::nullopt;
    }

    if (!checkDocumentPermissions(*document, userId, needWrite))
    {
        LOG_WARN
            << "checkDocumentAccess: пользователь " << userId
            << " не имеет доступа к документу " << documentId;
        return std::nullopt;
    }

    return document;
}

bool DocumentService::validateDocument(
    const dto::Document& document,
    std::string& errorMessage
)
{
    if (!document.caption.has_value() || document.caption->empty())
    {
        errorMessage = "Название документа обязательно для заполнения";
        return false;
    }

    if (document.caption->length() > 255)
    {
        errorMessage = "Название документа не может превышать 255 символов";
        return false;
    }

    if (!document.path.has_value() || document.path->empty())
    {
        errorMessage = "Путь к файлу обязателен";
        return false;
    }

    if (!document.filename.has_value() || document.filename->empty())
    {
        errorMessage = "Имя файла обязательно";
        return false;
    }

    if (!document.size.has_value() || *document.size < 0)
    {
        errorMessage = "Размер файла должен быть указан и неотрицательным";
        return false;
    }

    if (!document.uploadedByUserId.has_value())
    {
        errorMessage = "Идентификатор загрузившего пользователя обязателен";
        return false;
    }

    if (document.description.has_value() && document.description->length() > 1000)
    {
        errorMessage = "Описание документа не может превышать 1000 символов";
        return false;
    }

    return true;
}

bool DocumentService::checkDocumentPermissions(
    const dto::Document& document,
    int64_t userId,
    bool needWrite
)
{
    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    // Для чтения документ доступен, если пользователь является владельцем
    // или имеет доступ к связанному элементу/комментарию.
    // Здесь мы проверяем только владение, остальное проверяется в
    // сервисах ItemDocument и CommentDocument.
    if (needWrite)
    {
        // Для записи нужны дополнительные права
        // только владелец может изменять/удалять документ
        if (!document.uploadedByUserId.has_value() || *document.uploadedByUserId != userId)
        {
            LOG_DEBUG
                << "checkDocumentPermissions: пользователь " << userId
                << " не является владельцем документа";
            return false;
        }
        return true;
    }

    // Для чтения: любой аутентифицированный пользователь может видеть документ,
    // если он привязан к доступному элементу или комментарию.
    // Проверка прав на чтение документа происходит через сервисы связей.
    // Здесь мы просто разрешаем доступ, так как проверка будет в других сервисах.
    return true;
}

} // namespace services
} // namespace server
