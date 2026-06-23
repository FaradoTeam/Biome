#pragma once

#include <optional>
#include <vector>

#include "logic/iteam_message_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для ITeamMessageService.
 */
class MockTeamMessageService : public services::ITeamMessageService
{
public:
    // ============================================================
    // Создание тестовых DTO
    // ============================================================

    static dto::TeamMessage createTestMessage(
        int64_t id,
        int64_t senderUserId,
        int64_t teamId,
        const std::string& content,
        const std::chrono::system_clock::time_point& timestamp
    )
    {
        dto::TeamMessage msg;
        msg.id = id;
        msg.senderUserId = senderUserId;
        msg.teamId = teamId;
        msg.content = content;
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
        m_getTeamMessagesCallCount = 0;
        m_sendMessageCallCount = 0;
        m_deleteMessageCallCount = 0;

        m_lastGetMessagesUserId = 0;
        m_lastGetMessagesPage = 0;
        m_lastGetMessagesPageSize = 0;
        m_lastGetMessagesTeamId = std::nullopt;
        m_lastGetMessagesSenderUserId = std::nullopt;

        m_lastGetMessageId = 0;
        m_lastGetTeamMessagesTeamId = 0;
        m_lastGetTeamMessagesUserId = 0;
        m_lastSendMessageSenderUserId = 0;
        m_lastDeletedMessageId = 0;
        m_lastDeleteMessageUserId = 0;

        m_getMessagesResult = services::TeamMessagesPage { {}, 0 };
        m_getMessageResult = std::nullopt;
        m_getTeamMessagesResult = {};
        m_sendMessageResult = std::nullopt;
        m_deleteMessageResult = services::TeamMessageResult { true, 0, "" };
    }

    MockTeamMessageService()
    {
        reset();
    }

    // ============================================================
    // Настройка возвращаемых значений
    // ============================================================

    void setGetMessagesResult(const services::TeamMessagesPage& result)
    {
        m_getMessagesResult = result;
    }

    void setGetMessageResult(const std::optional<dto::TeamMessage>& result)
    {
        m_getMessageResult = result;
    }

    void setGetTeamMessagesResult(const std::vector<dto::TeamMessage>& result)
    {
        m_getTeamMessagesResult = result;
    }

    void setSendMessageResult(const std::optional<dto::TeamMessage>& result)
    {
        m_sendMessageResult = result;
    }

    void setDeleteMessageResult(const services::TeamMessageResult& result)
    {
        m_deleteMessageResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetMessagesCallCount() const { return m_getMessagesCallCount; }
    int getGetMessageCallCount() const { return m_getMessageCallCount; }
    int getGetTeamMessagesCallCount() const { return m_getTeamMessagesCallCount; }
    int getSendMessageCallCount() const { return m_sendMessageCallCount; }
    int getDeleteMessageCallCount() const { return m_deleteMessageCallCount; }

    int64_t getLastGetMessagesUserId() const { return m_lastGetMessagesUserId; }
    int getLastGetMessagesPage() const { return m_lastGetMessagesPage; }
    int getLastGetMessagesPageSize() const { return m_lastGetMessagesPageSize; }
    std::optional<int64_t> getLastGetMessagesTeamId() const { return m_lastGetMessagesTeamId; }
    std::optional<int64_t> getLastGetMessagesSenderUserId() const { return m_lastGetMessagesSenderUserId; }

    int64_t getLastGetMessageId() const { return m_lastGetMessageId; }
    int64_t getLastGetTeamMessagesTeamId() const { return m_lastGetTeamMessagesTeamId; }
    int64_t getLastGetTeamMessagesUserId() const { return m_lastGetTeamMessagesUserId; }
    int64_t getLastSendMessageSenderUserId() const { return m_lastSendMessageSenderUserId; }
    int64_t getLastDeletedMessageId() const { return m_lastDeletedMessageId; }
    int64_t getLastDeleteMessageUserId() const { return m_lastDeleteMessageUserId; }

    // ============================================================
    // Реализация интерфейса ITeamMessageService
    // ============================================================

    services::TeamMessagesPage getMessages(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> teamId,
        std::optional<int64_t> senderUserId
    ) override
    {
        m_getMessagesCallCount++;
        m_lastGetMessagesUserId = userId;
        m_lastGetMessagesPage = page;
        m_lastGetMessagesPageSize = pageSize;
        m_lastGetMessagesTeamId = teamId;
        m_lastGetMessagesSenderUserId = senderUserId;
        return m_getMessagesResult;
    }

    std::optional<dto::TeamMessage> getMessage(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getMessageCallCount++;
        m_lastGetMessageId = id;
        m_lastGetMessageUserId = userId;
        return m_getMessageResult;
    }

    std::vector<dto::TeamMessage> getTeamMessages(
        int64_t teamId,
        int64_t userId
    ) override
    {
        m_getTeamMessagesCallCount++;
        m_lastGetTeamMessagesTeamId = teamId;
        m_lastGetTeamMessagesUserId = userId;
        return m_getTeamMessagesResult;
    }

    std::optional<dto::TeamMessage> sendMessage(
        const dto::TeamMessage& message,
        int64_t senderUserId
    ) override
    {
        m_sendMessageCallCount++;
        m_lastSendMessageSenderUserId = senderUserId;
        return m_sendMessageResult;
    }

    services::TeamMessageResult deleteMessage(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteMessageCallCount++;
        m_lastDeletedMessageId = id;
        m_lastDeleteMessageUserId = userId;
        return m_deleteMessageResult;
    }

private:
    // Счётчики вызовов
    int m_getMessagesCallCount = 0;
    int m_getMessageCallCount = 0;
    int m_getTeamMessagesCallCount = 0;
    int m_sendMessageCallCount = 0;
    int m_deleteMessageCallCount = 0;

    // Параметры последних вызовов
    int64_t m_lastGetMessagesUserId = 0;
    int m_lastGetMessagesPage = 0;
    int m_lastGetMessagesPageSize = 0;
    std::optional<int64_t> m_lastGetMessagesTeamId;
    std::optional<int64_t> m_lastGetMessagesSenderUserId;

    int64_t m_lastGetMessageId = 0;
    int64_t m_lastGetMessageUserId = 0;
    int64_t m_lastGetTeamMessagesTeamId = 0;
    int64_t m_lastGetTeamMessagesUserId = 0;
    int64_t m_lastSendMessageSenderUserId = 0;
    int64_t m_lastDeletedMessageId = 0;
    int64_t m_lastDeleteMessageUserId = 0;

    // Возвращаемые значения
    services::TeamMessagesPage m_getMessagesResult;
    std::optional<dto::TeamMessage> m_getMessageResult;
    std::vector<dto::TeamMessage> m_getTeamMessagesResult;
    std::optional<dto::TeamMessage> m_sendMessageResult;
    services::TeamMessageResult m_deleteMessageResult;
};

} // namespace tests
} // namespace server
