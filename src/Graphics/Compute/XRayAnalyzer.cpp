/*
 * src/Graphics/Compute/XRayAnalyzer.cpp
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

#include "XRayAnalyzer.hpp"

/* STL inclusions. */
#include <chrono>
#include <cstring>

/* Local inclusions. */
#include "Vulkan/Device.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Queue.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Compute
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Libs::VertexFactory;

	/* ---- Compute shader source ---- */

	static const std::string ComputeShaderSource = R"(
#version 450

layout(local_size_x = 16, local_size_y = 16) in;

struct Triangle
{
	vec4 v0;
	vec4 v1;
	vec4 v2;
};

/* Binding 0: All triangles. */
layout(std430, set = 0, binding = 0) readonly buffer TriangleBuffer
{
	Triangle triangles[];
};

/* Binding 1: Output pixels. */
layout(std430, set = 0, binding = 1) writeonly buffer OutputBuffer
{
	uint pixels[];
};

/* Binding 2: Grid cells — each cell has (offset, count) into the index buffer. */
layout(std430, set = 0, binding = 2) readonly buffer GridCellBuffer
{
	uvec2 gridCells[]; /* x = offset, y = count */
};

/* Binding 3: Grid triangle indices — flat array referenced by grid cells. */
layout(std430, set = 0, binding = 3) readonly buffer GridIndexBuffer
{
	uint gridTriIndices[];
};

layout(push_constant) uniform PushConstants
{
	vec4 rayOriginBase;
	vec4 rayDirRight;
	vec4 rayDirUp;
	vec4 rayDirForward;
	uint resolution;
	uint gridResolution;
	float sliceDepthZ;
	float squareSize;
	float squareMinX;
	float squareMinY;
	float pad0;
	float pad1;
} pc;

bool rayTriangleIntersect(vec3 origin, vec3 dir, vec3 v0, vec3 v1, vec3 v2)
{
	const float eps = 1e-7;

	vec3 e1 = v1 - v0;
	vec3 e2 = v2 - v0;
	vec3 h = cross(dir, e2);
	float a = dot(e1, h);

	if ( abs(a) < eps )
		return false;

	float f = 1.0 / a;
	vec3 s = origin - v0;
	float u = f * dot(s, h);

	if ( u < 0.0 || u > 1.0 )
		return false;

	vec3 q = cross(s, e1);
	float v = f * dot(dir, q);

	if ( v < 0.0 || u + v > 1.0 )
		return false;

	float t = f * dot(e2, q);

	return t > eps;
}

void main()
{
	uint px = gl_GlobalInvocationID.x;
	uint py = gl_GlobalInvocationID.y;

	if ( px >= pc.resolution || py >= pc.resolution )
		return;

	/* Compute view-space XY for this pixel. */
	float viewX = pc.squareMinX + (float(px) + 0.5) / float(pc.resolution) * pc.squareSize;
	float viewY = pc.squareMinY + (float(py) + 0.5) / float(pc.resolution) * pc.squareSize;

	/* World position on slice plane. */
	vec3 worldPoint = pc.rayOriginBase.xyz
		+ pc.rayDirRight.xyz * (float(px) + 0.5)
		+ pc.rayDirUp.xyz * (float(py) + 0.5)
		+ pc.rayDirForward.xyz * pc.sliceDepthZ;

	vec3 rayDir = normalize(pc.rayDirForward.xyz);

	/* Lookup grid cell for this pixel. */
	uint cellX = clamp(uint((viewX - pc.squareMinX) / pc.squareSize * float(pc.gridResolution)), 0u, pc.gridResolution - 1u);
	uint cellY = clamp(uint((viewY - pc.squareMinY) / pc.squareSize * float(pc.gridResolution)), 0u, pc.gridResolution - 1u);
	uint cellIdx = cellY * pc.gridResolution + cellX;

	uint offset = gridCells[cellIdx].x;
	uint count = gridCells[cellIdx].y;

	/* Count intersections only with triangles in this grid cell. */
	uint intersections = 0;

	for ( uint i = 0; i < count; ++i )
	{
		uint triIdx = gridTriIndices[offset + i];
		Triangle tri = triangles[triIdx];

		if ( rayTriangleIntersect(worldPoint, rayDir, tri.v0.xyz, tri.v1.xyz, tri.v2.xyz) )
		{
			++intersections;
		}
	}

	/* Bit-pack: each uint32 stores 32 pixels. atomicOr for thread-safe bit setting. */
	uint pixelIdx = py * pc.resolution + px;
	uint wordIdx = pixelIdx / 32u;
	uint bitIdx = pixelIdx % 32u;

	if ( (intersections & 1u) != 0u )
	{
		atomicOr(pixels[wordIdx], 1u << bitIdx);
	}
}
)";

	/* ---- Push constants structure (must match shader layout) ---- */

	struct PushConstants
	{
		float rayOriginBase[4];
		float rayDirRight[4];
		float rayDirUp[4];
		float rayDirForward[4];
		uint32_t resolution;
		uint32_t gridResolution;
		float sliceDepthZ;
		float squareSize;
		float squareMinX;
		float squareMinY;
		float pad0;
		float pad1;
	};

	static constexpr uint32_t GPUGridRes = 128;

	/* ---- Implementation ---- */

	struct XRayAnalyzer::Impl
	{
		std::shared_ptr< Vulkan::Device > device;
		Saphir::ShaderManager * shaderManager{nullptr};

		/* Shape data. */
		struct ShapeEntry
		{
			const Shape< float > * shape{nullptr};
			CartesianFrame< float > frame;
		};

		std::vector< ShapeEntry > shapes;
		CartesianFrame< float > viewpoint;

		/* GPU resources. */
		std::shared_ptr< Vulkan::ShaderModule > shaderModule;
		std::shared_ptr< Vulkan::DescriptorSetLayout > descriptorSetLayout;
		std::shared_ptr< Vulkan::PipelineLayout > pipelineLayout;
		std::unique_ptr< Vulkan::ComputePipeline > computePipeline;
		std::unique_ptr< Vulkan::ShaderStorageBufferObject > triangleSSBO;
		std::unique_ptr< Vulkan::Buffer > outputSSBO; /* device-local, GPU writes here */
		std::unique_ptr< Vulkan::Buffer > stagingBuffer; /* host-visible, CPU reads from here */
		std::unique_ptr< Vulkan::ShaderStorageBufferObject > gridCellSSBO;
		std::unique_ptr< Vulkan::ShaderStorageBufferObject > gridIndexSSBO;
		std::shared_ptr< Vulkan::DescriptorPool > descriptorPool;
		std::unique_ptr< Vulkan::DescriptorSet > descriptorSet;
		std::shared_ptr< Vulkan::CommandPool > commandPool;
		std::unique_ptr< Vulkan::CommandBuffer > commandBuffer;

		/* Computed bounds in view space. */
		float boundsMin[3]{0, 0, 0};
		float boundsMax[3]{0, 0, 0};
		float squareMinX{0};
		float squareMinY{0};
		float squareSize{0};
		float sliceDepthRange{0};

		/* View axes. */
		Vector< 3, float > forward;
		Vector< 3, float > right;
		Vector< 3, float > up;

		uint32_t resolution{0};
		uint32_t triangleCount{0};
		bool prepared{false};

		/* Transform a point to world space. */
		static Vector< 3, float > transformPoint (const Vector< 3, float > & point, const CartesianFrame< float > & frame)
		{
			const auto r = frame.rightVector();
			const auto u = Vector< 3, float >::crossProduct(frame.forwardVector(), frame.rightVector()).normalized();
			const auto f = frame.forwardVector();

			return frame.position() + r * point[X] + u * point[Y] + f * point[Z];
		}
	};

	/* ---- Public methods ---- */

	XRayAnalyzer::XRayAnalyzer (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept
		: m_impl(std::make_unique< Impl >())
	{
		m_impl->device = device;
		m_impl->shaderManager = &shaderManager;
	}

	XRayAnalyzer::~XRayAnalyzer () noexcept = default;

	void
	XRayAnalyzer::addShape (const Shape< float > & shape, const CartesianFrame< float > & frame) noexcept
	{
		m_impl->shapes.push_back({&shape, frame});
	}

	void
	XRayAnalyzer::setViewpoint (const CartesianFrame< float > & viewpoint) noexcept
	{
		m_impl->viewpoint = viewpoint;
	}

	bool
	XRayAnalyzer::prepare (uint32_t resolution) noexcept
	{
		auto & d = *m_impl;

		if ( d.shapes.empty() || resolution == 0 )
		{
			return false;
		}

		d.resolution = resolution;

		/* Compute view axes. */
		d.forward = d.viewpoint.forwardVector();
		d.right = d.viewpoint.rightVector();
		d.up = Vector< 3, float >::crossProduct(d.forward, d.right).normalized();

		/* Transform all triangles and compute bounds. */
		struct GPUTriangle
		{
			float v0[4]; /* xyz + padding */
			float v1[4];
			float v2[4];
		};

		std::vector< GPUTriangle > gpuTriangles;

		d.boundsMin[0] = d.boundsMin[1] = d.boundsMin[2] = std::numeric_limits< float >::max();
		d.boundsMax[0] = d.boundsMax[1] = d.boundsMax[2] = std::numeric_limits< float >::lowest();

		for ( const auto & entry : d.shapes )
		{
			const auto & tris = entry.shape->triangles();
			const auto & verts = entry.shape->vertices();

			for ( size_t t = 0; t < tris.size(); ++t )
			{
				GPUTriangle gt{};

				for ( int i = 0; i < 3; ++i )
				{
					const auto worldPos = Impl::transformPoint(verts[tris[t].vertexIndex(i)].position(), entry.frame);

					float * dst = (i == 0) ? gt.v0 : (i == 1) ? gt.v1 : gt.v2;
					dst[0] = worldPos[X];
					dst[1] = worldPos[Y];
					dst[2] = worldPos[Z];
					dst[3] = 0.0F;

					/* Update view-space bounds. */
					const auto relative = worldPos - d.viewpoint.position();
					const auto vx = Vector< 3, float >::dotProduct(relative, d.right);
					const auto vy = Vector< 3, float >::dotProduct(relative, d.up);
					const auto vz = Vector< 3, float >::dotProduct(relative, d.forward);

					d.boundsMin[0] = std::min(d.boundsMin[0], vx);
					d.boundsMin[1] = std::min(d.boundsMin[1], vy);
					d.boundsMin[2] = std::min(d.boundsMin[2], vz);
					d.boundsMax[0] = std::max(d.boundsMax[0], vx);
					d.boundsMax[1] = std::max(d.boundsMax[1], vy);
					d.boundsMax[2] = std::max(d.boundsMax[2], vz);
				}

				gpuTriangles.push_back(gt);
			}
		}

		d.triangleCount = static_cast< uint32_t >(gpuTriangles.size());

		/* Pad bounds. */
		for ( int i = 0; i < 3; ++i )
		{
			d.boundsMin[i] -= 0.01F;
			d.boundsMax[i] += 0.01F;
		}

		/* Square the XY extent. */
		const auto sliceWidth = d.boundsMax[0] - d.boundsMin[0];
		const auto sliceHeight = d.boundsMax[1] - d.boundsMin[1];
		const auto maxExtent = std::max(sliceWidth, sliceHeight);

		d.squareMinX = d.boundsMin[0] - (maxExtent - sliceWidth) * 0.5F;
		d.squareMinY = d.boundsMin[1] - (maxExtent - sliceHeight) * 0.5F;
		d.squareSize = maxExtent;
		d.sliceDepthRange = d.boundsMax[2] - d.boundsMin[2];

		TraceInfo{ClassId} << "GPU XRay: " << d.triangleCount << " triangles, " << resolution << "x" << resolution << " resolution.";

		/* Step 1: Compile compute shader. */
		d.shaderModule = d.shaderManager->getShaderModuleFromSourceCode(d.device, "XRayComputeShader", Saphir::ShaderType::ComputeShader, ComputeShaderSource);

		if ( d.shaderModule == nullptr )
		{
			Tracer::error(ClassId, "Failed to compile compute shader !");

			return false;
		}

		/* Step 2: Create descriptor set layout. */
		d.descriptorSetLayout = std::make_shared< Vulkan::DescriptorSetLayout >(d.device, "XRayDSLayout");

		/* Binding 0: Triangle SSBO (read-only). */
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 0;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			d.descriptorSetLayout->declare(binding);
		}

		/* Binding 1: Output SSBO (write-only). */
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			d.descriptorSetLayout->declare(binding);
		}

		/* Binding 2: Grid cells SSBO (read-only). */
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 2;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			d.descriptorSetLayout->declare(binding);
		}

		/* Binding 3: Grid triangle indices SSBO (read-only). */
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 3;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			d.descriptorSetLayout->declare(binding);
		}

		if ( !d.descriptorSetLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create descriptor set layout !");

			return false;
		}

		/* Step 3: Pipeline layout with push constants. */
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants);

		d.pipelineLayout = std::make_shared< Vulkan::PipelineLayout >(
			d.device, "XRayPipelineLayout",
			Libs::StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 4 >{d.descriptorSetLayout},
			Libs::StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
		);

		if ( !d.pipelineLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create pipeline layout !");

			return false;
		}

		/* Step 4: Compute pipeline. */
		d.computePipeline = std::make_unique< Vulkan::ComputePipeline >(d.pipelineLayout);
		d.computePipeline->setShaderModule(d.shaderModule->handle());

		if ( !d.computePipeline->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create compute pipeline !");

			return false;
		}

		/* Step 5: Create SSBOs. */
		const auto triangleBufferSize = static_cast< VkDeviceSize >(gpuTriangles.size() * sizeof(GPUTriangle));
		d.triangleSSBO = std::make_unique< Vulkan::ShaderStorageBufferObject >(d.device, triangleBufferSize, true);

		if ( !d.triangleSSBO->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create triangle SSBO !");

			return false;
		}

		/* Upload triangle data. */
		{
			auto * mapped = d.triangleSSBO->mapMemoryAs< GPUTriangle >();

			if ( mapped == nullptr )
			{
				Tracer::error(ClassId, "Failed to map triangle SSBO !");

				return false;
			}

			std::memcpy(mapped, gpuTriangles.data(), triangleBufferSize);
			d.triangleSSBO->unmapMemory();
		}

		/* Build 2D spatial grid in view-space XY and upload. */
		{
			struct GridCell
			{
				uint32_t offset;
				uint32_t count;
			};

			std::vector< std::vector< uint32_t > > cellTriangles(GPUGridRes * GPUGridRes);

			for ( uint32_t t = 0; t < d.triangleCount; ++t )
			{
				const auto & gt = gpuTriangles[t];

				/* Project triangle to view space XY. */
				float minVX = std::numeric_limits< float >::max();
				float maxVX = std::numeric_limits< float >::lowest();
				float minVY = std::numeric_limits< float >::max();
				float maxVY = std::numeric_limits< float >::lowest();

				for ( int i = 0; i < 3; ++i )
				{
					const float * v = (i == 0) ? gt.v0 : (i == 1) ? gt.v1 : gt.v2;
					const Vector< 3, float > worldPos{v[0], v[1], v[2]};
					const auto relative = worldPos - d.viewpoint.position();
					const auto vx = Vector< 3, float >::dotProduct(relative, d.right);
					const auto vy = Vector< 3, float >::dotProduct(relative, d.up);

					minVX = std::min(minVX, vx);
					maxVX = std::max(maxVX, vx);
					minVY = std::min(minVY, vy);
					maxVY = std::max(maxVY, vy);
				}

				const auto gridScale = static_cast< float >(GPUGridRes);
				auto cellMinX = static_cast< uint32_t >(std::clamp((minVX - d.squareMinX) / d.squareSize * gridScale, 0.0F, static_cast< float >(GPUGridRes - 1)));
				auto cellMaxX = static_cast< uint32_t >(std::clamp((maxVX - d.squareMinX) / d.squareSize * gridScale, 0.0F, static_cast< float >(GPUGridRes - 1)));
				auto cellMinY = static_cast< uint32_t >(std::clamp((minVY - d.squareMinY) / d.squareSize * gridScale, 0.0F, static_cast< float >(GPUGridRes - 1)));
				auto cellMaxY = static_cast< uint32_t >(std::clamp((maxVY - d.squareMinY) / d.squareSize * gridScale, 0.0F, static_cast< float >(GPUGridRes - 1)));

				for ( uint32_t cy = cellMinY; cy <= cellMaxY; ++cy )
				{
					for ( uint32_t cx = cellMinX; cx <= cellMaxX; ++cx )
					{
						cellTriangles[cy * GPUGridRes + cx].push_back(t);
					}
				}
			}

			/* Flatten into offset/count + index array. */
			std::vector< GridCell > gridCells(GPUGridRes * GPUGridRes);
			std::vector< uint32_t > gridIndices;

			for ( uint32_t c = 0; c < GPUGridRes * GPUGridRes; ++c )
			{
				gridCells[c].offset = static_cast< uint32_t >(gridIndices.size());
				gridCells[c].count = static_cast< uint32_t >(cellTriangles[c].size());

				gridIndices.insert(gridIndices.end(), cellTriangles[c].begin(), cellTriangles[c].end());
			}

			/* Upload grid cells SSBO. */
			const auto gridCellSize = static_cast< VkDeviceSize >(gridCells.size() * sizeof(GridCell));
			d.gridCellSSBO = std::make_unique< Vulkan::ShaderStorageBufferObject >(d.device, gridCellSize, true);

			if ( !d.gridCellSSBO->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create grid cell SSBO !");

				return false;
			}

			{
				auto * mapped = d.gridCellSSBO->mapMemoryAs< GridCell >();
				std::memcpy(mapped, gridCells.data(), gridCellSize);
				d.gridCellSSBO->unmapMemory();
			}

			/* Upload grid indices SSBO. */
			if ( gridIndices.empty() )
			{
				gridIndices.push_back(0); /* Dummy to avoid zero-size buffer. */
			}

			const auto gridIndexSize = static_cast< VkDeviceSize >(gridIndices.size() * sizeof(uint32_t));
			d.gridIndexSSBO = std::make_unique< Vulkan::ShaderStorageBufferObject >(d.device, gridIndexSize, true);

			if ( !d.gridIndexSSBO->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create grid index SSBO !");

				return false;
			}

			{
				auto * mapped = d.gridIndexSSBO->mapMemoryAs< uint32_t >();
				std::memcpy(mapped, gridIndices.data(), gridIndexSize);
				d.gridIndexSSBO->unmapMemory();
			}

			TraceInfo{ClassId} << "GPU grid: " << GPUGridRes << "x" << GPUGridRes << " cells, " << gridIndices.size() << " index entries.";
		}

		/* Bit-packed: 32 pixels per uint32. */
		const auto totalPixels = static_cast< VkDeviceSize >(resolution) * resolution;
		const auto outputWordCount = (totalPixels + 31) / 32;
		const auto outputBufferSize = outputWordCount * sizeof(uint32_t);

		/* Output SSBO: device-local for fast GPU writes + transfer source for staging copy. */
		d.outputSSBO = std::make_unique< Vulkan::Buffer >(
			d.device,
			0,
			outputBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			false /* device-local */
		);

		if ( !d.outputSSBO->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create output SSBO !");

			return false;
		}

		/* Staging buffer: host-visible + host-cached for fast CPU reads. */
		d.stagingBuffer = std::make_unique< Vulkan::Buffer >(
			d.device,
			0, /* createFlags */
			outputBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT, /* destination for vkCmdCopyBuffer */
			true /* hostVisible */
		);

		d.stagingBuffer->setHostReadable(true); /* HOST_CACHED_BIT — CPU reads from L3 cache, not PCIe. */

		if ( !d.stagingBuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create staging buffer !");

			return false;
		}

		/* Step 6: Descriptor pool and set. */
		std::vector< VkDescriptorPoolSize > poolSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4}
		};

		d.descriptorPool = std::make_shared< Vulkan::DescriptorPool >(d.device, poolSizes, 1);

		if ( !d.descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create descriptor pool !");

			return false;
		}

		d.descriptorSet = std::make_unique< Vulkan::DescriptorSet >(d.descriptorPool, d.descriptorSetLayout);

		if ( !d.descriptorSet->create() )
		{
			Tracer::error(ClassId, "Failed to allocate descriptor set !");

			return false;
		}

		/* Write descriptors. */
		VkDescriptorBufferInfo triBufferInfo{};
		triBufferInfo.buffer = d.triangleSSBO->handle();
		triBufferInfo.offset = 0;
		triBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo outBufferInfo{};
		outBufferInfo.buffer = d.outputSSBO->handle();
		outBufferInfo.offset = 0;
		outBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo gridCellBufferInfo{};
		gridCellBufferInfo.buffer = d.gridCellSSBO->handle();
		gridCellBufferInfo.offset = 0;
		gridCellBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo gridIndexBufferInfo{};
		gridIndexBufferInfo.buffer = d.gridIndexSSBO->handle();
		gridIndexBufferInfo.offset = 0;
		gridIndexBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet writes[4]{};

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = d.descriptorSet->handle();
		writes[0].dstBinding = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[0].descriptorCount = 1;
		writes[0].pBufferInfo = &triBufferInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = d.descriptorSet->handle();
		writes[1].dstBinding = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[1].descriptorCount = 1;
		writes[1].pBufferInfo = &outBufferInfo;

		writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet = d.descriptorSet->handle();
		writes[2].dstBinding = 2;
		writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[2].descriptorCount = 1;
		writes[2].pBufferInfo = &gridCellBufferInfo;

		writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[3].dstSet = d.descriptorSet->handle();
		writes[3].dstBinding = 3;
		writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[3].descriptorCount = 1;
		writes[3].pBufferInfo = &gridIndexBufferInfo;

		vkUpdateDescriptorSets(d.device->handle(), 4, writes, 0, nullptr);

		/* Step 7: Command pool and buffer. */
		const auto queueFamilyIndex = d.device->hasComputeQueues() ? d.device->getComputeFamilyIndex() : d.device->getGraphicsFamilyIndex();
		d.commandPool = std::make_shared< Vulkan::CommandPool >(d.device, queueFamilyIndex, true, true, false);

		if ( !d.commandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create command pool !");

			return false;
		}

		d.commandBuffer = std::make_unique< Vulkan::CommandBuffer >(d.commandPool, true);
		d.prepared = true;

		TraceInfo{ClassId} << "GPU XRay prepared successfully.";

		return true;
	}

	Pixmap< uint8_t >
	XRayAnalyzer::scan (float depth) noexcept
	{
		auto & d = *m_impl;

		if ( !d.prepared )
		{
			return {};
		}

		const auto res = d.resolution;

		/* Build push constants. */
		const auto pixelStepX = d.squareSize / static_cast< float >(res);
		const auto pixelStepY = d.squareSize / static_cast< float >(res);

		const auto baseOrigin = d.viewpoint.position() + d.right * d.squareMinX + d.up * d.squareMinY;
		const auto depthZ = d.boundsMin[2] + depth * d.sliceDepthRange;

		PushConstants pc{};
		pc.rayOriginBase[0] = baseOrigin[X];
		pc.rayOriginBase[1] = baseOrigin[Y];
		pc.rayOriginBase[2] = baseOrigin[Z];
		pc.rayDirRight[0] = d.right[X] * pixelStepX;
		pc.rayDirRight[1] = d.right[Y] * pixelStepX;
		pc.rayDirRight[2] = d.right[Z] * pixelStepX;
		pc.rayDirUp[0] = d.up[X] * pixelStepY;
		pc.rayDirUp[1] = d.up[Y] * pixelStepY;
		pc.rayDirUp[2] = d.up[Z] * pixelStepY;
		pc.rayDirForward[0] = d.forward[X];
		pc.rayDirForward[1] = d.forward[Y];
		pc.rayDirForward[2] = d.forward[Z];
		pc.resolution = res;
		pc.gridResolution = GPUGridRes;
		pc.sliceDepthZ = depthZ;
		pc.squareSize = d.squareSize;
		pc.squareMinX = d.squareMinX;
		pc.squareMinY = d.squareMinY;

		/* Record commands. */
		static_cast< void >(d.commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
		d.commandBuffer->bind(*d.computePipeline);
		d.commandBuffer->bind(*d.descriptorSet, *d.pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

		vkCmdPushConstants(d.commandBuffer->handle(), d.pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pc);

		const auto groupsX = (res + 15) / 16;
		const auto groupsY = (res + 15) / 16;

		d.commandBuffer->dispatch(groupsX, groupsY, 1);

		/* Barrier: compute writes → transfer reads. */
		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.buffer = d.outputSSBO->handle();
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		d.commandBuffer->pipelineBarrier(
			std::span< const VkBufferMemoryBarrier >{&barrier, 1},
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT
		);

		/* Copy output SSBO (device-local) → staging buffer (host-visible). */
		VkBufferCopy copyRegion{};
		copyRegion.size = static_cast< VkDeviceSize >(res) * res * sizeof(uint32_t);

		vkCmdCopyBuffer(d.commandBuffer->handle(), d.outputSSBO->handle(), d.stagingBuffer->handle(), 1, &copyRegion);

		static_cast< void >(d.commandBuffer->end());

		/* Submit and wait. */
		auto gpuStart = std::chrono::steady_clock::now();

		auto * queue = d.device->hasComputeQueues() ? d.device->getComputeQueue(Vulkan::QueuePriority::High) : d.device->getGraphicsQueue(Vulkan::QueuePriority::High);
		static_cast< void >(queue->submit(*d.commandBuffer));
		static_cast< void >(queue->waitIdle());

		auto gpuEnd = std::chrono::steady_clock::now();
		auto gpuMs = std::chrono::duration_cast< std::chrono::milliseconds >(gpuEnd - gpuStart).count();

		/* Reset command buffer for reuse. */
		static_cast< void >(d.commandBuffer->reset());

		/* Readback. */

		Pixmap< uint8_t > output(res, res, ChannelMode::RGBA, Color< float >{0.0F, 0.0F, 0.0F, 1.0F});

		auto readStart = std::chrono::steady_clock::now();

		auto * mapped = d.stagingBuffer->mapMemory();

		if ( mapped != nullptr )
		{
			const auto byteCount = static_cast< size_t >(res) * res * 4; /* RGBA = 4 bytes/pixel */

			std::memcpy(output.data().data(), mapped, byteCount);

			d.stagingBuffer->unmapMemory();
		}

		auto readEnd = std::chrono::steady_clock::now();
		auto readMs = std::chrono::duration_cast< std::chrono::milliseconds >(readEnd - readStart).count();

		TraceInfo{ClassId} << "GPU: " << gpuMs << " ms, readback: " << readMs << " ms.";

		return output;
	}

	void
	XRayAnalyzer::scanAll (uint32_t sliceCount, const std::function< void (uint32_t, const Pixmap< uint8_t > &) > & callback) noexcept
	{
		auto & d = *m_impl;

		if ( !d.prepared || sliceCount == 0 )
		{
			return;
		}

		const auto res = d.resolution;

		const auto totalPixels = static_cast< size_t >(res) * res;
		const auto packedWordCount = (totalPixels + 31) / 32;
		const auto packedByteCount = packedWordCount * sizeof(uint32_t);

		/* CPU-side buffer for packed bits readback. */
		std::vector< uint32_t > packedData(packedWordCount);

		/* Dummy pixmap for callback signature (empty — timing mode). */
		Pixmap< uint8_t > dummyOutput;

		/* Precompute constants outside the loop. */
		const auto pixelStepX = d.squareSize / static_cast< float >(res);
		const auto pixelStepY = d.squareSize / static_cast< float >(res);
		const auto baseOrigin = d.viewpoint.position() + d.right * d.squareMinX + d.up * d.squareMinY;
		const auto groupsX = (res + 15) / 16;
		const auto groupsY = (res + 15) / 16;

		auto * queue = d.device->hasComputeQueues() ? d.device->getComputeQueue(Vulkan::QueuePriority::High) : d.device->getGraphicsQueue(Vulkan::QueuePriority::High);

		for ( uint32_t i = 0; i < sliceCount; ++i )
		{
			const auto depth = static_cast< float >(i) / static_cast< float >(sliceCount - 1);
			const auto depthZ = d.boundsMin[2] + depth * d.sliceDepthRange;

			PushConstants pc{};
			pc.rayOriginBase[0] = baseOrigin[X]; pc.rayOriginBase[1] = baseOrigin[Y]; pc.rayOriginBase[2] = baseOrigin[Z];
			pc.rayDirRight[0] = d.right[X] * pixelStepX; pc.rayDirRight[1] = d.right[Y] * pixelStepX; pc.rayDirRight[2] = d.right[Z] * pixelStepX;
			pc.rayDirUp[0] = d.up[X] * pixelStepY; pc.rayDirUp[1] = d.up[Y] * pixelStepY; pc.rayDirUp[2] = d.up[Z] * pixelStepY;
			pc.rayDirForward[0] = d.forward[X]; pc.rayDirForward[1] = d.forward[Y]; pc.rayDirForward[2] = d.forward[Z];
			pc.resolution = res;
			pc.gridResolution = GPUGridRes;
			pc.sliceDepthZ = depthZ;
			pc.squareSize = d.squareSize;
			pc.squareMinX = d.squareMinX;
			pc.squareMinY = d.squareMinY;

			static_cast< void >(d.commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));

			/* Clear the output buffer (atomicOr accumulates — must start at 0). */
			vkCmdFillBuffer(d.commandBuffer->handle(), d.outputSSBO->handle(), 0, VK_WHOLE_SIZE, 0);

			/* Barrier: transfer clear → compute write. */
			{
				VkBufferMemoryBarrier clearBarrier{};
				clearBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				clearBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				clearBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				clearBarrier.buffer = d.outputSSBO->handle();
				clearBarrier.offset = 0;
				clearBarrier.size = VK_WHOLE_SIZE;

				d.commandBuffer->pipelineBarrier(
					std::span< const VkBufferMemoryBarrier >{&clearBarrier, 1},
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				);
			}

			d.commandBuffer->bind(*d.computePipeline);
			d.commandBuffer->bind(*d.descriptorSet, *d.pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

			vkCmdPushConstants(d.commandBuffer->handle(), d.pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pc);

			d.commandBuffer->dispatch(groupsX, groupsY, 1);

			/* Barrier: compute write → transfer read. */
			{
				VkBufferMemoryBarrier computeBarrier{};
				computeBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				computeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				computeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				computeBarrier.buffer = d.outputSSBO->handle();
				computeBarrier.offset = 0;
				computeBarrier.size = VK_WHOLE_SIZE;

				d.commandBuffer->pipelineBarrier(
					std::span< const VkBufferMemoryBarrier >{&computeBarrier, 1},
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* Copy packed output → staging. */
			VkBufferCopy copyRegion{};
			copyRegion.size = packedByteCount;
			vkCmdCopyBuffer(d.commandBuffer->handle(), d.outputSSBO->handle(), d.stagingBuffer->handle(), 1, &copyRegion);

			static_cast< void >(d.commandBuffer->end());

			static_cast< void >(queue->submit(*d.commandBuffer));
			static_cast< void >(queue->waitIdle());
			static_cast< void >(d.commandBuffer->reset());

			/* Readback: bulk memcpy packed bits from staging. */
			auto * mapped = d.stagingBuffer->mapMemory();

			if ( mapped != nullptr )
			{
				std::memcpy(packedData.data(), mapped, packedByteCount);
				d.stagingBuffer->unmapMemory();
			}

			callback(i, dummyOutput);
		}
	}
}
