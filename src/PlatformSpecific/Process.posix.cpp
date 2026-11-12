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

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <optional>
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
#include "BlobTrait.hpp"
#include "IO/IO.hpp"
#include "Network/SocketInheritance.hpp"
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

		/** @brief The most output kept from a captured child (the rest is read and dropped, so the child never blocks). */
		constexpr size_t MaxCapturedOutputBytes{size_t{1024} * 1024};

		/** @name The CHILD's standard descriptors (POSIX numbers: the spawn configuration names them, nothing is written). @{ */
#if IS_MACOS
		constexpr int ChildInput{0};
#endif
		constexpr int ChildOutput{1};
		constexpr int ChildError{2};
		/** @} */

		/**
		 * @brief The spawn configuration of a child (RAII over posix_spawn_file_actions_t and posix_spawnattr_t): the child
		 * keeps its standard streams (stdout optionally redirected) and NOTHING ELSE of this process — no socket, no pipe,
		 * no file: a dialog or a desktop command started from here must not hold the engine's server ports after the
		 * engine ended (measured 2026-11-11).
		 * @note Linux: posix_spawn_file_actions_addclosefrom_np() (glibc 2.34). macOS: POSIX_SPAWN_CLOEXEC_DEFAULT with
		 * the three standard streams inherited explicitly.
		 */
		class SpawnSetup final
		{
			public:

				/** @brief Prepares an empty configuration; ready() tells whether it can be used. */
				SpawnSetup () noexcept
					: m_actionsReady{posix_spawn_file_actions_init(&m_actions) == 0},
					m_attributesReady{posix_spawnattr_init(&m_attributes) == 0}
				{

				}

				SpawnSetup (const SpawnSetup &) = delete;
				SpawnSetup (SpawnSetup &&) = delete;
				SpawnSetup & operator= (const SpawnSetup &) = delete;
				SpawnSetup & operator= (SpawnSetup &&) = delete;

				/** @brief Releases the configuration. */
				~SpawnSetup ()
				{
					if ( m_attributesReady )
					{
						posix_spawnattr_destroy(&m_attributes);
					}

					if ( m_actionsReady )
					{
						posix_spawn_file_actions_destroy(&m_actions);
					}
				}

				/**
				 * @brief Makes the child's stdout the given descriptor (a pipe's write end).
				 * @param descriptor The descriptor.
				 * @return bool
				 */
				[[nodiscard]]
				bool
				redirectStandardOutput (int descriptor) noexcept
				{
					return m_actionsReady && posix_spawn_file_actions_adddup2(&m_actions, descriptor, ChildOutput) == 0;
				}

				/**
				 * @brief Closes in the child every descriptor above the standard streams. Call it LAST (after any redirection).
				 * @return bool
				 */
				[[nodiscard]]
				bool
				closeEverythingElse () noexcept
				{
					if ( !m_actionsReady || !m_attributesReady )
					{
						return false;
					}
#if IS_MACOS
					return
						posix_spawnattr_setflags(&m_attributes, POSIX_SPAWN_CLOEXEC_DEFAULT) == 0 &&
						posix_spawn_file_actions_addinherit_np(&m_actions, ChildInput) == 0 &&
						posix_spawn_file_actions_addinherit_np(&m_actions, ChildOutput) == 0 &&
						posix_spawn_file_actions_addinherit_np(&m_actions, ChildError) == 0;
#else
					return posix_spawn_file_actions_addclosefrom_np(&m_actions, ChildError + 1) == 0;
#endif
				}

				/** @brief Returns the file actions for posix_spawnp(). */
				[[nodiscard]]
				const posix_spawn_file_actions_t *
				actions () const noexcept
				{
					return &m_actions;
				}

				/** @brief Returns the attributes for posix_spawnp(). */
				[[nodiscard]]
				const posix_spawnattr_t *
				attributes () const noexcept
				{
					return &m_attributes;
				}

			private:

				posix_spawn_file_actions_t m_actions{};
				posix_spawnattr_t m_attributes{};
				bool m_actionsReady{false};
				bool m_attributesReady{false};
		};

		/**
		 * @brief Starts a program (looked up in PATH) with the given configuration.
		 * @param arguments The program then its arguments (not empty).
		 * @param setup The spawn configuration.
		 * @return std::optional< pid_t > The child, std::nullopt when it could not be started (logged).
		 */
		[[nodiscard]]
		std::optional< pid_t >
		spawn (const std::vector< std::string > & arguments, const SpawnSetup & setup) noexcept
		{
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

			if ( const auto result = posix_spawnp(&child, argv.front(), setup.actions(), setup.attributes(), argv.data(), processEnvironment()); result != 0 )
			{
				TraceError{ClassId} << "Unable to start '" << arguments.front() << "' : " << Base::IO::systemErrorText(std::error_code{result, std::generic_category()}) << " !";

				return std::nullopt;
			}

			return child;
		}

		/**
		 * @brief Waits for a child's end, resumed when a signal interrupts the wait (EINTR).
		 * @param child The child.
		 * @param name The program, for the traces.
		 * @return std::optional< int > The exit code, std::nullopt when it did not end normally (logged).
		 */
		[[nodiscard]]
		std::optional< int >
		waitChild (pid_t child, const std::string & name) noexcept
		{
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

					TraceError{ClassId} << "'" << name << "' did not end normally (status " << status << ") !";

					return std::nullopt;
				}

				if ( errno != EINTR )
				{
					TraceError{ClassId} << "Unable to wait for '" << name << "' : " << Base::IO::systemErrorText(std::error_code{errno, std::generic_category()}) << " !";

					return std::nullopt;
				}
			}

			TraceError{ClassId} << "The wait for '" << name << "' was interrupted " << MaxInterruptedWaits << " times : given up.";

			return std::nullopt;
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

		SpawnSetup setup;

		if ( !setup.closeEverythingElse() )
		{
			TraceError{ClassId} << "Unable to prepare the start of '" << arguments.front() << "' !";

			return std::nullopt;
		}

		const auto child = spawn(arguments, setup);

		if ( !child.has_value() )
		{
			return std::nullopt;
		}

		return waitChild(child.value(), arguments.front());
	}

	std::optional< int >
	runProcessCapturingOutput (const std::vector< std::string > & arguments, std::string & output) noexcept
	{
		output.clear();

		if ( arguments.empty() || arguments.front().empty() )
		{
			TraceError{ClassId} << "No program to run !";

			return std::nullopt;
		}

		std::array< int, 2 > pipeEnds{-1, -1};

		if ( ::pipe(pipeEnds.data()) != 0 )
		{
			TraceError{ClassId} << "Unable to create the output pipe of '" << arguments.front() << "' : " << Base::IO::systemErrorText(std::error_code{errno, std::generic_category()}) << " !";

			return std::nullopt;
		}

		const auto [readEnd, writeEnd] = pipeEnds;

		/* NOTE: Both ends private to this process: another child started meanwhile must not hold this pipe open. */
		static_cast< void >(Base::Network::preventHandleInheritance(static_cast< std::uintptr_t >(readEnd)));
		static_cast< void >(Base::Network::preventHandleInheritance(static_cast< std::uintptr_t >(writeEnd)));

		std::optional< pid_t > child;

		{
			SpawnSetup setup;

			if ( setup.redirectStandardOutput(writeEnd) && setup.closeEverythingElse() )
			{
				child = spawn(arguments, setup);
			}
			else
			{
				TraceError{ClassId} << "Unable to prepare the start of '" << arguments.front() << "' !";
			}
		}

		/* The parent never writes: the read below ends when the child's copy of the write end is closed. */
		::close(writeEnd);

		if ( !child.has_value() )
		{
			::close(readEnd);

			return std::nullopt;
		}

		std::array< char, 4096 > buffer{};

		for ( ;; )
		{
			const auto bytes = ::read(readEnd, buffer.data(), buffer.size());

			if ( bytes > 0 )
			{
				const auto kept = std::min(static_cast< size_t >(bytes), MaxCapturedOutputBytes - std::min(output.size(), MaxCapturedOutputBytes));

				output.append(buffer.data(), kept);

				continue;
			}

			if ( bytes < 0 && errno == EINTR )
			{
				continue;
			}

			break;
		}

		::close(readEnd);

		return waitChild(child.value(), arguments.front());
	}
}
