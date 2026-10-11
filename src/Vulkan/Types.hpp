/*
 * src/Vulkan/Types.hpp
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
#include <array>
#include <cstdint>
#include <string>

namespace EmEn::Vulkan
{
	/** @brief The device work type enumeration. */
	enum class EMEN_API DeviceWorkType : uint8_t
	{
		General = 0,
		Graphics = 1,
		Compute = 2
	};

	/** @brief The device auto-selection mode enumeration. */
	enum class EMEN_API DeviceAutoSelectMode : uint8_t
	{
		/** @brief DontCare mode: First available. */
		DontCare = 0,
		/** @brief Performance mode: Select the best GPU. */
		Performance = 1,
		/** @brief PowerSaving mode: Select the economic GPU. */
		PowerSaving = 2,
		/**
		 * @brief RawPerformance mode: the most powerful GPU, whatever its optional features: the device type first
		 * (discrete > integrated > virtual > CPU), then the largest device-local memory heap (VRAM). The required
		 * features and extensions still filter. 'Performance' (the default) also rewards the advertised extensions
		 * and the queue count, and can prefer a feature-rich iGPU over a modest discrete card (owner request,
		 * 2026-10-11: an iGPU won over a Quadro P600).
		 */
		RawPerformance = 3
	};

	/**
	 * @brief Identifies a physical device across APIs and processes: the adapter LUID (Windows: the DXGI adapter LUID,
	 * the same value as VkPhysicalDeviceIDProperties::deviceLUID) and the device UUID (VkPhysicalDeviceIDProperties::
	 * deviceUUID, stable across Vulkan instances and processes on the same machine and driver).
	 * @note Used by an application to make the engine pick the GPU another component already uses (e.g. the adapter a
	 * Chromium GPU process was pinned to): Instance::preferGraphicsDevice().
	 */
	struct EMEN_API DeviceIdentity final
	{
		std::array< uint8_t, 8 > LUID{};  /**< VK_LUID_SIZE bytes, meaningful when hasLUID. */
		std::array< uint8_t, 16 > UUID{}; /**< VK_UUID_SIZE bytes, meaningful when hasUUID. */
		bool hasLUID{false};
		bool hasUUID{false};

		/**
		 * @brief Returns whether two identities name the same device: equal LUIDs when both carry one, else equal UUIDs
		 * when both carry one.
		 * @param other A reference to the other identity.
		 * @return bool False when they share no identifier.
		 */
		[[nodiscard]]
		bool
		matches (const DeviceIdentity & other) const noexcept
		{
			if ( hasLUID && other.hasLUID )
			{
				return LUID == other.LUID;
			}

			if ( hasUUID && other.hasUUID )
			{
				return UUID == other.UUID;
			}

			return false;
		}
	};

	/** @brief Known GPU vendor IDs. */
	enum class EMEN_API Vendor : std::uint16_t
	{
		Unknown = 0,
		AMD = 0x1002,
		ImgTec = 0x1010,
		Nvidia = 0x10DE,
		ARM = 0x13B5,
		Qualcomm = 0x5143,
		Apple = 0x106B,
		Intel = 0x8086
	};

	/**
	 * @brief Converts a raw vendor ID to a Vendor enum.
	 * @param vendorID The raw vendor ID from Vulkan.
	 * @return Vendor The corresponding Vendor enum value.
	 */
	[[nodiscard]]
	constexpr Vendor
	toVendor (uint32_t vendorID) noexcept
	{
		switch ( vendorID )
		{
			case static_cast< uint32_t >(Vendor::AMD) :
				return Vendor::AMD;

			case static_cast< uint32_t >(Vendor::ImgTec) :
				return Vendor::ImgTec;

			case static_cast< uint32_t >(Vendor::Nvidia) :
				return Vendor::Nvidia;

			case static_cast< uint32_t >(Vendor::ARM) :
				return Vendor::ARM;

			case static_cast< uint32_t >(Vendor::Qualcomm) :
				return Vendor::Qualcomm;

			case static_cast< uint32_t >(Vendor::Apple) :
				return Vendor::Apple;

			case static_cast< uint32_t >(Vendor::Intel) :
				return Vendor::Intel;

			default :
				return Vendor::Unknown;
		}
	}

	/**
	 * @brief Returns the vendor name as a string.
	 * @param vendor The Vendor enum value.
	 * @return const char * The vendor name.
	 */
	[[nodiscard]]
	constexpr
	const char *
	to_cstring (Vendor vendor) noexcept
	{
		switch ( vendor )
		{
			case Vendor::AMD :
				return "AMD";

			case Vendor::ImgTec :
				return "ImgTec";

			case Vendor::Nvidia :
				return "Nvidia";

			case Vendor::ARM :
				return "ARM";

			case Vendor::Qualcomm :
				return "Qualcomm";

			case Vendor::Apple :
				return "Apple";

			case Vendor::Intel :
				return "Intel";

			case Vendor::Unknown :
				return "Unknown";
		}

		return "Unknown";
	}

	/**
	 * @brief Information about detected hybrid GPU configuration.
	 */
	struct EMEN_API HybridGPUConfig
	{
		/** @brief True if Nvidia Optimus configuration detected (laptop with iGPU + mobile Nvidia dGPU). */
		bool isOptimusDetected{false};

		/** @brief True if hybrid GPU detected but NOT Optimus (desktop with iGPU in CPU + discrete RTX with own outputs). */
		bool isHybridNonOptimus{false};

		/** @brief Name of the integrated GPU. */
		std::string integratedGPUName;

		/** @brief Name of the discrete GPU. */
		std::string discreteGPUName;

		/** @brief Vendor ID of the integrated GPU. */
		uint32_t integratedVendorID{0};

		/** @brief Vendor ID of the discrete GPU. */
		uint32_t discreteVendorID{0};
	};

	/** @brief This enumeration describes the purpose of a queue from the engine point of view. */
	enum class EMEN_API QueueFamilyJob : uint8_t
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
	enum class EMEN_API QueuePriority : uint8_t
	{
		High = 0,
		Medium = 1,
		Low = 2
	};

	/** @brief The swap-chain status enumeration. */
	enum class EMEN_API SwapChainStatus : uint8_t
	{
		Uninitialized = 0,
		Ready = 1,
		Degraded = 2,
		UnderConstruction = 3,
		Failure = 4
	};
}
