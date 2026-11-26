/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace MyGame::Persistence
{
    // Plain C++20 data transfer object (safe to include in O3DE -fno-exceptions translation units).
    struct PlayerSaveProfile
    {
        std::int32_t slotId = 1;
        std::string playerName;
        std::int32_t level = 1;
        std::int64_t gold = 0;
    };

    struct SaveStatus
    {
        bool ok = false;
        std::string error;
    };

    struct LoadProfileResult
    {
        bool ok = false;
        std::optional<PlayerSaveProfile> profile;
        std::string error;
    };

    // Writes (upserts) a player save profile to `dbPath`, then closes the connection.
    // All C++ exceptions are caught inside the C++26 implementation before returning.
    SaveStatus SavePlayerProfile(const std::string& dbPath, const PlayerSaveProfile& profile) noexcept;

    // Reopens `dbPath` and reads back the profile for `slotId` (or std::nullopt if absent).
    // All C++ exceptions are caught inside the C++26 implementation before returning.
    LoadProfileResult LoadPlayerProfile(const std::string& dbPath, std::int32_t slotId) noexcept;

    // Returns true when built against Gem::sqlgenCPP.ORM (false for the optional-consumer stub).
    bool IsOrmBackendAvailable() noexcept;
} // namespace MyGame::Persistence
