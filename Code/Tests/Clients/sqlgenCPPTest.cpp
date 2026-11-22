/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Clients/SqlgenCodeFirstTestImpl.h>
#include <Clients/sqlgenCPPSystemComponent.h>
#include <sqlgenCPP/sqlgenCPPBus.h>

#include <filesystem>

namespace
{
    TEST(SqlgenCPPCodeFirstTest, SchemaCreation_CreatesTablesAndForeignKeys)
    {
        const auto res = sqlgenCPP::Tests::RunSchemaCreationTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, SeedInitialDatabase_PopulatesBaselineGameData)
    {
        const auto res = sqlgenCPP::Tests::RunSeedInitialDatabaseTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, Crud_AddEditDeleteAndSaveChanges)
    {
        const auto res = sqlgenCPP::Tests::RunCrudLifecycleTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, Transaction_RollbackRestoresStateOnError)
    {
        const auto res = sqlgenCPP::Tests::RunTransactionRollbackTest(":memory:");
        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, Persistence_ReopenSQLiteDatabaseFromDisk)
    {
        const auto tempDb = std::filesystem::temp_directory_path() / "o3de_sqlgencpp_persist_test.db";
        std::error_code ec;
        std::filesystem::remove(tempDb, ec);

        const auto res = sqlgenCPP::Tests::RunReopenPersistenceTest(tempDb.string());
        std::filesystem::remove(tempDb, ec);
        std::filesystem::remove(tempDb.string() + "-wal", ec);
        std::filesystem::remove(tempDb.string() + "-shm", ec);

        EXPECT_TRUE(res.ok) << res.error;
    }

    TEST(SqlgenCPPCodeFirstTest, SystemComponent_ResolvesDatabasePathAndPersists)
    {
        sqlgenCPP::sqlgenCPPSystemComponent component;
        auto* requests = sqlgenCPP::sqlgenCPPInterface::Get();
        ASSERT_NE(requests, nullptr);
        EXPECT_TRUE(requests->IsCodeFirstOrmEnabled());

        const auto pathOutcome = requests->ResolveDatabasePath("Tests/o3de_sqlgencpp_component_test.db", true);
        ASSERT_TRUE(pathOutcome.IsSuccess()) << pathOutcome.GetError().c_str();

        const std::string dbPath(pathOutcome.GetValue().c_str());
        std::error_code ec;
        std::filesystem::remove(dbPath, ec);

        const auto res = sqlgenCPP::Tests::RunReopenPersistenceTest(dbPath);
        std::filesystem::remove(dbPath, ec);
        std::filesystem::remove(dbPath + "-wal", ec);
        std::filesystem::remove(dbPath + "-shm", ec);

        EXPECT_TRUE(res.ok) << res.error;
    }
} // namespace

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
