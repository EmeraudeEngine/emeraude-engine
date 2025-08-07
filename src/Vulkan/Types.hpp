/*
 * src/Vulkan/Types.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>

namespace EmEn::Vulkan
{
	/**
	 * @brief The GPU work type enumeration.
	 */
	enum class GPUWorkType : uint8_t
	{
		Graphics = 0,
		Physics = 1
	};

	/**
	 * @brief The RunningMode enum
	 */
	enum class RunningMode : uint8_t
	{
		Performance,
		PowerSaving,
		DontCare
	};

	/**
	 * @brief This enumeration describe the purpose of
	 * a queue from the engine point of view.
	 */
	enum class QueueJob : uint8_t
	{
		Graphics,
		GraphicsTransfer,
		Presentation,
		Compute,
		ComputeTransfer,
		Transfer
	};

	static constexpr auto GraphicsString{"Graphics"};
	static constexpr auto GraphicsTransferString{"GraphicsTransfer"};
	static constexpr auto PresentationString{"Presentation"};
	static constexpr auto ComputeString{"Compute"};
	static constexpr auto ComputeTransferString{"ComputeTransfer"};
	static constexpr auto TransferString{"Transfer"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (QueueJob value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (QueueJob value)
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief The queue priority enum
	 */
	enum class QueuePriority : uint8_t
	{
		High,
		Medium,
		Low
	};

	/**
	 * @brief A hint for the main job of the device.
	 */
	enum class DeviceJobHint : uint8_t
	{
		General,
		Graphics,
		Compute
	};
}
