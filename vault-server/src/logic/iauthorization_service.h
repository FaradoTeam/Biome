#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace server::services
{

/**
 * @brief Результат проверки прав доступа.
 */
struct AuthzResult
{
    bool granted = false;
    int errorCode = 403; // HTTP 403 Forbidden по умолчанию
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса авторизации.
 *
 * Отвечает за проверку прав пользователя на выполнение операций
 * над различными объектами системы.
 */
class IAuthorizationService
{
public:
    virtual ~IAuthorizationService() = default;

    // ========== Глобальные проверки ==========

    /**
     * @brief Проверяет, является ли пользователь супер-администратором.
     * @param userId ID пользователя
     * @return true если супер-админ
     */
    virtual bool isSuperAdmin(int64_t userId) = 0;

    // ========== Права на проекты (RuleProject) ==========

    /**
     * @brief Проверяет право на чтение проекта.
     * @param userId ID пользователя
     * @param projectId ID проекта
     * @return Результат проверки
     */
    virtual AuthzResult canReadProject(int64_t userId, int64_t projectId) = 0;

    /**
     * @brief Проверяет право на запись (создание/изменение элементов) в проекте.
     * @param userId ID пользователя
     * @param projectId ID проекта
     * @return Результат проверки
     */
    virtual AuthzResult canWriteToProject(int64_t userId, int64_t projectId) = 0;

    /**
     * @brief Проверяет право на редактирование проекта (изменение метаданных, создание подпроектов).
     * @param userId ID пользователя
     * @param projectId ID проекта
     * @return Результат проверки
     */
    virtual AuthzResult canEditProject(int64_t userId, int64_t projectId) = 0;

    /**
     * @brief Проверяет право на редактирование фаз проекта.
     * @param userId ID пользователя
     * @param projectId ID проекта
     * @return Результат проверки
     */
    virtual AuthzResult canEditPhases(int64_t userId, int64_t projectId) = 0;

    /**
     * @brief Проверяет право на редактирование досок проекта.
     * @param userId ID пользователя
     * @param projectId ID проекта
     * @return Результат проверки
     */
    virtual AuthzResult canEditBoards(int64_t userId, int64_t projectId) = 0;

    // ========== Права на типы элементов (RuleItemType) ==========

    /**
     * @brief Проверяет право на чтение элементов указанного типа.
     * @param userId ID пользователя
     * @param projectId ID проекта (контекст)
     * @param itemTypeId ID типа элемента
     * @return Результат проверки
     */
    virtual AuthzResult canReadItemType(int64_t userId, int64_t projectId, int64_t itemTypeId) = 0;

    /**
     * @brief Проверяет право на создание/изменение элементов указанного типа.
     * @param userId ID пользователя
     * @param projectId ID проекта (контекст)
     * @param itemTypeId ID типа элемента
     * @return Результат проверки
     */
    virtual AuthzResult canWriteItemType(int64_t userId, int64_t projectId, int64_t itemTypeId) = 0;

    // ========== Права на состояния (RuleState) ==========

    /**
     * @brief Проверяет право на переход элемента в указанное состояние.
     * @param userId ID пользователя
     * @param projectId ID проекта (контекст)
     * @param stateId ID целевого состояния
     * @return Результат проверки
     */
    virtual AuthzResult canTransitionToState(int64_t userId, int64_t projectId, int64_t stateId) = 0;

    // ========== Права на создание корневых проектов (Rule) ==========

    /**
     * @brief Проверяет право на создание корневого проекта.
     * @param userId ID пользователя
     * @return Результат проверки
     */
    virtual AuthzResult canCreateRootProject(int64_t userId) = 0;

    // ========== Вспомогательные методы ==========

    /**
     * @brief Получает все ID проектов, доступные пользователю для чтения.
     * @param userId ID пользователя
     * @return Вектор ID проектов
     */
    virtual std::vector<int64_t> getReadableProjectIds(int64_t userId) = 0;
};

} // namespace server::services
