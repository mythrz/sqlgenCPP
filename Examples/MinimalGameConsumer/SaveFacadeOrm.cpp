/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Compiled in a dedicated C++26 static sub-target (`SCAN_FOR_MODULES`, `CXX_STANDARD 26`)
// linked privately to `Gem::sqlgenCPP.ORM`. Never `#include <AzCore/...>` in this TU.

#include "SaveFacade.h"

import std;
import codefirst.core;
import codefirst.dal;

namespace MyGame::Persistence
{
    namespace
    {
        struct [[=DAL::Schema::table{ std::define_static_string("PlayerSaves") }]] PlayerSaveRow
        {
            [[=DAL::Schema::primary_key{ false }]] std::int32_t slot_id = 1;
            std::string player_name;
            std::int32_t level = 1;
            std::int64_t gold = 0;
        };

        struct SaveDbContext : DAL::DbContextBase<DAL::SQLiteConnection>
        {
            Set<PlayerSaveRow> saves{ this };

            explicit SaveDbContext(std::shared_ptr<DAL::SQLiteConnection> conn)
                : DAL::DbContextBase<DAL::SQLiteConnection>(std::move(conn))
            {
            }
        };

        DAL::SQLiteConnectionOptions MakeSaveConnectionOptions()
        {
            DAL::SQLiteConnectionOptions opts;
            opts.foreign_keys = true;
            opts.wal_mode = true;
            opts.busy_timeout = std::chrono::milliseconds{ 5000 };
            return opts;
        }

        std::optional<std::string> VerifyWalModeIfDisk(SaveDbContext& db, const std::string& dbPath)
        {
            if (dbPath == ":memory:")
            {
                return std::nullopt;
            }
            auto walCheck = db.query_rows("PRAGMA journal_mode;");
            if (!walCheck || walCheck->rows.empty() || walCheck->rows[0].cells.empty())
            {
                return "Failed to query PRAGMA journal_mode";
            }
            const auto* modeStr = std::get_if<std::string>(&walCheck->rows[0].cells[0]);
            if (!modeStr)
            {
                return "Unexpected non-string result from PRAGMA journal_mode";
            }
            std::string modeLower = *modeStr;
            std::ranges::transform(modeLower, modeLower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (modeLower != "wal")
            {
                return "Expected PRAGMA journal_mode='wal', got '" + modeLower + "'";
            }
            return std::nullopt;
        }
    } // namespace

    SaveStatus SavePlayerProfile(const std::string& dbPath, const PlayerSaveProfile& profile) noexcept
    {
        try
        {
            auto conn = DAL::SQLiteConnection::connect(dbPath, MakeSaveConnectionOptions());
            if (!conn)
            {
                return SaveStatus{ false, "SQLite connect failed: " + conn.error().to_string() };
            }

            SaveDbContext db(*conn);
            if (auto walErr = VerifyWalModeIfDisk(db, dbPath); walErr.has_value())
            {
                return SaveStatus{ false, *walErr };
            }
            if (auto schemaRes = db.ensure_created(); !schemaRes)
            {
                return SaveStatus{ false, "Schema creation failed: " + schemaRes.error().to_string() };
            }

            PlayerSaveRow row{
                .slot_id = profile.slotId,
                .player_name = profile.playerName,
                .level = profile.level,
                .gold = profile.gold
            };

            if (auto upsertRes = db.saves.upsert(row); !upsertRes)
            {
                return SaveStatus{ false, "Save upsert failed: " + upsertRes.error().to_string() };
            }

            return SaveStatus{ true, {} };
        }
        catch (const std::exception& ex)
        {
            return SaveStatus{ false, std::string("Unhandled exception in SavePlayerProfile: ") + ex.what() };
        }
        catch (...)
        {
            return SaveStatus{ false, "Unhandled unknown exception in SavePlayerProfile" };
        }
    }

    LoadProfileResult LoadPlayerProfile(const std::string& dbPath, std::int32_t slotId) noexcept
    {
        try
        {
            if (dbPath != ":memory:")
            {
                std::error_code ec;
                const bool exists = std::filesystem::exists(dbPath, ec);
                if (ec)
                {
                    return LoadProfileResult{ false, std::nullopt, "Failed to inspect save file: " + ec.message() };
                }
                if (!exists)
                {
                    // Avoid creating an empty .db file on disk during a read of a missing save slot.
                    return LoadProfileResult{ true, std::nullopt, {} };
                }
            }

            auto conn = DAL::SQLiteConnection::connect(dbPath, MakeSaveConnectionOptions());
            if (!conn)
            {
                return LoadProfileResult{ false, std::nullopt, "SQLite connect failed: " + conn.error().to_string() };
            }

            SaveDbContext db(*conn);
            if (auto schemaRes = db.ensure_created(); !schemaRes)
            {
                return LoadProfileResult{ false, std::nullopt, "Schema creation failed: " + schemaRes.error().to_string() };
            }

            auto found = db.saves.try_find(slotId);
            if (!found)
            {
                return LoadProfileResult{ false, std::nullopt, "Save lookup failed: " + found.error().to_string() };
            }

            if (!found->has_value())
            {
                return LoadProfileResult{ true, std::nullopt, {} };
            }

            const PlayerSaveRow& row = **found;
            return LoadProfileResult{
                true,
                PlayerSaveProfile{
                    .slotId = row.slot_id,
                    .playerName = row.player_name,
                    .level = row.level,
                    .gold = row.gold
                },
                {}
            };
        }
        catch (const std::exception& ex)
        {
            return LoadProfileResult{ false, std::nullopt, std::string("Unhandled exception in LoadPlayerProfile: ") + ex.what() };
        }
        catch (...)
        {
            return LoadProfileResult{ false, std::nullopt, "Unhandled unknown exception in LoadPlayerProfile" };
        }
    }

    bool IsOrmBackendAvailable() noexcept
    {
        return true;
    }
} // namespace MyGame::Persistence
