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
#include <string>

#if !defined(_WIN32)
#include <unistd.h>
#endif

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
                std::error_code canonEc;
                const std::filesystem::path rawDir(resolved->c_str());
                const std::filesystem::path canonicalDir = std::filesystem::weakly_canonical(rawDir, canonEc);
                const std::filesystem::path resultDir = canonEc ? rawDir.lexically_normal() : canonicalDir;
                return AZStd::string(resultDir.string().c_str());
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
#if !defined(_WIN32)
            const std::string fallbackLeaf =
                "O3DE_sqlgenCPP_" + std::to_string(static_cast<unsigned long>(::geteuid()));
#else
            const std::string fallbackLeaf = "O3DE_sqlgenCPP";
#endif
            const std::filesystem::path rawFallback = (tempBase / fallbackLeaf).lexically_normal();
            std::error_code canonEc;
            const std::filesystem::path canonicalFallback = std::filesystem::weakly_canonical(rawFallback, canonEc);
            const std::filesystem::path resultDir = canonEc ? rawFallback : canonicalFallback;
            return AZStd::string(resultDir.string().c_str());
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

        if (relativeDbPath.find('\0') != AZStd::string_view::npos)
        {
            return AZ::Failure(AZStd::string("Database path must not contain embedded NUL characters."));
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
        if (relativeDbPath.back() == '/' || relativeDbPath.back() == '\\' ||
            !inputPath.has_filename() || inputPath.filename() == "." || inputPath.filename() == "..")
        {
            return AZ::Failure(AZStd::string("Database path must name a file, not a directory."));
        }

        const AZStd::string baseDirStr = GetDefaultDatabaseDirectory(allowTempFallbackWhenNoFileIO);
        if (baseDirStr.empty())
        {
            return AZ::Failure(AZStd::string("Failed to resolve @user@/Database directory (FileIOBase unavailable or @user@ alias unset)."));
        }

        const std::filesystem::path baseDir =
            std::filesystem::path(std::string_view(baseDirStr.data(), baseDirStr.size())).lexically_normal();
        const std::filesystem::path resolvedPath = (baseDir / inputPath).lexically_normal();

        if (!IsStrictlyContainedInBase(baseDir, resolvedPath) || !resolvedPath.has_filename())
        {
            return AZ::Failure(AZStd::string("Database path escapes the configured database directory."));
        }

        // Reject any symbolic link (live or dangling, intermediate or leaf) inside the database directory.
        // Note: weakly_canonical() alone does not resolve dangling symlinks because exists(status()) is false.
        std::error_code canonBaseEc;
        const std::filesystem::path canonicalBase = std::filesystem::weakly_canonical(baseDir, canonBaseEc);
        if (canonBaseEc)
        {
            return AZ::Failure(AZStd::string("Failed to canonicalize configured database directory."));
        }

        const std::filesystem::path relFromBase = resolvedPath.lexically_relative(baseDir);
        {
            std::filesystem::path walked = canonicalBase;
            for (auto it = relFromBase.begin(); it != relFromBase.end(); ++it)
            {
                walked /= *it;
                auto nextIt = it;
                ++nextIt;
                const bool isLeaf = (nextIt == relFromBase.end());

                std::error_code symEc;
                const std::filesystem::file_status symStatus = std::filesystem::symlink_status(walked, symEc);
                if (symEc)
                {
                    if (symEc == std::errc::no_such_file_or_directory || symEc == std::errc::not_a_directory)
                    {
                        break;
                    }
                    return AZ::Failure(AZStd::string::format(
                        "Failed to inspect database path component '%s': %s",
                        walked.string().c_str(),
                        symEc.message().c_str()));
                }
                if (std::filesystem::is_symlink(symStatus))
                {
                    return AZ::Failure(AZStd::string("Database path must not traverse or target a symbolic link."));
                }
                if (symStatus.type() == std::filesystem::file_type::not_found)
                {
                    break;
                }
                if (!isLeaf && !std::filesystem::is_directory(symStatus))
                {
                    return AZ::Failure(AZStd::string("Database parent path component is not a directory."));
                }
            }
        }

        // Defense-in-depth canonical containment check
        std::error_code canonPathEc;
        const std::filesystem::path canonicalResolved = std::filesystem::weakly_canonical(resolvedPath, canonPathEc);
        if (canonPathEc || !IsStrictlyContainedInBase(canonicalBase, canonicalResolved))
        {
            return AZ::Failure(AZStd::string("Database path escapes the configured database directory via symlink."));
        }

        std::error_code statusEc;
        const std::filesystem::file_status existingStatus = std::filesystem::symlink_status(canonicalResolved, statusEc);
        if (statusEc && statusEc != std::errc::no_such_file_or_directory && statusEc != std::errc::not_a_directory)
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to inspect resolved database path '%s': %s",
                canonicalResolved.string().c_str(),
                statusEc.message().c_str()));
        }
        if (!statusEc && std::filesystem::is_symlink(existingStatus))
        {
            return AZ::Failure(AZStd::string("Database path must not target a symbolic link."));
        }
        if (!statusEc && std::filesystem::exists(existingStatus))
        {
            if (std::filesystem::is_directory(existingStatus))
            {
                return AZ::Failure(AZStd::string("Database path points to an existing directory, not a file."));
            }
            if (!std::filesystem::is_regular_file(existingStatus))
            {
                return AZ::Failure(AZStd::string("Database path points to a non-regular file."));
            }
        }

        if (createParentDirectory && canonicalResolved.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(canonicalResolved.parent_path(), ec);
            if (ec)
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to create database parent directory '%s': %s",
                    canonicalResolved.parent_path().string().c_str(),
                    ec.message().c_str()));
            }
        }

        return AZ::Success(AZStd::string(canonicalResolved.string().c_str()));
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
