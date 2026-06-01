#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/phase.h"
#include "common/helpers/time_helpers.h"

#include "logic/iphase_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-реализация сервиса фаз для тестирования.
 */
class MockPhaseService : public services::IPhaseService
{
public:
    using PhasesPage = services::PhasesPage;

    MockPhaseService() = default;
    ~MockPhaseService() override = default;

    // ============================================================
    // Настройка результатов (простой режим)
    // ============================================================

    void setGetPhasesResult(const PhasesPage& result)
    {
        m_getPhasesResult = result;
        m_getPhasesCallback = nullptr;
    }

    void setGetPhaseResult(std::optional<dto::Phase> phase)
    {
        m_getPhaseResult = std::move(phase);
        m_getPhaseCallback = nullptr;
    }

    void setCreatePhaseResult(std::optional<dto::Phase> phase)
    {
        m_createPhaseResult = std::move(phase);
        m_createPhaseCallback = nullptr;
    }

    void setUpdatePhaseResult(std::optional<dto::Phase> phase)
    {
        m_updatePhaseResult = std::move(phase);
        m_updatePhaseCallback = nullptr;
    }

    void setArchivePhaseResult(bool result)
    {
        m_archivePhaseResult = result;
        m_archivePhaseCallback = nullptr;
    }

    void setRestorePhaseResult(bool result)
    {
        m_restorePhaseResult = result;
        m_restorePhaseCallback = nullptr;
    }

    // ============================================================
    // Настройка данных для автоматической фильтрации
    // ============================================================

    void setAllPhases(const std::vector<dto::Phase>& phases)
    {
        m_allPhases = phases;
    }

    void setUserAccessibleProjects(int64_t userId, const std::vector<int64_t>& projectIds)
    {
        m_userAccessibleProjects[userId] = projectIds;
    }

    void clearAllPhases()
    {
        m_allPhases.clear();
    }

    void clearUserAccessibleProjects()
    {
        m_userAccessibleProjects.clear();
    }

    // ============================================================
    // Публичные методы для использования в callback (обёртки)
    // ============================================================

    PhasesPage filterPhases(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId,
        std::optional<bool> isArchive
    )
    {
        std::vector<dto::Phase> filtered = m_allPhases;

        // Фильтр по проекту
        if (projectId.has_value())
        {
            filtered.erase(
                std::remove_if(filtered.begin(), filtered.end(), [&projectId](const dto::Phase& p)
                               { return !p.projectId.has_value() || *p.projectId != *projectId; }),
                filtered.end()
            );
        }

        // Фильтр по архиву
        if (isArchive.has_value())
        {
            // Если isArchive явно указан, фильтруем по нему
            filtered.erase(
                std::remove_if(filtered.begin(), filtered.end(), [&isArchive](const dto::Phase& p)
                               { return p.isArchive.value_or(false) != *isArchive; }),
                filtered.end()
            );
        }
        else
        {
            // Если isArchive НЕ указан:
            // - Для супер-админа (userId=1) показываем ВСЕ фазы
            // - Для обычных пользователей показываем только НЕАРХИВНЫЕ
            if (userId != 1)
            {
                filtered.erase(
                    std::remove_if(filtered.begin(), filtered.end(), [](const dto::Phase& p)
                                   { return p.isArchive.value_or(false) == true; }),
                    filtered.end()
                );
            }
        }

        // Фильтр по правам доступа (если не супер-админ)
        if (userId != 1)
        {
            auto it = m_userAccessibleProjects.find(userId);
            if (it != m_userAccessibleProjects.end())
            {
                const auto& accessibleProjects = it->second;
                filtered.erase(
                    std::remove_if(filtered.begin(), filtered.end(), [&accessibleProjects](const dto::Phase& p)
                                   { return !p.projectId.has_value() || std::find(accessibleProjects.begin(), accessibleProjects.end(), *p.projectId) == accessibleProjects.end(); }),
                    filtered.end()
                );
            }
            else
            {
                filtered.clear();
            }
        }

        int64_t totalCount = filtered.size();
        int startIndex = (page - 1) * pageSize;
        int endIndex = std::min(startIndex + pageSize, static_cast<int>(totalCount));

        std::vector<dto::Phase> pagedPhases;
        if (startIndex < totalCount)
        {
            pagedPhases.assign(filtered.begin() + startIndex, filtered.begin() + endIndex);
        }

        PhasesPage result;
        result.phases = pagedPhases;
        result.totalCount = totalCount;
        return result;
    }

    std::optional<dto::Phase> getPhaseById(int64_t id, int64_t userId)
    {
        auto it = std::find_if(m_allPhases.begin(), m_allPhases.end(), [id](const dto::Phase& p)
                               { return p.id.has_value() && *p.id == id; });

        if (it == m_allPhases.end())
        {
            return std::nullopt;
        }

        if (userId != 1)
        {
            auto projectsIt = m_userAccessibleProjects.find(userId);
            if (projectsIt == m_userAccessibleProjects.end() || !it->projectId.has_value() || std::find(projectsIt->second.begin(), projectsIt->second.end(), *it->projectId) == projectsIt->second.end())
            {
                return std::nullopt;
            }
        }

        return *it;
    }

    std::optional<dto::Phase> createPhaseInternal(const dto::Phase& phase, int64_t userId)
    {
        if (userId != 1)
        {
            auto projectsIt = m_userAccessibleProjects.find(userId);
            if (projectsIt == m_userAccessibleProjects.end() || !phase.projectId.has_value() || std::find(projectsIt->second.begin(), projectsIt->second.end(), *phase.projectId) == projectsIt->second.end())
            {
                return std::nullopt;
            }
        }

        dto::Phase newPhase = phase;
        newPhase.id = m_nextPhaseId++;

        if (!newPhase.beginDate.has_value())
        {
            newPhase.beginDate = std::chrono::system_clock::now();
        }
        if (!newPhase.endDate.has_value())
        {
            newPhase.endDate = newPhase.beginDate.value() + std::chrono::hours(24 * 30);
        }

        m_allPhases.push_back(newPhase);
        return newPhase;
    }

    std::optional<dto::Phase> updatePhaseInternal(const dto::Phase& phase, int64_t userId)
    {
        if (!phase.id.has_value())
        {
            return std::nullopt;
        }

        auto it = std::find_if(m_allPhases.begin(), m_allPhases.end(), [&phase](const dto::Phase& p)
                               { return p.id.has_value() && *p.id == *phase.id; });

        if (it == m_allPhases.end())
        {
            return std::nullopt;
        }

        if (userId != 1)
        {
            auto projectsIt = m_userAccessibleProjects.find(userId);
            if (projectsIt == m_userAccessibleProjects.end() || !it->projectId.has_value() || std::find(projectsIt->second.begin(), projectsIt->second.end(), *it->projectId) == projectsIt->second.end())
            {
                return std::nullopt;
            }
        }

        if (phase.caption.has_value())
            it->caption = phase.caption;
        if (phase.description.has_value())
            it->description = phase.description;
        if (phase.beginDate.has_value())
            it->beginDate = phase.beginDate;
        if (phase.endDate.has_value())
            it->endDate = phase.endDate;
        if (phase.isArchive.has_value())
            it->isArchive = phase.isArchive;
        if (phase.projectId.has_value())
            it->projectId = phase.projectId;

        return *it;
    }

    bool archivePhaseInternal(int64_t id, int64_t userId)
    {
        auto it = std::find_if(m_allPhases.begin(), m_allPhases.end(), [id](const dto::Phase& p)
                               { return p.id.has_value() && *p.id == id; });

        if (it == m_allPhases.end())
        {
            return false;
        }

        if (userId != 1)
        {
            auto projectsIt = m_userAccessibleProjects.find(userId);
            if (projectsIt == m_userAccessibleProjects.end() || !it->projectId.has_value() || std::find(projectsIt->second.begin(), projectsIt->second.end(), *it->projectId) == projectsIt->second.end())
            {
                return false;
            }
        }

        it->isArchive = true;
        return true;
    }

    bool restorePhaseInternal(int64_t id, int64_t userId)
    {
        auto it = std::find_if(m_allPhases.begin(), m_allPhases.end(), [id](const dto::Phase& p)
                               { return p.id.has_value() && *p.id == id; });

        if (it == m_allPhases.end())
        {
            return false;
        }

        if (userId != 1)
        {
            auto projectsIt = m_userAccessibleProjects.find(userId);
            if (projectsIt == m_userAccessibleProjects.end() || !it->projectId.has_value() || std::find(projectsIt->second.begin(), projectsIt->second.end(), *it->projectId) == projectsIt->second.end())
            {
                return false;
            }
        }

        it->isArchive = false;
        return true;
    }

    // ============================================================
    // Реализация интерфейса IPhaseService
    // ============================================================

    PhasesPage phases(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<bool> isArchive = std::nullopt
    ) override
    {
        m_lastGetPhasesPage = page;
        m_lastGetPhasesPageSize = pageSize;
        m_lastGetPhasesUserId = userId;
        m_lastGetPhasesProjectId = projectId;
        m_lastGetPhasesIsArchive = isArchive;
        ++m_getPhasesCallCount;

        if (m_getPhasesCallback)
        {
            return m_getPhasesCallback(page, pageSize, userId, projectId, isArchive);
        }

        if (!m_allPhases.empty())
        {
            return filterPhases(page, pageSize, userId, projectId, isArchive);
        }

        return m_getPhasesResult;
    }

    std::optional<dto::Phase> phase(int64_t id, int64_t userId) override
    {
        m_lastGetPhaseId = id;
        m_lastGetPhaseUserId = userId;
        ++m_getPhaseCallCount;

        if (m_getPhaseCallback)
        {
            return m_getPhaseCallback(id, userId);
        }

        if (!m_allPhases.empty())
        {
            return getPhaseById(id, userId);
        }

        return m_getPhaseResult;
    }

    std::optional<dto::Phase> createPhase(
        const dto::Phase& phase,
        int64_t userId
    ) override
    {
        m_lastCreatedPhase = phase;
        m_lastCreatePhaseUserId = userId;
        ++m_createPhaseCallCount;

        if (m_createPhaseCallback)
        {
            return m_createPhaseCallback(phase, userId);
        }

        if (!m_allPhases.empty() || m_createPhaseResult.has_value())
        {
            return createPhaseInternal(phase, userId);
        }

        return m_createPhaseResult;
    }

    std::optional<dto::Phase> updatePhase(
        const dto::Phase& phase,
        int64_t userId
    ) override
    {
        m_lastUpdatedPhase = phase;
        m_lastUpdatePhaseUserId = userId;
        ++m_updatePhaseCallCount;

        if (m_updatePhaseCallback)
        {
            return m_updatePhaseCallback(phase, userId);
        }

        if (!m_allPhases.empty())
        {
            return updatePhaseInternal(phase, userId);
        }

        return m_updatePhaseResult;
    }

    bool archivePhase(int64_t id, int64_t userId) override
    {
        m_lastArchivedPhaseId = id;
        m_lastArchivePhaseUserId = userId;
        ++m_archivePhaseCallCount;

        if (m_archivePhaseCallback)
        {
            return m_archivePhaseCallback(id, userId);
        }

        if (!m_allPhases.empty())
        {
            return archivePhaseInternal(id, userId);
        }

        return m_archivePhaseResult;
    }

    bool restorePhase(int64_t id, int64_t userId) override
    {
        m_lastRestoredPhaseId = id;
        m_lastRestorePhaseUserId = userId;
        ++m_restorePhaseCallCount;

        if (m_restorePhaseCallback)
        {
            return m_restorePhaseCallback(id, userId);
        }

        if (!m_allPhases.empty())
        {
            return restorePhaseInternal(id, userId);
        }

        return m_restorePhaseResult;
    }

    // ============================================================
    // Настройка callback'ов
    // ============================================================

    void setGetPhasesCallback(
        std::function<PhasesPage(int, int, int64_t, std::optional<int64_t>, std::optional<bool>)> callback
    )
    {
        m_getPhasesCallback = std::move(callback);
    }

    void setGetPhaseCallback(
        std::function<std::optional<dto::Phase>(int64_t, int64_t)> callback
    )
    {
        m_getPhaseCallback = std::move(callback);
    }

    void setCreatePhaseCallback(
        std::function<std::optional<dto::Phase>(const dto::Phase&, int64_t)> callback
    )
    {
        m_createPhaseCallback = std::move(callback);
    }

    void setUpdatePhaseCallback(
        std::function<std::optional<dto::Phase>(const dto::Phase&, int64_t)> callback
    )
    {
        m_updatePhaseCallback = std::move(callback);
    }

    void setArchivePhaseCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_archivePhaseCallback = std::move(callback);
    }

    void setRestorePhaseCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_restorePhaseCallback = std::move(callback);
    }

    // ============================================================
    // Методы для проверки вызовов
    // ============================================================

    int getGetPhasesCallCount() const { return m_getPhasesCallCount; }
    int getGetPhaseCallCount() const { return m_getPhaseCallCount; }
    int getCreatePhaseCallCount() const { return m_createPhaseCallCount; }
    int getUpdatePhaseCallCount() const { return m_updatePhaseCallCount; }
    int getArchivePhaseCallCount() const { return m_archivePhaseCallCount; }
    int getRestorePhaseCallCount() const { return m_restorePhaseCallCount; }

    int getLastGetPhasesPage() const { return m_lastGetPhasesPage; }
    int getLastGetPhasesPageSize() const { return m_lastGetPhasesPageSize; }
    int64_t getLastGetPhasesUserId() const { return m_lastGetPhasesUserId; }
    std::optional<int64_t> getLastGetPhasesProjectId() const { return m_lastGetPhasesProjectId; }
    std::optional<bool> getLastGetPhasesIsArchive() const { return m_lastGetPhasesIsArchive; }

    int64_t getLastGetPhaseId() const { return m_lastGetPhaseId; }
    int64_t getLastGetPhaseUserId() const { return m_lastGetPhaseUserId; }

    const dto::Phase& getLastCreatedPhase() const { return m_lastCreatedPhase; }
    int64_t getLastCreatePhaseUserId() const { return m_lastCreatePhaseUserId; }

    const dto::Phase& getLastUpdatedPhase() const { return m_lastUpdatedPhase; }
    int64_t getLastUpdatePhaseUserId() const { return m_lastUpdatePhaseUserId; }

    int64_t getLastArchivedPhaseId() const { return m_lastArchivedPhaseId; }
    int64_t getLastArchivePhaseUserId() const { return m_lastArchivePhaseUserId; }

    int64_t getLastRestoredPhaseId() const { return m_lastRestoredPhaseId; }
    int64_t getLastRestorePhaseUserId() const { return m_lastRestorePhaseUserId; }

    const std::vector<dto::Phase>& getAllPhases() const { return m_allPhases; }

    // ============================================================
    // Утилиты для создания тестовых данных
    // ============================================================

    static dto::Phase createTestPhase(
        int64_t id,
        const std::string& caption,
        int64_t projectId,
        bool isArchive = false
    )
    {
        dto::Phase phase;
        phase.id = id;
        phase.caption = caption;
        phase.projectId = projectId;
        phase.isArchive = isArchive;
        phase.description = "Test description for " + caption;

        auto now = std::chrono::system_clock::now();
        phase.beginDate = now;
        phase.endDate = now + std::chrono::hours(24 * 30);

        return phase;
    }

    static PhasesPage createTestPhasesPage(
        const std::vector<dto::Phase>& phases,
        int64_t totalCount = -1
    )
    {
        PhasesPage page;
        page.phases = phases;
        page.totalCount = (totalCount >= 0) ? totalCount : static_cast<int64_t>(phases.size());
        return page;
    }

    // ============================================================
    // Сброс состояния
    // ============================================================

    void reset()
    {
        m_getPhasesCallCount = 0;
        m_getPhaseCallCount = 0;
        m_createPhaseCallCount = 0;
        m_updatePhaseCallCount = 0;
        m_archivePhaseCallCount = 0;
        m_restorePhaseCallCount = 0;

        m_lastGetPhasesPage = 0;
        m_lastGetPhasesPageSize = 0;
        m_lastGetPhasesUserId = 0;
        m_lastGetPhasesProjectId = std::nullopt;
        m_lastGetPhasesIsArchive = std::nullopt;

        m_lastGetPhaseId = 0;
        m_lastGetPhaseUserId = 0;

        m_lastCreatedPhase = dto::Phase {};
        m_lastCreatePhaseUserId = 0;

        m_lastUpdatedPhase = dto::Phase {};
        m_lastUpdatePhaseUserId = 0;

        m_lastArchivedPhaseId = 0;
        m_lastArchivePhaseUserId = 0;

        m_lastRestoredPhaseId = 0;
        m_lastRestorePhaseUserId = 0;

        m_getPhasesCallback = nullptr;
        m_getPhaseCallback = nullptr;
        m_createPhaseCallback = nullptr;
        m_updatePhaseCallback = nullptr;
        m_archivePhaseCallback = nullptr;
        m_restorePhaseCallback = nullptr;

        m_getPhasesResult = PhasesPage {};
        m_getPhaseResult = std::nullopt;
        m_createPhaseResult = std::nullopt;
        m_updatePhaseResult = std::nullopt;
        m_archivePhaseResult = false;
        m_restorePhaseResult = false;

        m_allPhases.clear();
        m_userAccessibleProjects.clear();
        m_nextPhaseId = 100;
    }

private:
    // Предустановленные результаты
    PhasesPage m_getPhasesResult;
    std::optional<dto::Phase> m_getPhaseResult;
    std::optional<dto::Phase> m_createPhaseResult;
    std::optional<dto::Phase> m_updatePhaseResult;
    bool m_archivePhaseResult = false;
    bool m_restorePhaseResult = false;

    // Данные для фильтрации
    std::vector<dto::Phase> m_allPhases;
    std::map<int64_t, std::vector<int64_t>> m_userAccessibleProjects;
    int64_t m_nextPhaseId = 100;

    // Callback'и
    std::function<PhasesPage(int, int, int64_t, std::optional<int64_t>, std::optional<bool>)> m_getPhasesCallback;
    std::function<std::optional<dto::Phase>(int64_t, int64_t)> m_getPhaseCallback;
    std::function<std::optional<dto::Phase>(const dto::Phase&, int64_t)> m_createPhaseCallback;
    std::function<std::optional<dto::Phase>(const dto::Phase&, int64_t)> m_updatePhaseCallback;
    std::function<bool(int64_t, int64_t)> m_archivePhaseCallback;
    std::function<bool(int64_t, int64_t)> m_restorePhaseCallback;

    // Счётчики вызовов
    int m_getPhasesCallCount = 0;
    int m_getPhaseCallCount = 0;
    int m_createPhaseCallCount = 0;
    int m_updatePhaseCallCount = 0;
    int m_archivePhaseCallCount = 0;
    int m_restorePhaseCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetPhasesPage = 0;
    int m_lastGetPhasesPageSize = 0;
    int64_t m_lastGetPhasesUserId = 0;
    std::optional<int64_t> m_lastGetPhasesProjectId;
    std::optional<bool> m_lastGetPhasesIsArchive;

    int64_t m_lastGetPhaseId = 0;
    int64_t m_lastGetPhaseUserId = 0;

    dto::Phase m_lastCreatedPhase;
    int64_t m_lastCreatePhaseUserId = 0;

    dto::Phase m_lastUpdatedPhase;
    int64_t m_lastUpdatePhaseUserId = 0;

    int64_t m_lastArchivedPhaseId = 0;
    int64_t m_lastArchivePhaseUserId = 0;

    int64_t m_lastRestoredPhaseId = 0;
    int64_t m_lastRestorePhaseUserId = 0;
};

} // namespace tests
} // namespace server
