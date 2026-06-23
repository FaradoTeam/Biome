#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_notification.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка подписок на уведомления.
 */
struct UserNotificationsPage
{
    std::vector<dto::UserNotification> notifications;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с подписками на уведомления.
 */
class IUserNotificationRepository
{
public:
    virtual ~IUserNotificationRepository() = default;

    /**
     * @brief Получает список подписок с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId Фильтр по пользователю (std::nullopt - все)
     * @param itemId Фильтр по элементу (std::nullopt - все)
     * @return Страница с подписками
     */
    virtual UserNotificationsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> itemId = std::nullopt
    ) = 0;

    /**
     * @brief Находит подписку по ID.
     * @param id Идентификатор подписки
     * @return DTO подписки или std::nullopt
     */
    virtual std::optional<dto::UserNotification> findById(int64_t id) = 0;

    /**
     * @brief Находит подписку пользователя на элемент.
     * @param userId Идентификатор пользователя
     * @param itemId Идентификатор элемента
     * @return DTO подписки или std::nullopt
     */
    virtual std::optional<dto::UserNotification> findByUserAndItem(
        int64_t userId,
        int64_t itemId
    ) = 0;

    /**
     * @brief Находит все подписки пользователя.
     * @param userId Идентификатор пользователя
     * @return Вектор подписок
     */
    virtual std::vector<dto::UserNotification> findByUserId(int64_t userId) = 0;

    /**
     * @brief Находит всех подписчиков элемента.
     * @param itemId Идентификатор элемента
     * @return Вектор подписок
     */
    virtual std::vector<dto::UserNotification> findByItemId(int64_t itemId) = 0;

    /**
     * @brief Проверяет, подписан ли пользователь на элемент.
     * @param userId Идентификатор пользователя
     * @param itemId Идентификатор элемента
     * @return true если подписка существует
     */
    virtual bool exists(int64_t userId, int64_t itemId) = 0;

    /**
     * @brief Создаёт новую подписку.
     * @param notification DTO подписки
     * @return ID созданной подписки или 0 при ошибке
     */
    virtual int64_t create(const dto::UserNotification& notification) = 0;

    /**
     * @brief Удаляет подписку по ID.
     * @param id Идентификатор подписки
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет подписку пользователя на элемент.
     * @param userId Идентификатор пользователя
     * @param itemId Идентификатор элемента
     * @return true если удаление успешно
     */
    virtual bool removeByUserAndItem(int64_t userId, int64_t itemId) = 0;

    /**
     * @brief Удаляет все подписки пользователя.
     * @param userId Идентификатор пользователя
     * @return Количество удалённых подписок
     */
    virtual int64_t removeByUserId(int64_t userId) = 0;

    /**
     * @brief Удаляет все подписки на элемент.
     * @param itemId Идентификатор элемента
     * @return Количество удалённых подписок
     */
    virtual int64_t removeByItemId(int64_t itemId) = 0;

    /**
     * @brief Получает ID всех подписчиков элемента.
     * @param itemId Идентификатор элемента
     * @return Вектор ID пользователей
     */
    virtual std::vector<int64_t> getSubscriberIds(int64_t itemId) = 0;
};

} // namespace repositories
} // namespace server
