
#pragma once

#include <sqlgenCPP/sqlgenCPPTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace sqlgenCPP
{
    class sqlgenCPPRequests
    {
    public:
        AZ_RTTI(sqlgenCPPRequests, sqlgenCPPRequestsTypeId);
        virtual ~sqlgenCPPRequests() = default;
        // Put your public methods here
    };

    class sqlgenCPPBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using sqlgenCPPRequestBus = AZ::EBus<sqlgenCPPRequests, sqlgenCPPBusTraits>;
    using sqlgenCPPInterface = AZ::Interface<sqlgenCPPRequests>;

} // namespace sqlgenCPP
