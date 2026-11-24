/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
    DbTestOutcome RunEmbeddedNulTextRoundTripTest(const std::string& dbPath);
    DbTestOutcome RunBatchUpsertAtomicityTest(const std::string& dbPath);
    DbTestOutcome RunSchemaMigrationTest(const std::string& dbPath);
} // namespace sqlgenCPP::Tests
