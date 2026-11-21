
#include <AzCore/Serialization/SerializeContext.h>
#include "sqlgenCPPEditorSystemComponent.h"

#include <sqlgenCPP/sqlgenCPPTypeIds.h>

namespace sqlgenCPP
{
    AZ_COMPONENT_IMPL(sqlgenCPPEditorSystemComponent, "sqlgenCPPEditorSystemComponent",
        sqlgenCPPEditorSystemComponentTypeId, BaseSystemComponent);

    void sqlgenCPPEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<sqlgenCPPEditorSystemComponent, sqlgenCPPSystemComponent>()
                ->Version(0);
        }
    }

    sqlgenCPPEditorSystemComponent::sqlgenCPPEditorSystemComponent() = default;

    sqlgenCPPEditorSystemComponent::~sqlgenCPPEditorSystemComponent() = default;

    void sqlgenCPPEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("sqlgenCPPEditorService"));
    }

    void sqlgenCPPEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("sqlgenCPPEditorService"));
    }

    void sqlgenCPPEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void sqlgenCPPEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void sqlgenCPPEditorSystemComponent::Activate()
    {
        sqlgenCPPSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void sqlgenCPPEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        sqlgenCPPSystemComponent::Deactivate();
    }

} // namespace sqlgenCPP
