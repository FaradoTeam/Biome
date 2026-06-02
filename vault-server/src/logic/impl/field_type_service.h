#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/ifield_type_service.h"
#include "repo/field_type_repository.h"

namespace server
{
namespace services
{

class FieldTypeService final : public IFieldTypeService
{
public:
    explicit FieldTypeService(
        std::shared_ptr<repositories::IFieldTypeRepository> fieldTypeRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    FieldTypesPage fieldTypes(
        int page,
        int pageSize,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<std::string> valueType = std::nullopt,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::FieldType> fieldType(int64_t id) override;

    std::optional<dto::FieldType> createFieldType(
        const dto::FieldType& fieldType,
        int64_t userId
    ) override;

    std::optional<dto::FieldType> updateFieldType(
        const dto::FieldType& fieldType,
        int64_t userId
    ) override;

    bool deleteFieldType(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::FieldType> fieldTypesByItemType(int64_t itemTypeId) override;

private:
    std::shared_ptr<repositories::IFieldTypeRepository> m_fieldTypeRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
