/*
 * tools/stress/StartupFaultInjection.cpp
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


/* STL inclusions. */
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>

/* Local inclusions. */
#include "Core.hpp"
#include "PlatformManager.hpp"

/*
 * The startup fault-injection runner (Ave Robustus II, phase P3, 2026-10-16; built with EMERAUDE_FAULT_INJECTION only).
 * A WINDOWED, CEF-less engine starts every service, runs one second and stops. Given
 * `--inject-startup-failure=<service name>`, that service's startup fails on purpose: the run must then exit cleanly —
 * no leak (an ASan / LSan build), no hang, no VUID. Without the argument it is the reference run (every service up, a
 * clean exit). The sweep over every service name: tools/stress/startup-fault-sweep.sh, docs/subsystems/source-tree/08-startup-fault-injection.md.
 */
namespace
{
	using namespace EmEn;

	/* How long a started engine runs before it stops. */
	constexpr std::chrono::seconds RunDuration{1};

	class StartupFaultInjection final : public Core
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"StartupFaultInjection"};

			StartupFaultInjection (int argc, char * * argv) noexcept
				: Core{argc, argv, "EmeraudeStartupFaultInjection", {1, 0, 0}, "EmeraudeEngine", "emeraude-engine.org"}
			{

			}

			StartupFaultInjection (const StartupFaultInjection & copy) noexcept = delete;

			StartupFaultInjection (StartupFaultInjection && copy) noexcept = delete;

			StartupFaultInjection & operator= (const StartupFaultInjection & copy) noexcept = delete;

			StartupFaultInjection & operator= (StartupFaultInjection && copy) noexcept = delete;

			~StartupFaultInjection () override = default;

			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return Base::Hash::FNV1a(ClassId);
			}

			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == Base::Hash::FNV1a(ClassId);
			}

			/** @brief Whether the engine reached its main loop (every service up). */
			[[nodiscard]]
			bool
			started () const noexcept
			{
				return m_started;
			}

		private:

			bool
			onCoreStarted (const Arguments & /*arguments*/, Settings & /*settings*/) noexcept override
			{
				m_start = std::chrono::steady_clock::now();
				m_started = true;

				return true;
			}

			void
			onCoreMainLoopCycle () noexcept override
			{
				if ( m_stopping || std::chrono::steady_clock::now() - m_start < RunDuration )
				{
					return;
				}

				m_stopping = true;

				this->stop();
			}

			void
			onCoreProcessLogics (size_t /*engineCycle*/) noexcept override
			{

			}

			/* NOTE: An automated run never waits on the native refusal dialog: the refusal is printed instead. */
			[[nodiscard]]
			bool
			onStartupRefused (const StartupRefusal & refusal) noexcept override
			{
				std::cout << "[StartupFaultInjection] startup refusal told (no dialog): " << refusal.title << "\n";

				return true;
			}

			std::chrono::steady_clock::time_point m_start;
			bool m_started{false};
			bool m_stopping{false};
	};
}

int
main (int argc, char * * argv)
{
	/* NOTE: Before any thread, as every consumer's main() (PlatformManager::prepareVulkanLoaderEnvironment()). */
	static_cast< void >(EmEn::PlatformManager::prepareVulkanLoaderEnvironment());

	StartupFaultInjection runner{argc, argv};

	/* NOTE: A failure in the constructor (the base level): the consumer's rule, no run() (Core::startupFailed()). */
	if ( runner.startupFailed() )
	{
		std::cout << "[StartupFaultInjection] verdict: refused in the constructor (base level)." "\n";

		return EXIT_FAILURE;
	}

	const auto exitCode = runner.run();

	std::cout << "[StartupFaultInjection] verdict: " << ( runner.started() ? "started, then stopped" : "refused at the core level" ) << ", exit code " << exitCode << "." "\n";

	return exitCode;
}
