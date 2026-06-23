#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/team_message.h"

namespace server::services
{

/**
 * @brief Страница с сообщениями в команде.
 */
struct TeamMessagesPage
{
    std::vector<dto::TeamMessage> messages;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с сообщением в команде.
 */
struct TeamMessageResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с сообщениями в командах.
 */
class ITeamMessageService
{
public:
    virtual ~ITeamMessageService() = default;

    /**
     * @brief Получает список сообщений с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param teamId Фильтр по команде (std::nullopt - все)
     * @param senderUserId Фильтр по отправителю (std::nullopt - все)
     * @return Страница с сообщениями
     */
    virtual TeamMessagesPage getMessages(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> senderUserId = std::nullopt
    ) = 0;

    /**
     * @brief Получает сообщение по ID.
     * @param id Идентификатор сообщения
     * @param userId ID пользователя для проверки прав
     * @return DTO сообщения или std::nullopt
     */
    virtual std::optional<dto::TeamMessage> getMessage(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все сообщения в команде.
     * @param teamId Идентификатор команды
     * @param userId ID пользователя для проверки прав
     * @return Вектор сообщений
     */
    virtual std::vector<dto::TeamMessage> getTeamMessages(
        int64_t teamId,
        int64_t userId
    ) = 0;

    /**
     * @brief Отправляет сообщение в команду.
     * @param message DTO сообщения (без id и timestamp)
     * @param senderUserId ID отправителя
     * @return Созданное сообщение или std::nullopt при ошибке
     */
    virtual std::optional<dto::TeamMessage> sendMessage(
        const dto::TeamMessage& message,
        int64_t senderUserId
    ) = 0;

    /**
     * @brief Удаляет сообщение из команды.
     * @param id Идентификатор сообщения
     * @param userId ID пользователя (должен быть отправителем или администратором)
     * @return Результат операции
     */
    virtual TeamMessageResult deleteMessage(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
