#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/workflow.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с рабочим процессом.
 */
struct WorkflowResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с рабочими процессами.
 */
struct WorkflowsPage
{
    std::vector<dto::Workflow> workflows;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с рабочими процессами.
 */
class IWorkflowService
{
public:
    virtual ~IWorkflowService() = default;

    /**
     * @brief Получает список рабочих процессов с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @return Страница с рабочими процессами
     */
    virtual WorkflowsPage workflows(int page, int pageSize) = 0;

    /**
     * @brief Получает рабочий процесс по ID.
     * @param id Идентификатор рабочего процесса
     * @return DTO рабочего процесса или std::nullopt
     */
    virtual std::optional<dto::Workflow> workflow(int64_t id) = 0;

    /**
     * @brief Создает новый рабочий процесс.
     * @param workflow DTO рабочего процесса
     * @param userId ID пользователя для проверки прав
     * @return Созданный рабочий процесс или std::nullopt при ошибке
     */
    virtual std::optional<dto::Workflow> createWorkflow(
        const dto::Workflow& workflow,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий рабочий процесс.
     * @param workflow DTO рабочего процесса с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленный рабочий процесс или std::nullopt при ошибке
     */
    virtual std::optional<dto::Workflow> updateWorkflow(
        const dto::Workflow& workflow,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет рабочий процесс.
     * @param id Идентификатор рабочего процесса
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual WorkflowResult deleteWorkflow(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Проверяет, можно ли удалить рабочий процесс.
     * @param id Идентификатор рабочего процесса
     * @return true если удаление возможно
     */
    virtual bool canDeleteWorkflow(int64_t id) = 0;
};

} // namespace services
} // namespace server
