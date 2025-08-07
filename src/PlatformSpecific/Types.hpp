/*
 * src/PlatformSpecific/Types.hpp
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

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief Describe the available CPU on the system in order to get/print information.
	 */
	struct CPU
	{
		/* The creator of the device, like AMD, Intel, ARM... */
		std::string vendorName{"Unknown"};
		uint32_t vendorID{0};
		/* The device model commercial name. */
		std::string deviceName{"Unknown"};
		uint32_t deviceID{0};
		int32_t family{-1};
		int32_t model{-1};
		int32_t stepping{-1};
		/* The real number of CPU core. */
		uint32_t physicalCores{0};
		/* The total number of usable cores. Taking in account threads (i.e. Hyper Threading with Intel CPU). */
		uint32_t logicalCores{0};
		/* The number of CPU core for efficiency/performance (new generation of CPU like M1/M2/M3 or Intel 12th gen and above). */
		uint32_t efficiencyCores{0};
		uint32_t performanceCores{0};
	};

	/**
	 * @brief Describe the available main memory (RAM) on the system in order to get/print information.
	 */
	struct SystemMemory
	{
		size_t total{0};
		size_t free{0};
		size_t used{0};
		size_t currentUsage{0};
		size_t peakUsage{0};
	};

	/**
	 * @brief The type of GPU device.
	 * @note This follows the Vulkan convention to describe a device.
	 */
	enum class GPUDeviceType : uint8_t
	{
		Other,
		IntegratedGPU,
		DiscreteGPU,
		VirtualGPU,
		CPU
	};

	static constexpr auto OtherString{"Other"};
	static constexpr auto IntegratedGPUString{"IntegratedGPU"};
	static constexpr auto DiscreteGPUString{"DiscreteGPU"};
	static constexpr auto VirtualGPUString{"VirtualGPU"};
	static constexpr auto CPUString{"CPU"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (GPUDeviceType value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (GPUDeviceType value)
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Describe a GPU device from the system in order to get/print information.
	 */
	struct GPUDevice
	{
		/* The creator of the main part of device, like NVIDIA, AMD, Intel... */
		std::string vendorName{"Unknown"};
		uint32_t vendorID{0};
		/* The device model commercial name. */
		std::string deviceName{"Unknown"};
		uint32_t deviceID{0};
		GPUDeviceType deviceType{GPUDeviceType::Other};
		/* The brand of the device distribution, like MSI, ASUS, ... */
		std::string manufacturerName{"Unknown"};
		uint32_t manufacturerID{0};
		/* Tell if the device has its own memory or share the system RAM.
		 * The size will be the dedicated memory or the allocated part in RAM in bytes. */
		bool dedicatedMemory{true};
		size_t availableMemory{0};
		/* The device identifier on the system bus (Something like PCI:1:0:0). */
		std::string bus;
	};

	/** @brief Describe the operating system. */
	struct OperatingSystem
	{
		std::string systemName;
		std::string systemVersion;
		std::string computerName;
		std::string machineUUID;
		std::string rawInformation;
	};
}
