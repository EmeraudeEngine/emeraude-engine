/*
 * src/Vulkan/PhysicalDeviceProbe.hpp
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
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "Types.hpp"

namespace EmEn::Vulkan
{
	/** @brief What probePhysicalDevices() reports about one physical device. */
	struct EMEN_API PhysicalDeviceSummary final
	{
		std::string name;                       /**< VkPhysicalDeviceProperties::deviceName (the ForceGPU spelling). */
		DeviceIdentity identity{};              /**< LUID (when valid) and UUID. */
		uint64_t deviceLocalMemory{0};          /**< The largest DEVICE_LOCAL heap, in bytes. */
		uint32_t vendorID{0};
		uint32_t deviceID{0};
		VkPhysicalDeviceType type{VK_PHYSICAL_DEVICE_TYPE_OTHER};
	};

	/**
	 * @brief Lists the physical devices BEFORE the engine exists: a minimal, temporary Vulkan instance (no layer, no
	 * surface extension) on the loader shipped with the application, destroyed before returning.
	 * @note For an application that must decide something GPU-related before the core starts — the case that
	 * motivated it: the adapter a Chromium GPU process is pinned to at CefInitialize, while the engine's device is
	 * selected later (resolve the ForceGPU NAME, a Vulkan name, to its LUID: DXGI names some adapters differently).
	 * Never touches volk's global pointers (Loader::VolkBinding::None), so it is safe whether or not the engine's
	 * loader is already open. Main thread. Cost: one instance creation (driver enumeration).
	 * @param binaryDirectory The directory of the running executable (where the loader is shipped).
	 * @return std::vector< PhysicalDeviceSummary > Empty when the loader or the instance is unavailable.
	 */
	[[nodiscard]]
	EMEN_API std::vector< PhysicalDeviceSummary > probePhysicalDevices (const std::filesystem::path & binaryDirectory) noexcept;
}
