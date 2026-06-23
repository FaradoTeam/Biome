#include "team_message_service.h"
#include "common/log/log.h"

namespace server::services
{

TeamMessageService::TeamMessageService(
    std::shared_ptr<repositories::ITeamMessageRepository> messageRepo,
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<repositories::IUserTeamRoleRepository> userTeamRoleRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_messageRepo(std::move(messageRepo))
    , m_teamRepo(std::move(teamRepo))
    , m_userRepo(std::move(userRepo))
    , m_userTeamRoleRepo(std::move(userTeamRoleRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_messageRepo || !m_teamRepo || !m_userRepo || !m_userTeamRoleRepo)
    {
        throw std::runtime_error("TeamMessageService: один или несколько репозиториев не инициализированы");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("TeamMessageService: сервис авторизации не инициализирован");
    }
}

TeamMessagesPage TeamMessageService::getMessages(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> teamId,
    std::optional<int64_t> senderUserId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    // Если указан teamId, проверяем доступ
    if (teamId.has_value())
    {
        if (!teamExists(*teamId))
        {
            LOG_WARN << "getMessages: команда " << *teamId << " не найдена";
            return { {}, 0 };
        }

        if (!m_authzService->isSuperAdmin(userId) && !isTeamMember(userId, *teamId))
        {
            LOG_WARN << "getMessages: пользователь " << userId << " не состоит в команде " << *teamId;
            return { {}, 0 };
        }
    }

    // Если указан senderUserId, проверяем его существование
    if (senderUserId.has_value() && !userExists(*senderUserId))
    {
        LOG_WARN << "getMessages: отправитель " << *senderUserId << " не найден";
        return { {}, 0 };
    }

    repositories::TeamMessagesPage repoPage = m_messageRepo->findAll(page, pageSize, teamId, senderUserId);

    TeamMessagesPage result;
    result.messages = std::move(repoPage.messages);
    result.totalCount = repoPage.totalCount;
    return result;
}

std::optional<dto::TeamMessage> TeamMessageService::getMessage(
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

    // Супер-админ может читать любые сообщения
    if (m_authzService->isSuperAdmin(userId))
    {
        return message;
    }

    // Обычный пользователь должен быть членом команды
    if (!message->teamId.has_value())
    {
        LOG_WARN << "getMessage: сообщение " << id << " не содержит teamId";
        return std::nullopt;
    }

    if (!isTeamMember(userId, *message->teamId))
    {
        LOG_WARN << "getMessage: пользователь " << userId << " не состоит в команде " << *message->teamId;
        return std::nullopt;
    }

    return message;
}

std::vector<dto::TeamMessage> TeamMessageService::getTeamMessages(
    int64_t teamId,
    int64_t userId
)
{
    if (teamId <= 0)
    {
        LOG_WARN << "getTeamMessages: неверный teamId " << teamId;
        return {};
    }

    if (!teamExists(teamId))
    {
        LOG_WARN << "getTeamMessages: команда " << teamId << " не найдена";
        return {};
    }

    // Супер-админ может читать сообщения любой команды
    if (m_authzService->isSuperAdmin(userId))
    {
        return m_messageRepo->findByTeamId(teamId);
    }

    // Обычный пользователь должен быть членом команды
    if (!isTeamMember(userId, teamId))
    {
        LOG_WARN << "getTeamMessages: пользователь " << userId << " не состоит в команде " << teamId;
        return {};
    }

    return m_messageRepo->findByTeamId(teamId);
}

std::optional<dto::TeamMessage> TeamMessageService::sendMessage(
    const dto::TeamMessage& message,
    int64_t senderUserId
)
{
    // 1. Валидация
    if (!message.teamId.has_value() || message.teamId <= 0)
    {
        LOG_WARN << "sendMessage: неверная команда";
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

    // 3. Проверяем существование команды
    if (!teamExists(*message.teamId))
    {
        LOG_WARN << "sendMessage: команда " << *message.teamId << " не найдена";
        return std::nullopt;
    }

    // 4. Проверяем, что пользователь состоит в команде
    if (!isTeamMember(senderUserId, *message.teamId))
    {
        LOG_WARN << "sendMessage: пользователь " << senderUserId << " не состоит в команде " << *message.teamId;
        return std::nullopt;
    }

    // 5. Создаём сообщение
    dto::TeamMessage newMessage = message;
    newMessage.creationTimestamp = std::chrono::system_clock::now();

    int64_t messageId = m_messageRepo->create(newMessage);
    if (messageId <= 0)
    {
        LOG_ERROR << "sendMessage: не удалось создать сообщение";
        return std::nullopt;
    }

    LOG_INFO << "Пользователь " << senderUserId << " отправил сообщение в команду " << *message.teamId;

    return m_messageRepo->findById(messageId);
}

TeamMessageResult TeamMessageService::deleteMessage(
    int64_t id,
    int64_t userId
)
{
    TeamMessageResult result;

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

    // Только отправитель может удалить своё сообщение
    if (!message->senderUserId.has_value() || *message->senderUserId != userId)
    {
        result.errorMessage = "Только отправитель может удалить сообщение";
        result.errorCode = 403;
        LOG_WARN << "deleteMessage: пользователь " << userId << " не является отправителем сообщения " << id;
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

bool TeamMessageService::isTeamMember(int64_t userId, int64_t teamId)
{
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);
    for (const auto& utr : userTeamRoles)
    {
        if (utr.teamId.has_value() && *utr.teamId == teamId)
        {
            return true;
        }
    }
    return false;
}

bool TeamMessageService::teamExists(int64_t teamId)
{
    return m_teamRepo->exists(teamId);
}

bool TeamMessageService::userExists(int64_t userId)
{
    return m_userRepo->findById(userId).has_value();
}

} // namespace server::services
