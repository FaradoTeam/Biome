#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_project.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для прав на проекты (RuleProject).
 */
class IRuleProjectRepository
{
public:
    virtual ~IRuleProjectRepository() = default;

    /**
     * @brief Получает список прав с пагинацией и фильтрацией.
     * @param page Номер страницы
     * @param pageSize Размер страницы
     * @param ruleId Фильтр по правилу
     * @param projectId Фильтр по проекту
     * @return Пара: вектор прав и общее количество
     */
    virtual std::pair<std::vector<dto::RuleProject>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> projectId = std::nullopt
    ) = 0;

    /**
     * @brief Находит право по ID.
     * @param id Идентификатор записи
     * @return DTO права или std::nullopt
     */
    virtual std::optional<dto::RuleProject> findById(int64_t id) = 0;

    /**
     * @brief Находит права по идентификатору правила.
     * @param ruleId Идентификатор правила
     * @return Вектор прав
     */
    virtual std::vector<dto::RuleProject> findByRuleId(int64_t ruleId) = 0;

    /**
     * @brief Находит права по идентификатору проекта.
     * @param projectId Идентификатор проекта
     * @return Вектор прав
     */
    virtual std::vector<dto::RuleProject> findByProjectId(int64_t projectId) = 0;

    /**
     * @brief Проверяет существование права для пары (ruleId, projectId).
     * @param ruleId Идентификатор правила
     * @param projectId Идентификатор проекта
     * @return true если существует
     */
    virtual bool exists(int64_t ruleId, int64_t projectId) = 0;

    /**
     * @brief Создаёт новое право.
     * @param ruleProject DTO права
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::RuleProject& ruleProject) = 0;

    /**
     * @brief Обновляет существующее право.
     * @param ruleProject DTO с новыми данными (поле id обязательно)
     * @return true если обновление успешно
     */
    virtual bool update(const dto::RuleProject& ruleProject) = 0;

    /**
     * @brief Удаляет право.
     * @param id Идентификатор записи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;
};

} // namespace server::repositories
