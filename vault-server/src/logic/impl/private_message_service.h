#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iprivate_message_service.h"

#include "repo/private_message_repository.h"
#include "repo/user_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для работы с личными сообщениями.
 */
class PrivateMessageService final : public IPrivateMessageService
{
public:
    PrivateMessageService(
        std::shared_ptr<repositories::IPrivateMessageRepository> messageRepo,
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IPrivateMessageService
    PrivateMessagesPage getMessages(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<bool> isViewed = std::nullopt
    ) override;

    std::optional<dto::PrivateMessage> getMessage(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::PrivateMessage> getConversation(
        int64_t userId1,
        int64_t userId2
    ) override;

    std::optional<dto::PrivateMessage> sendMessage(
        const dto::PrivateMessage& message,
        int64_t senderUserId
    ) override;

    PrivateMessageResult markAsViewed(
        int64_t messageId,
        int64_t userId
    ) override;

    PrivateMessageResult markAllAsViewed(
        int64_t senderUserId,
        int64_t receiverUserId
    ) override;

    PrivateMessageResult deleteMessage(
        int64_t id,
        int64_t userId
    ) override;

    int64_t countUnviewed(int64_t userId) override;

private:
    /**
     * @brief Проверяет, имеет ли пользователь доступ к сообщению.
     * @param message DTO сообщения
     * @param userId ID пользователя
     * @return true если пользователь является отправителем или получателем
     */
    bool hasAccessToMessage(const dto::PrivateMessage& message, int64_t userId);

    /**
     * @brief Проверяет существование пользователя.
     * @param userId ID пользователя
     * @return true если пользователь существует
     */
    bool userExists(int64_t userId);

private:
    std::shared_ptr<repositories::IPrivateMessageRepository> m_messageRepo;
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
