/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <sqlgenCPP/sqlgenCPPBus.h>

namespace sqlgenCPP
{
    class sqlgenCPPSystemComponent
        : public AZ::Component
        , protected sqlgenCPPRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(sqlgenCPPSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        sqlgenCPPSystemComponent();
        ~sqlgenCPPSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // sqlgenCPPRequestBus interface implementation
        AZ::Outcome<AZStd::string, AZStd::string> ResolveDatabasePath(
            AZStd::string_view relativeDbPath,
            bool createParentDirectory = true) const override;
        AZStd::string GetDefaultDatabaseDirectory() const override;
        bool IsCodeFirstOrmEnabled() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace sqlgenCPP
