
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/sqlgenCPPSystemComponent.h>

namespace sqlgenCPP
{
    /// System component for sqlgenCPP editor
    class sqlgenCPPEditorSystemComponent
        : public sqlgenCPPSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = sqlgenCPPSystemComponent;
    public:
        AZ_COMPONENT_DECL(sqlgenCPPEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        sqlgenCPPEditorSystemComponent();
        ~sqlgenCPPEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace sqlgenCPP
