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
#include "emeraude_config.hpp" // IWYU pragma: keep (EMERAUDE_FAULT_INJECTION selects the code below)

/* STL inclusions. */
#include <atomic> // IWYU pragma: keep (the test build's failure count, EMERAUDE_FAULT_INJECTION)
#include <charconv> // IWYU pragma: keep (the test build's arguments, EMERAUDE_FAULT_INJECTION)
#include <string> // IWYU pragma: keep (the test build's injected service name, EMERAUDE_FAULT_INJECTION)

/* Local inclusions. */
#include "Arguments.hpp" // IWYU pragma: keep (the test build's arguments, EMERAUDE_FAULT_INJECTION)
#include "Tracer.hpp" // IWYU pragma: keep (the test build's traces, EMERAUDE_FAULT_INJECTION)

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

		/** @brief Whether the save of the pipeline cache dies as a driver crash would (same discipline). */
		[[nodiscard]]
		bool &
		pipelineCacheSaveCrash () noexcept
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

		/** @brief Whether one frame's main submission fails (same discipline). */
		[[nodiscard]]
		bool &
		frameSubmitFailure () noexcept
		{
			static bool injected{false};

			return injected;
		}

		/** @brief The swap-chain image count replacing the driver's answer, 0 for none (same discipline). */
		[[nodiscard]]
		uint32_t &
		swapChainImageCount () noexcept
		{
			static uint32_t count{0};

			return count;
		}

		/** @brief The shader whose compilation fails, empty for none (same discipline). */
		[[nodiscard]]
		std::string &
		injectedShader () noexcept
		{
			static std::string name;

			return name;
		}

		/** @brief The text a source must contain for its compilation to fail, empty for any source (same discipline). */
		[[nodiscard]]
		std::string &
		injectedShaderText () noexcept
		{
			static std::string text;

			return text;
		}

		/** @brief How many compilations fail, 0 for every one (same discipline). */
		[[nodiscard]]
		uint32_t &
		injectedShaderFailureLimit () noexcept
		{
			static uint32_t limit{0};

			return limit;
		}

		/** @brief The failures given so far: the compilations run on several threads. */
		[[nodiscard]]
		std::atomic< uint32_t > &
		injectedShaderFailures () noexcept
		{
			static std::atomic< uint32_t > count{0};

			return count;
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

		if ( arguments.isSwitchPresent(PipelineCacheSaveCrashArgument) )
		{
			pipelineCacheSaveCrash() = true;

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: the process will die while the pipeline cache is saved (" << PipelineCacheSaveCrashArgument << ").";
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

		if ( arguments.isSwitchPresent(FrameSubmitFailureArgument) )
		{
			frameSubmitFailure() = true;

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: the main submission of one frame will fail (" << FrameSubmitFailureArgument << ").";
		}

		if ( const auto value = arguments.get(SwapChainImageCountArgument); value.has_value() && !value->empty() )
		{
			uint32_t count = 0;

			if ( const auto [pointer, errorCode] = std::from_chars(value->data(), value->data() + value->size(), count); errorCode == std::errc{} && pointer == value->data() + value->size() )
			{
				swapChainImageCount() = count;

				TraceWarning{"FaultInjection"} << "FAULT INJECTION: every swap-chain will report " << count << " images (" << SwapChainImageCountArgument << ").";
			}
			else
			{
				TraceError{"FaultInjection"} << "FAULT INJECTION: '" << value.value() << "' is not an image count (" << SwapChainImageCountArgument << "): ignored.";
			}
		}

		if ( const auto value = arguments.get(ShaderCompilationFailureArgument); value.has_value() && !value->empty() )
		{
			injectedShader() = value.value();

			TraceWarning{"FaultInjection"} << "FAULT INJECTION: the compilation of the shader '" << injectedShader() << "' will fail on purpose (" << ShaderCompilationFailureArgument << ").";

			if ( const auto text = arguments.get(ShaderCompilationFailureContainingArgument); text.has_value() && !text->empty() )
			{
				injectedShaderText() = text.value();

				TraceWarning{"FaultInjection"} << "FAULT INJECTION: only for a source containing '" << injectedShaderText() << "' (" << ShaderCompilationFailureContainingArgument << ").";
			}

			if ( const auto count = arguments.get(ShaderCompilationFailureCountArgument); count.has_value() && !count->empty() )
			{
				uint32_t limit = 0;

				if ( const auto [pointer, errorCode] = std::from_chars(count->data(), count->data() + count->size(), limit); errorCode == std::errc{} && pointer == count->data() + count->size() )
				{
					injectedShaderFailureLimit() = limit;

					TraceWarning{"FaultInjection"} << "FAULT INJECTION: only for its first " << limit << " compilation(s) (" << ShaderCompilationFailureCountArgument << ").";
				}
				else
				{
					TraceError{"FaultInjection"} << "FAULT INJECTION: '" << count.value() << "' is not a compilation count (" << ShaderCompilationFailureCountArgument << "): every compilation fails.";
				}
			}
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
	pipelineCacheSaveCrashInjected () noexcept
	{
		return pipelineCacheSaveCrash();
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

	bool
	frameSubmitFailureInjected () noexcept
	{
		return frameSubmitFailure();
	}

	uint32_t
	swapChainImageCountInjected () noexcept
	{
		return swapChainImageCount();
	}

	bool
	shaderCompilationFailureInjected (std::string_view shaderName, std::string_view sourceCode) noexcept
	{
		if ( injectedShader().empty() || shaderName != injectedShader() )
		{
			return false;
		}

		if ( !injectedShaderText().empty() && sourceCode.find(injectedShaderText()) == std::string_view::npos )
		{
			return false;
		}

		const auto limit = injectedShaderFailureLimit();

		if ( limit == 0 )
		{
			return true;
		}

		/* NOTE: The first 'limit' matching compilations fail, whatever thread runs them. */
		return injectedShaderFailures().fetch_add(1, std::memory_order_relaxed) < limit;
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
	pipelineCacheSaveCrashInjected () noexcept
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

	bool
	frameSubmitFailureInjected () noexcept
	{
		return false;
	}

	uint32_t
	swapChainImageCountInjected () noexcept
	{
		return 0;
	}

	bool
	shaderCompilationFailureInjected (std::string_view /*shaderName*/, std::string_view /*sourceCode*/) noexcept
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
