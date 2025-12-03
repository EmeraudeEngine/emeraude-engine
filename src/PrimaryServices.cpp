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
	using namespace Libs;

	PrimaryServices::PrimaryServices (int argc, char * * argv, const Identification & identification) noexcept
		: m_processName{"main"},
		m_arguments{argc, argv, false},
		m_fileSystem{m_arguments, m_userInfo, identification, false},
		m_settings{m_arguments, m_fileSystem, false},
		m_networkManager{m_fileSystem, m_threadPool}
	{
		/* NOTE: This must be done immediately! */
		if ( !m_arguments.initialize(m_servicesEnabled) )
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute!";
		}

		Tracer::getInstance().earlySetup(m_arguments, m_processName, false);

		if ( m_arguments.isSwitchPresent("--verbose") )
		{
			m_showInformation = true;
		}
	}

	PrimaryServices::PrimaryServices (int argc, char * * argv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept
		: m_processName{std::move(processName)},
		m_arguments{argc, argv, true},
		m_fileSystem{m_arguments, m_userInfo, identification, true},
		m_settings{m_arguments, m_fileSystem, true},
		m_networkManager{m_fileSystem, m_threadPool},
		m_childProcess{true}
	{
		/* NOTE: This must be done immediately! */
		if ( m_arguments.initialize(m_servicesEnabled) )
		{
			if ( !additionalArguments.empty() )
			{
				for ( const auto & [name, value] : additionalArguments )
				{
					if ( value.empty() )
					{
						m_arguments.addSwitch(name);
					}
					else
					{
						m_arguments.addArgument(name, value);
					}
				}
			}

			Tracer::getInstance().earlySetup(m_arguments, m_processName, true);
		}
		else
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute!";
		}
	}

#if IS_WINDOWS
	PrimaryServices::PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification) noexcept
		: m_processName{"main"},
		m_arguments{argc, wargv, false},
		m_fileSystem{m_arguments, m_userInfo, identification, false},
		m_settings{m_arguments, m_fileSystem, false},
		m_networkManager{m_fileSystem, m_threadPool}
	{
		/* NOTE: This must be done immediately! */
		if ( !m_arguments.initialize(m_servicesEnabled) )
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute!";
		}

		Tracer::getInstance().earlySetup(m_arguments, m_processName, false);

		if ( m_arguments.isSwitchPresent("--verbose") )
		{
			m_showInformation = true;
		}
	}

	PrimaryServices::PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification, const std::string & processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept
		: m_processName{std::move(processName)},
		m_arguments{argc, wargv, true},
		m_fileSystem{m_arguments, m_userInfo, identification, true},
		m_settings{m_arguments, m_fileSystem, true},
		m_networkManager{m_fileSystem, m_threadPool},
		m_childProcess{true}
	{
		/* NOTE: This must be done immediately! */
		if ( m_arguments.initialize(m_servicesEnabled) )
		{
			if ( !additionalArguments.empty() )
			{
				for ( const auto & [name, value] : additionalArguments )
				{
					if ( value.empty() )
					{
						m_arguments.addSwitch(name);
					}
					else
					{
						m_arguments.addArgument(name, value);
					}
				}
			}

			Tracer::getInstance().earlySetup(m_arguments, m_processName, true);
		}
		else
		{
			std::cerr << ClassId << ", " << m_arguments.name() << " service failed to execute!";
		}
	}
#endif

	bool
	PrimaryServices::initialize () noexcept
	{
		if constexpr ( ThreadPoolDebugEnabled )
		{
			m_threadPool = std::make_shared< ThreadPool >(ThreadPoolDebugEnabledNumThreads);
		}
		else
		{
			m_threadPool = std::make_shared< ThreadPool >(std::thread::hardware_concurrency());
		}

		/* Initialize the file system to reach every useful directory. */
		if ( m_fileSystem.initialize(m_servicesEnabled) )
		{
			TraceSuccess{ClassId} << m_fileSystem.name() << " primary service up! [" << m_processName << "]";

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
					TraceWarning{ClassId} << "Unable to create captures directory " << directory << "!";
				}
			}
		}
		else
		{
			TraceFatal{ClassId} << m_fileSystem.name() << " primary service failed to execute! [" << m_processName << "!";

			return false;
		}

		/* Initialize core settings.
		 * NOTE: Settings class manages to write a default file. */
		if ( m_settings.initialize(m_servicesEnabled) )
		{
			/* NOTE: Now the core settings are initialized, we can update the tracer service configuration. */
			Tracer::getInstance().lateSetup(m_arguments, m_fileSystem, m_settings);

			TraceSuccess{ClassId} << m_settings.name() << " primary service up! [" << m_processName << "]";
		}
		else
		{
			TraceError{ClassId} <<
				m_fileSystem.name() << " primary service failed to execute! [" << m_processName << "]" "\n"
				"There is a problem to read or write the core settings file." "\n"
				"The engine will use the default configuration.";
		}

		return true;
	}

	void
	PrimaryServices::terminate () noexcept
	{
		if ( m_threadPool != nullptr )
		{
			m_threadPool->wait();
		}

		/* Terminate primary services. */
		for ( auto * service : std::ranges::reverse_view(m_servicesEnabled) )
		{
			if ( service->terminate() )
			{
				TraceSuccess{ClassId} << service->name() << " primary service terminated gracefully! [" << m_processName << "]";
			}
			else
			{
				TraceError{ClassId} << service->name() << " primary service failed to terminate properly! [" << m_processName << "]";
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
