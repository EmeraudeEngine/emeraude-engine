/*
 * src/Arguments.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <functional>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#ifdef IS_WINDOWS
#include "PlatformSpecific/Helpers.hpp"
#endif

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

			/**
			 * @brief Constructs the argument service.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param argv The argument value from the standard C/C++ main() function.
			 * @param childProcess Declares a child process.
			 */
			Arguments (int argc, char * * argv, bool childProcess) noexcept
				: ServiceInterface{ClassId},
				m_childProcess{childProcess}
			{
				/* NOTE: Create a copy of main() arguments. */
				if ( argc > 0 && argv != nullptr )
				{
					m_rawArguments.reserve(argc);

					for ( int argIndex = 0; argIndex < argc; argIndex++ )
					{
						m_rawArguments.emplace_back(argv[argIndex]);
					}
				}
			}

#if IS_WINDOWS
			/**
			 * @brief Constructs the argument service.
			 * @note Windows version.
			 * @param argc The argument count from the standard C/C++ main() function.
			 * @param wargv The argument value from the standard C/C++ main() function.
			 * @param childProcess Declares a child process.
			 */
			Arguments (int argc, wchar_t * * wargv, bool childProcess) noexcept
				: ServiceInterface{ClassId},
				m_childProcess{childProcess}
			{
				/* NOTE: Create a copy of main() arguments. */
				if ( argc > 0 && wargv != nullptr )
				{
					m_rawArguments.reserve(argc);

					for ( int argIndex = 0; argIndex < argc; argIndex++ )
					{
						std::wstring tmp{wargv[argIndex]};

						m_rawArguments.emplace_back(PlatformSpecific::convertWideToUTF8(tmp));
					}
				}
			}
#endif

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
			 * @param argument A string view.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRawArgumentPresent (std::string_view argument) const noexcept
			{
				return std::ranges::any_of(m_rawArguments, [&argument] (const auto & currentArgument) {
					return currentArgument == argument;
				});
			}

			/**
			 * @brief Returns whether is arguments is for a child process.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isChildProcess () const noexcept
			{
				return m_childProcess;
			}

			/**
			 * @brief Adds a switch.
			 * @param name A string view.
			 * @param completeRawArguments Set the switch to the raw arguments. Default true.
			 * @return void
			 */
			void addSwitch (std::string_view name, bool completeRawArguments = true) noexcept;

			/**
			 * @brief Adds an argument.
			 * @param name A string view.
			 * @param value A string view.
			 * @param completeRawArguments Set the switch to the raw arguments. Default true.
			 * @return void
			 */
			void addArgument (std::string_view name, std::string_view value, bool completeRawArguments = true) noexcept;

			/**
			 * @brief Returns whether a switch is present in the argument.
			 * @param argument A string view.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSwitchPresent (std::string_view argument) const noexcept;

			/**
			 * @brief Returns whether a switch is present in the argument.
			 * @param argument A string view.
			 * @param alternateArgument A string view for an alternate argument name.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSwitchPresent (std::string_view argument, std::string_view alternateArgument) const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line.
			 * @param argument A string view.
			 * @return std::optional< std::string >
			 */
			[[nodiscard]]
			std::optional< std::string > get (std::string_view argument) const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line.
			 * @param argument A string view.
			 * @param alternateArgument A string view for an alternate argument name.
			 * @return std::optional< std::string >
			 */
			[[nodiscard]]
			std::optional< std::string > get (std::string_view argument, std::string_view alternateArgument) const noexcept;

			/**
			 * @brief Packs arguments to use in a command line.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string packForCommandLine () const noexcept;

			/**
			 * @brief For each switch present.
			 * @param lambda A reference to a function using the signature "bool method (const std::string &, const std::string &)".
			 * @return void
			 */
			void
			forEachSwitch (const std::function< bool (const std::string & name) > & lambda) const noexcept
			{
				for ( const auto & name : m_switches )
				{
					if ( lambda(name))
					{
						break;
					}
				}
			}

			/**
			 * @brief For each argument present.
			 * @param lambda A reference to a function using the signature "bool method (const std::string &, const std::string &)".
			 * @return void
			 */
			void
			forEachArgument (const std::function< bool (const std::string & name, const std::string & value) > & lambda) const noexcept
			{
				for ( const auto & [name, value] : m_arguments )
				{
					if ( lambda(name, value))
					{
						break;
					}
				}
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Arguments & obj);

			/* Flag names */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto ChildProcess{1UL};

			std::filesystem::path m_binaryFilepath;
			std::vector< std::string > m_rawArguments;
			std::set< std::string > m_switches;
			std::map< std::string, std::string, std::less<> > m_arguments;
			const bool m_childProcess{false};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Arguments & obj)
	{
		if ( obj.m_switches.empty() )
		{
			out << "Executable switches : NONE" "\n";
		}
		else
		{
			out << "Executable switches :" "\n";

			for ( const auto & name : obj.m_switches )
			{
				out << name << '\n';
			}
		}

		if ( obj.m_arguments.empty() )
		{
			out << "Executable arguments : NONE" "\n";
		}
		else
		{
			out << "Executable arguments :" "\n";

			for ( const auto & [name, value] : obj.m_arguments )
			{
				out << name << " = " << value << '\n';
			}
		}

		return out;
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Arguments & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
