/*
 * src/Arguments.hpp
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
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <ranges>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions. */
#include "Argument.hpp"

namespace EmEn
{
	/**
	 * @brief The application arguments service.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class Arguments final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ArgumentsService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Constructs the argument service.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param argv The argument value from the standard C/C++ main() function.
			 * @param childProcess Declares a child process.
			 */
			Arguments (int argc, char * * argv, bool childProcess) noexcept;

#if IS_WINDOWS
			/**
			 * @brief Constructs the argument service.
			 * @note Windows version.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param wargv The argument value from the standard C/C++ main() function.
			 * @param childProcess Declares a child process.
			 */
			Arguments (int argc, wchar_t * * wargv, bool childProcess) noexcept;
#endif

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Arguments (const Arguments & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Arguments (Arguments && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Arguments &
			 */
			Arguments & operator= (const Arguments & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Arguments &
			 */
			Arguments & operator= (Arguments && copy) noexcept = delete;

			/**
			 * @brief Destructs the application arguments service.
			 */
			~Arguments () override;

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
			 * @brief Adds an argument.
			 * @param argument A reference to a string.
			 * @return void
			 */
			void addArgument (const std::string & argument) noexcept;

			/**
			 * @brief Returns a list of argument copies.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			rawArguments () const noexcept
			{
				return m_rawArguments;
			}

			/**
			 * @brief Returns whether a raw argument is present.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRawArgumentPresent (const std::string & argument) const noexcept
			{
				return std::ranges::any_of(m_rawArguments, [&argument] (const auto & currentArgument) {
					return currentArgument == argument;
				});
			}

			/**
			 * @brief Returns the argument count from the standard C main function.
			 * @return int
			 */
			[[nodiscard]]
			int getArgc () const noexcept;

			/**
			 * @brief Returns the argument values from the standard C main function.
			 * @return char * *
			 */
			[[nodiscard]]
			char * * getArgv () const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line. Unique name version.
			 * @param name A reference to a string to define the argument name from the command line.
			 * @return Argument
			 */
			[[nodiscard]]
			Argument get (const std::string & name) const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line. The name an is short version.
			 * @param name A reference to a string defining the argument name from the command line.
			 * @param alternateName A reference to a string for an alternate name.
			 * @return Argument
			 */
			[[nodiscard]]
			Argument get (const std::string & name, const std::string & alternateName) const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line. Multiple name versions.
			 * @param namesList A reference to a vector of string defining all possible argument names from the command line.
			 * @return Argument
			 */
			[[nodiscard]]
			Argument get (const std::vector< std::string > & namesList) const noexcept;

			/**
			 * @brief Returns the argument map.
			 * @return const std::map< std::string, Argument > &
			 */
			[[nodiscard]]
			const std::map< std::string, Argument > &
			argumentList () const noexcept
			{
				return m_arguments;
			}

			/**
			 * @brief Packs arguments to use in a command line.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string packForCommandLine () const noexcept;

			/**
			 * @brief Returns the application executable path.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			binaryFilepath () const noexcept
			{
				return m_binaryFilepath;
			}

			/**
			 * @brief Returns the instance of the argument manager.
			 * @return Arguments *
			 */
			[[nodiscard]]
			static
			Arguments *
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
			friend std::ostream & operator<< (std::ostream & out, const Arguments & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const Arguments & obj) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Recreates argc and argv from the main parameters after modifications.
			 * @return void
			 */
			void recreateRawArguments () const noexcept;

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto ChildProcess{1UL};
			static constexpr auto ShowInformation{2UL};

			/** @brief Singleton pointer. */
			static Arguments * s_instance;

			std::vector< std::string > m_rawArguments;
			mutable int m_argc{0};
			mutable char * * m_argv{nullptr};
			std::filesystem::path m_binaryFilepath;
			std::map< std::string, Argument > m_arguments;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
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
