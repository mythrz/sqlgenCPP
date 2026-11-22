#pragma once

#include <string>

namespace sqlgenCPP::Tests
{
    struct DbTestOutcome
    {
        bool ok = false;
        std::string error;
    };

    DbTestOutcome RunSchemaCreationTest(const std::string& dbPath);
    DbTestOutcome RunSeedInitialDatabaseTest(const std::string& dbPath);
    DbTestOutcome RunCrudLifecycleTest(const std::string& dbPath);
    DbTestOutcome RunTransactionRollbackTest(const std::string& dbPath);
    DbTestOutcome RunReopenPersistenceTest(const std::string& dbPath);
} // namespace sqlgenCPP::Tests
