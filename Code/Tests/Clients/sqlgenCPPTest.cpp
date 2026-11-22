#include <AzTest/AzTest.h>

#include <Clients/SqlgenCodeFirstTestImpl.h>

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
} // namespace

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
