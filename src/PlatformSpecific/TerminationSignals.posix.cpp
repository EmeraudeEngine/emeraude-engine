/*
 * src/PlatformSpecific/TerminationSignals.posix.cpp
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

#include "Helpers.hpp"

/* STL inclusions. */
#include <atomic>
#include <csignal>

/* Local inclusions. */
#include "Tracer.hpp"

namespace
{
	/* NOTE: The only state a signal handler may touch: a lock-free atomic (async-signal-safe). 0 = none pending. */
	std::atomic< int > s_pendingSignal{0};

	static_assert(std::atomic< int >::is_always_lock_free, "A signal handler may only store into a lock-free atomic.");
}

/* NOTE: C language linkage (a sigaction handler is a C function) with INTERNAL linkage (static): no exported symbol,
 * and no -Wmissing-prototypes under clang (an extern "C" function in an anonymous namespace still has an external C
 * name — Apple clang -Werror, macOS peer 2026-10-11). */
extern "C"
{
	/**
	 * @brief The signal handler: records the signal, nothing else.
	 * @param signalNumber The caught signal.
	 */
	static
	void
	onTerminationSignal (int signalNumber) noexcept
	{
		s_pendingSignal.store(signalNumber, std::memory_order_relaxed);
	}
}

namespace EmEn::PlatformSpecific
{
	namespace
	{
		constexpr auto ClassId{"TerminationSignalCatcher"};
	}

	TerminationSignalCatcher::TerminationSignalCatcher () noexcept
	{
		struct sigaction action{};
		action.sa_handler = onTerminationSignal;
		action.sa_flags = SA_RESTART;
		sigemptyset(&action.sa_mask);

		for ( auto & caughtSignal : m_signals )
		{
			if ( sigaction(caughtSignal.number, &action, &caughtSignal.previousAction) == 0 )
			{
				caughtSignal.installed = true;
			}
			else
			{
				TraceWarning{ClassId} << "Unable to catch the signal #" << caughtSignal.number << " : the system may end the process without a clean stop.";
			}
		}
	}

	TerminationSignalCatcher::~TerminationSignalCatcher ()
	{
		for ( const auto & caughtSignal : m_signals )
		{
			if ( caughtSignal.installed )
			{
				static_cast< void >(sigaction(caughtSignal.number, &caughtSignal.previousAction, nullptr));
			}
		}
	}

	const char *
	TerminationSignalCatcher::takePendingSignalName () noexcept
	{
		switch ( s_pendingSignal.exchange(0, std::memory_order_relaxed) )
		{
			case 0 :
				return nullptr;

			case SIGTERM :
				return "SIGTERM";

			case SIGHUP :
				return "SIGHUP";

			case SIGINT :
				return "SIGINT";

			default :
				return "a termination signal";
		}
	}
}
