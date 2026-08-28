/*
 * src/PrimaryServices.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "PrimaryServices.hpp"

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <iostream>
#include <ranges>
#include <sstream>
#include <thread>
#include <utility>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "ThreadPool.hpp"
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "Identification.hpp"
#include "Net/APIClient.hpp"
#include "Net/Manager.hpp"
#include "PlatformSpecific/SystemInfo.hpp"
#include "PlatformSpecific/UserInfo.hpp"
#include "ServiceInterface.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Base;

	/**
	 * @brief Private implementation holding the concrete primary services (pimpl).
	 * @note The method bodies below are kept identical to the former non-pimpl
	 * implementation; only the qualified names change (PrimaryServices:: -> PrimaryServices::Impl::).
	 */
	class PrimaryServices::Impl final
	{
		public:

			Impl (int argc, char * * argv, const Identification & identification) noexcept;

			Impl (int argc, char * * argv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept;

#if IS_WINDOWS
			Impl (int argc, wchar_t * * wargv, const Identification & identification) noexcept;

			Impl (int argc, wchar_t * * wargv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept;
#endif

			[[nodiscard]]
			bool initialize () noexcept;

			void terminate () noexcept;

			[[nodiscard]]
			std::string information () const noexcept;

			std::string m_processName;
			Arguments m_arguments;
			PlatformSpecific::UserInfo m_userInfo;
			FileSystem m_fileSystem;
			Settings m_settings;
			PlatformSpecific::SystemInfo m_systemInfo{m_arguments, m_settings};
			std::shared_ptr< Base::ThreadPool > m_threadPool;
			Net::Manager m_networkManager{m_fileSystem, m_settings, m_threadPool};
			Net::APIClient m_apiClient{m_settings, m_threadPool};
			std::vector< ServiceInterface * > m_servicesEnabled;
			bool m_childProcess{false};
			bool m_showInformation{false};
	};

	PrimaryServices::Impl::Impl (int argc, char * * argv, const Identification & identification) noexcept
		: m_processName{"main"},
		m_arguments{argc, argv, false},
		m_fileSystem{m_arguments, m_userInfo, identification, false},
		m_settings{m_arguments, m_fileSystem, Base::to_string(identification.applicationVersion()), false}
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

	PrimaryServices::Impl::Impl (int argc, char * * argv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept
		: m_processName{std::move(processName)},
		m_arguments{argc, argv, true},
		m_fileSystem{m_arguments, m_userInfo, identification, true},
		m_settings{m_arguments, m_fileSystem, Base::to_string(identification.applicationVersion()), true},
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
	PrimaryServices::Impl::Impl (int argc, wchar_t * * wargv, const Identification & identification) noexcept
		: m_processName{"main"},
		m_arguments{argc, wargv, false},
		m_fileSystem{m_arguments, m_userInfo, identification, false},
		m_settings{m_arguments, m_fileSystem, Base::to_string(identification.applicationVersion()), false}
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

	PrimaryServices::Impl::Impl (int argc, wchar_t * * wargv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept
		: m_processName{std::move(processName)},
		m_arguments{argc, wargv, true},
		m_fileSystem{m_arguments, m_userInfo, identification, true},
		m_settings{m_arguments, m_fileSystem, Base::to_string(identification.applicationVersion()), true},
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

	PrimaryServices::PrimaryServices (int argc, char * * argv, const Identification & identification) noexcept
		: m_impl{std::make_unique< Impl >(argc, argv, identification)}
	{

	}

	PrimaryServices::PrimaryServices (int argc, char * * argv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept
		: m_impl{std::make_unique< Impl >(argc, argv, identification, std::move(processName), additionalArguments)}
	{

	}

#if IS_WINDOWS
	PrimaryServices::PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification) noexcept
		: m_impl{std::make_unique< Impl >(argc, wargv, identification)}
	{

	}

	PrimaryServices::PrimaryServices (int argc, wchar_t * * wargv, const Identification & identification, std::string processName, const std::vector< std::pair< std::string, std::string > > & additionalArguments) noexcept
		: m_impl{std::make_unique< Impl >(argc, wargv, identification, std::move(processName), additionalArguments)}
	{

	}
#endif

	/* NOTE: Must be defined here, where Impl is a complete type (std::unique_ptr requirement). */
	PrimaryServices::~PrimaryServices () = default;

	bool
	PrimaryServices::Impl::initialize () noexcept
	{
		if ( m_userInfo.initialize(m_servicesEnabled) )
		{
			TraceSuccess{ClassId} << m_userInfo.name() << " primary service up! [" << m_processName << "]";
		}
		else
		{
			TraceFatal{ClassId} << m_userInfo.name() << " primary service failed to execute! [" << m_processName << "!";

			return false;
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

		if ( m_systemInfo.initialize(m_servicesEnabled) )
		{
			TraceSuccess{ClassId} << m_systemInfo.name() << " primary service up! [" << m_processName << "]";
		}
		else
		{
			TraceFatal{ClassId} << m_systemInfo.name() << " primary service failed to execute! [" << m_processName << "!";

			return false;
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			m_threadPool = std::make_shared< ThreadPool >(ThreadPoolDebugEnabledNumThreads);
		}
		else
		{
			m_threadPool = std::make_shared< ThreadPool >(std::thread::hardware_concurrency());
		}

		if ( m_apiClient.initialize(m_servicesEnabled) )
		{
			TraceSuccess{ClassId} << m_apiClient.name() << " primary service up! [" << m_processName << "]";
		}
		else
		{
			TraceFatal{ClassId} << m_apiClient.name() << " primary service failed to execute! [" << m_processName << "!";

			return false;
		}

		if ( m_networkManager.initialize(m_servicesEnabled) )
		{
			TraceSuccess{ClassId} << m_networkManager.name() << " primary service up! [" << m_processName << "]";
		}
		else
		{
			TraceFatal{ClassId} << m_networkManager.name() << " primary service failed to execute! [" << m_processName << "!";

			return false;
		}

		return true;
	}

	bool
	PrimaryServices::initialize () noexcept
	{
		return m_impl->initialize();
	}

	void
	PrimaryServices::Impl::terminate () noexcept
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

	void
	PrimaryServices::terminate () noexcept
	{
		m_impl->terminate();
	}

	bool
	PrimaryServices::isChildProcess () const noexcept
	{
		return m_impl->m_childProcess;
	}

	std::shared_ptr< Base::ThreadPool >
	PrimaryServices::threadPool () const noexcept
	{
		return m_impl->m_threadPool;
	}

	const PlatformSpecific::SystemInfo &
	PrimaryServices::systemInfo () const noexcept
	{
		return m_impl->m_systemInfo;
	}

	const PlatformSpecific::UserInfo &
	PrimaryServices::userInfo () const noexcept
	{
		return m_impl->m_userInfo;
	}

	Arguments &
	PrimaryServices::arguments () noexcept
	{
		return m_impl->m_arguments;
	}

	const Arguments &
	PrimaryServices::arguments () const noexcept
	{
		return m_impl->m_arguments;
	}

	FileSystem &
	PrimaryServices::fileSystem () noexcept
	{
		return m_impl->m_fileSystem;
	}

	const FileSystem &
	PrimaryServices::fileSystem () const noexcept
	{
		return m_impl->m_fileSystem;
	}

	Settings &
	PrimaryServices::settings () noexcept
	{
		return m_impl->m_settings;
	}

	const Settings &
	PrimaryServices::settings () const noexcept
	{
		return m_impl->m_settings;
	}

	Net::Manager &
	PrimaryServices::netManager () noexcept
	{
		return m_impl->m_networkManager;
	}

	const Net::Manager &
	PrimaryServices::netManager () const noexcept
	{
		return m_impl->m_networkManager;
	}

	Net::APIClient &
	PrimaryServices::apiClient () noexcept
	{
		return m_impl->m_apiClient;
	}

	const Net::APIClient &
	PrimaryServices::apiClient () const noexcept
	{
		return m_impl->m_apiClient;
	}

	std::string
	PrimaryServices::Impl::information () const noexcept
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

	std::string
	PrimaryServices::information () const noexcept
	{
		return m_impl->information();
	}
}
