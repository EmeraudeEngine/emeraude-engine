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
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

/* Local inclusions. */
#include "Arguments.hpp"
#include "Core.hpp"
#include "FaultInjection.hpp"
#include "Graphics/Renderer.hpp"
#include "PlatformManager.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DeviceMemory.hpp"
#include "Window.hpp"

/*
 * The startup fault-injection runner (Ave Robustus II, phase P3, 2026-10-16; built with EMERAUDE_FAULT_INJECTION only).
 * A WINDOWED, CEF-less engine starts every service, runs one second and stops. Given
 * `--inject-startup-failure=<service name>`, that service's startup fails on purpose: the run must then exit cleanly —
 * no leak (an ASan / LSan build), no hang, no VUID. Without the argument it is the reference run (every service up, a
 * clean exit). The sweep over every service name: tools/stress/startup-fault-sweep.sh, docs/subsystems/source-tree/08-startup-fault-injection.md.
 *
 * Two runtime modes stop on a swap-chain given up for lack of video memory (exit code Core::OutOfVideoMemoryExitCode,
 * docs/subsystems/vulkan/24-swap-chain-given-up-for-lack-of-video-memory.md):
 * - `--inject-swap-chain-out-of-video-memory` (FaultInjection): every recreation fails as a full video memory makes it
 *   fail — deterministic, the message printed instead of the dialog;
 * - `--eat-video-memory`: the real test — the runner itself fills the video memory with device-local allocations of
 *   decreasing size until the driver refuses them, then GROWS the window until a swap-chain recreation fails; the
 *   native dialog shows. ⚠️ It fills the GPU of the machine it runs on (freed at the stop). Its allocations go through
 *   the engine's Vulkan::DeviceMemory, so their refusal is recorded like a refused import of the application (the
 *   production case). Measured 2026-11-10: at a CONSTANT window size every recreation succeeds with the video memory
 *   full — recreate() frees the attachments first and takes the same room back — so only a recreation that needs more
 *   than it frees can fail (a maximise, a bigger monitor; in production, another process taking the freed room).
 */
namespace
{
	using namespace EmEn;

	/* How long a started engine runs before it stops. */
	constexpr std::chrono::seconds RunDuration{1};

	/* The runtime video-memory modes: how long the swap-chain may hold before the runner stops by itself. */
	constexpr std::chrono::seconds VideoMemoryRunLimit{60};

	/* The real test: the first and the last size of the device-local allocations filling the video memory. */
	constexpr VkDeviceSize FirstEatenChunk{256ULL * 1024ULL * 1024ULL};
	constexpr VkDeviceSize LastEatenChunk{16ULL * 1024ULL * 1024ULL};

	/* The real test: how often the window grows (by a quarter) once the video memory is full. */
	constexpr std::chrono::milliseconds DegradeInterval{500};

	/* The real test: the largest window side asked for. ⚠️ Measured 2026-11-10 (Linux, KDE Wayland, libdecor GTK plugin):
	 * a window grown to 11441x6416 crashed the process inside the decoration plugin ("creating a buffer file for
	 * -1527818496 B failed", then SIGSEGV) — 9153x5133 was fine. */
	constexpr uint32_t LargestWindowSide{8192};

	/* The real test's switch. */
	constexpr auto EatVideoMemoryArgument{"--eat-video-memory"};

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
			onCoreStarted (const Arguments & arguments, Settings & /*settings*/) noexcept override
			{
				m_start = std::chrono::steady_clock::now();
				m_started = true;
				m_eatVideoMemory = arguments.isSwitchPresent(EatVideoMemoryArgument);
				m_injectVideoMemory = FaultInjection::swapChainOutOfVideoMemoryInjected();

				return true;
			}

			void
			onCoreMainLoopCycle () noexcept override
			{
				if ( m_stopping )
				{
					return;
				}

				const auto elapsed = std::chrono::steady_clock::now() - m_start;

				if ( !m_eatVideoMemory && !m_injectVideoMemory )
				{
					if ( elapsed >= RunDuration )
					{
						m_stopping = true;

						this->stop();
					}

					return;
				}

				/* NOTE: The runtime modes end with the engine's own stop (exit code 95); this is the bound when the
				 * swap-chain holds. */
				if ( elapsed >= VideoMemoryRunLimit )
				{
					std::cout << "[StartupFaultInjection] the swap-chain held for " << VideoMemoryRunLimit.count() << " s (" << ( m_eatenBytes >> 20 ) << " MiB eaten): stopping." "\n";

					m_stopping = true;

					this->stop();

					return;
				}

				if ( m_eatVideoMemory && !m_videoMemoryFull )
				{
					this->eatVideoMemory();

					return;
				}

				/* NOTE: The injection needs one degradation; the real test grows the window while the memory is full, so
				 * each recreation needs more than it frees (a recreation that succeeds ends the episode, the next one starts
				 * it again). */
				if ( const auto now = std::chrono::steady_clock::now(); now - m_lastDegrade >= DegradeInterval && (m_eatVideoMemory || m_lastDegrade == std::chrono::steady_clock::time_point{}) )
				{
					m_lastDegrade = now;

					if ( m_eatVideoMemory )
					{
						const auto size = this->window().getSize();
						const auto width = std::min(size[0] + size[0] / 4, LargestWindowSide);
						const auto height = std::min(size[1] + size[1] / 4, LargestWindowSide);

						std::cout << "[StartupFaultInjection] the window grows from " << size[0] << "x" << size[1] << " to " << width << "x" << height << "." "\n";

						this->printMemoryHeaps();

						static_cast< void >(this->window().resize(static_cast< int32_t >(width), static_cast< int32_t >(height)));
					}

					this->graphicsRenderer().setSwapChainDegraded();
				}
			}

			/* NOTE: The memory eaten by the real test goes back before the device does. */
			[[nodiscard]]
			bool
			onBeforeCoreStop () noexcept override
			{
				this->releaseVideoMemory();

				return true;
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

			/* NOTE: The injection is automated: the message is printed. The real test shows the engine's native dialog. */
			[[nodiscard]]
			bool
			onGraphicsFailure (const GraphicsFailure & failure) noexcept override
			{
				std::cout << "[StartupFaultInjection] graphics failure told" << ( m_eatVideoMemory ? "" : " (no dialog)" ) << ": " << failure.title << " — " << failure.message << "\n";

				/* NOTE: The eaten memory goes back now: the native dialog needs none, but the desktop does. */
				this->releaseVideoMemory();

				return !m_eatVideoMemory;
			}

			/**
			 * @brief Takes one more device-local allocation; past a refusal, the next ones are four times smaller, down to
			 * LastEatenChunk.
			 */
			void
			eatVideoMemory () noexcept
			{
				const auto & device = this->graphicsRenderer().device();

				if ( device == nullptr )
				{
					m_videoMemoryFull = true;

					return;
				}

				VkMemoryRequirements2 requirement{};
				requirement.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
				requirement.pNext = nullptr;
				requirement.memoryRequirements.size = m_eatenChunk;
				requirement.memoryRequirements.alignment = 1;
				requirement.memoryRequirements.memoryTypeBits = ~0U;

				auto memory = std::make_unique< Vulkan::DeviceMemory >(device, requirement, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				if ( memory->createOnHardware() )
				{
					m_eatenMemories.emplace_back(std::move(memory));
					m_eatenBytes += m_eatenChunk;

					return;
				}

				std::cout << "[StartupFaultInjection] a device-local allocation of " << ( m_eatenChunk >> 20 ) << " MiB refused after " << ( m_eatenBytes >> 20 ) << " MiB eaten." "\n";

				if ( m_eatenChunk > LastEatenChunk )
				{
					m_eatenChunk /= 4;

					return;
				}

				m_videoMemoryFull = true;

				std::cout << "[StartupFaultInjection] the video memory is full: " << ( m_eatenBytes >> 20 ) << " MiB eaten in " << m_eatenMemories.size() << " allocations; the swap-chain is now recreated every " << DegradeInterval.count() << " ms." "\n";
			}

			/** @brief Prints every memory heap: where the engine's allocations went while the video memory is full. */
			void
			printMemoryHeaps () noexcept
			{
				const auto & device = this->graphicsRenderer().device();

				if ( device == nullptr )
				{
					return;
				}

				for ( const auto & heap : device->memoryBudgets() )
				{
					std::cout << "[StartupFaultInjection]   heap " << heap.heapIndex << ( heap.deviceLocal ? " (device-local)" : " (system)" ) <<
						": size " << ( heap.size >> 20 ) << " MiB, budget " << ( heap.budget >> 20 ) << " MiB, process usage " << ( heap.usage >> 20 ) <<
						" MiB, VMA allocations " << ( heap.allocationBytes >> 20 ) << " MiB in " << ( heap.blockBytes >> 20 ) << " MiB of blocks." "\n";
				}
			}

			/** @brief Gives the eaten video memory back (idempotent). */
			void
			releaseVideoMemory () noexcept
			{
				if ( m_eatenMemories.empty() )
				{
					return;
				}

				/* NOTE: Each DeviceMemory frees its allocation when destroyed. */
				std::cout << "[StartupFaultInjection] " << ( m_eatenBytes >> 20 ) << " MiB of video memory given back." "\n";

				m_eatenMemories.clear();
				m_eatenBytes = 0;
			}

			std::vector< std::unique_ptr< Vulkan::DeviceMemory > > m_eatenMemories;
			std::chrono::steady_clock::time_point m_start;
			std::chrono::steady_clock::time_point m_lastDegrade;
			VkDeviceSize m_eatenChunk{FirstEatenChunk};
			uint64_t m_eatenBytes{0};
			bool m_started{false};
			bool m_stopping{false};
			bool m_eatVideoMemory{false};
			bool m_injectVideoMemory{false};
			bool m_videoMemoryFull{false};
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
