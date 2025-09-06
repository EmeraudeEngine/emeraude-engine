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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <filesystem>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

namespace EmEn
{
	/**
	 * @brief This class describes one argument.
	 */
	class Argument final
	{
		public:

			/**
			 * @brief Constructs a argument.
			 * @param noValue Set to true for a simple argument with no value.
			 */
			explicit
			Argument (bool noValue = false) noexcept
				: m_isSwitch{noValue}
			{

			}

			/**
			 * @brief Constructs an argument with a value.
			 * @param value A reference to a string [std::move].
			 */
			explicit
			Argument (std::string value) noexcept
				: m_value{std::move(value)}
			{

			}

			/**
			 * @brief Returns whether the argument is a simple switch with no value.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSwitch () const noexcept
			{
				return m_isSwitch;
			}

			/**
			 * @brief Returns whether the argument is present.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPresent () const noexcept
			{
				return m_isSwitch || !m_value.empty();
			}

			/**
			 * @brief Returns the value associated with the argument.
			 * @note This can be empty.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			value () const noexcept
			{
				return m_value;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Argument & obj);

			std::string m_value;
			bool m_isSwitch{false};
	};

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

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_serviceInitialized;
			}

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
			 * @brief Returns the argument map.
			 * @return const std::map< std::string, Argument, std::less<> > &
			 */
			[[nodiscard]]
			const std::map< std::string, Argument, std::less<> > &
			argumentList () const noexcept
			{
				return m_arguments;
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
			 * @brief Returns a parsed argument from the command line. Unique name version.
			 * @param argument A string view.
			 * @return Argument
			 */
			[[nodiscard]]
			Argument get (std::string_view argument) const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line. The name is a short version.
			 * @param argument A string view.
			 * @param alternateArgument A string view for an alternate argument name.
			 * @return Argument
			 */
			[[nodiscard]]
			Argument get (std::string_view argument, std::string_view alternateArgument) const noexcept;

			/**
			 * @brief Returns a parsed argument from the command line. Multiple name versions.
			 * @param namesList A reference to a vector of string defining all possible argument names from the command line.
			 * @return Argument
			 */
			[[nodiscard]]
			Argument get (const std::vector< std::string > & namesList) const noexcept;

			/**
			 * @brief Adds an argument.
			 * @param argument A reference to a string.
			 * @return void
			 */
			void addArgument (const std::string & argument) noexcept;

			/**
			 * @brief Packs arguments to use in a command line.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string packForCommandLine () const noexcept;

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
			std::map< std::string, Argument, std::less<> > m_arguments;
			bool m_serviceInitialized{false};
			const bool m_childProcess{false};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Argument & obj)
	{
		return out << ( obj.value().empty() ? "{NO_VALUE}" : obj.value() );
	}

	inline
	std::ostream &
	operator<< (std::ostream & out, const Arguments & obj)
	{
		if ( obj.m_arguments.empty() )
		{
			return out << "Executable arguments : NONE" "\n";
		}

		out << "Executable arguments :" "\n";

		for ( const auto & [name, argument] : obj.m_arguments )
		{
			if ( argument.isSwitch() )
			{
				out << name << '\n';
			}
			else
			{
				out << name << " = " << argument << '\n';
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
	to_string (const Argument & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
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
