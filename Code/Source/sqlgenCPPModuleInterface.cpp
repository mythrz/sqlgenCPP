
#include "sqlgenCPPModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <sqlgenCPP/sqlgenCPPTypeIds.h>

#include <Clients/sqlgenCPPSystemComponent.h>

namespace sqlgenCPP
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(sqlgenCPPModuleInterface,
        "sqlgenCPPModuleInterface", sqlgenCPPModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(sqlgenCPPModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(sqlgenCPPModuleInterface, AZ::SystemAllocator);

    sqlgenCPPModuleInterface::sqlgenCPPModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            sqlgenCPPSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList sqlgenCPPModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<sqlgenCPPSystemComponent>(),
        };
    }
} // namespace sqlgenCPP
