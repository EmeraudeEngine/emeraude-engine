/*
 * src/Vulkan/Utility.hpp
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
#include <source_location>
#include <string>
#include <type_traits>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

namespace EmEn::Vulkan
{
	/**
	 * @brief Returns a Vulkan result code to a C-String.
	 * @param code The vulkan code.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_API const char * vkResultToCString (VkResult code) noexcept;

	/**
	 * @brief Returns what a Vulkan result code actually means for diagnosis, for the codes that are
	 * systematically misread, or nullptr when there is nothing worth adding.
	 * @note Exists because VK_ERROR_SURFACE_LOST_KHR keeps being investigated as a GPU fault
	 * alongside VK_ERROR_DEVICE_LOST, which sends the reader into the wrong subsystem entirely: the
	 * GPU is fine, the WINDOW is gone. On Wayland it is usually a compositor protocol error killing
	 * the wl_display connection, printed on stderr by libwayland itself just above — nothing the
	 * engine can catch or recover from, since every Wayland object of the process dies with it.
	 * @param code The vulkan code.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_API const char * vkResultDiagnosticHint (VkResult code) noexcept;

	/**
	 * @brief Gets the validation layers available from Vulkan in a string.
	 * @param validationLayers A reference to a validation layer list.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string getItemListAsString (const std::vector< VkLayerProperties > & validationLayers) noexcept;

	/**
	 * @brief Gets the extensions available from Vulkan in a string.
	 * @param type Which type of extensions.
	 * @param extensions A reference to an extension list.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string getItemListAsString (const char * type, const std::vector< VkExtensionProperties > & extensions) noexcept;

	class Device;

	/** @brief A pipeline creation that takes longer than this is reported (three frames at 60 Hz). */
	inline constexpr std::chrono::milliseconds SlowPipelineCreationThreshold{50};

	/**
	 * @brief Returns whether a device accepts VkPipelineCreationFeedbackCreateInfo (core in Vulkan 1.3): whether the
	 * pipeline cache served a creation.
	 * @param device A reference to the device.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool pipelineCreationFeedbackAvailable (const Device & device) noexcept;

	/**
	 * @brief Reports a slow pipeline creation: its duration, whether the pipeline cache served it, the creating
	 * thread (its native ID, printed in the line). A runtime compile is a stall on its thread — hundreds of ms under MoltenVK
	 * (SPIR-V to MSL, then a Metal library) — and was invisible until 2026-10-03.
	 * @param classId The pipeline class.
	 * @param label The pipeline's label (empty: reported as unlabelled).
	 * @param elapsed The creation's duration.
	 * @param feedback The creation feedback, nullptr when unavailable.
	 * @param location The caller's location (the creation site, not this function).
	 */
	EMEN_API void reportPipelineCreation (const char * classId, const std::string & label, std::chrono::steady_clock::duration elapsed, const VkPipelineCreationFeedback * feedback, const std::source_location & location = std::source_location::current()) noexcept;

	/**
	 * @brief The outcome of a two-call Vulkan enumeration (enumerate()).
	 */
	struct EnumerationResult final
	{
		/** @brief The last answer of the implementation: VK_SUCCESS, VK_INCOMPLETE (valid data, maybe not all) or an error. */
		VkResult result{VK_SUCCESS};
		/** @brief How many times the list changed between the count and the fill (VK_INCOMPLETE answers). */
		uint32_t retries{0};

		/**
		 * @brief Returns whether the enumeration produced valid data: any non-negative VkResult (VK_INCOMPLETE is a
		 * success code).
		 * @return bool
		 */
		[[nodiscard]]
		bool
		succeeded () const noexcept
		{
			return result >= 0;
		}
	};

	/**
	 * @brief The two-call enumeration of the Vulkan specification: query the count, size the array, fill it; repeated
	 * while the implementation answers VK_INCOMPLETE (the list grew between the two calls — a hybrid laptop whose
	 * discrete GPU appears late, a loader whose ICDs answer differently), up to a bounded number of attempts.
	 * @note VK_INCOMPLETE is a SUCCESS code (Vulkan specification § Return Codes): the data written is valid. A final
	 * VK_INCOMPLETE keeps the last answer; only a negative VkResult is a failure, and then the output is empty. Measured:
	 * a Windows AMD laptop answered VK_INCOMPLETE to the COUNT query itself (engine item
	 * physical-device-enumeration-refuses-vk-incomplete, 2026-10-15) — its count is used all the same.
	 * @tparam element_t The element type.
	 * @tparam query_t A callable (uint32_t * count, element_t * data) -> VkResult wrapping the Vulkan call.
	 * @param output The array filled (cleared first).
	 * @param query The Vulkan call.
	 * @param prototype The value every element starts from (a structure's sType / pNext, VUID-...-sType-sType).
	 * @return EnumerationResult
	 */
	template< typename element_t, typename query_t >
	requires std::is_invocable_r_v< VkResult, query_t &, uint32_t *, element_t * >
	[[nodiscard]]
	EnumerationResult
	enumerate (std::vector< element_t > & output, query_t && query, const element_t & prototype = element_t{}) noexcept
	{
		/* NOTE: The list changes only when a device or an ICD appears: a few attempts settle it. */
		constexpr uint32_t MaxAttempts{8};

		EnumerationResult outcome{.result = VK_INCOMPLETE, .retries = 0};

		output.clear();

		for ( uint32_t attempt = 0; attempt < MaxAttempts; ++attempt )
		{
			uint32_t count = 0;

			/* NOTE: A typed null pointer: the query may be a generic lambda. */
			outcome.result = query(&count, static_cast< element_t * >(nullptr));

			if ( outcome.result < 0 )
			{
				return outcome;
			}

			if ( count == 0 )
			{
				output.clear();
				outcome.result = VK_SUCCESS;

				return outcome;
			}

			output.assign(count, prototype);

			outcome.result = query(&count, output.data());

			if ( outcome.result < 0 )
			{
				output.clear();

				return outcome;
			}

			/* NOTE: The implementation wrote `count` elements (never more than the array holds). */
			output.resize(count, prototype);

			if ( outcome.result != VK_INCOMPLETE )
			{
				return outcome;
			}

			outcome.retries = attempt + 1;
		}

		/* NOTE: Still VK_INCOMPLETE after every attempt: the last answer is kept (valid, maybe not complete). */
		return outcome;
	}
}
