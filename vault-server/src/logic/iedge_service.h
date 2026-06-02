#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/edge.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с переходом.
 */
struct EdgeResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с переходами.
 */
struct EdgesPage
{
    std::vector<dto::Edge> edges;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с переходами.
 */
class IEdgeService
{
public:
    virtual ~IEdgeService() = default;

    /**
     * @brief Получает список переходов с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param beginStateId Фильтр по начальному состоянию (опционально)
     * @param endStateId Фильтр по конечному состоянию (опционально)
     * @return Страница с переходами
     */
    virtual EdgesPage edges(
        int page,
        int pageSize,
        std::optional<int64_t> beginStateId = std::nullopt,
        std::optional<int64_t> endStateId = std::nullopt
    ) = 0;

    /**
     * @brief Получает переход по ID.
     * @param id Идентификатор перехода
     * @return DTO перехода или std::nullopt
     */
    virtual std::optional<dto::Edge> edge(int64_t id) = 0;

    /**
     * @brief Создает новый переход.
     * @param edge DTO перехода
     * @param userId ID пользователя для проверки прав
     * @return Созданный переход или std::nullopt при ошибке
     */
    virtual std::optional<dto::Edge> createEdge(
        const dto::Edge& edge,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет переход.
     * @param id Идентификатор перехода
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual EdgeResult deleteEdge(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все переходы для рабочего процесса.
     * @param workflowId Идентификатор рабочего процесса
     * @return Вектор переходов
     */
    virtual std::vector<dto::Edge> getWorkflowEdges(int64_t workflowId) = 0;

    /**
     * @brief Проверяет возможность создания перехода.
     * @param beginStateId Начальное состояние
     * @param endStateId Конечное состояние
     * @return Результат проверки
     */
    virtual EdgeResult validateEdge(int64_t beginStateId, int64_t endStateId) = 0;
};

} // namespace services
} // namespace server
