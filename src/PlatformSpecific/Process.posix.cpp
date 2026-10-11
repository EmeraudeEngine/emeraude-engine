/*
 * src/PlatformSpecific/Process.posix.cpp
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
#include <cerrno>
#include <cstddef>
#include <string>
#include <system_error>
#include <vector>

/* Third-party inclusions. */
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#if IS_MACOS
	/* NOTE: A shared library reaches the environment through _NSGetEnviron() on macOS (no direct `environ` there). */
	#include <crt_externs.h>
#endif

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific
{
	namespace
	{
		constexpr auto ClassId{"Process"};

		/** @brief How many times an interrupted wait is retried: a bound, not a timeout (each retry follows a signal). */
		constexpr size_t MaxInterruptedWaits{1024};

		/**
		 * @brief Returns the environment handed to the child: the current process environment.
		 * @return char **
		 */
		[[nodiscard]]
		char **
		processEnvironment () noexcept
		{
#if IS_MACOS
			return *_NSGetEnviron();
#else
			return environ;
#endif
		}
	}

	std::optional< int >
	runProcessAndWait (const std::vector< std::string > & arguments) noexcept
	{
		if ( arguments.empty() || arguments.front().empty() )
		{
			TraceError{ClassId} << "No program to run !";

			return std::nullopt;
		}

		/* NOTE: posix_spawnp() takes `char * const argv[]`: mutable copies, never a cast of the caller's strings. */
		std::vector< std::string > storage{arguments};
		std::vector< char * > argv;
		argv.reserve(storage.size() + 1);

		for ( auto & argument : storage )
		{
			argv.push_back(argument.data());
		}

		argv.push_back(nullptr);

		pid_t child = 0;

		if ( const auto result = posix_spawnp(&child, argv.front(), nullptr, nullptr, argv.data(), processEnvironment()); result != 0 )
		{
			TraceError{ClassId} << "Unable to start '" << arguments.front() << "' : " << std::error_code{result, std::generic_category()}.message() << " !";

			return std::nullopt;
		}

		/* NOTE: A signal handled during the wait interrupts waitpid() with EINTR (reproc did not retry it: "Interrupted
		 * system call" on macOS, 2026-10-11, the child having run anyway). The wait is resumed, a bounded number of times. */
		int status = 0;

		for ( size_t attempt = 0; attempt < MaxInterruptedWaits; ++attempt )
		{
			if ( waitpid(child, &status, 0) == child )
			{
				if ( WIFEXITED(status) )
				{
					return WEXITSTATUS(status);
				}

				TraceError{ClassId} << "'" << arguments.front() << "' did not end normally (status " << status << ") !";

				return std::nullopt;
			}

			if ( errno != EINTR )
			{
				TraceError{ClassId} << "Unable to wait for '" << arguments.front() << "' : " << std::error_code{errno, std::generic_category()}.message() << " !";

				return std::nullopt;
			}
		}

		TraceError{ClassId} << "The wait for '" << arguments.front() << "' was interrupted " << MaxInterruptedWaits << " times : given up.";

		return std::nullopt;
	}
}
