/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "sqlgenCPPSystemComponent.h"

#include <sqlgenCPP/sqlgenCPPTypeIds.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <filesystem>

namespace sqlgenCPP
{
    namespace
    {
        bool IsStrictlyContainedInBase(const std::filesystem::path& baseDir, const std::filesystem::path& candidate)
        {
            const std::filesystem::path rel = candidate.lexically_relative(baseDir);
            if (rel.empty() || rel == ".")
            {
                return false;
            }
            const auto first = *rel.begin();
            return first != ".." && !rel.is_absolute();
        }
    } // namespace

    AZ_COMPONENT_IMPL(sqlgenCPPSystemComponent, "sqlgenCPPSystemComponent",
        sqlgenCPPSystemComponentTypeId);

    void sqlgenCPPSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<sqlgenCPPSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void sqlgenCPPSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("sqlgenCPPService"));
    }

    void sqlgenCPPSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("sqlgenCPPService"));
    }

    void sqlgenCPPSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void sqlgenCPPSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    sqlgenCPPSystemComponent::sqlgenCPPSystemComponent()
    {
        if (sqlgenCPPInterface::Get() == nullptr)
        {
            sqlgenCPPInterface::Register(this);
        }
    }

    sqlgenCPPSystemComponent::~sqlgenCPPSystemComponent()
    {
        if (sqlgenCPPInterface::Get() == this)
        {
            sqlgenCPPInterface::Unregister(this);
        }
    }

    AZStd::string sqlgenCPPSystemComponent::GetDefaultDatabaseDirectory(bool allowTempFallbackWhenNoFileIO) const
    {
        if (auto* fileIO = AZ::IO::FileIOBase::GetInstance(); fileIO != nullptr)
        {
            if (fileIO->GetAlias("@user@") == nullptr)
            {
                // FileIO is active but @user@ alias is unset: fail explicitly rather than creating "./@user@" or falling back to /tmp.
                return {};
            }
            if (auto resolved = fileIO->ResolvePath(AZ::IO::PathView("@user@/Database")); resolved.has_value())
            {
                return AZStd::string(resolved->c_str());
            }
            // FileIO is active but @user@/Database failed to resolve: never silently fall back to OS temp.
            return {};
        }

        if (!allowTempFallbackWhenNoFileIO)
        {
            return {};
        }

        std::error_code ec;
        const auto tempBase = std::filesystem::temp_directory_path(ec);
        if (!ec)
        {
            const auto fallbackDir = (tempBase / "O3DE_sqlgenCPP").lexically_normal();
            return AZStd::string(fallbackDir.string().c_str());
        }

        return {};
    }

    AZ::Outcome<AZStd::string, AZStd::string> sqlgenCPPSystemComponent::ResolveDatabasePath(
        AZStd::string_view relativeDbPath,
        bool createParentDirectory,
        bool allowTempFallbackWhenNoFileIO) const
    {
        if (relativeDbPath.empty())
        {
            return AZ::Failure(AZStd::string("Database path must not be empty."));
        }

        if (relativeDbPath == ":memory:")
        {
            return AZ::Success(AZStd::string(":memory:"));
        }

        const std::filesystem::path inputPath(std::string_view(relativeDbPath.data(), relativeDbPath.size()));
        if (inputPath.is_absolute() || inputPath.has_root_path() || inputPath.has_root_name())
        {
            return AZ::Failure(AZStd::string("Database path must be relative to the database directory (absolute paths are rejected)."));
        }

        const AZStd::string baseDirStr = GetDefaultDatabaseDirectory(allowTempFallbackWhenNoFileIO);
        if (baseDirStr.empty())
        {
            return AZ::Failure(AZStd::string("Failed to resolve @user@/Database directory (FileIOBase unavailable or @user@ alias unset)."));
        }

        const std::filesystem::path baseDir =
            std::filesystem::path(std::string_view(baseDirStr.data(), baseDirStr.size())).lexically_normal();
        const std::filesystem::path resolvedPath = (baseDir / inputPath).lexically_normal();

        if (!IsStrictlyContainedInBase(baseDir, resolvedPath))
        {
            return AZ::Failure(AZStd::string("Database path escapes the configured database directory."));
        }

        if (createParentDirectory && resolvedPath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(resolvedPath.parent_path(), ec);
            if (ec)
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to create database parent directory '%s': %s",
                    resolvedPath.parent_path().string().c_str(),
                    ec.message().c_str()));
            }
        }

        return AZ::Success(AZStd::string(resolvedPath.string().c_str()));
    }

    bool sqlgenCPPSystemComponent::IsCodeFirstOrmEnabled() const
    {
#if defined(SQLGENCPP_ORM_ENABLED) && SQLGENCPP_ORM_ENABLED
        return true;
#else
        return false;
#endif
    }

    void sqlgenCPPSystemComponent::Init()
    {
    }

    void sqlgenCPPSystemComponent::Activate()
    {
        sqlgenCPPRequestBus::Handler::BusConnect();
    }

    void sqlgenCPPSystemComponent::Deactivate()
    {
        sqlgenCPPRequestBus::Handler::BusDisconnect();
    }

} // namespace sqlgenCPP
