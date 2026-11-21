
#include "sqlgenCPPSystemComponent.h"

#include <sqlgenCPP/sqlgenCPPTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace sqlgenCPP
{
    AZ_COMPONENT_IMPL(sqlgenCPPSystemComponent, "sqlgenCPPSystemComponent",
        sqlgenCPPSystemComponentTypeId);

    void sqlgenCPPSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<sqlgenCPPSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void sqlgenCPPSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("sqlgenCPPService"));
    }

    void sqlgenCPPSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("sqlgenCPPService"));
    }

    void sqlgenCPPSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void sqlgenCPPSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    sqlgenCPPSystemComponent::sqlgenCPPSystemComponent()
    {
        if (sqlgenCPPInterface::Get() == nullptr)
        {
            sqlgenCPPInterface::Register(this);
        }
    }

    sqlgenCPPSystemComponent::~sqlgenCPPSystemComponent()
    {
        if (sqlgenCPPInterface::Get() == this)
        {
            sqlgenCPPInterface::Unregister(this);
        }
    }

    void sqlgenCPPSystemComponent::Init()
    {
    }

    void sqlgenCPPSystemComponent::Activate()
    {
        sqlgenCPPRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void sqlgenCPPSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        sqlgenCPPRequestBus::Handler::BusDisconnect();
    }

    void sqlgenCPPSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace sqlgenCPP
