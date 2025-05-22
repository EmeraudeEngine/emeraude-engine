/*
 * src/PrimaryServices.cpp
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

#include "PrimaryServices.hpp"

/* STL inclusions. */
#include <iostream>
#include <sstream>
#include <ranges>
#include <utility>

/* Local inclusions. */
#include "Libs/IO/IO.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;

	PrimaryServices::PrimaryServices (int argc, char * * argv, const Identification & identification) noexcept
		: m_processName("main"),
		m_arguments(argc, argv, false),
		m_tracer(m_arguments, m_processName, false),
		m_fileSystem(m_arguments, m_userInfo, identification, false),
		m_settings(m_arguments, m_fileSystem, false)
	{
		/* NOTE: This must be done immediately! */
		if ( !m_arguments.initialize(m_primaryServicesEnabled) )
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute !";
		}

		if ( m_arguments.get("--verbose").isPresent() )
		{
			m_flags[ShowInformation] = true;
		}
	}

	PrimaryServices::PrimaryServices (int argc, char * * argv, const Identification & identification, std::string processName, const std::vector< std::string > & additionalArguments) noexcept
		: m_processName(std::move(processName)),
		m_arguments(argc, argv, true),
		m_tracer(m_arguments, m_processName, true),
		m_fileSystem(m_arguments, m_userInfo, identification, true),
		m_settings(m_arguments, m_fileSystem, true)
	{
		m_flags[ChildProcess] = true;

		/* NOTE: This must be done immediately! */
		if ( m_arguments.initialize(m_primaryServicesEnabled) )
		{
			if ( !additionalArguments.empty() )
			{
				for ( const auto & argument: additionalArguments )
				{
					m_arguments.addArgument(argument);
				}
			}
		}
		else
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute !";
		}
	}

#if IS_WINDOWS
	PrimaryServices::PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification) noexcept
		: m_arguments(argc, wargv, false),
		m_tracer(m_arguments, "main", false),
		m_fileSystem(m_arguments, m_userInfo, identification, false),
		m_settings(m_arguments, m_fileSystem, false)
	{
		/* NOTE: This must be done immediately! */
		if ( !m_arguments.initialize(m_primaryServicesEnabled) )
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute !";
		}

		if ( m_arguments.get("--verbose").isPresent() )
		{
			m_flags[ShowInformation] = true;
		}
	}

	PrimaryServices::PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification, const std::string & processName, const std::vector< std::string > & additionalArguments) noexcept
		: m_arguments(argc, wargv, true),
		m_tracer(m_arguments, processName, true),
		m_fileSystem(m_arguments, m_userInfo, identification, true),
		m_settings(m_arguments, m_fileSystem, true)
	{
		m_flags[ChildProcess] = true;

		/* NOTE: This must be done immediately! */
		if ( m_arguments.initialize(m_primaryServicesEnabled) )
		{
			if ( !additionalArguments.empty() )
			{
				for ( const auto & argument: additionalArguments )
				{
					m_arguments.addArgument(argument);
				}
			}
		}
		else
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute !";
		}
	}
#endif

	bool
	PrimaryServices::initialize () noexcept
	{
		/* Initialize tracer. */
		if ( m_tracer.initialize(m_primaryServicesEnabled) )
		{
			std::cout << ClassId << ", " << m_tracer.name() << " primary service up [" << m_processName << "] !" "\n";
		}
		else
		{
			std::cerr << ClassId << ", " << m_tracer.name() << " primary service failed to execute [" << m_processName << "] !" "\n";

			return false;
		}

		/* Initialize the file system to reach every useful directory. */
		if ( m_fileSystem.initialize(m_primaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_fileSystem.name() << " primary service up [" << m_processName << "] !";

			/* Creating some basic paths. */
			const auto directory = m_fileSystem.userDataDirectory("captures");

			if ( !IO::directoryExists(directory) )
			{
				if ( IO::createDirectory(directory) )
				{
					TraceSuccess{ClassId} << "Captures directory " << directory << " created.";
				}
				else
				{
					TraceWarning{ClassId} << "Unable to create captures directory " << directory << " !";
				}
			}
		}
		else
		{
			TraceFatal{ClassId} << m_fileSystem.name() << " primary service failed to execute [" << m_processName << "] !";

			return false;
		}

		/* Initialize core settings.
		 * NOTE: Settings class manages to write a default file. */
		if ( m_settings.initialize(m_primaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_settings.name() << " primary service up [" << m_processName << "] !";

			/* NOTE: Now the core settings are initialized, we can update the tracer service configuration. */
			m_tracer.lateInitialize(m_fileSystem, m_settings);
		}
		else
		{
			TraceError{ClassId} <<
				m_fileSystem.name() << " primary service failed to execute [" << m_processName << "] !" "\n"
				"There is a problem to read or write the core settings file." "\n"
				"The engine will use the default configuration.";
		}

		return true;
	}

	void
	PrimaryServices::terminate () noexcept
	{
		/* Terminate primary services. */
		for ( auto * service : std::ranges::reverse_view(m_primaryServicesEnabled) )
		{
			if ( service->terminate() )
			{
				TraceSuccess{ClassId} << service->name() << " primary service terminated gracefully [" << m_processName << "] !";
			}
			else
			{
				TraceError{ClassId} << service->name() << " primary service failed to terminate properly [" << m_processName << "] !";
			}
		}
	}

	std::string
	PrimaryServices::information () const noexcept
	{
		std::stringstream output;

		output << "\n"
			" ================== GENERAL INFORMATION ==================" "\n"
			<< m_systemInfo << "\n"
			<< m_userInfo << "\n"
			<< m_arguments << "\n"
			<< m_fileSystem << "\n"
			<< m_settings << "\n"
			" ================ GENERAL INFORMATION EOF ================" "\n\n";

		return output.str();
	}
}
