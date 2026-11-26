/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Example O3DE C++20 (-fno-exceptions) caller that resolves a save path through
// `sqlgenCPP::sqlgenCPPInterface`, crosses the C++20 <-> C++26 façade boundary,
// writes a save, closes the connection, and reopens it to verify persistence.
//
// In a real game, invoke `SavePlayerProfile` / `LoadPlayerProfile` from a dedicated
// database worker thread and marshal results back via `AZ::TickBus::QueueFunction`.

#include "SaveFacade.h"

#include <AzCore/Console/ILogger.h>
#include <AzCore/std/string/string.h>
#include <sqlgenCPP/sqlgenCPPBus.h>

#include <string>

namespace MyGame
{
    bool SaveAndReloadProfileRoundTrip(const Persistence::PlayerSaveProfile& profile)
    {
        auto* dbService = sqlgenCPP::sqlgenCPPInterface::Get();
        if (dbService == nullptr)
        {
            AZLOG_ERROR("MyGame", "sqlgenCPPInterface is not registered. Is the sqlgenCPP Gem enabled?");
            return false;
        }

        if (!Persistence::IsOrmBackendAvailable() || !dbService->IsCodeFirstOrmEnabled())
        {
            AZLOG_WARN("MyGame", "CodeFirst ORM backend is disabled in this build; skipping SQLite save.");
            return false;
        }

        // Resolve strictly under @user@/Database, creating parent folders if needed,
        // and NEVER silently falling back to OS temp storage for player saves.
        const auto pathOutcome = dbService->ResolveDatabasePath(
            "Saves/profile.db",
            /*createParentDirectory=*/true,
            /*allowTempFallbackWhenNoFileIO=*/false);
        if (!pathOutcome.IsSuccess())
        {
            // Explicitly surface errors such as unset @user@ alias, invalid relative path,
            // or a pre-existing directory passed as a filename.
            AZLOG_ERROR("MyGame", "Failed to resolve save database path: %s", pathOutcome.GetError().c_str());
            return false;
        }

        const std::string dbPath(pathOutcome.GetValue().c_str(), pathOutcome.GetValue().size());

        // 1. Open, upsert, and close the connection inside SavePlayerProfile.
        const auto saveRes = Persistence::SavePlayerProfile(dbPath, profile);
        if (!saveRes.ok)
        {
            AZLOG_ERROR("MyGame", "Failed to save player profile: %s", saveRes.error.c_str());
            return false;
        }

        // 2. Reopen the SQLite file from disk and read the saved row back.
        const auto loadRes = Persistence::LoadPlayerProfile(dbPath, profile.slotId);
        if (!loadRes.ok || !loadRes.profile.has_value())
        {
            AZLOG_ERROR("MyGame", "Failed to reload player profile: %s", loadRes.error.c_str());
            return false;
        }

        return loadRes.profile->playerName == profile.playerName
            && loadRes.profile->level == profile.level
            && loadRes.profile->gold == profile.gold;
    }
} // namespace MyGame
