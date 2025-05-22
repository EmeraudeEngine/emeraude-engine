/*
 * src/FileSystem.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <filesystem>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "PlatformSpecific/UserInfo.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Identification;
	class Arguments;
}

namespace EmEn
{
	/**
	 * @brief The file system service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class FileSystem final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FileSystemService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Constructs the file system.
			 * @param arguments A reference to arguments.
			 * @param userInfo A reference to the user info.
			 * @param identification A reference to the application identification.
			 * @param childProcess Declares a child process.
			 */
			FileSystem (const Arguments & arguments, const PlatformSpecific::UserInfo & userInfo, const Identification & identification, bool childProcess) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			FileSystem (const FileSystem & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			FileSystem (FileSystem && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return FileSystem &
			 */
			FileSystem & operator= (const FileSystem & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return FileSystem &
			 */
			FileSystem & operator= (FileSystem && copy) noexcept = delete;

			/**
			 * @brief Destructs the file system.
			 */
			~FileSystem () override;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
			}

			/**
			 * @brief Returns the application binary file name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			binaryName () const noexcept
			{
				return m_binaryName;
			}

			/**
			 * @brief Returns the parent directory of the binary.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			binaryDirectory () const noexcept
			{
				return m_binaryDirectory;
			}

			/**
			 * @brief Returns the parent directory of the binary with an append to the path.
			 * @param append A reference to a string.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path
			binaryDirectory (const std::string & append) const noexcept
			{
				auto path = m_binaryDirectory;
				path.append(append);

				return path;
			}

			/**
			 * @brief Returns the home directory of the current user.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			userDirectory () const noexcept
			{
				return m_userDirectory;
			}

			/**
			 * @brief Returns the home directory of the current user with an append to the path.
			 * @param append A reference to a string.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path
			userDirectory (const std::string & append) const noexcept
			{
				auto path = m_userDirectory;
				path.append(append);

				return path;
			}

			/**
			 * @brief Returns the user data directory for the application of the current user.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			userDataDirectory () const noexcept
			{
				return m_userDataDirectory;
			}

			/**
			 * @brief Returns the user data directory for the application of the current user with an append to the path.
			 * @param append A reference to a string.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path
			userDataDirectory (const std::string & append) const noexcept
			{
				auto path = m_userDataDirectory;
				path.append(append);

				return path;
			}

			/**
			 * @brief Returns the config directory of this application.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			configDirectory () const noexcept
			{
				return m_configDirectory;
			}

			/**
			 * @brief Returns the config directory of this application with an append to the path.
			 * @param append A reference to a string.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path
			configDirectory (const std::string & append) const noexcept
			{
				auto path = m_configDirectory;
				path.append(append);

				return path;
			}

			/**
			 * @brief Returns the cache directory of this application.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			cacheDirectory () const noexcept
			{
				return m_cacheDirectory;
			}

			/**
			 * @brief Returns the cache directory of this application with an append to the path.
			 * @param append A reference to a string.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path
			cacheDirectory (const std::string & append) const noexcept
			{
				auto path = m_cacheDirectory;
				path.append(append);

				return path;
			}

			/**
			 * @brief Returns a list of available data directories for this application.
			 * @return const std::vector< std::filesystem::path > &
			 */
			[[nodiscard]]
			const std::vector< std::filesystem::path > &
			dataDirectories () const noexcept
			{
				return m_dataDirectories;
			}

			/**
			 * @brief Returns a possible filepath from data directories.
			 * @param path A reference to a string for the directory inside a data directory.
			 * @param filename A reference to a string for the filename.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path getFilepathFromDataDirectories (const std::string & path, const std::string & filename) const noexcept;

			/**
			 * @brief Returns the instance of the file system.
			 * @return FileSystem *
			 */
			[[nodiscard]]
			static
			FileSystem *
			instance () noexcept
			{
				return s_instance;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const FileSystem & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const FileSystem & obj) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Checks the binary path.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkBinaryPath () noexcept;

			/**
			 * @brief Checks the binary name.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkBinaryName () noexcept;

			/**
			 * @brief Searches or creates a user data directory for the application (writable, per user).
			 * @note This is where data created by the user will be stored, like saved games or screenshots.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkUserDataDirectory () noexcept;

			/**
			 * @brief Searches or creates a config directory for the application (writable, per user, per machine).
			 * @note This is where configuration files will be stored according to the machine.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkConfigDirectory () noexcept;

			/**
			 * @brief Searches or creates a cache directory for the application (writable, per user, per machine).
			 * @note This is where all volatile (non-important) data will be stored according to the machine.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkCacheDirectory () noexcept;

			/**
			 * @brief Searches for application data directories (read-only, per app).
			 * @note This is where the application data will be looking for.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDataDirectories () noexcept;

			/**
			 * @brief Verifies the requirements for a searched directory.
			 * @param directory A reference to a directory.
			 * @param createDirectory If no directory was found, create it.
			 * @param writableRequested Is directory need to be writable.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkDirectoryRequirements (const std::filesystem::path & directory, bool createDirectory, bool writableRequested) const noexcept;

			/**
			 * @brief Registers a directory when tested.
			 * @param directoryPath A reference to a filesystem path.
			 * @param createDirectory If no directory was found, create it.
			 * @param writableRequested Is directory needed to be writable?
			 * @param finalDirectoryPath A reference to a filesystem path.
			 * @return bool
			 */
			[[nodiscard]]
			bool registerDirectory (const std::filesystem::path & directoryPath, bool createDirectory, bool writableRequested, std::filesystem::path & finalDirectoryPath) const noexcept;

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto ChildProcess{1UL};
			static constexpr auto ShowInformation{2UL};
			static constexpr auto StandAlone{3UL};

			/** @brief Singleton pointer. */
			static FileSystem * s_instance;

			const Arguments & m_arguments;
			const PlatformSpecific::UserInfo & m_userInfo;
			std::string m_organizationName;
			std::string m_applicationName;
			std::string m_applicationReverseId;
			std::string m_binaryName;
			std::filesystem::path m_binaryDirectory;
			std::filesystem::path m_userDirectory;
			std::filesystem::path m_userDataDirectory;
			std::filesystem::path m_configDirectory;
			std::filesystem::path m_cacheDirectory;
			std::vector< std::filesystem::path > m_dataDirectories;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*ChildProcess*/,
				false/*ShowInformation*/,
				false/*StandAlone*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
