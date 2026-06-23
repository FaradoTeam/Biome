#include "private_message_service.h"
#include "common/log/log.h"

namespace server::services
{

PrivateMessageService::PrivateMessageService(
    std::shared_ptr<repositories::IPrivateMessageRepository> messageRepo,
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_messageRepo(std::move(messageRepo))
    , m_userRepo(std::move(userRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_messageRepo)
    {
        throw std::runtime_error("PrivateMessageService: репозиторий сообщений не инициализирован");
    }
    if (!m_userRepo)
    {
        throw std::runtime_error("PrivateMessageService: репозиторий пользователей не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("PrivateMessageService: сервис авторизации не инициализирован");
    }
}

PrivateMessagesPage PrivateMessageService::getMessages(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> filterUserId,
    std::optional<bool> isViewed
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    // Если указан фильтр по собеседнику, проверяем его существование
    if (filterUserId.has_value() && !userExists(*filterUserId))
    {
        LOG_WARN << "getMessages: пользователь " << *filterUserId << " не найден";
        return { {}, 0 };
    }

    repositories::PrivateMessagesPage repoPage;

    // Проверяем права: супер-админ может видеть все сообщения
    if (m_authzService->isSuperAdmin(userId))
    {
        repoPage = m_messageRepo->findAll(page, pageSize, filterUserId, isViewed);
    }
    else
    {
        // Обычный пользователь видит только свои сообщения
        repoPage = m_messageRepo->findAll(page, pageSize, userId, isViewed);
    }

    // Преобразуем из репозиторной структуры в сервисную
    PrivateMessagesPage result;
    result.messages = std::move(repoPage.messages);
    result.totalCount = repoPage.totalCount;
    return result;
}

std::optional<dto::PrivateMessage> PrivateMessageService::getMessage(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "getMessage: неверный идентификатор " << id;
        return std::nullopt;
    }

    auto message = m_messageRepo->findById(id);
    if (!message.has_value())
    {
        LOG_DEBUG << "getMessage: сообщение " << id << " не найдено";
        return std::nullopt;
    }

    // Проверяем доступ: супер-админ или участник переписки
    if (!m_authzService->isSuperAdmin(userId) && !hasAccessToMessage(*message, userId))
    {
        LOG_WARN << "getMessage: пользователь " << userId << " не имеет доступа к сообщению " << id;
        return std::nullopt;
    }

    return message;
}

std::vector<dto::PrivateMessage> PrivateMessageService::getConversation(
    int64_t userId1,
    int64_t userId2
)
{
    if (userId1 <= 0 || userId2 <= 0)
    {
        LOG_WARN << "getConversation: неверные ID пользователей";
        return {};
    }

    // Проверяем существование пользователей
    if (!userExists(userId1) || !userExists(userId2))
    {
        LOG_WARN << "getConversation: один из пользователей не найден";
        return {};
    }

    return m_messageRepo->findConversation(userId1, userId2);
}

std::optional<dto::PrivateMessage> PrivateMessageService::sendMessage(
    const dto::PrivateMessage& message,
    int64_t senderUserId
)
{
    // 1. Валидация
    if (!message.receiverUserId.has_value() || message.receiverUserId <= 0)
    {
        LOG_WARN << "sendMessage: неверный получатель";
        return std::nullopt;
    }

    if (!message.content.has_value() || message.content->empty())
    {
        LOG_WARN << "sendMessage: пустое сообщение";
        return std::nullopt;
    }

    if (message.content->length() > 10000)
    {
        LOG_WARN << "sendMessage: сообщение превышает 10000 символов";
        return std::nullopt;
    }

    // 2. Проверяем, что отправитель совпадает с текущим пользователем
    if (*message.senderUserId != senderUserId)
    {
        LOG_WARN << "sendMessage: отправитель не совпадает с текущим пользователем";
        return std::nullopt;
    }

    // 3. Проверяем существование получателя
    if (!userExists(*message.receiverUserId))
    {
        LOG_WARN << "sendMessage: получатель " << *message.receiverUserId << " не найден";
        return std::nullopt;
    }

    // 4. Запрещаем отправку самому себе
    if (*message.senderUserId == *message.receiverUserId)
    {
        LOG_WARN << "sendMessage: нельзя отправить сообщение самому себе";
        return std::nullopt;
    }

    // 5. Создаём сообщение
    dto::PrivateMessage newMessage = message;
    newMessage.creationTimestamp = std::chrono::system_clock::now();
    newMessage.isViewed = false;

    int64_t messageId = m_messageRepo->create(newMessage);
    if (messageId <= 0)
    {
        LOG_ERROR << "sendMessage: не удалось создать сообщение";
        return std::nullopt;
    }

    LOG_INFO << "Пользователь " << senderUserId << " отправил сообщение пользователю " << *message.receiverUserId;

    return m_messageRepo->findById(messageId);
}

PrivateMessageResult PrivateMessageService::markAsViewed(
    int64_t messageId,
    int64_t userId
)
{
    PrivateMessageResult result;

    if (messageId <= 0)
    {
        result.errorMessage = "Неверный идентификатор сообщения";
        result.errorCode = 400;
        return result;
    }

    auto message = m_messageRepo->findById(messageId);
    if (!message.has_value())
    {
        result.errorMessage = "Сообщение не найдено";
        result.errorCode = 404;
        return result;
    }

    // Только получатель может отметить сообщение как прочитанное
    if (!message->receiverUserId.has_value() || *message->receiverUserId != userId)
    {
        result.errorMessage = "Только получатель может отметить сообщение как прочитанное";
        result.errorCode = 403;
        LOG_WARN << "markAsViewed: пользователь " << userId << " не является получателем сообщения " << messageId;
        return result;
    }

    // Если уже прочитано
    if (message->isViewed.value_or(false))
    {
        result.success = true;
        return result;
    }

    dto::PrivateMessage updateData;
    updateData.id = messageId;
    updateData.isViewed = true;

    if (!m_messageRepo->update(updateData))
    {
        result.errorMessage = "Не удалось обновить сообщение";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO << "Пользователь " << userId << " отметил сообщение " << messageId << " как прочитанное";
    return result;
}

PrivateMessageResult PrivateMessageService::markAllAsViewed(
    int64_t senderUserId,
    int64_t receiverUserId
)
{
    PrivateMessageResult result;

    if (senderUserId <= 0 || receiverUserId <= 0)
    {
        result.errorMessage = "Неверные ID пользователей";
        result.errorCode = 400;
        return result;
    }

    if (!userExists(senderUserId) || !userExists(receiverUserId))
    {
        result.errorMessage = "Один из пользователей не найден";
        result.errorCode = 404;
        return result;
    }

    int64_t updated = m_messageRepo->markAllAsViewed(senderUserId, receiverUserId);
    result.success = true;
    LOG_INFO << "Отмечены как прочитанные " << updated << " сообщений от " << senderUserId << " для " << receiverUserId;
    return result;
}

PrivateMessageResult PrivateMessageService::deleteMessage(
    int64_t id,
    int64_t userId
)
{
    PrivateMessageResult result;

    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор сообщения";
        result.errorCode = 400;
        return result;
    }

    auto message = m_messageRepo->findById(id);
    if (!message.has_value())
    {
        result.errorMessage = "Сообщение не найдено";
        result.errorCode = 404;
        return result;
    }

    // Супер-админ может удалить любое сообщение
    if (m_authzService->isSuperAdmin(userId))
    {
        if (m_messageRepo->remove(id))
        {
            result.success = true;
            LOG_INFO << "Супер-админ " << userId << " удалил сообщение " << id;
            return result;
        }
        result.errorMessage = "Не удалось удалить сообщение";
        result.errorCode = 500;
        return result;
    }

    // Обычный пользователь может удалить только свои сообщения (отправитель или получатель)
    if (!hasAccessToMessage(*message, userId))
    {
        result.errorMessage = "Нет прав на удаление этого сообщения";
        result.errorCode = 403;
        LOG_WARN << "deleteMessage: пользователь " << userId << " не имеет прав на удаление сообщения " << id;
        return result;
    }

    if (m_messageRepo->remove(id))
    {
        result.success = true;
        LOG_INFO << "Пользователь " << userId << " удалил сообщение " << id;
        return result;
    }

    result.errorMessage = "Не удалось удалить сообщение";
    result.errorCode = 500;
    return result;
}

int64_t PrivateMessageService::countUnviewed(int64_t userId)
{
    if (userId <= 0)
    {
        LOG_WARN << "countUnviewed: неверный ID пользователя";
        return 0;
    }

    return m_messageRepo->countUnviewed(userId);
}

bool PrivateMessageService::hasAccessToMessage(
    const dto::PrivateMessage& message,
    int64_t userId
)
{
    if (!message.senderUserId.has_value() || !message.receiverUserId.has_value())
    {
        return false;
    }
    return *message.senderUserId == userId || *message.receiverUserId == userId;
}

bool PrivateMessageService::userExists(int64_t userId)
{
    return m_userRepo->findById(userId).has_value();
}

} // namespace server::services
