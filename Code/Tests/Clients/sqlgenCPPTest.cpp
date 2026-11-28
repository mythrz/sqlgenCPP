/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>

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

    struct ScopedLocalFileIO
    {
        AZ::IO::FileIOBase* previous = nullptr;
        AZ::IO::LocalFileIO localFileIO;

        explicit ScopedLocalFileIO(const char* userAliasPath = nullptr)
        {
            previous = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(&localFileIO);
            if (userAliasPath != nullptr)
            {
                localFileIO.SetAlias("@user@", userAliasPath);
            }
        }

        ~ScopedLocalFileIO()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            if (previous != nullptr)
            {
                AZ::IO::FileIOBase::SetInstance(previous);
            }
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

    TEST(SqlgenCPPPathServiceTest, RejectsEmptyAbsoluteDirectoryAndTraversalPaths)
    {
        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        EXPECT_FALSE(requests->ResolveDatabasePath("", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("/tmp/sqlgencpp_should_not_exist.db", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("../escape.db", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("sub/../../escape.db", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath(".", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("Saves/", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath("Saves/.", false, true).IsSuccess());
        EXPECT_FALSE(requests->ResolveDatabasePath(AZStd::string_view("valid.db\0/../evil", 17), false, true).IsSuccess());
    }

    TEST(SqlgenCPPPathServiceTest, RejectsSymlinkEscapeOutsideUserDatabaseDir)
    {
        ScopedTempDir userRoot("o3de_sqlgencpp_sym_user");
        ScopedTempDir outsideDir("o3de_sqlgencpp_sym_outside");
        ScopedLocalFileIO fileIO(userRoot.dir.string().c_str());

        std::error_code ec;
        const auto dbDir = userRoot.dir / "Database";
        std::filesystem::create_directories(dbDir, ec);
        ASSERT_FALSE(ec);

        const auto escapeLink = dbDir / "escape_link";
        std::filesystem::create_directory_symlink(outsideDir.dir, escapeLink, ec);
        ASSERT_FALSE(ec);

        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        EXPECT_FALSE(requests->ResolveDatabasePath("escape_link/evil.db", true, false).IsSuccess());
        EXPECT_FALSE(std::filesystem::exists(outsideDir.dir / "evil.db"));
    }

    TEST(SqlgenCPPPathServiceTest, RejectsExistingDirectoryWithoutTrailingSlash)
    {
        ScopedTempDir userRoot("o3de_sqlgencpp_existing_dir");
        ScopedLocalFileIO fileIO(userRoot.dir.string().c_str());

        std::error_code ec;
        const auto savesDir = userRoot.dir / "Database" / "Saves";
        std::filesystem::create_directories(savesDir, ec);
        ASSERT_FALSE(ec);

        const auto savesLink = userRoot.dir / "Database" / "SavesLink";
        std::filesystem::create_directory_symlink(savesDir, savesLink, ec);
        ASSERT_FALSE(ec);

        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        const auto directOutcome = requests->ResolveDatabasePath("Saves", false, false);
        EXPECT_FALSE(directOutcome.IsSuccess());

        const auto createParentOutcome = requests->ResolveDatabasePath("Saves", true, false);
        EXPECT_FALSE(createParentOutcome.IsSuccess());

        const auto symlinkDirOutcome = requests->ResolveDatabasePath("SavesLink", false, false);
        EXPECT_FALSE(symlinkDirOutcome.IsSuccess());
    }

    TEST(SqlgenCPPPathServiceTest, EBusBroadcastsWhenComponentActivated)
    {
        struct TestableSystemComponent : sqlgenCPP::sqlgenCPPSystemComponent
        {
            using sqlgenCPPSystemComponent::Activate;
            using sqlgenCPPSystemComponent::Deactivate;
        };

        ScopedTempDir userRoot("o3de_sqlgencpp_ebus");
        ScopedLocalFileIO fileIO(userRoot.dir.string().c_str());

        TestableSystemComponent component;
        component.Activate();

        AZ::Outcome<AZStd::string, AZStd::string> outcome = AZ::Failure(AZStd::string("Not handled"));
        sqlgenCPP::sqlgenCPPRequestBus::BroadcastResult(
            outcome,
            &sqlgenCPP::sqlgenCPPRequests::ResolveDatabasePath,
            AZStd::string_view("Saves/ebus_test.db"),
            true,
            false);

        component.Deactivate();

        ASSERT_TRUE(outcome.IsSuccess()) << outcome.GetError().c_str();
        const std::filesystem::path expected =
            std::filesystem::weakly_canonical(userRoot.dir / "Database" / "Saves" / "ebus_test.db");
        EXPECT_EQ(std::filesystem::path(outcome.GetValue().c_str()), expected);
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

    TEST(SqlgenCPPPathServiceTest, ResolvesUserAliasWhenFileIOIsActive)
    {
        ScopedTempDir userRoot("o3de_sqlgencpp_user_alias");
        ScopedLocalFileIO fileIO(userRoot.dir.string().c_str());

        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        const std::filesystem::path expectedBase =
            std::filesystem::weakly_canonical(userRoot.dir / "Database");

        const AZStd::string defaultDir = requests->GetDefaultDatabaseDirectory(false);
        ASSERT_FALSE(defaultDir.empty());
        EXPECT_EQ(std::filesystem::path(defaultDir.c_str()), expectedBase);

        const auto saveOutcome = requests->ResolveDatabasePath("Saves/slot1.db", true, false);
        ASSERT_TRUE(saveOutcome.IsSuccess()) << saveOutcome.GetError().c_str();
        const std::filesystem::path resolved(saveOutcome.GetValue().c_str());
        EXPECT_EQ(resolved, expectedBase / "Saves" / "slot1.db");
        EXPECT_TRUE(std::filesystem::exists(resolved.parent_path()));

        // Containment still holds under live FileIO
        EXPECT_FALSE(requests->ResolveDatabasePath("../outside.db", true, false).IsSuccess());
    }

    TEST(SqlgenCPPPathServiceTest, NeverFallsBackToTempWhenLiveFileIOUnsetUserAlias)
    {
        ScopedLocalFileIO fileIO(nullptr);

        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);

        // Even with allowTempFallbackWhenNoFileIO=true, a live FileIO with unset @user@ MUST fail explicitly
        EXPECT_TRUE(requests->GetDefaultDatabaseDirectory(true).empty());
        EXPECT_FALSE(requests->ResolveDatabasePath("save.db", false, true).IsSuccess());
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

    TEST(SqlgenCPPCodeFirstTest, TypedRead_PreservesEmbeddedNulBytes)
    {
        const auto res = sqlgenCPP::Tests::RunEmbeddedNulTextRoundTripTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, BatchUpsert_RollsBackEntireBatchOnConstraintError)
    {
        const auto res = sqlgenCPP::Tests::RunBatchUpsertAtomicityTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, CompositeFkUpsert_SingleAndBatchUpdateExistingRows)
    {
        const auto res = sqlgenCPP::Tests::RunCompositeFkUpsertTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, SchemaMigration_UpgradesUserVersionTransactionally)
    {
        ScopedTempDir tempDir("o3de_sqlgencpp_migrate");
        const auto dbFile = tempDir.dir / "migrated.db";

        const auto res = sqlgenCPP::Tests::RunSchemaMigrationTest(dbFile.string());
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, SystemComponent_ResolvesDatabasePathAndPersists)
    {
        ScopedTempDir userRoot("o3de_sqlgencpp_comp_persist");
        ScopedLocalFileIO fileIO(userRoot.dir.string().c_str());

        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);
        ASSERT_TRUE(requests->IsCodeFirstOrmEnabled());

        const auto pathOutcome = requests->ResolveDatabasePath("Saves/component_test.db", true, false);
        ASSERT_TRUE(pathOutcome.IsSuccess()) << pathOutcome.GetError().c_str();

        const std::filesystem::path dbPath(pathOutcome.GetValue().c_str());
        const std::filesystem::path expectedDir =
            std::filesystem::weakly_canonical(userRoot.dir / "Database" / "Saves");
        EXPECT_EQ(dbPath.parent_path(), expectedDir);

        const auto res = sqlgenCPP::Tests::RunReopenPersistenceTest(dbPath.string());
        EXPECT_TRUE(res.ok) << res.error;
    }

#endif // SQLGENCPP_ORM_ENABLED
} // namespace

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
