/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Clients/sqlgenCPPSystemComponent.h>
#include <sqlgenCPP/sqlgenCPPBus.h>

#if defined(SQLGENCPP_ORM_ENABLED) && SQLGENCPP_ORM_ENABLED
#include <Clients/SqlgenCodeFirstTestImpl.h>
#endif

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

namespace
{
    struct ScopedTempDir
    {
        std::filesystem::path dir;

        explicit ScopedTempDir(const char* prefix)
        {
            static std::atomic<uint64_t> counter{ 0 };
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const std::string name = std::string(prefix) + "_" + std::to_string(stamp) + "_" + std::to_string(counter.fetch_add(1));
            dir = std::filesystem::temp_directory_path() / name;
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
        }

        ~ScopedTempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(dir, ec);
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    // Path Service & SystemComponent Tests (Always built — even with ORM OFF)
    ////////////////////////////////////////////////////////////////////////////

    TEST(SqlgenCPPPathServiceTest, ReportsCompileTimeOrmFlagAccurately)
    {
        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

#if defined(SQLGENCPP_ORM_ENABLED) && SQLGENCPP_ORM_ENABLED
        EXPECT_TRUE(requests->IsCodeFirstOrmEnabled());
#else
        EXPECT_FALSE(requests->IsCodeFirstOrmEnabled());
#endif
    }

    TEST(SqlgenCPPPathServiceTest, RejectsEmptyAbsoluteAndTraversalPaths)
    {
        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        EXPECT_FALSE(requests->ResolveDatabasePath("", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("/tmp/sqlgencpp_should_not_exist.db", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("../escape.db", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("sub/../../escape.db", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath(".", false, true).IsSuccess());
    }

    TEST(SqlgenCPPPathServiceTest, AllowsMemoryAndRequiresExplicitTempFallbackWithoutFileIO)
    {
        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        // ":memory:" always succeeds
        const auto memOutcome = requests->ResolveDatabasePath(":memory:", false, false);
        ASSERT_TRUE(memOutcome.IsSuccess()) << memOutcome.GetError().c_str();
        EXPECT_STREQ(memOutcome.GetValue().c_str(), ":memory:");

        // Headless unit test has no AZ::IO::FileIOBase: default (allowTempFallbackWhenNoFileIO=false) must fail
        EXPECT_TRUE(requests->GetDefaultDatabaseDirectory(false).empty());
        EXPECT_FALSE(requests->ResolveDatabasePath("save.db", false, false).IsSuccess());

        // Explicit opt-in temp fallback succeeds and stays strictly inside GetDefaultDatabaseDirectory(true)
        const AZStd::string baseDirStr = requests->GetDefaultDatabaseDirectory(true);
        ASSERT_FALSE(baseDirStr.empty());

        const auto relOutcome = requests->ResolveDatabasePath("UnitTests/slot1.db", false, true);
        ASSERT_TRUE(relOutcome.IsSuccess()) << relOutcome.GetError().c_str();

        const std::filesystem::path baseDir(baseDirStr.c_str());
        const std::filesystem::path resolved(relOutcome.GetValue().c_str());
        const std::filesystem::path rel = resolved.lexically_relative(baseDir);
        EXPECT_FALSE(rel.empty());
        EXPECT_NE(*rel.begin(), "..");
        EXPECT_EQ(rel, std::filesystem::path("UnitTests/slot1.db"));
    }

    ////////////////////////////////////////////////////////////////////////////
    // CodeFirstCppDB_sqlgen C++26 ORM Tests (Enabled when SQLGENCPP_ORM_ENABLED=1)
    ////////////////////////////////////////////////////////////////////////////
#if defined(SQLGENCPP_ORM_ENABLED) && SQLGENCPP_ORM_ENABLED

    TEST(SqlgenCPPCodeFirstTest, SchemaCreation_CreatesTablesAndEnforcesForeignKeys)
    {
        const auto res = sqlgenCPP::Tests::RunSchemaCreationTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, SeedInitialDatabase_PopulatesBaselineGameData)
    {
        const auto res = sqlgenCPP::Tests::RunSeedInitialDatabaseTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, Crud_DeferredUpdateDeleteAndSaveChanges)
    {
        const auto res = sqlgenCPP::Tests::RunCrudLifecycleTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, Transaction_RollbackRestoresStateOnError)
    {
        const auto res = sqlgenCPP::Tests::RunTransactionRollbackTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, Persistence_ReopenSQLiteDatabaseWithVerifiedWalMode)
    {
        ScopedTempDir tempDir("o3de_sqlgencpp_wal");
        const auto dbFile = tempDir.dir / "persist.db";

        const auto res = sqlgenCPP::Tests::RunReopenPersistenceTest(dbFile.string());
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, SystemComponent_ResolvesDatabasePathAndPersists)
    {
        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);
        ASSERT_TRUE(requests->IsCodeFirstOrmEnabled());

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const AZStd::string relSubDir = AZStd::string::format("Tests/run_%lld", static_cast<long long>(stamp));
        const AZStd::string relDbPath = AZStd::string::format("%s/component_test.db", relSubDir.c_str());

        const auto pathOutcome = requests->ResolveDatabasePath(relDbPath, true, true);
        ASSERT_TRUE(pathOutcome.IsSuccess()) << pathOutcome.GetError().c_str();

        const std::filesystem::path dbPath(pathOutcome.GetValue().c_str());
        const std::filesystem::path baseDir(requests->GetDefaultDatabaseDirectory(true).c_str());
        const std::filesystem::path rel = dbPath.lexically_relative(baseDir);
        EXPECT_FALSE(rel.empty());
        EXPECT_NE(*rel.begin(), "..");

        const auto res = sqlgenCPP::Tests::RunReopenPersistenceTest(dbPath.string());

        std::error_code ec;
        std::filesystem::remove_all(dbPath.parent_path(), ec);

        EXPECT_TRUE(res.ok) << res.error;
    }

#endif // SQLGENCPP_ORM_ENABLED
} // namespace

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
