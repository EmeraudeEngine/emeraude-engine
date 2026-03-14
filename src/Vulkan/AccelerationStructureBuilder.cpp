/*
 * src/Vulkan/AccelerationStructureBuilder.cpp
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

#include "AccelerationStructureBuilder.hpp"

/* Local inclusions. */
#include "Buffer.hpp"
#include "CommandPool.hpp"
#include "CommandBuffer.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include "Sync/Fence.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	AccelerationStructureBuilder::AccelerationStructureBuilder (const std::shared_ptr< Device > & device) noexcept
		: m_device{device}
	{

	}

	AccelerationStructureBuilder::~AccelerationStructureBuilder () noexcept
	{
		m_commandBuffer.reset();
		m_fence.reset();
		m_commandPool.reset();
	}

	bool
	AccelerationStructureBuilder::initialize () noexcept
	{
		if ( m_device == nullptr )
		{
			Tracer::error(ClassId, "No device provided !");

			return false;
		}

		const auto deviceHandle = m_device->handle();

		/* 1. Load extension function pointers.
		 * NOTE: vkGetBufferDeviceAddress is core in Vulkan 1.2+ (no KHR suffix needed).
		 * The KHR variant requires the VK_KHR_buffer_device_address extension to be explicitly
		 * enabled, so we try the core name first, then fall back to the KHR alias. */
		m_fpGetBuildSizes = reinterpret_cast< PFN_vkGetAccelerationStructureBuildSizesKHR >(vkGetDeviceProcAddr(deviceHandle, "vkGetAccelerationStructureBuildSizesKHR"));
		m_fpCmdBuild = reinterpret_cast< PFN_vkCmdBuildAccelerationStructuresKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCmdBuildAccelerationStructuresKHR"));
		m_fpGetBufferDeviceAddress = reinterpret_cast< PFN_vkGetBufferDeviceAddressKHR >(vkGetDeviceProcAddr(deviceHandle, "vkGetBufferDeviceAddress"));

		if ( m_fpGetBufferDeviceAddress == nullptr )
		{
			m_fpGetBufferDeviceAddress = reinterpret_cast< PFN_vkGetBufferDeviceAddressKHR >(vkGetDeviceProcAddr(deviceHandle, "vkGetBufferDeviceAddressKHR"));
		}

		if ( m_fpGetBuildSizes == nullptr || m_fpCmdBuild == nullptr || m_fpGetBufferDeviceAddress == nullptr )
		{
			TraceError{ClassId} <<
				"Unable to load acceleration structure build function pointers !"
				" GetBuildSizes=" << (m_fpGetBuildSizes != nullptr ? "OK" : "NULL") <<
				" CmdBuild=" << (m_fpCmdBuild != nullptr ? "OK" : "NULL") <<
				" GetBufferDeviceAddress=" << (m_fpGetBufferDeviceAddress != nullptr ? "OK" : "NULL") << " !";

			return false;
		}

		/* 2. Cache queue family indices for ownership transfer barriers. */
		m_graphicsFamilyIndex = m_device->getGraphicsFamilyIndex();
		m_transferFamilyIndex = m_device->getGraphicsTransferFamilyIndex();

		/* 3. Create command pool for graphics queue (AS builds require graphics or compute queue). */
		m_commandPool = std::make_shared< CommandPool >(m_device, m_graphicsFamilyIndex, true, true, false);

		if ( !m_commandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the command pool !");

			return false;
		}

		/* 4. Create command buffer. */
		m_commandBuffer = std::make_unique< CommandBuffer >(m_commandPool, true);

		if ( !m_commandBuffer->isCreated() )
		{
			Tracer::error(ClassId, "Unable to create the command buffer !");

			return false;
		}

		/* 5. Create fence for synchronization. */
		m_fence = std::make_unique< Sync::Fence >(m_device, 0);

		if ( !m_fence->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the fence !");

			return false;
		}

		Tracer::success(ClassId, "Acceleration structure builder initialized.");

		return true;
	}

	std::unique_ptr< AccelerationStructure >
	AccelerationStructureBuilder::buildBLAS (const BLASGeometryInput & geometry) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_buildAccess};

		if ( m_deviceLost )
		{
			Tracer::error(ClassId, "Device is lost, cannot build BLAS !");

			return nullptr;
		}

		/* Wait for pending buffer transfers before reading vertex/index data.
		 * Only stall the transfer queue — the graphics/present queue keeps
		 * running so the renderer is not blocked during scene loading. */
		if ( auto * transferQueue = m_device->getGraphicsTransferQueue(QueuePriority::High); transferQueue != nullptr )
		{
			if ( !transferQueue->waitIdle() )
			{
				Tracer::error(ClassId, "Transfer queue wait failed !");

				m_deviceLost = true;

				return nullptr;
			}
		}

		const auto deviceHandle = m_device->handle();

		/* 1. Get vertex buffer device address. */
		VkBufferDeviceAddressInfo vertexAddressInfo{};
		vertexAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		vertexAddressInfo.buffer = geometry.vertexBuffer;
		const auto vertexAddress = m_fpGetBufferDeviceAddress(deviceHandle, &vertexAddressInfo);

		/* 2. Handle index buffer: either from CPU indices (temporary upload) or existing GPU buffer. */
		VkDeviceAddress indexAddress = 0;
		uint32_t effectiveIndexCount = 0;
		Buffer cpuIndexBuffer{m_device, 0, 0, 0, false}; /* Placeholder; only used when cpuIndices != null. */

		if ( geometry.cpuIndices != nullptr && geometry.cpuIndexCount > 0 )
		{
			/* Upload CPU-provided triangle-list indices to a temporary GPU buffer. */
			const auto bufferSize = static_cast< VkDeviceSize >(geometry.cpuIndexCount * sizeof(uint32_t));

			cpuIndexBuffer = Buffer{m_device, 0, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true};

			if ( !cpuIndexBuffer.createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create temporary index buffer for BLAS build !");

				return nullptr;
			}

			if ( !cpuIndexBuffer.writeData(MemoryRegion{geometry.cpuIndices, bufferSize}) )
			{
				Tracer::error(ClassId, "Unable to write temporary index data for BLAS build !");

				return nullptr;
			}

			VkBufferDeviceAddressInfo cpuIndexAddressInfo{};
			cpuIndexAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			cpuIndexAddressInfo.buffer = cpuIndexBuffer.handle();
			indexAddress = m_fpGetBufferDeviceAddress(deviceHandle, &cpuIndexAddressInfo);
			effectiveIndexCount = geometry.cpuIndexCount;
		}
		else if ( geometry.indexBuffer != VK_NULL_HANDLE )
		{
			VkBufferDeviceAddressInfo indexAddressInfo{};
			indexAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			indexAddressInfo.buffer = geometry.indexBuffer;
			indexAddress = m_fpGetBufferDeviceAddress(deviceHandle, &indexAddressInfo);
			effectiveIndexCount = geometry.indexCount;
		}

		const bool hasIndices = indexAddress != 0;

		/* 3. Describe the geometry. */
		VkAccelerationStructureGeometryTrianglesDataKHR trianglesData{};
		trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		trianglesData.pNext = nullptr;
		trianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		trianglesData.vertexData.deviceAddress = vertexAddress;
		trianglesData.vertexStride = geometry.vertexStride;
		trianglesData.maxVertex = geometry.vertexCount - 1;
		trianglesData.indexType = hasIndices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_NONE_KHR;
		trianglesData.indexData.deviceAddress = indexAddress;
		trianglesData.transformData.deviceAddress = 0;

		VkAccelerationStructureGeometryKHR asGeometry{};
		asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeometry.pNext = nullptr;
		asGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeometry.geometry.triangles = trianglesData;
		asGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		const uint32_t primitiveCount = hasIndices
			? effectiveIndexCount / 3
			: geometry.vertexCount / 3;

		/* 3. Query build sizes. */
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.pNext = nullptr;
		buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo.geometryCount = 1;
		buildGeometryInfo.pGeometries = &asGeometry;

		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		m_fpGetBuildSizes(deviceHandle, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, &primitiveCount, &buildSizesInfo);

		/* 4. Create the acceleration structure. */
		auto blas = std::make_unique< AccelerationStructure >(m_device, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, buildSizesInfo.accelerationStructureSize);

		if ( !blas->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create BLAS (" << buildSizesInfo.accelerationStructureSize << " bytes) !";

			return nullptr;
		}

		/* 5. Create scratch buffer (over-allocate for alignment padding). */
		constexpr VkDeviceSize ScratchAlignment = 256;
		const auto scratchAllocSize = buildSizesInfo.buildScratchSize + ScratchAlignment;
		Buffer scratchBuffer{m_device, 0, scratchAllocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false};

		if ( !scratchBuffer.createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create BLAS scratch buffer !");

			return nullptr;
		}

		VkBufferDeviceAddressInfo scratchAddressInfo{};
		scratchAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchAddressInfo.buffer = scratchBuffer.handle();
		const auto scratchAddress = m_fpGetBufferDeviceAddress(deviceHandle, &scratchAddressInfo);
		const auto alignedScratchAddress = (scratchAddress + ScratchAlignment - 1) & ~(ScratchAlignment - 1);

		/* 6. Fill build info with the created AS and scratch buffer. */
		buildGeometryInfo.dstAccelerationStructure = blas->handle();
		buildGeometryInfo.scratchData.deviceAddress = alignedScratchAddress;

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
		buildRangeInfo.primitiveCount = primitiveCount;
		buildRangeInfo.primitiveOffset = 0;
		buildRangeInfo.firstVertex = 0;
		buildRangeInfo.transformOffset = 0;

		const auto * pBuildRangeInfo = &buildRangeInfo;

		TraceDebug{ClassId} <<
			"BLAS build: vertices=" << geometry.vertexCount <<
			" stride=" << geometry.vertexStride <<
			" indices=" << effectiveIndexCount <<
			" primitives=" << primitiveCount <<
			" vertexAddr=0x" << std::hex << vertexAddress <<
			" indexAddr=0x" << indexAddress <<
			" scratchSize=" << std::dec << buildSizesInfo.buildScratchSize <<
			" asSize=" << buildSizesInfo.accelerationStructureSize <<
			" scratchAddr=0x" << std::hex << alignedScratchAddress << std::dec;

		/* 7. Record and submit the build command. */
		if ( !this->submitOneShot([&] (VkCommandBuffer cmdBuf) {
			if ( m_transferFamilyIndex != m_graphicsFamilyIndex )
			{
				/* Acquire queue family ownership for vertex/index buffers that
				 * were transferred on a dedicated transfer queue.  The matching
				 * release barrier is recorded in BufferTransferOperation::transfer(). */
				VkBufferMemoryBarrier acquireBarriers[2]{};
				uint32_t barrierCount = 0;

				acquireBarriers[barrierCount].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				acquireBarriers[barrierCount].srcAccessMask = 0;
				acquireBarriers[barrierCount].dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
				acquireBarriers[barrierCount].srcQueueFamilyIndex = m_transferFamilyIndex;
				acquireBarriers[barrierCount].dstQueueFamilyIndex = m_graphicsFamilyIndex;
				acquireBarriers[barrierCount].buffer = geometry.vertexBuffer;
				acquireBarriers[barrierCount].offset = 0;
				acquireBarriers[barrierCount].size = VK_WHOLE_SIZE;
				barrierCount++;

				if ( geometry.indexBuffer != VK_NULL_HANDLE )
				{
					acquireBarriers[barrierCount].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
					acquireBarriers[barrierCount].srcAccessMask = 0;
					acquireBarriers[barrierCount].dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
					acquireBarriers[barrierCount].srcQueueFamilyIndex = m_transferFamilyIndex;
					acquireBarriers[barrierCount].dstQueueFamilyIndex = m_graphicsFamilyIndex;
					acquireBarriers[barrierCount].buffer = geometry.indexBuffer;
					acquireBarriers[barrierCount].offset = 0;
					acquireBarriers[barrierCount].size = VK_WHOLE_SIZE;
					barrierCount++;
				}

				vkCmdPipelineBarrier(
					cmdBuf,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
					0,
					0, nullptr,
					barrierCount, acquireBarriers,
					0, nullptr
				);
			}
			else
			{
				/* Same queue family: a simple memory barrier ensures prior
				 * transfer writes are visible to the AS build stage. */
				VkMemoryBarrier memoryBarrier{};
				memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					cmdBuf,
					VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
					0,
					1, &memoryBarrier,
					0, nullptr,
					0, nullptr
				);
			}

			m_fpCmdBuild(cmdBuf, 1, &buildGeometryInfo, &pBuildRangeInfo);
		}) )
		{
			Tracer::error(ClassId, "BLAS build command submission failed !");

			return nullptr;
		}

		TraceDebug{ClassId} << "BLAS built successfully (" << primitiveCount << " triangles, " << buildSizesInfo.accelerationStructureSize << " bytes).";

		return blas;
	}

	std::unique_ptr< TLASBuildRequest >
	AccelerationStructureBuilder::prepareTLAS (const std::vector< TLASInstanceInput > & instances) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_buildAccess};

		if ( m_deviceLost )
		{
			Tracer::error(ClassId, "Device is lost, cannot prepare TLAS !");

			return nullptr;
		}

		if ( instances.empty() )
		{
			Tracer::error(ClassId, "No instances provided for TLAS prepare !");

			return nullptr;
		}

		const auto deviceHandle = m_device->handle();

		/* 1. Create VkAccelerationStructureInstanceKHR array. */
		std::vector< VkAccelerationStructureInstanceKHR > vkInstances;
		vkInstances.reserve(instances.size());

		for ( const auto & input : instances )
		{
			VkAccelerationStructureInstanceKHR instance{};
			instance.transform = input.transform;
			instance.instanceCustomIndex = input.instanceCustomIndex & 0x00FFFFFF;
			instance.mask = input.mask;
			instance.instanceShaderBindingTableRecordOffset = input.shaderBindingTableRecordOffset & 0x00FFFFFF;
			instance.flags = input.flags;
			instance.accelerationStructureReference = input.blasDeviceAddress;

			vkInstances.emplace_back(instance);
		}

		/* 2. Create per-request instance buffer (owned by the request to survive until GPU is done). */
		const auto instanceBufferSize = static_cast< VkDeviceSize >(vkInstances.size() * sizeof(VkAccelerationStructureInstanceKHR));

		auto instanceBuffer = std::make_unique< Buffer >(m_device, 0, instanceBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);

		if ( !instanceBuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create TLAS instance buffer !");

			return nullptr;
		}

		if ( !instanceBuffer->writeData(MemoryRegion{vkInstances.data(), instanceBufferSize}) )
		{
			Tracer::error(ClassId, "Unable to write TLAS instance data !");

			return nullptr;
		}

		VkBufferDeviceAddressInfo instanceAddressInfo{};
		instanceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		instanceAddressInfo.buffer = instanceBuffer->handle();
		const auto instanceAddress = m_fpGetBufferDeviceAddress(deviceHandle, &instanceAddressInfo);

		/* 3. Build the request object. Keep structs alive for deferred recording. */
		auto request = std::make_unique< TLASBuildRequest >();
		request->instanceBuffer = std::move(instanceBuffer);

		request->instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		request->instancesData.pNext = nullptr;
		request->instancesData.arrayOfPointers = VK_FALSE;
		request->instancesData.data.deviceAddress = instanceAddress;

		request->asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		request->asGeometry.pNext = nullptr;
		request->asGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		request->asGeometry.geometry.instances = request->instancesData;
		request->asGeometry.flags = 0;

		const auto primitiveCount = static_cast< uint32_t >(vkInstances.size());

		/* 4. Query build sizes. */
		request->buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		request->buildGeometryInfo.pNext = nullptr;
		request->buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		request->buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		request->buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		request->buildGeometryInfo.geometryCount = 1;
		request->buildGeometryInfo.pGeometries = &request->asGeometry;

		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		m_fpGetBuildSizes(deviceHandle, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &request->buildGeometryInfo, &primitiveCount, &buildSizesInfo);

		/* 5. Create the acceleration structure. */
		request->tlas = std::make_unique< AccelerationStructure >(m_device, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, buildSizesInfo.accelerationStructureSize);

		if ( !request->tlas->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create TLAS (" << buildSizesInfo.accelerationStructureSize << " bytes) !";

			return nullptr;
		}

		/* 6. Create per-request scratch buffer (owned by the request to survive until GPU is done). */
		constexpr VkDeviceSize ScratchAlignment = 256;
		const auto scratchAllocSize = buildSizesInfo.buildScratchSize + ScratchAlignment;

		auto scratchBuffer = std::make_unique< Buffer >(m_device, 0, scratchAllocSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false);

		if ( !scratchBuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create TLAS scratch buffer !");

			return nullptr;
		}

		VkBufferDeviceAddressInfo scratchAddressInfo{};
		scratchAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchAddressInfo.buffer = scratchBuffer->handle();
		const auto scratchAddress = m_fpGetBufferDeviceAddress(deviceHandle, &scratchAddressInfo);
		const auto alignedScratchAddress = (scratchAddress + ScratchAlignment - 1) & ~(ScratchAlignment - 1);

		request->scratchBuffer = std::move(scratchBuffer);

		/* 7. Fill build info with the created AS and scratch buffer. */
		request->buildGeometryInfo.dstAccelerationStructure = request->tlas->handle();
		request->buildGeometryInfo.scratchData.deviceAddress = alignedScratchAddress;

		request->buildRangeInfo.primitiveCount = primitiveCount;
		request->buildRangeInfo.primitiveOffset = 0;
		request->buildRangeInfo.firstVertex = 0;
		request->buildRangeInfo.transformOffset = 0;

		return request;
	}

	void
	AccelerationStructureBuilder::recordTLASBuild (VkCommandBuffer cmdBuf, TLASBuildRequest & request) noexcept
	{
		const auto * pBuildRangeInfo = &request.buildRangeInfo;

		m_fpCmdBuild(cmdBuf, 1, &request.buildGeometryInfo, &pBuildRangeInfo);

		/* Memory barrier: ensure the TLAS build completes before fragment shaders read it.
		 * NOTE: The engine uses ray queries (GL_EXT_ray_query) in fragment/compute shaders,
		 * NOT dedicated ray tracing pipelines. The correct dst stage is FRAGMENT_SHADER,
		 * not RAY_TRACING_SHADER (which requires VK_KHR_ray_tracing_pipeline). */
		VkMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuf,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr
		);
	}

	template< typename function_t >
	bool
	AccelerationStructureBuilder::submitOneShot (function_t && recordCommands) noexcept
	{
		/* 1. Reset and begin command buffer. */
		if ( !m_commandBuffer->reset() )
		{
			return false;
		}

		if ( !m_commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		/* 2. Record commands. */
		recordCommands(m_commandBuffer->handle());

		/* 3. End recording. */
		if ( !m_commandBuffer->end() )
		{
			return false;
		}

		/* 4. Submit to graphics queue. */
		auto * queue = m_device->getGraphicsQueue(QueuePriority::High);

		if ( queue == nullptr )
		{
			Tracer::error(ClassId, "No graphics queue available for AS build !");

			return false;
		}

		if ( !m_fence->reset() )
		{
			return false;
		}

		SynchInfo synchInfo;
		synchInfo.withFence(m_fence->handle());

		if ( !queue->submit(*m_commandBuffer, synchInfo) )
		{
			Tracer::error(ClassId, "Queue submit failed !");

			return false;
		}

		/* 5. Wait for completion. */
		if ( !m_fence->wait() )
		{
			Tracer::error(ClassId, "Fence wait failed, device may be lost !");

			m_deviceLost = true;

			/* Try to wait for the device to idle to release the pending
			 * command buffer, preventing cascade errors on subsequent
			 * reset attempts. */
			m_device->waitIdle("AccelerationStructureBuilder::submitOneShot - recovery");

			return false;
		}

		return true;
	}

	VkDeviceAddress
	AccelerationStructureBuilder::getBufferDeviceAddress (VkBuffer buffer) const noexcept
	{
		if ( m_fpGetBufferDeviceAddress == nullptr || buffer == VK_NULL_HANDLE )
		{
			return 0;
		}

		VkBufferDeviceAddressInfo addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.buffer = buffer;

		return m_fpGetBufferDeviceAddress(m_device->handle(), &addressInfo);
	}
}
