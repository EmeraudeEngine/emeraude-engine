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

namespace EmEn::Vulkan
{
	/** @brief The device work type enumeration. */
	enum class DeviceWorkType : uint8_t
	{
		General = 0,
		Graphics = 1,
		Compute = 2
	};

	/** @brief The device run mode enumeration. */
	enum class DeviceRunMode : uint8_t
	{
		DontCare = 0,
		Performance = 1,
		PowerSaving = 2,
	};

	/** @brief This enumeration describes the purpose of a queue from the engine point of view. */
	enum class QueueFamilyJob : uint8_t
	{
		/** @brief The graphics and presentation queues. */
		GraphicsAndPresentation = 0,
		/** @brief The graphics queues for offscreen application. */
		Graphics = 1,
		/** @brief The compute queues. */
		Compute = 2,
		/** @brief The dedicated queues to transfer. */
		Transfer = 3
	};

	/** @brief The queue priority enum */
	enum class QueuePriority : uint8_t
	{
		High = 0,
		Medium = 1,
		Low = 2
	};

	/** @brief The swap-chain status enumeration. */
	enum class Status : uint8_t
	{
		Uninitialized = 0,
		Ready = 1,
		Degraded = 2,
		UnderConstruction = 3,
		Failure = 4
	};
}
