/*
 * src/PrimaryServices.hpp
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
#include <vector>
#include <string>
#include <array>

/* Local inclusions for usages. */
#include "Identification.hpp"
#include "PlatformSpecific/SystemInfo.hpp"
#include "PlatformSpecific/UserInfo.hpp"
#include "Arguments.hpp"
#include "Tracer.hpp"
#include "FileSystem.hpp"
#include "Settings.hpp"

namespace EmEn
{
	/**
	 * @brief This class holds the primary services.
	 */
	class PrimaryServices final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PrimaryServices"};

			/**
			 * @brief Constructs the primary services manager for a main process.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param argv The argument values it from the standard C/C++ main() function.
			 * @param identification A reference to the application identification.
			 */
			PrimaryServices (int argc, char * * argv, const Identification & identification) noexcept;

			/**
			 * @brief Constructs the primary services manager for a child process.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param argv The argument values it from the standard C/C++ main() function.
			 * @param identification A reference to the application identification.
			 * @param processName A string  [std::move].
			 * @param additionalArguments A reference to a vector of strings. Default none.
			 */
			PrimaryServices (int argc, char * * argv, const Identification & identification, std::string processName, const std::vector< std::string > & additionalArguments = {}) noexcept;

#if IS_WINDOWS
			/**
			 * @brief Constructs the primary services manager for a main process.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param wargv The argument values it from the standard C/C++ main() function.
			 * @param identification A reference to the application identification.
			 */
			PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification) noexcept;

			/**
			 * @brief Constructs the primary services manager for a child process.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param wargv The argument values it from the standard C/C++ main() function.
			 * @param identification A reference to the application identification.
			 * @param processName A reference to a string.
			 * @param additionalArguments A reference to a vector of strings. Default none.
			 */
			PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification, const std::string & processName, const std::vector< std::string > & additionalArguments = {}) noexcept;
#endif

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrimaryServices (const PrimaryServices & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PrimaryServices (PrimaryServices && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PrimaryServices &
			 */
			PrimaryServices & operator= (const PrimaryServices & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PrimaryServices &
			 */
			PrimaryServices & operator= (PrimaryServices && copy) noexcept = delete;

			/**
			 * @brief Destructs the primary services.
			 */
			~PrimaryServices () = default;

			/**
			 * @brief Main initialization method for primary services.
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize () noexcept;

			/**
			 * @brief Main termination method for primary services.
			 * @return void
			 */
			void terminate () noexcept;

			/**
			 * @brief Returns the reference to the system info.
			 * @return const PlatformSpecific::SystemInfo &
			 */
			[[nodiscard]]
			const PlatformSpecific::SystemInfo &
			systemInfo () const noexcept
			{
				return m_systemInfo;
			}

			/**
			 * @brief Returns the reference to the user info.
			 * @return const PlatformSpecific::UserInfo &
			 */
			[[nodiscard]]
			const PlatformSpecific::UserInfo &
			userInfo () const noexcept
			{
				return m_userInfo;
			}

			/**
			 * @brief Returns the reference to the argument service.
			 * @return Arguments &
			 */
			[[nodiscard]]
			Arguments &
			arguments () noexcept
			{
				return m_arguments;
			}

			/**
			 * @brief Returns the reference to the argument service.
			 * @return const Arguments &
			 */
			[[nodiscard]]
			const Arguments &
			arguments () const noexcept
			{
				return m_arguments;
			}

			/**
			 * @brief Returns the reference to the tracer service.
			 * @return Tracer &
			 */
			[[nodiscard]]
			Tracer &
			tracer () noexcept
			{
				return m_tracer;
			}

			/**
			 * @brief Returns the reference to the tracer service.
			 * @return const Tracer &
			 */
			[[nodiscard]]
			const Tracer &
			tracer () const noexcept
			{
				return m_tracer;
			}

			/**
			 * @brief Returns the reference to the file system service.
			 * @return FileSystem &
			 */
			[[nodiscard]]
			FileSystem &
			fileSystem () noexcept
			{
				return m_fileSystem;
			}

			/**
			 * @brief Returns the reference to the file system service.
			 * @return const FileSystem &
			 */
			[[nodiscard]]
			const FileSystem &
			fileSystem () const noexcept
			{
				return m_fileSystem;
			}

			/**
			 * @brief Returns the reference to the settings service.
			 * @return Settings &
			 */
			[[nodiscard]]
			Settings &
			settings () noexcept
			{
				return m_settings;
			}

			/**
			 * @brief Returns the reference to the settings service.
			 * @return const Settings &
			 */
			[[nodiscard]]
			const Settings &
			settings () const noexcept
			{
				return m_settings;
			}

			/**
			 * @brief Returns general information about the primary services.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string information () const noexcept;

		private:

			/* Flag names. */
			static constexpr auto Initialized{0UL};
			static constexpr auto ChildProcess{1UL};
			static constexpr auto ShowInformation{2UL};

			std::string m_processName;
			PlatformSpecific::SystemInfo m_systemInfo;
			PlatformSpecific::UserInfo m_userInfo;
			Arguments m_arguments;
			Tracer m_tracer;
			FileSystem m_fileSystem;
			Settings m_settings;
			std::vector< ServiceInterface * > m_primaryServicesEnabled;
			std::array< bool, 8 > m_flags{
				false/*Initialized*/,
				false/*ChildProcess*/,
				false/*ShowInformation*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
