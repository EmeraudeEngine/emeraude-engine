/*
 * src/FileSystem.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "FileSystem.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* Local inclusion. */
#include "Libs/IO/IO.hpp"
#include "PlatformSpecific/SystemInfo.hpp"
#include "Arguments.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Libs;

	bool
	FileSystem::onInitialize () noexcept
	{
		if ( !m_childProcess )
		{
			m_showInformation = m_arguments.isSwitchPresent("--verbose");
		}

		m_standAlone = m_arguments.isSwitchPresent("--standalone");

		if ( m_organizationName.empty() || m_applicationName.empty() )
		{
			Tracer::error(ClassId, "The name of the organization or the application is invalid !");

			return false;
		}

		if ( !this->checkBinaryName() )
		{
			Tracer::error(ClassId, "Unable to determine the binary name !");

			return false;
		}

		if ( !this->checkBinaryPath() )
		{
			Tracer::error(ClassId, "Unable to determine the binary parent directory !");

			return false;
		}

		if ( !this->checkUserDataDirectory() )
		{
			Tracer::error(ClassId, "Unable to use the user directory !");

			return false;
		}

		if ( !this->checkConfigDirectory() )
		{
			Tracer::error(ClassId, "Unable to reach a valid config data directory ! You can provide a custom path with argument '--config-directory'.");

			return false;
		}

		if ( !this->checkCacheDirectory() )
		{
			Tracer::error(ClassId, "Unable to reach a valid cache directory ! You can provide a custom path with argument '--cache-directory'.");

			return false;
		}

		if ( !this->checkDataDirectories() )
		{
			Tracer::error(ClassId, "Unable to reach a valid data directory ! You can provide a custom path with argument '--data-directory'.");

			return false;
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << *this;
		}

		m_serviceInitialized = true;

		return true;
	}

	bool
	FileSystem::onTerminate () noexcept
	{
		m_serviceInitialized = false;

		return true;
	}

	bool
	FileSystem::checkBinaryPath () noexcept
	{
		m_binaryDirectory = PlatformSpecific::SystemInfo::getRealApplicationDir();

		if ( m_binaryDirectory.empty() )
		{
			Tracer::error(ClassId, "The binary path is empty !");

			return false;
		}

		return true;
	}

	bool
	FileSystem::checkBinaryName () noexcept
	{
		m_binaryName = m_arguments.binaryFilepath().filename().string();

		return !m_binaryName.empty();
	}

	bool
	FileSystem::checkUserDataDirectory () noexcept
	{
		if ( m_showInformation )
		{
			Tracer::info(ClassId, "Looking for user data directories ...");
		}

		const auto & homePath = m_userInfo.homePath();

		if ( homePath.empty() )
		{
			Tracer::error(ClassId, "Unable to detect the current user home directory !");

			return false;
		}

		m_userDirectory = homePath;

		if ( !IO::directoryExists(m_userDirectory) )
		{
			return false;
		}

		/* Check next to binary. */
		if ( m_standAlone )
		{
			auto directoryPath = m_binaryDirectory;
			directoryPath.append("data");

			return registerDirectory(directoryPath, true, true, m_userDataDirectory);
		}

		auto directoryPath = m_userDirectory;

		if constexpr  ( IsLinux )
		{
			directoryPath.append(".local");
			directoryPath.append("share");
			directoryPath.append(m_organizationName);
			directoryPath.append(m_applicationName);
		}
		else if constexpr ( IsMacOS )
		{
			directoryPath.append("Library");
			directoryPath.append("Application Support");
			directoryPath.append(m_applicationReverseId);
		}
		else if constexpr ( IsWindows )
		{
			directoryPath.append("AppData");
			directoryPath.append("Roaming");
			directoryPath.append(m_organizationName);
			directoryPath.append(m_applicationName);
		}

		return registerDirectory(directoryPath, true, true, m_userDataDirectory);
	}

	bool
	FileSystem::checkConfigDirectory () noexcept
	{
		if ( m_showInformation )
		{
			Tracer::info(ClassId, "Looking for config directories ...");
		}

		/* Check for a forced config directory from command line arguments. */
		if ( const auto forcedPath = m_arguments.get("--config-directory") )
		{
			return registerDirectory(forcedPath.value(), false, true, m_configDirectory);
		}

		/* Check next to binary. */
		if ( m_standAlone )
		{
			auto directoryPath = m_binaryDirectory;
			directoryPath.append("config");

			return registerDirectory(directoryPath, true, true, m_configDirectory);
		}

		/* Check for standard OS config directories. */
		std::vector< std::filesystem::path > paths{};

		if ( !m_userDirectory.empty() )
		{
			auto directoryPath = m_userDirectory;

			if constexpr  ( IsLinux )
			{
				directoryPath.append(".config");
				directoryPath.append(m_organizationName);
				directoryPath.append(m_applicationName);
			}
			else if constexpr ( IsMacOS )
			{
				directoryPath.append("Library");
				directoryPath.append("Preferences");
				directoryPath.append(m_applicationReverseId);
			}
			else if constexpr ( IsWindows )
			{
				directoryPath.append("AppData");
				directoryPath.append("Local");
				directoryPath.append(m_organizationName);
				directoryPath.append(m_applicationName);
			}

			paths.emplace_back(directoryPath);
		}

		for ( auto it = paths.cbegin(); it != paths.cend(); ++it )
		{
			const bool last = (it != paths.cend() && std::next(it) == paths.cend());

			if ( registerDirectory(*it, last, true, m_configDirectory) )
			{
				return true;
			}
		}

		return !m_configDirectory.empty();
	}

	bool
	FileSystem::checkCacheDirectory () noexcept
	{
		if ( m_showInformation )
		{
			Tracer::info(ClassId, "Looking for cache directories ...");
		}

		/* Check for a forced cache directory from command line arguments. */
		if ( const auto forcedPath = m_arguments.get("--cache-directory") )
		{
			return registerDirectory(forcedPath.value(), false, true, m_cacheDirectory);
		}

		/* Check next to binary. */
		if ( m_standAlone )
		{
			auto directoryPath = m_binaryDirectory;
			directoryPath.append("cache");

			return registerDirectory(directoryPath, true, true, m_cacheDirectory);
		}

		/* Check for standard OS config directories. */
		std::vector< std::filesystem::path > paths{};

		if ( !m_userDirectory.empty() )
		{
			auto directoryPath = m_userDirectory;

			if constexpr  ( IsLinux )
			{
				directoryPath.append(".cache");
				directoryPath.append(m_organizationName);
				directoryPath.append(m_applicationName);
			}
			else if constexpr ( IsMacOS )
			{
				directoryPath.append("Library");
				directoryPath.append("Caches");
				directoryPath.append(m_applicationReverseId);
			}
			else if constexpr ( IsWindows )
			{
				directoryPath.append("AppData");
				directoryPath.append("Local");
				directoryPath.append(m_organizationName);
				directoryPath.append(m_applicationName);
			}

			paths.emplace_back(directoryPath);
		}

		for ( auto it = paths.cbegin(); it != paths.cend(); ++it )
		{
			const bool last = (it != paths.cend() && std::next(it) == paths.cend());

			if ( registerDirectory(*it, last, true, m_cacheDirectory) )
			{
				return true;
			}
		}

		return !m_cacheDirectory.empty();
	}

	bool
	FileSystem::checkDataDirectories () noexcept
	{
		if ( m_showInformation )
		{
			Tracer::info(ClassId, "Looking for data directories ...");
		}

		/* Check for a forced data directory from command line arguments. */
		if ( const auto forcedPath = m_arguments.get("--data-directory") )
		{
			const std::filesystem::path tempDirectory{forcedPath.value()};

			if ( !this->checkDirectoryRequirements(tempDirectory, false, false) )
			{
				return false;
			}

			m_dataDirectories.emplace_back(tempDirectory);

			return true;
		}

		std::vector< std::filesystem::path > paths{};

		/* Check for a custom data directory from command line arguments. */
		if ( const auto customDirectory = m_arguments.get("--add-data-directory") )
		{
			paths.emplace_back(customDirectory.value());
		}

		/* Check next to binary [FORCED]. */
		if ( m_standAlone )
		{
			auto directoryPath = m_binaryDirectory;
			directoryPath.append("data");

			if ( !this->checkDirectoryRequirements(directoryPath, true, false) )
			{
				return false;
			}

			m_dataDirectories.emplace_back(directoryPath);

			return true;
		}

		/* NOTE: POSIX folders. */
		if constexpr  ( IsLinux || IsMacOS )
		{
			{
				std::filesystem::path directoryPath{"/usr/share/games"};
				directoryPath.append(m_applicationName);

				paths.emplace_back(directoryPath);
			}

			{
				std::filesystem::path directoryPath{"/usr/local/share/games"};
				directoryPath.append(m_applicationName);

				paths.emplace_back(directoryPath);
			}
		}

		/* Check for standard OS config directories. */
		if ( !m_userDirectory.empty() )
		{
			auto directoryPath = m_userDirectory;

			if constexpr  ( IsLinux )
			{
				directoryPath.append(".local");
				directoryPath.append("share");
				directoryPath.append(m_organizationName);
				directoryPath.append(m_applicationName);
			}
			else if constexpr ( IsMacOS )
			{
				directoryPath.append("Library");
				directoryPath.append("Application Support");
				directoryPath.append(m_applicationReverseId);
			}
			else if constexpr ( IsWindows )
			{
				directoryPath.append("AppData");
				directoryPath.append("Local");
				directoryPath.append(m_organizationName);
				directoryPath.append(m_applicationName);
			}

			paths.emplace_back(directoryPath);
		}

		/* Check next to binary. */
		{
			std::filesystem::path nextBinaryDirectory = m_binaryDirectory;

			/* NOTE: Specific to the bundle .app */
			if constexpr ( IsMacOS )
			{
				nextBinaryDirectory.append("..");
				nextBinaryDirectory.append("Resources");
			}

			nextBinaryDirectory.append("data");

			if ( this->checkDirectoryRequirements(nextBinaryDirectory, true, false) )
			{
				m_dataDirectories.emplace_back(nextBinaryDirectory);
			}
		}

		for ( auto it = paths.cbegin(); it != paths.cend(); ++it )
		{
			const std::filesystem::path & tempDirectory{*it};

			const bool last = (it != paths.cend() && std::next(it) == paths.cend());

			if ( this->checkDirectoryRequirements(tempDirectory, last, false) )
			{
				m_dataDirectories.emplace_back(tempDirectory);
			}
		}

		return true;
	}

	bool
	FileSystem::checkDirectoryRequirements (const std::filesystem::path & directory, bool createDirectory, bool writableRequested) const noexcept
	{
		/* If the directory doesn't exist, we skip it. */
		if ( IO::directoryExists(directory) )
		{
			/* If the directory exists, but we need permission to write to it,
			 * we test, and if the permission is revoked, we skip it. */
			if ( writableRequested && !IO::writable(directory) )
			{
				TraceError{ClassId} << "The directory '" << directory.string() << "' exists, but it's not writable !";

				return false;
			}
		}
		else
		{
			/* NOTE: If no directory was found, we try to create the default one. */
			if ( createDirectory )
			{
				/* If we can't write the directory, we set an error! */
				if ( !IO::createDirectory(directory) )
				{
					TraceError{ClassId} << "Unable to create the directory '" << directory.string() << "' !";

					return false;
				}
			}
			else
			{
				if ( m_showInformation )
				{
					TraceInfo{ClassId} << "Trying to use directory '" << directory.string() << "', but doesn't exists ...";
				}

				return false;
			}
		}

		if ( m_showInformation )
		{
			TraceSuccess{ClassId} << "The directory '" << directory.string() << "' is valid !";
		}

		return true;
	}

	bool
	FileSystem::registerDirectory (const std::filesystem::path & directoryPath, bool createDirectory, bool writableRequested, std::filesystem::path & finalDirectoryPath) const noexcept
	{
		if ( !this->checkDirectoryRequirements(directoryPath, createDirectory, writableRequested) )
		{
			return false;
		}

		finalDirectoryPath = directoryPath;

		return true;
	}

	std::filesystem::path
	FileSystem::getFilepathFromDataDirectories (const std::string & path, const std::string & filename) const noexcept
	{
		for ( auto filepath : m_dataDirectories )
		{
			filepath.append(path);
			filepath.append(filename);

			if ( IO::fileExists(filepath) )
			{
				return filepath;
			}
		}

		return {};
	}
}
