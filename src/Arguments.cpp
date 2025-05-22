/*
 * src/Arguments.cpp
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

#include "Arguments.hpp"

/* STL inclusions. */
#include <cstring>
#include <iostream>

/* Local inclusions. */
#if IS_WINDOWS
#include "PlatformSpecific/Helpers.hpp"
#endif
#include "Libs/String.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;

	const size_t Arguments::ClassUID{getClassUID(ClassId)};
	Arguments * Arguments::s_instance{nullptr};

	Arguments::Arguments (int argc, char * * argv, bool childProcess) noexcept
		: ServiceInterface(ClassId)
	{
		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;

		m_flags[ChildProcess] = childProcess;

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
	Arguments::Arguments (int argc, wchar_t * * wargv, bool childProcess) noexcept
		: ServiceInterface(ClassId)
	{
		m_flags[ChildProcess] = childProcess;

		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;

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

	Arguments::~Arguments ()
	{
		s_instance = nullptr;

		if ( m_argv != nullptr )
		{
			for ( int argIndex = 0; argIndex < m_argc; argIndex++ )
			{
				delete[] m_argv[argIndex];

				m_argv[argIndex] = nullptr;
			}

			delete[] m_argv;

			m_argv = nullptr;
		}
	}

	void
	Arguments::addArgument (const std::string & argument) noexcept
	{
		m_rawArguments.emplace_back(argument);

		if ( argument.find_first_of('=') != std::string::npos )
		{
			const auto list = String::explode(argument, '=');

			m_arguments.emplace(list[0], list[1]);
		}
		else
		{
			m_arguments.emplace(argument, "1");
		}
	}

	void
	Arguments::recreateRawArguments () const noexcept
	{
		m_argc = static_cast< int >(m_rawArguments.size());
		m_argv = new char * [static_cast< size_t >(m_argc)];

		for ( auto it = m_rawArguments.cbegin(); it != m_rawArguments.cend(); ++it )
		{
			const auto argIndex = std::distance(m_rawArguments.cbegin(), it);
			const auto strlen = it->size();

			m_argv[argIndex] = new char[strlen + 1];

			strncpy(m_argv[argIndex], it->data(), strlen);

			m_argv[argIndex][strlen] = '\0';
		}
	}

	int
	Arguments::getArgc () const noexcept
	{
		if ( m_argv == nullptr )
		{
			this->recreateRawArguments();
		}

		return m_argc;
	}

	char * *
	Arguments::getArgv () const noexcept
	{
		if ( m_argv == nullptr )
		{
			this->recreateRawArguments();
		}

		return m_argv;
	}

	Argument
	Arguments::get (const std::string & name) const noexcept
	{
		const auto argIt = m_arguments.find(name);

		if ( argIt == m_arguments.cend() )
		{
			return Argument{false};
		}

		return argIt->second;
	}

	Argument
	Arguments::get (const std::string & name, const std::string & alternateName) const noexcept
	{
		auto argIt = m_arguments.find(name);

		if ( argIt != m_arguments.cend() )
		{
			return argIt->second;
		}

		argIt = m_arguments.find(alternateName);

		if ( argIt != m_arguments.cend() )
		{
			return argIt->second;
		}

		return Argument{false};
	}

	Argument
	Arguments::get (const std::vector< std::string > & namesList) const noexcept
	{
		for ( const auto & name : namesList )
		{
			const auto argIt = m_arguments.find(name);

			if ( argIt != m_arguments.cend() )
			{
				return argIt->second;
			}
		}

		return Argument{false};
	}

	std::string
	Arguments::packForCommandLine () const noexcept
	{
		std::stringstream output;

		for ( const auto & [name, argument] : m_arguments )
		{
			if ( argument.isSwitch() )
			{
				output << name << ' ';
			}
			else
			{
				output << name << '=' << argument << ' ';
			}
		}

		return output.str();
	}

	bool
	Arguments::onInitialize () noexcept
	{
		if ( m_rawArguments.empty() )
		{
			TraceError{ClassId} << "There is no argument to evaluate !";

			return false;
		}

		auto argIt = m_rawArguments.cbegin();

		m_binaryFilepath = *argIt;

		for ( ++argIt; argIt != m_rawArguments.cend(); ++argIt )
		{
			const auto & value = *argIt;

			if ( !value.starts_with('-') )
			{
				std::cerr << "Invalid argument : " << value << '\n';

				continue;
			}

			/* NOTE: Checking the form --xxx=yyy */
			if ( value.find_first_of('=') != std::string::npos )
			{
				auto chunks = String::explode(value, '=', false);

				m_arguments.emplace(std::piecewise_construct, std::forward_as_tuple(chunks[0]), std::forward_as_tuple(chunks[1]));

				continue;
			}

			auto nextArgIt = std::next(argIt);

			/* NOTE: Checking the form --xxx yyy */
			if ( nextArgIt != m_rawArguments.cend() && !nextArgIt->starts_with('-') )
			{
				const auto & nextValue = *nextArgIt;

				/* We assume the arg is the parameter value. */
				m_arguments.emplace(std::piecewise_construct, std::forward_as_tuple(value), std::forward_as_tuple(nextValue));

				++argIt;

				continue;
			}

			/* NOTE: we put "1" to make argument returning true when calling ArgumentValue::isPresent(). */
			m_arguments.emplace(std::piecewise_construct, std::forward_as_tuple(value), std::forward_as_tuple(true));
		}

		if ( !m_flags[ChildProcess] )
		{
			m_flags[ShowInformation] = this->get("--verbose").isPresent();
		}

		/* NOTE: At this point, the tracer is not yet initialized. */
		if ( m_flags[ShowInformation] )
		{
			std::cout << *this << '\n';
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	Arguments::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		m_rawArguments.clear();
		m_arguments.clear();

		return true;
	}

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

	std::string
	to_string (const Arguments & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
