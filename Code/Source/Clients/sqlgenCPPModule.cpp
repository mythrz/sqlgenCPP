
#include <sqlgenCPP/sqlgenCPPTypeIds.h>
#include <sqlgenCPPModuleInterface.h>
#include "sqlgenCPPSystemComponent.h"

namespace sqlgenCPP
{
    class sqlgenCPPModule
        : public sqlgenCPPModuleInterface
    {
    public:
        AZ_RTTI(sqlgenCPPModule, sqlgenCPPModuleTypeId, sqlgenCPPModuleInterface);
        AZ_CLASS_ALLOCATOR(sqlgenCPPModule, AZ::SystemAllocator);
    };
}// namespace sqlgenCPP

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), sqlgenCPP::sqlgenCPPModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_sqlgenCPP, sqlgenCPP::sqlgenCPPModule)
#endif
