/*
 * src/FaultInjection.cpp
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


#include "FaultInjection.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions. */
#include "Arguments.hpp"
#include "Tracer.hpp"

namespace EmEn::FaultInjection
{
#ifdef EMERAUDE_FAULT_INJECTION
	namespace
	{
		/**
		 * @brief The service whose startup fails (empty: none).
		 * @note Written once by configure() before any service but the arguments starts (main thread), read-only
		 * afterwards: no lock. Test builds only.
		 * @return std::string &
		 */
		[[nodiscard]]
		std::string &
		injectedService () noexcept
		{
			static std::string name;

			return name;
		}
	}

	void
	configure (const Arguments & arguments) noexcept
	{
		if ( const auto value = arguments.get(StartupFailureArgument); value.has_value() && !value->empty() )
		{
			injectedService() = value.value();

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: the startup of the service '" << injectedService() << "' will fail on purpose (" << StartupFailureArgument << ").";
		}
	}

	bool
	startupFailureInjected (std::string_view serviceName) noexcept
	{
		if ( injectedService().empty() || serviceName != injectedService() )
		{
			return false;
		}

		TraceError{"FaultInjection"} << "FAULT INJECTION: the startup of the service '" << serviceName << "' fails on purpose.";

		return true;
	}
#else
	/* NOTE: A normal build: no injection, nothing stored (the configuration selects the code, emeraude_config.hpp). */
	void
	configure (const Arguments & /*arguments*/) noexcept
	{
	}

	bool
	startupFailureInjected (std::string_view /*serviceName*/) noexcept
	{
		return false;
	}
#endif
}
