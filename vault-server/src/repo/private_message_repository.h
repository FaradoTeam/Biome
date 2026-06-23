#pragma once

#include <optional>
#include <vector>

#include "common/dto/private_message.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка личных сообщений.
 */
struct PrivateMessagesPage
{
    std::vector<dto::PrivateMessage> messages;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с личными сообщениями.
 */
class IPrivateMessageRepository
{
public:
    virtual ~IPrivateMessageRepository() = default;

    /**
     * @brief Получает список личных сообщений с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId Фильтр по участнику (отправитель или получатель)
     * @param isViewed Фильтр по статусу прочтения (std::nullopt - все)
     * @return Страница с сообщениями
     */
    virtual PrivateMessagesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<bool> isViewed = std::nullopt
    ) = 0;

    /**
     * @brief Находит личное сообщение по ID.
     * @param id Идентификатор сообщения
     * @return DTO сообщения или std::nullopt
     */
    virtual std::optional<dto::PrivateMessage> findById(int64_t id) = 0;

    /**
     * @brief Находит все сообщения между двумя пользователями.
     * @param userId1 Идентификатор первого пользователя
     * @param userId2 Идентификатор второго пользователя
     * @return Вектор сообщений (отсортирован по времени)
     */
    virtual std::vector<dto::PrivateMessage> findConversation(
        int64_t userId1,
        int64_t userId2
    ) = 0;

    /**
     * @brief Находит все сообщения, отправленные пользователем.
     * @param senderUserId Идентификатор отправителя
     * @return Вектор сообщений
     */
    virtual std::vector<dto::PrivateMessage> findBySender(int64_t senderUserId) = 0;

    /**
     * @brief Находит все сообщения, полученные пользователем.
     * @param receiverUserId Идентификатор получателя
     * @param onlyUnviewed Только непрочитанные
     * @return Вектор сообщений
     */
    virtual std::vector<dto::PrivateMessage> findByReceiver(
        int64_t receiverUserId,
        bool onlyUnviewed = false
    ) = 0;

    /**
     * @brief Создаёт новое личное сообщение.
     * @param message DTO сообщения
     * @return ID созданного сообщения или 0 при ошибке
     */
    virtual int64_t create(const dto::PrivateMessage& message) = 0;

    /**
     * @brief Обновляет существующее сообщение (например, отметка о прочтении).
     * @param message DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::PrivateMessage& message) = 0;

    /**
     * @brief Удаляет сообщение по ID.
     * @param id Идентификатор сообщения
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование сообщения.
     * @param id Идентификатор сообщения
     * @return true если сообщение существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Отмечает все сообщения от отправителя как прочитанные.
     * @param senderUserId Идентификатор отправителя
     * @param receiverUserId Идентификатор получателя
     * @return Количество обновлённых сообщений
     */
    virtual int64_t markAllAsViewed(int64_t senderUserId, int64_t receiverUserId) = 0;

    /**
     * @brief Получает количество непрочитанных сообщений для пользователя.
     * @param userId Идентификатор пользователя
     * @return Количество непрочитанных сообщений
     */
    virtual int64_t countUnviewed(int64_t userId) = 0;
};

} // namespace repositories
} // namespace server
