/*
 * src/Vulkan/Device.hpp
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
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Third-party forward declarations (the VMA implementation header is only
 * needed by the few .cpp files that call vma* functions — VmaAllocator is
 * an opaque handle, identical to VMA's own VK_DEFINE_HANDLE definition). */
typedef struct VmaAllocator_T * VmaAllocator;

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"
#include "NameableTrait.hpp"

/* Local inclusions for usages. */
#include "PhysicalDevice.hpp"
#include "DeviceQueueConfiguration.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Instance;
	class DeviceRequirements;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines a logical device from a physical device.
	 * @extends EmEn::Vulkan::AbstractObject This is the device, so a simple object is ok.
	 * @extends EmEn::Base::NameableTrait To set a name on a device.
	 */
	class EMEN_LEAN_API Device final : public std::enable_shared_from_this< Device >, public AbstractObject, public Base::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDevice"};

			/**
			 * @brief Constructs a device.
			 * @param instance A reference to the Vulkan instance.
			 * @param deviceName A string [std::move].
			 * @param physicalDevice A reference to a physical device smart pointer.
			 * @param showInformation Enable the device information in the terminal.
			 */
			Device (const Instance & instance, std::string deviceName, const std::shared_ptr< PhysicalDevice > & physicalDevice, bool showInformation) noexcept
				: NameableTrait{std::move(deviceName)},
				m_instance{instance},
				m_physicalDevice{physicalDevice},
				m_showInformation{showInformation}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Device (const Device & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Device (Device && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Device & operator= (const Device & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Device & operator= (Device && copy) noexcept = delete;

			/**
			 * @brief Destructs the device.
			 */
			~Device () override
			{
				this->destroy();
			}

			/**
			 * @brief Creates a device.
			 * @param requirements A reference to a device requirement.
			 * @param extensions A reference to a vector of extensions.
			 * @param useVMA Use Vulkan Memory Allocator.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (const DeviceRequirements & requirements, const std::vector< const char * > & extensions, bool useVMA) noexcept;

			/**
			 * @brief Destroys the device.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Returns the physical device smart pointer.
			 * @return std::shared_ptr< PhysicalDevice >
			 */
			[[nodiscard]]
			std::shared_ptr< PhysicalDevice >
			physicalDevice () const noexcept
			{
				return m_physicalDevice;
			}

			/**
			 * @brief Returns the pipeline cache the driver may reuse across pipeline creations.
			 * @note VK_NULL_HANDLE when the feature is disabled or the cache failed to be created,
			 * which every vkCreate*Pipelines call accepts as "no cache".
			 * @warning The returned object is INTERNALLY synchronized by the specification: it may
			 * be handed to concurrent vkCreate*Pipelines calls without external locking.
			 * @return VkPipelineCache
			 */
			[[nodiscard]]
			VkPipelineCache
			pipelineCache () const noexcept
			{
				return m_pipelineCache;
			}

			/**
			 * @brief Creates the pipeline cache, optionally primed with a previously saved blob.
			 * @warning The caller MUST have validated the blob first (see Graphics::Renderer): the
			 * specification's "incompatible data is ignored" promise is gated by valid-usage rules
			 * that make corrupt, truncated or foreign bytes UNDEFINED BEHAVIOUR — real drivers
			 * crash inside vkCreatePipelineCache on such input.
			 * @param initialData A pointer to a validated blob, or nullptr for an empty cache.
			 * @param initialSize The blob size in bytes.
			 * @return bool
			 */
			bool createPipelineCache (const void * initialData, size_t initialSize) noexcept;

			/**
			 * @brief Retrieves the driver-side pipeline cache content, ready to be persisted.
			 * @note Uses the two-call idiom and zeroes the destination first: drivers are known to
			 * leave the padding uninitialized, which both breaks hash stability and leaks process
			 * memory into the file.
			 * @param data A reference to the destination byte vector.
			 * @return bool
			 */
			[[nodiscard]]
			bool getPipelineCacheData (std::vector< uint8_t > & data) const noexcept;

			/**
			 * @brief Returns the device handle.
			 * @return VkDevice
			 */
			[[nodiscard]]
			VkDevice
			handle () const noexcept
			{
				return m_deviceHandle;
			}

			/**
			 * @brief Returns whether the memory allocator is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			useMemoryAllocator () const noexcept
			{
				return m_useMemoryAllocator;
			}

			/**
			 * @brief Returns a human-readable summary of the GPU memory the allocator holds.
			 *
			 * @note Reports what VMA has RESERVED from the driver (block bytes) alongside what is
			 * actually in use (allocation bytes). The gap between the two is the suballocator's
			 * slack, and it is the number that matters when many small buffers are created: a
			 * thousand tiny allocations can reserve orders of magnitude more than they hold.
			 *
			 * @note Without this, "the process eats memory" cannot be told from "the GPU
			 * allocator eats memory", and the search starts in the wrong place — which is exactly
			 * what happened while chasing a 26 MB-per-instance-object cost.
			 *
			 * @return std::string
			 */
			[[nodiscard]]
			std::string memoryStatisticsString () const noexcept;

			/**
			 * @brief Returns the memory allocator handle.
			 * @return VmaAllocator
			 */
			[[nodiscard]]
			VmaAllocator
			memoryAllocatorHandle () const noexcept
			{
				return m_memoryAllocatorHandle;
			}

			/**
			 * @brief Returns whether the device has only one family queue for all.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasBasicSupport () const noexcept
			{
				return m_basicSupport;
			}

			/**
			 * @brief Returns whether ray tracing extensions are enabled on this device.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			rayTracingEnabled () const noexcept
			{
				return m_rayTracingEnabled;
			}

			/**
			 * @brief Returns whether Vulkan Video H.265 hardware encode is available on this device.
			 * @note True when VK_KHR_video_queue + VK_KHR_video_encode_queue + VK_KHR_video_encode_h265
			 * are enabled AND a VIDEO_ENCODE queue family was configured. The RushMaker uses the
			 * hardware path when true, the software VP9 fallback otherwise.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			videoEncodeH265Enabled () const noexcept
			{
				return m_videoEncodeH265Enabled;
			}

			/**
			 * @brief Returns the queue family index for video-encode queues.
			 * @warning Be sure of calling Device::videoEncodeH265Enabled() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getVideoEncodeFamilyIndex () const noexcept
			{
				return m_videoEncodeQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a video-encode queue.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getVideoEncodeQueue (QueuePriority priority) const noexcept
			{
				if ( !m_videoEncodeQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_videoEncodeQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns whether the Win32 external-memory import extension (VK_KHR_external_memory_win32) is enabled on this device.
			 * @note Used by the zero-copy CEF accelerated-paint path to import D3D11 shared textures as Vulkan images. Always false outside Windows.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			externalMemoryWin32Enabled () const noexcept
			{
				return m_externalMemoryWin32Enabled;
			}

			/**
			 * @brief Returns whether VK_EXT_metal_objects is enabled on this device (macOS IOSurface import, see Image::importFromIOSurface()).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			metalObjectsEnabled () const noexcept
			{
				return m_metalObjectsEnabled;
			}

			/**
			 * @brief Returns whether the device has been set up for graphics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasGraphicsQueues () const noexcept
			{
				return m_graphicsQueueConfiguration.enabled();
			}

			/**
			 * @brief Returns the queue family index for graphics queues.
			 * @note This may return the same family queue index from another configuration.
			 * @warning Be sure of calling Device::hasGraphicsQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getGraphicsFamilyIndex () const noexcept
			{
				return m_graphicsQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a graphics queue.
			 * @note This may return the same queue as another configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getGraphicsQueue (QueuePriority priority) const noexcept
			{
				if ( !m_graphicsQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_graphicsQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns whether the device has been set up for compute queues.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasComputeQueues () const noexcept
			{
				return m_computeQueueConfiguration.enabled();
			}

			/**
			 * @brief Returns the queue family index for compute queues.
			 * @note This may return the same family queue index from another configuration.
			 * @warning Be sure of calling Device::hasComputeQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getComputeFamilyIndex () const noexcept
			{
				return m_computeQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a compute queue.
			 * @note This may return the same queue as another configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getComputeQueue (QueuePriority priority) const noexcept
			{
				if ( !m_computeQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_computeQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns whether the device has been set up for transfer-only queues.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasTransferQueues () const noexcept
			{
				return m_transferQueueConfiguration.enabled();
			}

			/**
			 * @brief Returns the transfer-only queue family index.
			 * @note This may return the same family queue index from another configuration.
			 * @warning Be sure of calling Device::hasTransferQueues() before trusting the index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getTransferFamilyIndex () const noexcept
			{
				return m_transferQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a transfer-only queue.
			 * @note This may return the same queue as another configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getTransferQueue (QueuePriority priority) const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return nullptr;
				}

				return m_transferQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Returns the transfer-only queue family index for graphics if available.
			 * @note This may return the same family queue index from the graphics configuration.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getGraphicsTransferFamilyIndex () const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_graphicsQueueConfiguration.queueFamilyIndex();
				}

				return m_transferQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a transfer-only queue for graphics if available.
			 * @note This may return a queue from the graphics configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getGraphicsTransferQueue (QueuePriority priority) const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_graphicsQueueConfiguration.queue(priority);
				}

				return m_transferQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Waits every queue of the transfer configuration to be idle.
			 * @note Queues are distributed round-robin: waiting a single queue does NOT
			 * guarantee a previously submitted transfer has completed — it may sit on a
			 * sibling queue. Use this before reading freshly uploaded buffer content
			 * from another queue family (e.g. acceleration-structure builds).
			 * Falls back to the graphics configuration when there is no dedicated
			 * transfer family (transfers then run on graphics queues).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			waitTransferQueuesIdle () const noexcept
			{
				const auto & configuration = m_transferQueueConfiguration.enabled()
					? m_transferQueueConfiguration
					: m_graphicsQueueConfiguration;

				bool success = true;

				for ( auto priority : {QueuePriority::High, QueuePriority::Medium, QueuePriority::Low} )
				{
					for ( const auto * queue : configuration.queues(priority) )
					{
						if ( queue != nullptr && !queue->waitIdle() )
						{
							success = false;
						}
					}
				}

				return success;
			}

			/**
			 * @brief Returns the transfer-only queue family index for compute if available.
			 * @note This may return the same family queue index from the compute configuration.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getComputeTransferFamilyIndex () const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_computeQueueConfiguration.queueFamilyIndex();
				}

				return m_transferQueueConfiguration.queueFamilyIndex();
			}

			/**
			 * @brief Returns a transfer-only queue for compute if available.
			 * @note This may return a queue from the compute configuration.
			 * @warning This may return a nullptr!
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			Queue *
			getComputeTransferQueue (QueuePriority priority) const noexcept
			{
				if ( !m_transferQueueConfiguration.enabled() )
				{
					return m_computeQueueConfiguration.queue(priority);
				}

				return m_transferQueueConfiguration.queue(priority);
			}

			/**
			 * @brief Waits for a device to become idle.
			 * @param location A point to string.
			 * @return void
			 */
			void waitIdle (const char * location) const noexcept;

			/**
			 * @brief Finds the suitable memory type.
			 * @param memoryTypeFilter The memory type.
			 * @param propertyFlags The access type of memory requested.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t findMemoryType (uint32_t memoryTypeFilter, VkMemoryPropertyFlags propertyFlags) const noexcept;

			/**
			 * @brief Finds a supported format from a device.
			 * @param formats A reference to a format vector.
			 * @param tiling
			 * @param featureFlags
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat findSupportedFormat (const std::vector< VkFormat > & formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) const noexcept;

			/**
			 * @brief Returns the available samples against the desired for multisampling rendering.
			 * @param samples The number of samples desired.
			 * @return VkSampleCountFlagBits
			 */
			[[nodiscard]]
			uint32_t checkMultisampleCount (uint32_t samples) const noexcept;

			/**
			 * @brief Returns sample flag for Vulkan.
			 * @param samples The number of samples desired.
			 * @return VkSampleCountFlagBits
			 */
			[[nodiscard]]
			static VkSampleCountFlagBits getSampleCountFlag (uint32_t samples) noexcept;

			/**
			 * @brief Lock the access to the device.
			 * @note std::lock_guard friendly.
			 * @return void
			 */
			void
			lock () const
			{
				m_logicalDeviceAccess.lock();
			}

			/**
			 * @brief Unlock the access to the device.
			 * @note std::lock_guard friendly.
			 * @return void
			 */
			void
			unlock () const
			{
				m_logicalDeviceAccess.unlock();
			}

			/**
			 * @brief Records a GPU diagnostic checkpoint into a command buffer (VK_NV_device_diagnostic_checkpoints).
			 * @note No-op when the extension is unavailable. The marker MUST point to storage that stays
			 * valid until a device-lost readback — always pass a string literal (static storage).
			 * @param commandBuffer The command buffer currently being recorded.
			 * @param marker A static string identifying the GPU command region.
			 * @return void
			 */
			void setCheckpoint (VkCommandBuffer commandBuffer, const char * marker) const noexcept;

			/**
			 * @brief Dumps all available GPU fault diagnostics after a VK_ERROR_DEVICE_LOST.
			 * @note Best-effort and self-guarded: reports only once per device. Combines VK_EXT_device_fault
			 * (faulting GPU addresses) and VK_NV_device_diagnostic_checkpoints (last command region reached).
			 * Takes NO device lock — safe to call from within a locked submit/wait path.
			 * @param context A short string naming the CPU call site that observed the loss.
			 * @return void
			 */
			void dumpDeviceLostDiagnostics (const char * context) const noexcept;

		private:

			/**
			 * @brief Creates the Vulkan memory allocator for this device.
			 * @note The memory allocator is part of the device to follow the recommendation that says one allocator per device.
			 * @return bool
			 */
			[[nodiscard]]
			bool createMemoryAllocator () noexcept;

			/**
			 * @brief Destroys the Vulkan memory allocator for this device.
			 * @return void
			 */
			void destroyMemoryAllocator () noexcept;

			/**
			 * @brief Adds a queue family to the createInfos list and returns the number of queues in this family.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static
			uint32_t addQueueFamilyToCreateInfo (uint32_t queueFamilyIndex, const Base::StaticVector<VkQueueFamilyProperties2, 8>& queueFamilyProperties, Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares queues for a graphics and compute device.
			 * @param requirements A reference to the device requirements.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchGraphicsAndComputeQueueConfiguration (const DeviceRequirements & requirements, const Base::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares queues for a graphics device.
			 * @param requirements A reference to the device requirements.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchGraphicsQueueConfiguration (const DeviceRequirements & requirements, const Base::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares queues for a compute device.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchComputeQueueConfiguration (const Base::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Prepares transfer-only queues for the device (optional).
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue creation information vector.
			 * @param queuePriorities A writable reference to a map for queue priorities.
			 * @return bool
			 */
			[[nodiscard]]
			bool searchTransferOnlyQueueConfiguration (const Base::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Searches a queue family with video-encode capability (Vulkan Video).
			 * @note Only called when the video-encode extensions were enabled by the instance.
			 * @param queueFamilyProperties A reference to the family properties.
			 * @param queueCreateInfos A writable reference to the queue create infos.
			 * @param queuePriorities A writable reference to the queue priorities.
			 * @return bool True when a VIDEO_ENCODE queue family was configured.
			 */
			bool searchVideoEncodeQueueConfiguration (const Base::StaticVector< VkQueueFamilyProperties2, 8 > & queueFamilyProperties, Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorities) noexcept;

			/**
			 * @brief Creates the device with the defined and verified queues.
			 * @param requirements A reference to a device requirement.
			 * @param queueCreateInfos A reference to a list of CreateInfo for Vulkan queues.
			 * @param extensions A reference to a list of extensions to enable with the device.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDevice (const DeviceRequirements & requirements, const Base::StaticVector< VkDeviceQueueCreateInfo, 8 > & queueCreateInfos, const std::vector< const char * > & extensions) noexcept;

			/**
			 * @brief Installs queues generated from the device.
			 * @param queuePriorityValues A reference to a map for the initial priorities selected.
			 * @param configuration A writable reference to the current configuration.
			 * @return bool
			 */
			[[nodiscard]]
			bool installQueues (const std::map< uint32_t, Base::StaticVector< float, 16 > > & queuePriorityValues, const DeviceQueueConfiguration & configuration) noexcept;

			const Instance & m_instance;
			std::shared_ptr< PhysicalDevice > m_physicalDevice;
			VkDevice m_deviceHandle{VK_NULL_HANDLE};
			VmaAllocator m_memoryAllocatorHandle{VK_NULL_HANDLE};
			VkPipelineCache m_pipelineCache{VK_NULL_HANDLE};
			PFN_vkGetDeviceFaultInfoEXT m_fpGetDeviceFaultInfo{nullptr};
			PFN_vkGetQueueCheckpointDataNV m_fpGetQueueCheckpointData{nullptr};
			PFN_vkCmdSetCheckpointNV m_fpCmdSetCheckpoint{nullptr};
			Base::StaticVector< std::unique_ptr< Queue >, 32 > m_queues;
			DeviceQueueConfiguration m_graphicsQueueConfiguration;
			DeviceQueueConfiguration m_computeQueueConfiguration;
			DeviceQueueConfiguration m_transferQueueConfiguration;
			DeviceQueueConfiguration m_videoEncodeQueueConfiguration;
			mutable std::mutex m_logicalDeviceAccess;
			mutable std::atomic_bool m_deviceLostReported{false};
			bool m_showInformation{false};
			bool m_basicSupport{false};
			bool m_videoEncodeH265Enabled{false};
			bool m_useMemoryAllocator{false};
			bool m_rayTracingEnabled{false};
			bool m_externalMemoryWin32Enabled{false};
			bool m_metalObjectsEnabled{false};
	};
}
