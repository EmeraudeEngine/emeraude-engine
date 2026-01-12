/*
 * src/Arguments.cpp
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

#include "Arguments.hpp"

/* Local inclusions. */
#if IS_WINDOWS
#include "PlatformSpecific/Helpers.hpp"
#endif
#include "Libs/String.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Libs;

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
				TraceWarning{ClassId} << "Invalid argument '" << value << "', skipping ...";

				continue;
			}

			/* NOTE: Checking the form --xxx=yyy */
			if ( value.find_first_of('=') != std::string::npos )
			{
				if ( const auto chunks = String::explode(value, '=', false); chunks.size() >= 2 )
				{
					m_arguments.emplace(chunks[0], chunks[1]);
				}
				else if ( chunks.size() == 1 )
				{
					m_arguments.emplace(chunks[0], "");
				}

				continue;
			}

			/* NOTE: Checking the form --xxx yyy */
			if ( auto nextArgIt = std::next(argIt); nextArgIt != m_rawArguments.cend() && !nextArgIt->starts_with('-') )
			{
				const auto & nextValue = *nextArgIt;

				/* We assume the arg is the parameter value. */
				m_arguments.emplace(value, nextValue);

				++argIt;

				continue;
			}

			/* NOTE: Simple switch. */
			m_switches.emplace(value);
		}

		/* NOTE: At this point, the tracer is not yet initialized. */
		if ( this->isSwitchPresent("--verbose") )
		{
			TraceInfo{ClassId} << *this;
		}

		return true;
	}

	bool
	Arguments::onTerminate () noexcept
	{
		m_rawArguments.clear();
		m_arguments.clear();

		return true;
	}

	void
	Arguments::addSwitch (std::string_view name, bool completeRawArguments) noexcept
	{
		m_switches.emplace(name);

		if ( completeRawArguments )
		{
			m_rawArguments.emplace_back(name);
		}
	}

	void
	Arguments::addArgument (std::string_view name, std::string_view value, bool completeRawArguments) noexcept
	{
		m_arguments.emplace(name, value);

		if ( completeRawArguments )
		{
			std::stringstream rawArgument;
			rawArgument << name << '=' << value;

			m_rawArguments.emplace_back(rawArgument.str());
		}
	}

	bool
	Arguments::isSwitchPresent (std::string_view argument) const noexcept
	{
		return m_switches.contains(argument.data());
	}

	bool
	Arguments::isSwitchPresent (std::string_view argument, std::string_view alternateArgument) const noexcept
	{
		if ( m_switches.contains(argument.data()) )
		{
			return true;
		}

		return m_switches.contains(alternateArgument.data());
	}

	std::optional< std::string >
	Arguments::get (std::string_view argument) const noexcept
	{
		if ( const auto argIt = m_arguments.find(argument); argIt != m_arguments.cend() )
		{
			return argIt->second;
		}

		return std::nullopt;
	}

	std::optional< std::string >
	Arguments::get (std::string_view argument, std::string_view alternateArgument) const noexcept
	{
		auto argIt = m_arguments.find(argument);

		if ( argIt != m_arguments.cend() )
		{
			return argIt->second;
		}

		argIt = m_arguments.find(alternateArgument);

		if ( argIt != m_arguments.cend() )
		{
			return argIt->second;
		}

		return std::nullopt;
	}

	std::string
	Arguments::packForCommandLine () const noexcept
	{
		std::stringstream output;

		for ( const auto & name : m_switches )
		{
			output << name << ' ';
		}

		for ( const auto & [name, argument] : m_arguments )
		{
			output << name << '=' << argument << ' ';
		}

		return output.str();
	}
}
