/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Clients/SqlgenCodeFirstTestImpl.h"

import std;
import codefirst.core;
import codefirst.dal;

namespace sqlgenCPP::Tests
{
    namespace
    {
        struct [[=DAL::Schema::table{ std::define_static_string("Players") }]] Player
        {
            [[=DAL::Schema::primary_key{ true }]] std::int32_t id = 0;
            [[=DAL::Schema::unique{}]] std::string handle;
            std::string display_name;
            std::int32_t level = 1;
        };

        struct [[=DAL::Schema::table{ std::define_static_string("Items") }]] Item
        {
            [[=DAL::Schema::primary_key{ true }]] std::int32_t id = 0;
            [[=DAL::Schema::unique{}]] std::string code;
            std::string name;
        };

        struct [[=DAL::Schema::table{ std::define_static_string("Inventory") }]] InventoryEntry
        {
            [[=DAL::Schema::primary_key{ true }]] std::int32_t id = 0;
            [[=DAL::Schema::references<Player>{ std::define_static_string("id") }]] std::int32_t player_id = 0;
            [[=DAL::Schema::references<Item>{ std::define_static_string("id") }]] std::int32_t item_id = 0;
            std::int32_t quantity = 1;
        };

        struct [[=DAL::Schema::table{ std::define_static_string("PlayerItemOwnership") }]] PlayerItemOwnership
        {
            [[=DAL::Schema::primary_key{ false }]]
            [[=DAL::Schema::references<Player>{ std::define_static_string("id") }]]
            std::int32_t player_id = 0;

            [[=DAL::Schema::primary_key{ false }]]
            [[=DAL::Schema::references<Item>{ std::define_static_string("id") }]]
            std::int32_t item_id = 0;

            std::int32_t quantity = 1;
        };

        struct GameTestDbContext : DAL::DbContextBase<DAL::SQLiteConnection>
        {
            // Declared before Player/Item to exercise topological FK table creation order
            Set<InventoryEntry> inventory{ this };
            Set<PlayerItemOwnership> ownership{ this };
            Set<Player> players{ this };
            Set<Item> items{ this };

            explicit GameTestDbContext(std::shared_ptr<DAL::SQLiteConnection> conn)
                : DAL::DbContextBase<DAL::SQLiteConnection>(std::move(conn))
            {
            }
        };

        DbTestOutcome Ok()
        {
            return DbTestOutcome{ true, {} };
        }

        DbTestOutcome Fail(std::string msg)
        {
            return DbTestOutcome{ false, std::move(msg) };
        }

        template <typename DbSetT>
        std::expected<void, std::string> ExpectCount(const DbSetT& set, std::size_t expected, std::string_view label)
        {
            const auto res = set.try_count();
            if (!res)
            {
                return std::unexpected(std::string(label) + " try_count failed: " + res.error().to_string());
            }
            if (*res != expected)
            {
                return std::unexpected(
                    std::string(label) + " expected count " + std::to_string(expected) +
                    ", got " + std::to_string(*res));
            }
            return {};
        }

        template <typename Fn>
        DbTestOutcome RunGuarded(Fn&& fn)
        {
            try
            {
                return fn();
            }
            catch (const std::exception& ex)
            {
                return Fail(std::string("Unhandled std::exception: ") + ex.what());
            }
            catch (...)
            {
                return Fail("Unhandled unknown exception");
            }
        }
    } // namespace

    DbTestOutcome RunSchemaCreationTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            GameTestDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            if (auto chk = ExpectCount(db.players, 0, "players"); !chk)
            {
                return Fail(chk.error());
            }
            if (auto chk = ExpectCount(db.items, 0, "items"); !chk)
            {
                return Fail(chk.error());
            }
            if (auto chk = ExpectCount(db.inventory, 0, "inventory"); !chk)
            {
                return Fail(chk.error());
            }

            // Verify foreign-key enforcement rejects an orphan InventoryEntry
            auto orphan = db.inventory.add_returning(
                InventoryEntry{ .id = 0, .player_id = 9999, .item_id = 9999, .quantity = 1 });
            if (orphan.has_value())
            {
                return Fail("Expected foreign-key constraint violation when inserting orphan InventoryEntry");
            }
            return Ok();
        });
    }

    DbTestOutcome RunSeedInitialDatabaseTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            GameTestDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            auto p1 = db.players.add_returning(Player{ .id = 0, .handle = "@hero", .display_name = "Hero", .level = 1 });
            if (!p1 || p1->id <= 0)
            {
                return Fail("add_returning(Player) failed");
            }

            auto sword = db.items.add_returning(Item{ .id = 0, .code = "SWD_01", .name = "Iron Sword" });
            if (!sword || sword->id <= 0)
            {
                return Fail("add_returning(Item) failed");
            }

            auto inv = db.inventory.add_returning(
                InventoryEntry{ .id = 0, .player_id = p1->id, .item_id = sword->id, .quantity = 1 });
            if (!inv || inv->id <= 0)
            {
                return Fail("add_returning(InventoryEntry) failed");
            }

            if (auto chk = ExpectCount(db.players, 1, "players"); !chk)
            {
                return Fail(chk.error());
            }
            if (auto chk = ExpectCount(db.items, 1, "items"); !chk)
            {
                return Fail(chk.error());
            }
            if (auto chk = ExpectCount(db.inventory, 1, "inventory"); !chk)
            {
                return Fail(chk.error());
            }
            return Ok();
        });
    }

    DbTestOutcome RunCrudLifecycleTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            GameTestDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            // Create (immediate add_returning to retrieve generated ID)
            auto created = db.players.add_returning(Player{ .id = 0, .handle = "@mage", .display_name = "Mage", .level = 5 });
            if (!created)
            {
                return Fail("add_returning failed: " + created.error().to_string());
            }

            // Read
            auto found = db.players.try_find(created->id);
            if (!found || !found->has_value() || (*found)->display_name != "Mage" || (*found)->level != 5)
            {
                return Fail("try_find failed to return created player");
            }

            // Enable deferred Unit-of-Work mode so save_changes() actually flushes staged writes
            db.defer_changes(true);

            // Deferred Update: verify NOT persisted before save_changes(), IS persisted after
            Player updated = **found;
            updated.display_name = "Archmage";
            updated.level = 10;
            if (auto updRes = db.players.update(updated); !updRes)
            {
                return Fail("deferred update staging failed: " + updRes.error().to_string());
            }

            auto beforeSave = db.players.try_find(created->id);
            if (!beforeSave || !beforeSave->has_value() || (*beforeSave)->level != 5)
            {
                return Fail("Deferred update became visible in SQLite before save_changes()");
            }

            if (auto saveRes = db.save_changes(); !saveRes)
            {
                return Fail("save_changes after update failed: " + saveRes.error().to_string());
            }

            auto refetched = db.players.try_find(created->id);
            if (!refetched || !refetched->has_value() || (*refetched)->display_name != "Archmage" || (*refetched)->level != 10)
            {
                return Fail("Updated player values were not persisted by save_changes()");
            }

            // Deferred Delete: verify row still exists before save_changes(), gone after
            if (auto remRes = db.players.remove(created->id); !remRes)
            {
                return Fail("deferred remove staging failed: " + remRes.error().to_string());
            }

            auto beforeDeleteSave = db.players.try_find(created->id);
            if (!beforeDeleteSave || !beforeDeleteSave->has_value())
            {
                return Fail("Deferred remove deleted row before save_changes()");
            }

            if (auto saveRes = db.save_changes(); !saveRes)
            {
                return Fail("save_changes after remove failed: " + saveRes.error().to_string());
            }

            auto afterDelete = db.players.try_find(created->id);
            if (!afterDelete || afterDelete->has_value())
            {
                return Fail("Deleted player was still found after save_changes()");
            }
            return Ok();
        });
    }

    DbTestOutcome RunTransactionRollbackTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            GameTestDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            auto txRes = db.transaction([&]() -> std::expected<void, Core::DbError> {
                auto first = db.players.add_returning(Player{ .id = 0, .handle = "@dup", .display_name = "First", .level = 1 });
                if (!first)
                {
                    return std::unexpected(first.error());
                }
                // Trigger unique constraint failure to force rollback
                auto second = db.players.add_returning(Player{ .id = 0, .handle = "@dup", .display_name = "Second", .level = 2 });
                if (!second)
                {
                    return std::unexpected(second.error());
                }
                return {};
            });

            if (txRes.has_value())
            {
                return Fail("Expected transaction to fail on duplicate unique handle");
            }
            if (auto chk = ExpectCount(db.players, 0, "players after rollback"); !chk)
            {
                return Fail(chk.error());
            }
            return Ok();
        });
    }

    DbTestOutcome RunReopenPersistenceTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            std::int32_t savedId = 0;
            {
                DAL::SQLiteConnectionOptions opts;
                opts.foreign_keys = true;
                opts.wal_mode = true;
                opts.busy_timeout = std::chrono::milliseconds{ 5000 };

                auto conn = DAL::SQLiteConnection::connect(dbPath, opts);
                if (!conn)
                {
                    return Fail("connect (write) failed: " + conn.error().to_string());
                }
                GameTestDbContext db(*conn);

                // Verify SQLite actually activated WAL journal mode on the disk database
                auto walCheck = db.query_rows("PRAGMA journal_mode;");
                if (!walCheck || walCheck->rows.empty() || walCheck->rows[0].cells.empty())
                {
                    return Fail("Failed to query PRAGMA journal_mode");
                }
                const auto* modeStr = std::get_if<std::string>(&walCheck->rows[0].cells[0]);
                if (!modeStr)
                {
                    return Fail("Unexpected non-string result from PRAGMA journal_mode");
                }
                std::string modeLower = *modeStr;
                std::ranges::transform(modeLower, modeLower.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                if (modeLower != "wal")
                {
                    return Fail("Expected effective PRAGMA journal_mode='wal', got '" + modeLower + "'");
                }

                if (auto res = db.ensure_created(); !res)
                {
                    return Fail("ensure_created failed: " + res.error().to_string());
                }
                auto created = db.players.add_returning(Player{ .id = 0, .handle = "@persist", .display_name = "Persisted", .level = 42 });
                if (!created)
                {
                    return Fail("add_returning failed: " + created.error().to_string());
                }
                savedId = created->id;
                (*conn)->close();
            }

            {
                auto conn = DAL::SQLiteConnection::connect(dbPath);
                if (!conn)
                {
                    return Fail("connect (reopen) failed: " + conn.error().to_string());
                }
                GameTestDbContext db(*conn);
                auto found = db.players.try_find(savedId);
                if (!found || !found->has_value() || (*found)->display_name != "Persisted" || (*found)->level != 42)
                {
                    return Fail("Persisted player not found after reopening SQLite database");
                }
            }
            return Ok();
        });
    }

    DbTestOutcome RunEmbeddedNulTextRoundTripTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            GameTestDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            const std::string payload("A\0B", 3);
            auto created = db.players.add_returning(
                Player{ .id = 0, .handle = "@nul_test", .display_name = payload, .level = 7 });
            if (!created)
            {
                return Fail("add_returning with embedded NUL failed: " + created.error().to_string());
            }

            auto found = db.players.try_find(created->id);
            if (!found || !found->has_value())
            {
                return Fail("try_find failed for embedded NUL row");
            }
            if ((*found)->display_name.size() != 3 || (*found)->display_name != payload)
            {
                return Fail(
                    "Embedded NUL TEXT was truncated: expected 3 bytes, got " +
                    std::to_string((*found)->display_name.size()));
            }
            return Ok();
        });
    }

    DbTestOutcome RunBatchUpsertAtomicityTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            struct [[=DAL::Schema::table{ std::define_static_string("ManualCatalog") }]] ManualItem
            {
                [[=DAL::Schema::primary_key{ false }]] std::int32_t id = 0;
                [[=DAL::Schema::unique{}]] std::string code;
                std::string name;
            };

            struct CatalogDbContext : DAL::DbContextBase<DAL::SQLiteConnection>
            {
                Set<ManualItem> catalog{ this };
                explicit CatalogDbContext(std::shared_ptr<DAL::SQLiteConnection> c)
                    : DAL::DbContextBase<DAL::SQLiteConnection>(std::move(c))
                {
                }
            };

            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            CatalogDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            // Two-row batch where row 2 violates the UNIQUE constraint on 'code'
            const std::vector<ManualItem> batch = {
                ManualItem{ .id = 1, .code = "DUP_CODE", .name = "First" },
                ManualItem{ .id = 2, .code = "DUP_CODE", .name = "Second" }
            };

            auto upsertRes = db.catalog.upsert_range(batch);
            if (upsertRes.has_value())
            {
                return Fail("Expected upsert_range to fail on duplicate unique code");
            }

            if (auto chk = ExpectCount(db.catalog, 0, "catalog after failed upsert_range"); !chk)
            {
                return Fail(chk.error());
            }
            return Ok();
        });
    }

    DbTestOutcome RunSchemaMigrationTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            {
                auto conn = DAL::SQLiteConnection::connect(dbPath);
                if (!conn)
                {
                    return Fail("connect (migrate) failed: " + conn.error().to_string());
                }
                GameTestDbContext db(*conn);

                auto initialVer = db.try_user_version();
                if (!initialVer || *initialVer != 0)
                {
                    return Fail("Expected initial PRAGMA user_version == 0");
                }

                auto migRes = db.migrate_to(2, [&](std::int32_t fromVer, std::int32_t toVer) -> std::expected<void, Core::DbError> {
                    if (fromVer != 0 || toVer != 2)
                    {
                        return std::unexpected(Core::DbError::unknown("Unexpected migration version arguments"));
                    }
                    return (*conn)->execute_prepared("ALTER TABLE Players ADD COLUMN bio TEXT NOT NULL DEFAULT '';");
                });
                if (!migRes)
                {
                    return Fail("migrate_to(2) failed: " + migRes.error().to_string());
                }

                auto verAfter = db.try_user_version();
                if (!verAfter || *verAfter != 2)
                {
                    return Fail("Expected PRAGMA user_version == 2 after migration");
                }
                (*conn)->close();
            }

            // Reopen from disk and verify schema version & migrated column persisted
            {
                auto conn = DAL::SQLiteConnection::connect(dbPath);
                if (!conn)
                {
                    return Fail("connect (verify) failed: " + conn.error().to_string());
                }
                GameTestDbContext db(*conn);

                auto ver = db.try_user_version();
                if (!ver || *ver != 2)
                {
                    return Fail("Expected persisted PRAGMA user_version == 2 on reopen");
                }
                auto colCheck = db.query_rows("SELECT bio FROM Players LIMIT 1;");
                if (!colCheck)
                {
                    return Fail("Migrated column 'bio' not queryable after reopen: " + colCheck.error().to_string());
                }
            }
            return Ok();
        });
    }

    DbTestOutcome RunCompositeFkUpsertTest(const std::string& dbPath)
    {
        return RunGuarded([&]() -> DbTestOutcome {
            auto conn = DAL::SQLiteConnection::connect(dbPath);
            if (!conn)
            {
                return Fail("connect failed: " + conn.error().to_string());
            }
            GameTestDbContext db(*conn);
            if (auto res = db.ensure_created(); !res)
            {
                return Fail("ensure_created failed: " + res.error().to_string());
            }

            auto p1 = db.players.add_returning(Player{ .id = 0, .handle = "@p1", .display_name = "Player 1", .level = 1 });
            auto p2 = db.players.add_returning(Player{ .id = 0, .handle = "@p2", .display_name = "Player 2", .level = 2 });
            auto i1 = db.items.add_returning(Item{ .id = 0, .code = "ITM_1", .name = "Potion" });
            auto i2 = db.items.add_returning(Item{ .id = 0, .code = "ITM_2", .name = "Elixir" });
            if (!p1 || !p2 || !i1 || !i2)
            {
                return Fail("Failed to seed parent rows for composite-FK upsert test");
            }

            // 1. Single-row upsert insert + conflict update on composite (PK + FK) key
            if (auto r = db.ownership.upsert(PlayerItemOwnership{ .player_id = p1->id, .item_id = i1->id, .quantity = 3 }); !r)
            {
                return Fail("Initial composite-FK upsert failed: " + r.error().to_string());
            }
            if (auto r = db.ownership.upsert(PlayerItemOwnership{ .player_id = p1->id, .item_id = i1->id, .quantity = 10 }); !r)
            {
                return Fail("Conflict-update composite-FK upsert failed: " + r.error().to_string());
            }
            if (auto chk = ExpectCount(db.ownership, 1, "ownership after single upsert"); !chk)
            {
                return Fail(chk.error());
            }
            const auto foundSingle = db.ownership.try_find(Core::CompositeKey<std::int32_t, std::int32_t>{ p1->id, i1->id });
            if (!foundSingle || !foundSingle->has_value() || (*foundSingle)->quantity != 10)
            {
                return Fail("Expected updated quantity == 10 after single composite-FK upsert");
            }

            // 2. Batch upsert_range mixing update of existing key + inserts of new composite keys
            std::vector<PlayerItemOwnership> batch{
                PlayerItemOwnership{ .player_id = p1->id, .item_id = i1->id, .quantity = 25 },
                PlayerItemOwnership{ .player_id = p1->id, .item_id = i2->id, .quantity = 4 },
                PlayerItemOwnership{ .player_id = p2->id, .item_id = i1->id, .quantity = 7 },
            };
            if (auto r = db.ownership.upsert_range(batch); !r)
            {
                return Fail("Batch composite-FK upsert_range failed: " + r.error().to_string());
            }
            if (auto chk = ExpectCount(db.ownership, 3, "ownership after batch upsert"); !chk)
            {
                return Fail(chk.error());
            }

            const auto checkRow = [&](std::int32_t pid, std::int32_t iid, std::int32_t expectedQty) -> bool {
                const auto f = db.ownership.try_find(Core::CompositeKey<std::int32_t, std::int32_t>{ pid, iid });
                return f && f->has_value() && (*f)->quantity == expectedQty;
            };
            if (!checkRow(p1->id, i1->id, 25) || !checkRow(p1->id, i2->id, 4) || !checkRow(p2->id, i1->id, 7))
            {
                return Fail("Batch composite-FK upsert_range did not persist expected quantities");
            }

            // 3. Batch upsert_range atomicity: FK violation on second row must roll back first row's update
            std::vector<PlayerItemOwnership> badBatch{
                PlayerItemOwnership{ .player_id = p1->id, .item_id = i1->id, .quantity = 99 },
                PlayerItemOwnership{ .player_id = 99999, .item_id = i1->id, .quantity = 1 },
            };
            if (auto r = db.ownership.upsert_range(badBatch); r.has_value())
            {
                return Fail("Expected batch composite-FK upsert_range to fail on invalid foreign key");
            }
            if (!checkRow(p1->id, i1->id, 25))
            {
                return Fail("Failed batch composite-FK upsert_range leaked partial update (expected quantity == 25)");
            }

            return Ok();
        });
    }
} // namespace sqlgenCPP::Tests
