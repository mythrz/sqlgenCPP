/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <sqlgenCPP/sqlgenCPPTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace sqlgenCPP
{
    class sqlgenCPPRequests
    {
    public:
        AZ_RTTI(sqlgenCPPRequests, sqlgenCPPRequestsTypeId);
        virtual ~sqlgenCPPRequests() = default;

        //! Resolves a relative SQLite database path strictly contained within O3DE's writable
        //! @user@/Database directory.
        //! - ":memory:" is returned unchanged without creating directories.
        //! - Empty paths, absolute paths, and paths escaping @user@/Database via ".." are rejected.
        //! - If AZ::IO::FileIOBase is active and @user@/Database fails to resolve, returns AZ::Failure
        //!   (never silently redirects player saves to OS temporary storage).
        //! - If AZ::IO::FileIOBase is absent (e.g. headless unit tests), returns AZ::Failure unless
        //!   allowTempFallbackWhenNoFileIO is explicitly set to true.
        virtual AZ::Outcome<AZStd::string, AZStd::string> ResolveDatabasePath(
            AZStd::string_view relativeDbPath,
            bool createParentDirectory = true,
            bool allowTempFallbackWhenNoFileIO = false) const = 0;

        //! Returns the resolved default directory used for runtime SQLite databases (@user@/Database),
        //! or an empty string if unavailable (unless allowTempFallbackWhenNoFileIO is true and FileIOBase is absent).
        virtual AZStd::string GetDefaultDatabaseDirectory(
            bool allowTempFallbackWhenNoFileIO = false) const = 0;

        //! Returns true if the Gem was built with the C++26 CodeFirstCppDB_sqlgen ORM enabled.
        virtual bool IsCodeFirstOrmEnabled() const = 0;
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
