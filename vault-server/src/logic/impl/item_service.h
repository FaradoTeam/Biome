#pragma once

#include <memory>
#include <optional>

#include "logic/iauthorization_service.h"
#include "logic/iitem_service.h"

#include "repo/field_type_repository.h"
#include "repo/item_field_repository.h"
#include "repo/item_repository.h"
#include "repo/item_type_repository.h"
#include "repo/phase_repository.h"
#include "repo/project_repository.h"
#include "repo/state_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с элементами и их полями.
 */
class ItemService final : public IItemService
{
public:
    ItemService(
        std::shared_ptr<repositories::IItemRepository> itemRepo,
        std::shared_ptr<repositories::IItemFieldRepository> itemFieldRepo,
        std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo,
        std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
        std::shared_ptr<repositories::IProjectRepository> projectRepo,
        std::shared_ptr<repositories::IStateRepository> stateRepo,
        std::shared_ptr<repositories::IFieldTypeRepository> fieldTypeRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IItemService
    ItemsPage items(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt,
        std::optional<bool> isDeleted = std::nullopt,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::Item> item(int64_t id, int64_t userId) override;

    std::optional<dto::Item> createItem(
        const dto::Item& item,
        int64_t userId
    ) override;

    std::optional<dto::Item> updateItem(
        const dto::Item& item,
        int64_t userId
    ) override;

    ItemResult deleteItem(int64_t id, int64_t userId) override;
    ItemResult restoreItem(int64_t id, int64_t userId) override;

    std::vector<dto::ItemField> getItemFields(
        int64_t itemId,
        int64_t userId
    ) override;

    std::optional<dto::ItemField> getItemField(
        int64_t itemId,
        int64_t fieldTypeId,
        int64_t userId
    ) override;

    std::optional<dto::ItemField> setItemField(
        const dto::ItemField& field,
        int64_t userId
    ) override;

    ItemResult deleteItemField(
        int64_t itemId,
        int64_t fieldTypeId,
        int64_t userId
    ) override;

private:
    /**
     * @brief Получает ID проекта по ID фазы.
     * @param phaseId ID фазы
     * @return ID проекта или std::nullopt
     */
    std::optional<int64_t> getProjectIdByPhaseId(int64_t phaseId);

    /**
     * @brief Проверяет, что элемент существует и пользователь имеет к нему доступ.
     * @param itemId ID элемента
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return DTO элемента или std::nullopt при ошибке
     */
    std::optional<dto::Item> checkItemAccess(
        int64_t itemId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Проверяет, что тип поля существует и пользователь имеет к нему доступ.
     * @param fieldTypeId ID типа поля
     * @param userId ID пользователя
     * @param projectId ID проекта
     * @param needWrite Требуется ли право на запись
     * @return true если доступ разрешён
     */
    bool checkFieldTypeAccess(
        int64_t fieldTypeId,
        int64_t userId,
        int64_t projectId,
        bool needWrite = false
    );

    /**
     * @brief Валидирует DTO элемента.
     */
    bool validateItem(const dto::Item& item, std::string& errorMessage);

    /**
     * @brief Валидирует DTO значения поля.
     */
    bool validateItemField(
        const dto::ItemField& field,
        const dto::FieldType& fieldType,
        std::string& errorMessage
    );

    /**
     * @brief Получает список ID проектов, доступных пользователю.
     */
    std::vector<int64_t> getAccessibleProjectIds(int64_t userId);

private:
    std::shared_ptr<repositories::IItemRepository> m_itemRepo;
    std::shared_ptr<repositories::IItemFieldRepository> m_itemFieldRepo;
    std::shared_ptr<repositories::IItemTypeRepository> m_itemTypeRepo;
    std::shared_ptr<repositories::IPhaseRepository> m_phaseRepo;
    std::shared_ptr<repositories::IProjectRepository> m_projectRepo;
    std::shared_ptr<repositories::IStateRepository> m_stateRepo;
    std::shared_ptr<repositories::IFieldTypeRepository> m_fieldTypeRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
