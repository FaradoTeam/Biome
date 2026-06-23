#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iteam_message_service.h"

#include "repo/team_message_repository.h"
#include "repo/team_repository.h"
#include "repo/user_repository.h"
#include "repo/user_team_role_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для работы с сообщениями в командах.
 */
class TeamMessageService final : public ITeamMessageService
{
public:
    TeamMessageService(
        std::shared_ptr<repositories::ITeamMessageRepository> messageRepo,
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<repositories::IUserTeamRoleRepository> userTeamRoleRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // ITeamMessageService
    TeamMessagesPage getMessages(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> senderUserId = std::nullopt
    ) override;

    std::optional<dto::TeamMessage> getMessage(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::TeamMessage> getTeamMessages(
        int64_t teamId,
        int64_t userId
    ) override;

    std::optional<dto::TeamMessage> sendMessage(
        const dto::TeamMessage& message,
        int64_t senderUserId
    ) override;

    TeamMessageResult deleteMessage(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет, является ли пользователь членом команды.
     * @param userId ID пользователя
     * @param teamId ID команды
     * @return true если пользователь состоит в команде
     */
    bool isTeamMember(int64_t userId, int64_t teamId);

    /**
     * @brief Проверяет существование команды.
     * @param teamId ID команды
     * @return true если команда существует
     */
    bool teamExists(int64_t teamId);

    /**
     * @brief Проверяет существование пользователя.
     * @param userId ID пользователя
     * @return true если пользователь существует
     */
    bool userExists(int64_t userId);

private:
    std::shared_ptr<repositories::ITeamMessageRepository> m_messageRepo;
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<repositories::IUserTeamRoleRepository> m_userTeamRoleRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
