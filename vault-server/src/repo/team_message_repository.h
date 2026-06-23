#pragma once

#include <optional>
#include <vector>

#include "common/dto/team_message.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка сообщений в команде.
 */
struct TeamMessagesPage
{
    std::vector<dto::TeamMessage> messages;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с сообщениями в командах.
 */
class ITeamMessageRepository
{
public:
    virtual ~ITeamMessageRepository() = default;

    /**
     * @brief Получает список сообщений с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param teamId Фильтр по команде (std::nullopt - все)
     * @param senderUserId Фильтр по отправителю (std::nullopt - все)
     * @return Страница с сообщениями
     */
    virtual TeamMessagesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> senderUserId = std::nullopt
    ) = 0;

    /**
     * @brief Находит сообщение по ID.
     * @param id Идентификатор сообщения
     * @return DTO сообщения или std::nullopt
     */
    virtual std::optional<dto::TeamMessage> findById(int64_t id) = 0;

    /**
     * @brief Находит все сообщения в команде.
     * @param teamId Идентификатор команды
     * @return Вектор сообщений (отсортирован по времени)
     */
    virtual std::vector<dto::TeamMessage> findByTeamId(int64_t teamId) = 0;

    /**
     * @brief Находит все сообщения, отправленные пользователем в команду.
     * @param senderUserId Идентификатор отправителя
     * @param teamId Идентификатор команды
     * @return Вектор сообщений
     */
    virtual std::vector<dto::TeamMessage> findBySenderAndTeam(
        int64_t senderUserId,
        int64_t teamId
    ) = 0;

    /**
     * @brief Создаёт новое сообщение в команде.
     * @param message DTO сообщения
     * @return ID созданного сообщения или 0 при ошибке
     */
    virtual int64_t create(const dto::TeamMessage& message) = 0;

    /**
     * @brief Обновляет существующее сообщение.
     * @param message DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::TeamMessage& message) = 0;

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
     * @brief Удаляет все сообщения в команде.
     * @param teamId Идентификатор команды
     * @return Количество удалённых сообщений
     */
    virtual int64_t removeByTeamId(int64_t teamId) = 0;
};

} // namespace repositories
} // namespace server
