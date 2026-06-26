#pragma once

#include <optional>
#include <vector>

#include "common/dto/comment.h"
#include "common/types.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка комментариев.
 */
struct CommentsPage
{
    std::vector<dto::Comment> comments;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с комментариями.
 */
class ICommentRepository
{
public:
    virtual ~ICommentRepository() = default;

    /**
     * @brief Получает список комментариев с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param itemId Фильтр по элементу (std::nullopt - все)
     * @param userId Фильтр по автору (std::nullopt - все)
     * @param dateFrom Фильтр по дате начала (std::nullopt - без ограничения)
     * @param dateTo Фильтр по дате окончания (std::nullopt - без ограничения)
     * @return Страница с комментариями
     */
    virtual CommentsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Находит комментарий по ID.
     * @param id Идентификатор комментария
     * @return DTO комментария или std::nullopt
     */
    virtual std::optional<dto::Comment> findById(int64_t id) = 0;

    /**
     * @brief Находит все комментарии для элемента.
     * @param itemId Идентификатор элемента
     * @param sortAsc Сортировать по возрастанию времени (true) или убыванию (false)
     * @return Вектор комментариев
     */
    virtual std::vector<dto::Comment> findByItemId(
        int64_t itemId,
        bool sortAsc = true
    ) = 0;

    /**
     * @brief Находит все комментарии автора.
     * @param userId Идентификатор пользователя
     * @return Вектор комментариев
     */
    virtual std::vector<dto::Comment> findByUserId(int64_t userId) = 0;

    /**
     * @brief Создаёт новый комментарий.
     * @param comment DTO комментария
     * @return ID созданного комментария или 0 при ошибке
     */
    virtual int64_t create(const dto::Comment& comment) = 0;

    /**
     * @brief Обновляет существующий комментарий.
     * @param comment DTO комментария с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Comment& comment) = 0;

    /**
     * @brief Удаляет комментарий по ID.
     * @param id Идентификатор комментария
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все комментарии для элемента.
     * @param itemId Идентификатор элемента
     * @return Количество удалённых комментариев
     */
    virtual int64_t removeByItemId(int64_t itemId) = 0;

    /**
     * @brief Проверяет существование комментария.
     * @param id Идентификатор комментария
     * @return true если комментарий существует
     */
    virtual bool exists(int64_t id) = 0;

    /**
     * @brief Получает количество комментариев для элемента.
     * @param itemId Идентификатор элемента
     * @return Количество комментариев
     */
    virtual int64_t countByItemId(int64_t itemId) = 0;
};

} // namespace repositories
} // namespace server
