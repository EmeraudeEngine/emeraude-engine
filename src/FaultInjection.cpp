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
#include <charconv>
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

		/**
		 * @brief Whether the swap-chain recreations fail for lack of video memory.
		 * @note Same discipline as injectedService(): written once by configure(), read-only afterwards.
		 * @return bool &
		 */
		[[nodiscard]]
		bool &
		swapChainOutOfVideoMemory () noexcept
		{
			static bool injected{false};

			return injected;
		}

		/** @brief Whether Core stops as on a lost device (same discipline). */
		[[nodiscard]]
		bool &
		deviceLoss () noexcept
		{
			static bool injected{false};

			return injected;
		}

		/** @brief Whether Core stops as on a lost display connection (same discipline). */
		[[nodiscard]]
		bool &
		displayLoss () noexcept
		{
			static bool injected{false};

			return injected;
		}

		/** @brief The delay of the runtime injections (same discipline). */
		[[nodiscard]]
		std::chrono::seconds &
		delay () noexcept
		{
			static std::chrono::seconds value{DefaultRuntimeFailureDelay};

			return value;
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

		if ( arguments.isSwitchPresent(SwapChainOutOfVideoMemoryArgument) )
		{
			swapChainOutOfVideoMemory() = true;

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: every swap-chain recreation will fail for lack of video memory (" << SwapChainOutOfVideoMemoryArgument << ").";
		}

		if ( arguments.isSwitchPresent(DeviceLossArgument) )
		{
			deviceLoss() = true;

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: the engine will stop as on a lost graphics device (" << DeviceLossArgument << ").";
		}

		if ( arguments.isSwitchPresent(DisplayLossArgument) )
		{
			displayLoss() = true;

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: the engine will stop as on a lost display connection (" << DisplayLossArgument << ").";
		}

		if ( const auto value = arguments.get(RuntimeFailureDelayArgument); value.has_value() && !value->empty() )
		{
			int seconds = 0;

			if ( const auto [pointer, errorCode] = std::from_chars(value->data(), value->data() + value->size(), seconds); errorCode == std::errc{} && pointer == value->data() + value->size() && seconds >= 0 )
			{
				delay() = std::chrono::seconds{seconds};
			}
			else
			{
				TraceError{"FaultInjection"} << "FAULT INJECTION: '" << value.value() << "' is not a delay in seconds (" << RuntimeFailureDelayArgument << "): " << DefaultRuntimeFailureDelay.count() << " s kept.";
			}
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

	bool
	swapChainOutOfVideoMemoryInjected () noexcept
	{
		return swapChainOutOfVideoMemory();
	}

	bool
	deviceLossInjected () noexcept
	{
		return deviceLoss();
	}

	bool
	displayLossInjected () noexcept
	{
		return displayLoss();
	}

	std::chrono::seconds
	runtimeFailureDelay () noexcept
	{
		return delay();
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

	bool
	swapChainOutOfVideoMemoryInjected () noexcept
	{
		return false;
	}

	bool
	deviceLossInjected () noexcept
	{
		return false;
	}

	bool
	displayLossInjected () noexcept
	{
		return false;
	}

	std::chrono::seconds
	runtimeFailureDelay () noexcept
	{
		return DefaultRuntimeFailureDelay;
	}
#endif
}
