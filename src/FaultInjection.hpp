/*
 * src/FaultInjection.hpp
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


#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <chrono>
#include <cstdint>
#include <string_view>

/* Forward declarations. */
namespace EmEn
{
	class Arguments;
}

/*
 * Startup fault injection (Ave Robustus II P3, owner decisions 2026-10-16): a TEST build (CMake option
 * EMERAUDE_FAULT_INJECTION, off by default, never a release) makes the startup of one named service fail, so every
 * failure path of the startup can be run in turn — each must exit cleanly, without a leak, a hang or a VUID.
 * docs/subsystems/source-tree/08-startup-fault-injection.md.
 */
namespace EmEn::FaultInjection
{
	/** @brief The command-line argument naming the service whose startup fails: `--inject-startup-failure=NAME`. */
	constexpr auto StartupFailureArgument{"--inject-startup-failure"};

	/** @brief The command-line switch making every swap-chain recreation fail for lack of video memory:
	 * `--inject-swap-chain-out-of-video-memory` (Graphics::Renderer::retryDegradedSwapChain()). Core degrades the
	 * swap-chain once, runtimeFailureDelay() after the main loop started. */
	constexpr auto SwapChainOutOfVideoMemoryArgument{"--inject-swap-chain-out-of-video-memory"};

	/** @brief The command-line switch making Core stop as on a lost graphics device (Core::stopOnDeviceLoss()),
	 * runtimeFailureDelay() after the main loop started: `--inject-device-loss`. */
	constexpr auto DeviceLossArgument{"--inject-device-loss"};

	/** @brief The command-line switch making Core stop as on a lost display connection (Core::stopOnDisplayLoss()),
	 * runtimeFailureDelay() after the main loop started: `--inject-display-loss`. */
	constexpr auto DisplayLossArgument{"--inject-display-loss"};

	/** @brief The command-line switch making the main submission of ONE frame fail as a refused vkQueueSubmit()
	 * (Graphics::Renderer::renderFrame()), runtimeFailureDelay() after the main loop started:
	 * `--inject-frame-submit-failure`. Nothing reaches the queue, as the specification states for a failed submission. */
	constexpr auto FrameSubmitFailureArgument{"--inject-frame-submit-failure"};

	/** @brief The command-line argument replacing the swap-chain image count the driver answered with N, at every
	 * swap-chain creation (Vulkan::SwapChain::prepareFrameData()): `--inject-swap-chain-image-count=N`. A driver may
	 * create more images than requested; this proves the refusal beyond the engine's capacity. */
	constexpr auto SwapChainImageCountArgument{"--inject-swap-chain-image-count"};

	/** @brief The delay of the runtime injections after the main loop started, in seconds:
	 * `--inject-runtime-failure-delay=N` (default DefaultRuntimeFailureDelay; time to load a scene before an emergency
	 * save is tested). */
	constexpr auto RuntimeFailureDelayArgument{"--inject-runtime-failure-delay"};

	/** @brief The delay of the runtime injections when the argument is absent. */
	constexpr std::chrono::seconds DefaultRuntimeFailureDelay{5};

	/**
	 * @brief Reads the argument once. Does nothing in a build without EMERAUDE_FAULT_INJECTION.
	 * @pre Main process, main thread, right after the arguments service and before any other service starts.
	 * @param arguments A reference to the arguments service.
	 */
	EMEN_API void configure (const Arguments & arguments) noexcept;

	/**
	 * @brief Returns whether the startup of this service is made to fail (the failure is traced). Always false in a
	 * build without EMERAUDE_FAULT_INJECTION.
	 * @note Read-only after configure(): any thread.
	 * @param serviceName The service's name (ServiceInterface::name()).
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool startupFailureInjected (std::string_view serviceName) noexcept;

	/**
	 * @brief Returns whether every swap-chain recreation is made to fail as if the video memory were exhausted (the
	 * renderer then gives up and Core stops with OutOfVideoMemoryExitCode). Always false in a build without
	 * EMERAUDE_FAULT_INJECTION.
	 * @note Read-only after configure(): any thread.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool swapChainOutOfVideoMemoryInjected () noexcept;

	/**
	 * @brief Returns whether Core stops as on a lost graphics device. Always false in a build without
	 * EMERAUDE_FAULT_INJECTION.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool deviceLossInjected () noexcept;

	/**
	 * @brief Returns whether Core stops as on a lost display connection. Always false in a build without
	 * EMERAUDE_FAULT_INJECTION.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool displayLossInjected () noexcept;

	/**
	 * @brief Returns whether one frame's main submission is made to fail. Always false in a build without
	 * EMERAUDE_FAULT_INJECTION.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool frameSubmitFailureInjected () noexcept;

	/**
	 * @brief Returns the swap-chain image count that replaces the driver's answer, 0 for none. Always 0 in a build
	 * without EMERAUDE_FAULT_INJECTION.
	 * @return uint32_t
	 */
	[[nodiscard]]
	EMEN_API uint32_t swapChainImageCountInjected () noexcept;

	/**
	 * @brief Returns the delay of the runtime injections after the main loop started.
	 * @return std::chrono::seconds
	 */
	[[nodiscard]]
	EMEN_API std::chrono::seconds runtimeFailureDelay () noexcept;
}
