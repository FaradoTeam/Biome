#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/private_message.h"

namespace server::services
{

/**
 * @brief Страница с личными сообщениями.
 */
struct PrivateMessagesPage
{
    std::vector<dto::PrivateMessage> messages;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с личным сообщением.
 */
struct PrivateMessageResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с личными сообщениями.
 */
class IPrivateMessageService
{
public:
    virtual ~IPrivateMessageService() = default;

    /**
     * @brief Получает список личных сообщений с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param filterUserId Фильтр по собеседнику (отправитель или получатель)
     * @param isViewed Фильтр по статусу прочтения (std::nullopt - все)
     * @return Страница с сообщениями
     */
    virtual PrivateMessagesPage getMessages(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<bool> isViewed = std::nullopt
    ) = 0;

    /**
     * @brief Получает личное сообщение по ID.
     * @param id Идентификатор сообщения
     * @param userId ID пользователя для проверки прав
     * @return DTO сообщения или std::nullopt
     */
    virtual std::optional<dto::PrivateMessage> getMessage(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает переписку между двумя пользователями.
     * @param userId1 Идентификатор первого пользователя
     * @param userId2 Идентификатор второго пользователя
     * @return Вектор сообщений
     */
    virtual std::vector<dto::PrivateMessage> getConversation(
        int64_t userId1,
        int64_t userId2
    ) = 0;

    /**
     * @brief Отправляет личное сообщение.
     * @param message DTO сообщения (без id и timestamp)
     * @param senderUserId ID отправителя
     * @return Созданное сообщение или std::nullopt при ошибке
     */
    virtual std::optional<dto::PrivateMessage> sendMessage(
        const dto::PrivateMessage& message,
        int64_t senderUserId
    ) = 0;

    /**
     * @brief Отмечает сообщение как прочитанное.
     * @param messageId Идентификатор сообщения
     * @param userId ID пользователя (должен быть получателем)
     * @return Результат операции
     */
    virtual PrivateMessageResult markAsViewed(
        int64_t messageId,
        int64_t userId
    ) = 0;

    /**
     * @brief Отмечает все сообщения от отправителя как прочитанные.
     * @param senderUserId ID отправителя
     * @param receiverUserId ID получателя
     * @return Результат операции
     */
    virtual PrivateMessageResult markAllAsViewed(
        int64_t senderUserId,
        int64_t receiverUserId
    ) = 0;

    /**
     * @brief Удаляет личное сообщение.
     * @param id Идентификатор сообщения
     * @param userId ID пользователя (должен быть отправителем или получателем)
     * @return Результат операции
     */
    virtual PrivateMessageResult deleteMessage(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает количество непрочитанных сообщений для пользователя.
     * @param userId ID пользователя
     * @return Количество непрочитанных сообщений
     */
    virtual int64_t countUnviewed(int64_t userId) = 0;
};

} // namespace server::services
