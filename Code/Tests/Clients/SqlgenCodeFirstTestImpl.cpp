#include "Clients/SqlgenCodeFirstTestImpl.h"

import std;
import core;
import dal;

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

        struct GameTestDbContext : DAL::DbContextBase<DAL::SQLiteConnection>
        {
            // Intentionally declared before Player/Item to verify topological FK sort
            Set<InventoryEntry> inventory{ this };
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
            if (db.players.count() != 0)
            {
                return Fail("Expected 0 players after schema creation");
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

            if (db.players.count() != 1 || db.items.count() != 1 || db.inventory.count() != 1)
            {
                return Fail("Unexpected seeded row counts");
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

            // Create
            auto created = db.players.add_returning(Player{ .id = 0, .handle = "@mage", .display_name = "Mage", .level = 5 });
            if (!created)
            {
                return Fail("add_returning failed: " + created.error().to_string());
            }

            // Read
            auto found = db.players.try_find(created->id);
            if (!found || !found->has_value() || (*found)->display_name != "Mage")
            {
                return Fail("try_find failed to return created player");
            }

            // Update
            Player updated = **found;
            updated.display_name = "Archmage";
            updated.level = 10;
            if (auto updRes = db.players.update(updated); !updRes)
            {
                return Fail("update failed: " + updRes.error().to_string());
            }
            if (auto saveRes = db.save_changes(); !saveRes)
            {
                return Fail("save_changes after update failed: " + saveRes.error().to_string());
            }

            auto refetched = db.players.try_find(created->id);
            if (!refetched || !refetched->has_value() || (*refetched)->display_name != "Archmage" || (*refetched)->level != 10)
            {
                return Fail("Updated player values were not persisted");
            }

            // Delete
            if (auto remRes = db.players.remove(created->id); !remRes)
            {
                return Fail("remove failed: " + remRes.error().to_string());
            }
            if (auto saveRes = db.save_changes(); !saveRes)
            {
                return Fail("save_changes after remove failed: " + saveRes.error().to_string());
            }

            auto afterDelete = db.players.try_find(created->id);
            if (!afterDelete || afterDelete->has_value())
            {
                return Fail("Deleted player was still found");
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
            if (db.players.count() != 0)
            {
                return Fail("Expected rollback to leave 0 players in database");
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
} // namespace sqlgenCPP::Tests
