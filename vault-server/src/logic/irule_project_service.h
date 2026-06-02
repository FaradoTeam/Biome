#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_project.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка прав на проекты.
 */
struct RuleProjectsPage
{
    std::vector<dto::RuleProject> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления правами на проекты (RuleProject).
 */
class IRuleProjectService
{
public:
    virtual ~IRuleProjectService() = default;

    /**
     * @brief Получает список прав на проекты с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param ruleId Фильтр по идентификатору правила (опционально)
     * @param projectId Фильтр по идентификатору проекта (опционально)
     * @return Страница с правами на проекты
     */
    virtual RuleProjectsPage getRuleProjects(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> projectId = std::nullopt
    ) = 0;

    /**
     * @brief Получает право на проект по ID.
     * @param id Идентификатор записи
     * @return DTO права или std::nullopt
     */
    virtual std::optional<dto::RuleProject> getRuleProject(int64_t id) = 0;

    /**
     * @brief Создает новое право на проект.
     * @param ruleProject DTO права
     * @param userId ID пользователя для проверки прав
     * @return Созданное право или std::nullopt при ошибке
     */
    virtual std::optional<dto::RuleProject> createRuleProject(
        const dto::RuleProject& ruleProject,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее право на проект.
     * @param ruleProject DTO права с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное право или std::nullopt при ошибке
     */
    virtual std::optional<dto::RuleProject> updateRuleProject(
        const dto::RuleProject& ruleProject,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет право на проект.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteRuleProject(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
