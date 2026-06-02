#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/state.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с состоянием.
 */
struct StateResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с состояниями.
 */
struct StatesPage
{
    std::vector<dto::State> states;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с состояниями.
 */
class IStateService
{
public:
    virtual ~IStateService() = default;

    /**
     * @brief Получает список состояний с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param workflowId Фильтр по рабочему процессу (опционально)
     * @return Страница с состояниями
     */
    virtual StatesPage states(
        int page,
        int pageSize,
        std::optional<int64_t> workflowId = std::nullopt
    ) = 0;

    /**
     * @brief Получает состояние по ID.
     * @param id Идентификатор состояния
     * @return DTO состояния или std::nullopt
     */
    virtual std::optional<dto::State> state(int64_t id) = 0;

    /**
     * @brief Создает новое состояние.
     * @param state DTO состояния
     * @param userId ID пользователя для проверки прав
     * @return Созданное состояние или std::nullopt при ошибке
     */
    virtual std::optional<dto::State> createState(
        const dto::State& state,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее состояние.
     * @param state DTO состояния с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное состояние или std::nullopt при ошибке
     */
    virtual std::optional<dto::State> updateState(
        const dto::State& state,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет состояние.
     * @param id Идентификатор состояния
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual StateResult deleteState(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все состояния рабочего процесса.
     * @param workflowId Идентификатор рабочего процесса
     * @return Вектор состояний
     */
    virtual std::vector<dto::State> getWorkflowStates(int64_t workflowId) = 0;

    /**
     * @brief Проверяет, можно ли удалить состояние.
     * @param id Идентификатор состояния
     * @return true если удаление возможно
     */
    virtual bool canDeleteState(int64_t id) = 0;
};

} // namespace services
} // namespace server
