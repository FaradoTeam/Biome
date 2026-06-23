#pragma once

#include <optional>
#include <vector>

#include "logic/iprivate_message_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для IPrivateMessageService.
 */
class MockPrivateMessageService : public services::IPrivateMessageService
{
public:
    // ============================================================
    // Создание тестовых DTO
    // ============================================================

    static dto::PrivateMessage createTestMessage(
        int64_t id,
        int64_t senderUserId,
        int64_t receiverUserId,
        const std::string& content,
        bool isViewed,
        const std::chrono::system_clock::time_point& timestamp
    )
    {
        dto::PrivateMessage msg;
        msg.id = id;
        msg.senderUserId = senderUserId;
        msg.receiverUserId = receiverUserId;
        msg.content = content;
        msg.isViewed = isViewed;
        msg.creationTimestamp = timestamp;
        return msg;
    }

    // ============================================================
    // Сброс состояния
    // ============================================================

    void reset()
    {
        m_getMessagesCallCount = 0;
        m_getMessageCallCount = 0;
        m_getConversationCallCount = 0;
        m_sendMessageCallCount = 0;
        m_markAsViewedCallCount = 0;
        m_deleteMessageCallCount = 0;
        m_countUnviewedCallCount = 0;

        m_lastGetMessagesUserId = 0;
        m_lastGetMessagesPage = 0;
        m_lastGetMessagesPageSize = 0;
        m_lastGetMessagesFilterUserId = std::nullopt;
        m_lastGetMessagesIsViewed = std::nullopt;

        m_lastGetMessageId = 0;
        m_lastGetConversationUserId1 = 0;
        m_lastGetConversationUserId2 = 0;
        m_lastSendMessageSenderUserId = 0;
        m_lastMarkAsViewedMessageId = 0;
        m_lastMarkAsViewedUserId = 0;
        m_lastDeletedMessageId = 0;
        m_lastDeleteMessageUserId = 0;
        m_lastCountUnviewedUserId = 0;

        m_getMessagesResult = services::PrivateMessagesPage { {}, 0 };
        m_getMessageResult = std::nullopt;
        m_getConversationResult = {};
        m_sendMessageResult = std::nullopt;
        m_markAsViewedResult = services::PrivateMessageResult { true, 0, "" };
        m_deleteMessageResult = services::PrivateMessageResult { true, 0, "" };
        m_countUnviewedResult = 0;
    }

    MockPrivateMessageService()
    {
        reset();
    }

    // ============================================================
    // Настройка возвращаемых значений
    // ============================================================

    void setGetMessagesResult(const services::PrivateMessagesPage& result)
    {
        m_getMessagesResult = result;
    }

    void setGetMessageResult(const std::optional<dto::PrivateMessage>& result)
    {
        m_getMessageResult = result;
    }

    void setGetConversationResult(const std::vector<dto::PrivateMessage>& result)
    {
        m_getConversationResult = result;
    }

    void setSendMessageResult(const std::optional<dto::PrivateMessage>& result)
    {
        m_sendMessageResult = result;
    }

    void setMarkAsViewedResult(const services::PrivateMessageResult& result)
    {
        m_markAsViewedResult = result;
    }

    void setDeleteMessageResult(const services::PrivateMessageResult& result)
    {
        m_deleteMessageResult = result;
    }

    void setCountUnviewedResult(int64_t result)
    {
        m_countUnviewedResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetMessagesCallCount() const { return m_getMessagesCallCount; }
    int getGetMessageCallCount() const { return m_getMessageCallCount; }
    int getGetConversationCallCount() const { return m_getConversationCallCount; }
    int getSendMessageCallCount() const { return m_sendMessageCallCount; }
    int getMarkAsViewedCallCount() const { return m_markAsViewedCallCount; }
    int getDeleteMessageCallCount() const { return m_deleteMessageCallCount; }
    int getCountUnviewedCallCount() const { return m_countUnviewedCallCount; }

    int64_t getLastGetMessagesUserId() const { return m_lastGetMessagesUserId; }
    int getLastGetMessagesPage() const { return m_lastGetMessagesPage; }
    int getLastGetMessagesPageSize() const { return m_lastGetMessagesPageSize; }
    std::optional<int64_t> getLastGetMessagesFilterUserId() const { return m_lastGetMessagesFilterUserId; }
    std::optional<bool> getLastGetMessagesIsViewed() const { return m_lastGetMessagesIsViewed; }

    int64_t getLastGetMessageId() const { return m_lastGetMessageId; }
    int64_t getLastGetConversationUserId1() const { return m_lastGetConversationUserId1; }
    int64_t getLastGetConversationUserId2() const { return m_lastGetConversationUserId2; }
    int64_t getLastSendMessageSenderUserId() const { return m_lastSendMessageSenderUserId; }
    int64_t getLastMarkAsViewedMessageId() const { return m_lastMarkAsViewedMessageId; }
    int64_t getLastMarkAsViewedUserId() const { return m_lastMarkAsViewedUserId; }
    int64_t getLastDeletedMessageId() const { return m_lastDeletedMessageId; }
    int64_t getLastDeleteMessageUserId() const { return m_lastDeleteMessageUserId; }
    int64_t getLastCountUnviewedUserId() const { return m_lastCountUnviewedUserId; }

    // ============================================================
    // Реализация интерфейса IPrivateMessageService
    // ============================================================

    services::PrivateMessagesPage getMessages(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId,
        std::optional<bool> isViewed
    ) override
    {
        m_getMessagesCallCount++;
        m_lastGetMessagesUserId = userId;
        m_lastGetMessagesPage = page;
        m_lastGetMessagesPageSize = pageSize;
        m_lastGetMessagesFilterUserId = filterUserId;
        m_lastGetMessagesIsViewed = isViewed;
        return m_getMessagesResult;
    }

    std::optional<dto::PrivateMessage> getMessage(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getMessageCallCount++;
        m_lastGetMessageId = id;
        m_lastGetMessageUserId = userId;
        return m_getMessageResult;
    }

    std::vector<dto::PrivateMessage> getConversation(
        int64_t userId1,
        int64_t userId2
    ) override
    {
        m_getConversationCallCount++;
        m_lastGetConversationUserId1 = userId1;
        m_lastGetConversationUserId2 = userId2;
        return m_getConversationResult;
    }

    std::optional<dto::PrivateMessage> sendMessage(
        const dto::PrivateMessage& message,
        int64_t senderUserId
    ) override
    {
        m_sendMessageCallCount++;
        m_lastSendMessageSenderUserId = senderUserId;
        return m_sendMessageResult;
    }

    services::PrivateMessageResult markAsViewed(
        int64_t messageId,
        int64_t userId
    ) override
    {
        m_markAsViewedCallCount++;
        m_lastMarkAsViewedMessageId = messageId;
        m_lastMarkAsViewedUserId = userId;
        return m_markAsViewedResult;
    }

    services::PrivateMessageResult markAllAsViewed(
        int64_t senderUserId,
        int64_t receiverUserId
    ) override
    {
        // Не используется в тестах, но требуется для интерфейса
        services::PrivateMessageResult result;
        result.success = true;
        return result;
    }

    services::PrivateMessageResult deleteMessage(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteMessageCallCount++;
        m_lastDeletedMessageId = id;
        m_lastDeleteMessageUserId = userId;
        return m_deleteMessageResult;
    }

    int64_t countUnviewed(int64_t userId) override
    {
        m_countUnviewedCallCount++;
        m_lastCountUnviewedUserId = userId;
        return m_countUnviewedResult;
    }

private:
    // Счётчики вызовов
    int m_getMessagesCallCount = 0;
    int m_getMessageCallCount = 0;
    int m_getConversationCallCount = 0;
    int m_sendMessageCallCount = 0;
    int m_markAsViewedCallCount = 0;
    int m_deleteMessageCallCount = 0;
    int m_countUnviewedCallCount = 0;

    // Параметры последних вызовов
    int64_t m_lastGetMessagesUserId = 0;
    int m_lastGetMessagesPage = 0;
    int m_lastGetMessagesPageSize = 0;
    std::optional<int64_t> m_lastGetMessagesFilterUserId;
    std::optional<bool> m_lastGetMessagesIsViewed;

    int64_t m_lastGetMessageId = 0;
    int64_t m_lastGetMessageUserId = 0;
    int64_t m_lastGetConversationUserId1 = 0;
    int64_t m_lastGetConversationUserId2 = 0;
    int64_t m_lastSendMessageSenderUserId = 0;
    int64_t m_lastMarkAsViewedMessageId = 0;
    int64_t m_lastMarkAsViewedUserId = 0;
    int64_t m_lastDeletedMessageId = 0;
    int64_t m_lastDeleteMessageUserId = 0;
    int64_t m_lastCountUnviewedUserId = 0;

    // Возвращаемые значения
    services::PrivateMessagesPage m_getMessagesResult;
    std::optional<dto::PrivateMessage> m_getMessageResult;
    std::vector<dto::PrivateMessage> m_getConversationResult;
    std::optional<dto::PrivateMessage> m_sendMessageResult;
    services::PrivateMessageResult m_markAsViewedResult;
    services::PrivateMessageResult m_deleteMessageResult;
    int64_t m_countUnviewedResult = 0;
};

} // namespace tests
} // namespace server
