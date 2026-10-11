/*
 * src/Vulkan/PhysicalDeviceProbe.cpp
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

#include "PhysicalDeviceProbe.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <span>
#include <string_view>

/* Local inclusions. */
#include "Loader.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	namespace
	{
		constexpr auto TracerTag{"VulkanPhysicalDeviceProbe"};

		/** @brief An upper bound on the enumerated devices (a validated count, never the driver's raw answer). */
		constexpr uint32_t MaxProbedDevices{64};

		/**
		 * @brief Fetches a Vulkan entry point through the loader's vkGetInstanceProcAddr.
		 * @note The binary interface of Vulkan: every entry point comes back as PFN_vkVoidFunction and is cast to its real
		 * signature (the reinterpret_cast is the documented use of vkGetInstanceProcAddr).
		 */
		template< typename function_t >
		[[nodiscard]]
		function_t
		entryPoint (PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkInstance instance, const char * name) noexcept
		{
			return reinterpret_cast< function_t >(getInstanceProcAddr(instance, name));
		}
	}

	std::vector< PhysicalDeviceSummary >
	probePhysicalDevices (const std::filesystem::path & binaryDirectory) noexcept
	{
		Loader loader;

		if ( !loader.open(binaryDirectory, Loader::VolkBinding::None) )
		{
			return {};
		}

		const auto getInstanceProcAddr = loader.getInstanceProcAddr();
		const auto enumerateExtensions = entryPoint< PFN_vkEnumerateInstanceExtensionProperties >(getInstanceProcAddr, nullptr, "vkEnumerateInstanceExtensionProperties");
		const auto createInstance = entryPoint< PFN_vkCreateInstance >(getInstanceProcAddr, nullptr, "vkCreateInstance");

		if ( enumerateExtensions == nullptr || createInstance == nullptr )
		{
			TraceError{TracerTag} << "The Vulkan loader exports no vkCreateInstance !";

			return {};
		}

		/* NOTE: MoltenVK is a portability driver: it is only listed with VK_KHR_portability_enumeration. */
		bool portabilityEnumeration = false;
		{
			uint32_t count = 0;

			if ( enumerateExtensions(nullptr, &count, nullptr) == VK_SUCCESS && count > 0 )
			{
				std::vector< VkExtensionProperties > extensions(count);

				if ( enumerateExtensions(nullptr, &count, extensions.data()) == VK_SUCCESS )
				{
					portabilityEnumeration = std::ranges::any_of(std::span{extensions}.first(std::min< size_t >(count, extensions.size())), [] (const VkExtensionProperties & extension) {
						const std::span name{extension.extensionName};

						return std::string_view{name.data(), static_cast< size_t >(std::ranges::find(name, '\0') - name.begin())} == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
					});
				}
			}
		}

		const std::array< const char *, 1 > portabilityExtension{VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

		const VkApplicationInfo applicationInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "EmeraudePhysicalDeviceProbe",
			.applicationVersion = 1,
			.pEngineName = "Emeraude",
			.engineVersion = 1,
			.apiVersion = VK_API_VERSION_1_1
		};

		const VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = portabilityEnumeration ? static_cast< VkInstanceCreateFlags >(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR) : 0U,
			.pApplicationInfo = &applicationInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = portabilityEnumeration ? static_cast< uint32_t >(portabilityExtension.size()) : 0U,
			.ppEnabledExtensionNames = portabilityEnumeration ? portabilityExtension.data() : nullptr
		};

		VkInstance instance = VK_NULL_HANDLE;

		if ( const auto result = createInstance(&createInfo, nullptr, &instance); result != VK_SUCCESS )
		{
			TraceError{TracerTag} << "Unable to create the probe instance (" << static_cast< int >(result) << ") !";

			return {};
		}

		const auto destroyInstance = entryPoint< PFN_vkDestroyInstance >(getInstanceProcAddr, instance, "vkDestroyInstance");
		const auto enumeratePhysicalDevices = entryPoint< PFN_vkEnumeratePhysicalDevices >(getInstanceProcAddr, instance, "vkEnumeratePhysicalDevices");
		const auto getProperties2 = entryPoint< PFN_vkGetPhysicalDeviceProperties2 >(getInstanceProcAddr, instance, "vkGetPhysicalDeviceProperties2");
		const auto getMemoryProperties = entryPoint< PFN_vkGetPhysicalDeviceMemoryProperties >(getInstanceProcAddr, instance, "vkGetPhysicalDeviceMemoryProperties");

		std::vector< PhysicalDeviceSummary > summaries;

		if ( destroyInstance == nullptr )
		{
			TraceError{TracerTag} << "The probe instance exports no vkDestroyInstance !";

			return {};
		}

		if ( enumeratePhysicalDevices != nullptr && getProperties2 != nullptr && getMemoryProperties != nullptr )
		{
			uint32_t count = 0;

			if ( enumeratePhysicalDevices(instance, &count, nullptr) == VK_SUCCESS && count > 0 )
			{
				std::vector< VkPhysicalDevice > devices(std::min(count, MaxProbedDevices));
				count = static_cast< uint32_t >(devices.size());

				if ( const auto result = enumeratePhysicalDevices(instance, &count, devices.data()); result == VK_SUCCESS || result == VK_INCOMPLETE )
				{
					devices.resize(std::min< size_t >(count, devices.size()));
					summaries.reserve(devices.size());

					for ( auto * const device : devices )
					{
						VkPhysicalDeviceIDProperties idProperties{};
						idProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

						VkPhysicalDeviceProperties2 properties{};
						properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
						properties.pNext = &idProperties;

						getProperties2(device, &properties);

						VkPhysicalDeviceMemoryProperties memory{};
						getMemoryProperties(device, &memory);

						PhysicalDeviceSummary summary{};
						const std::span deviceName{properties.properties.deviceName};
						summary.name.assign(deviceName.begin(), std::ranges::find(deviceName, '\0'));
						summary.vendorID = properties.properties.vendorID;
						summary.deviceID = properties.properties.deviceID;
						summary.type = properties.properties.deviceType;

						static_assert(sizeof(summary.identity.LUID) == VK_LUID_SIZE && sizeof(summary.identity.UUID) == VK_UUID_SIZE);

						std::ranges::copy(idProperties.deviceLUID, summary.identity.LUID.begin());
						std::ranges::copy(idProperties.deviceUUID, summary.identity.UUID.begin());
						summary.identity.hasLUID = idProperties.deviceLUIDValid == VK_TRUE;
						summary.identity.hasUUID = true;

						for ( const auto & heap : std::span{memory.memoryHeaps}.first(std::min< size_t >(memory.memoryHeapCount, VK_MAX_MEMORY_HEAPS)) )
						{
							if ( (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0 )
							{
								summary.deviceLocalMemory = std::max< uint64_t >(summary.deviceLocalMemory, heap.size);
							}
						}

						summaries.emplace_back(std::move(summary));
					}
				}
			}
		}

		destroyInstance(instance, nullptr);

		return summaries;
	}
}
