/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Compiled in ordinary C++20 (-fno-exceptions) builds for optional consumers
// when `Gem::sqlgenCPP.ORM` is unavailable (e.g., `SQLGENCPP_ORM_MODE=OFF`
// or a toolchain that does not support C++26 reflection + modules).

#include "SaveFacade.h"

namespace MyGame::Persistence
{
    SaveStatus SavePlayerProfile(const std::string& /*dbPath*/, const PlayerSaveProfile& /*profile*/) noexcept
    {
        return SaveStatus{
            false,
            "SQLite ORM backend is disabled in this build (configure with -DSQLGENCPP_ORM_MODE=REQUIRED on a supported toolchain)."
        };
    }

    LoadProfileResult LoadPlayerProfile(const std::string& /*dbPath*/, std::int32_t /*slotId*/) noexcept
    {
        return LoadProfileResult{
            false,
            std::nullopt,
            "SQLite ORM backend is disabled in this build (configure with -DSQLGENCPP_ORM_MODE=REQUIRED on a supported toolchain)."
        };
    }

    bool IsOrmBackendAvailable() noexcept
    {
        return false;
    }
} // namespace MyGame::Persistence
