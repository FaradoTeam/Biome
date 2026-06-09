#pragma once

#include <optional>
#include <vector>

#include "common/dto/item_user_state.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка истории состояний.
 */
struct ItemUserStatesPage
{
    std::vector<dto::ItemUserState> states;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с историей состояний элементов.
 */
class IItemUserStateRepository
{
public:
    virtual ~IItemUserStateRepository() = default;

    /**
     * @brief Получает список записей истории состояний с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param itemId Фильтр по элементу (std::nullopt - все)
     * @param userId Фильтр по пользователю (std::nullopt - все)
     * @return Страница с записями истории
     */
    virtual ItemUserStatesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt
    ) = 0;

    /**
     * @brief Находит запись истории по ID.
     * @param id Идентификатор записи
     * @return DTO записи или std::nullopt
     */
    virtual std::optional<dto::ItemUserState> findById(int64_t id) = 0;

    /**
     * @brief Получает все записи истории для элемента.
     * @param itemId Идентификатор элемента
     * @return Вектор записей истории
     */
    virtual std::vector<dto::ItemUserState> findByItemId(int64_t itemId) = 0;

    /**
     * @brief Получает все записи истории для пользователя.
     * @param userId Идентификатор пользователя
     * @return Вектор записей истории
     */
    virtual std::vector<dto::ItemUserState> findByUserId(int64_t userId) = 0;

    /**
     * @brief Получает последнюю запись истории для элемента.
     * @param itemId Идентификатор элемента
     * @return DTO записи или std::nullopt
     */
    virtual std::optional<dto::ItemUserState> findLastByItemId(int64_t itemId) = 0;

    /**
     * @brief Создаёт новую запись истории состояния.
     * @param state DTO записи истории
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::ItemUserState& state) = 0;

    /**
     * @brief Обновляет существующую запись истории.
     * @param state DTO записи с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::ItemUserState& state) = 0;

    /**
     * @brief Удаляет запись истории по ID.
     * @param id Идентификатор записи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все записи истории для элемента.
     * @param itemId Идентификатор элемента
     * @return Количество удалённых записей
     */
    virtual int64_t removeByItemId(int64_t itemId) = 0;

    /**
     * @brief Проверяет существование записи.
     * @param id Идентификатор записи
     * @return true если запись существует
     */
    virtual bool exists(int64_t id) = 0;
};

} // namespace repositories
} // namespace server
