#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/team.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для работы с командами.
 */
class ITeamRepository
{
public:
    virtual ~ITeamRepository() = default;

    /**
     * @brief Получает список команд с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param searchCaption Поиск по названию (пустая строка – без поиска)
     * @return Пара: вектор DTO команд и общее количество
     */
    virtual std::pair<std::vector<dto::Team>, int64_t> findAll(
        int page,
        int pageSize,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Находит команду по ID.
     * @param id Идентификатор команды
     * @return DTO команды или std::nullopt, если не найдена
     */
    virtual std::optional<dto::Team> findById(int64_t id) = 0;

    /**
     * @brief Создаёт новую команду.
     * @param team DTO команды
     * @return ID созданной команды или 0 при ошибке
     */
    virtual int64_t create(const dto::Team& team) = 0;

    /**
     * @brief Обновляет существующую команду.
     * @param team DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::Team& team) = 0;

    /**
     * @brief Удаляет команду.
     * @param id Идентификатор команды
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Проверяет существование команды с указанным ID.
     * @param id Идентификатор команды
     * @return true если команда существует
     */
    virtual bool exists(int64_t id) = 0;
};

} // namespace server::repositories
