/*
 * src/Vulkan/AccelerationStructureBuilder.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "AccelerationStructure.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Buffer;
	class Device;
	class CommandPool;
	class CommandBuffer;
	namespace Sync { class Fence; }
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Describes a geometry input for BLAS building.
	 */
	struct BLASGeometryInput final
	{
		VkBuffer vertexBuffer{VK_NULL_HANDLE};
		uint32_t vertexCount{0};
		uint32_t vertexStride{0};
		VkBuffer indexBuffer{VK_NULL_HANDLE};
		uint32_t indexCount{0};
		VkIndexType indexType{VK_INDEX_TYPE_UINT32};

		/* Optional CPU-side triangle-list indices (e.g. converted from TriangleStrip).
		 * When non-null, the builder creates a temporary GPU buffer from these indices
		 * and ignores indexBuffer/indexCount above. */
		const uint32_t * cpuIndices{nullptr};
		uint32_t cpuIndexCount{0};
	};

	/**
	 * @brief Describes an instance for TLAS building.
	 */
	struct TLASInstanceInput final
	{
		VkDeviceAddress blasDeviceAddress{0};
		VkTransformMatrixKHR transform{};
		uint32_t instanceCustomIndex{0};
		uint32_t mask{0xFF};
		uint32_t shaderBindingTableRecordOffset{0};
		VkGeometryInstanceFlagsKHR flags{VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR};
	};

	/**
	 * @brief Holds the prepared state for a deferred TLAS build.
	 * @details Created by prepareTLAS(), consumed by recordTLASBuild().
	 */
	struct TLASBuildRequest final
	{
		std::unique_ptr< AccelerationStructure > tlas;
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
		VkAccelerationStructureGeometryKHR asGeometry{};
		VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
	};

	/**
	 * @brief Utility class for building acceleration structures (BLAS and TLAS).
	 * @details Manages its own command pool, command buffer, and fence for one-shot
	 * GPU operations. Loads KHR extension function pointers dynamically.
	 *
	 * @todo A compute queue variant would further improve performance by overlapping
	 * TLAS builds with graphics work on separate hardware queues.
	 */
	class AccelerationStructureBuilder final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanAccelerationStructureBuilder"};

			/**
			 * @brief Constructs the builder.
			 * @param device A reference to a device smart pointer.
			 */
			explicit AccelerationStructureBuilder (const std::shared_ptr< Device > & device) noexcept;

			/**
			 * @brief Destructs the builder.
			 */
			~AccelerationStructureBuilder () noexcept;

			AccelerationStructureBuilder (const AccelerationStructureBuilder &) noexcept = delete;
			AccelerationStructureBuilder (AccelerationStructureBuilder &&) noexcept = delete;
			AccelerationStructureBuilder & operator= (const AccelerationStructureBuilder &) noexcept = delete;
			AccelerationStructureBuilder & operator= (AccelerationStructureBuilder &&) noexcept = delete;

			/**
			 * @brief Initializes the builder (loads function pointers, creates GPU resources).
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize () noexcept;

			/**
			 * @brief Builds a bottom-level acceleration structure from geometry.
			 * @note This is a synchronous operation (fence wait). BLAS are typically
			 * built once at load time, so the stall is acceptable.
			 * @param geometry The geometry input describing vertex/index buffers.
			 * @return std::unique_ptr< AccelerationStructure > The built BLAS, or nullptr on failure.
			 */
			[[nodiscard]]
			std::unique_ptr< AccelerationStructure > buildBLAS (const BLASGeometryInput & geometry) noexcept;

			/**
			 * @brief Builds a top-level acceleration structure synchronously (legacy path).
			 * @note Prefer prepareTLAS() + recordTLASBuild() for per-frame rebuilds
			 * to avoid CPU stalls from the internal fence wait.
			 * @param instances A reference to a vector of instance inputs.
			 * @return std::unique_ptr< AccelerationStructure > The built TLAS, or nullptr on failure.
			 */
			[[nodiscard]]
			std::unique_ptr< AccelerationStructure > buildTLAS (const std::vector< TLASInstanceInput > & instances) noexcept;

			/**
			 * @brief Prepares a TLAS build (CPU-side): uploads instances, allocates AS and scratch.
			 * @note Does NOT submit any GPU commands. The caller must record the build
			 * into a command buffer via recordTLASBuild().
			 * @param instances A reference to a vector of instance inputs.
			 * @return std::unique_ptr< TLASBuildRequest > The prepared build state, or nullptr on failure.
			 *
			 * @todo Move to a dedicated compute queue for full async overlap with graphics work.
			 */
			[[nodiscard]]
			std::unique_ptr< TLASBuildRequest > prepareTLAS (const std::vector< TLASInstanceInput > & instances) noexcept;

			/**
			 * @brief Records TLAS build commands into an external command buffer.
			 * @note Inserts a memory barrier after the build to ensure the TLAS is
			 * visible to subsequent ray tracing shader reads.
			 * @param cmdBuf The Vulkan command buffer to record into (must be in recording state).
			 * @param request The prepared build request from prepareTLAS().
			 */
			void recordTLASBuild (VkCommandBuffer cmdBuf, TLASBuildRequest & request) noexcept;

			/**
			 * @brief Returns the device address of a buffer.
			 * @note Requires VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT on the buffer.
			 * @param buffer The Vulkan buffer handle.
			 * @return VkDeviceAddress The GPU virtual address, or 0 on failure.
			 */
			[[nodiscard]]
			VkDeviceAddress getBufferDeviceAddress (VkBuffer buffer) const noexcept;

		private:

			/**
			 * @brief Records and submits a one-shot command buffer, then waits for completion.
			 * @tparam function_t The type of lambda to record commands. Signature: void (VkCommandBuffer).
			 * @param recordCommands A reference to a function that records commands.
			 * @return bool
			 */
			template< typename function_t >
			[[nodiscard]]
			bool submitOneShot (function_t && recordCommands) noexcept;

			std::shared_ptr< Device > m_device;
			std::shared_ptr< CommandPool > m_commandPool;
			std::unique_ptr< CommandBuffer > m_commandBuffer;
			std::unique_ptr< Sync::Fence > m_fence;
			std::mutex m_buildAccess;
			uint32_t m_graphicsFamilyIndex{0};
			uint32_t m_transferFamilyIndex{0};

			/* Persistent TLAS build buffers (grow-only, reused across frames). */
			std::unique_ptr< Buffer > m_tlasInstanceBuffer;
			VkDeviceSize m_tlasInstanceBufferCapacity{0};
			std::unique_ptr< Buffer > m_tlasScratchBuffer;
			VkDeviceSize m_tlasScratchBufferCapacity{0};

			/* Extension function pointers. */
			PFN_vkGetAccelerationStructureBuildSizesKHR m_fpGetBuildSizes{nullptr};
			PFN_vkCmdBuildAccelerationStructuresKHR m_fpCmdBuild{nullptr};
			PFN_vkGetBufferDeviceAddressKHR m_fpGetBufferDeviceAddress{nullptr};
			bool m_deviceLost{false};
	};
}
