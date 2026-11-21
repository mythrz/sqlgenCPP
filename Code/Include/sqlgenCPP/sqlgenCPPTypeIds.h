
#pragma once

namespace sqlgenCPP
{
    // System Component TypeIds
    inline constexpr const char* sqlgenCPPSystemComponentTypeId = "{CDE9CC35-3E87-4FCF-BCE8-51EFE2A497BF}";
    inline constexpr const char* sqlgenCPPEditorSystemComponentTypeId = "{153FA3B3-B1FF-4E3B-8D52-430AC2C1B11D}";

    // Module derived classes TypeIds
    inline constexpr const char* sqlgenCPPModuleInterfaceTypeId = "{1977DCC9-8DDB-4E7D-AEA0-7D8FCC5215F0}";
    inline constexpr const char* sqlgenCPPModuleTypeId = "{096E588A-2DC7-4EC4-8291-189EF964E53A}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* sqlgenCPPEditorModuleTypeId = sqlgenCPPModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* sqlgenCPPRequestsTypeId = "{390F76EE-2B7B-4BD9-B94C-46F230C97AE8}";
} // namespace sqlgenCPP
