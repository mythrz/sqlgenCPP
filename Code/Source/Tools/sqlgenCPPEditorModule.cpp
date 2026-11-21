
#include <sqlgenCPP/sqlgenCPPTypeIds.h>
#include <sqlgenCPPModuleInterface.h>
#include "sqlgenCPPEditorSystemComponent.h"

namespace sqlgenCPP
{
    class sqlgenCPPEditorModule
        : public sqlgenCPPModuleInterface
    {
    public:
        AZ_RTTI(sqlgenCPPEditorModule, sqlgenCPPEditorModuleTypeId, sqlgenCPPModuleInterface);
        AZ_CLASS_ALLOCATOR(sqlgenCPPEditorModule, AZ::SystemAllocator);

        sqlgenCPPEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                sqlgenCPPEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<sqlgenCPPEditorSystemComponent>(),
            };
        }
    };
}// namespace sqlgenCPP

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), sqlgenCPP::sqlgenCPPEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_sqlgenCPP_Editor, sqlgenCPP::sqlgenCPPEditorModule)
#endif
