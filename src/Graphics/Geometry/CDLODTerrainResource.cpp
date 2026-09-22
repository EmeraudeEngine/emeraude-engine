/*
 * src/Graphics/Geometry/CDLODTerrainResource.cpp
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

#include "CDLODTerrainResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "PrimaryServices.hpp"
#include "Saphir/Generator/HeightfieldSurfaceHelper.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "ThreadPool.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/UniformBufferObject.hpp"

namespace
{
	/* The normal of every texel of a clip level, from the heights of that level (central differences,
	 * toroidal neighbours). The heights are a tent-filtered pyramid, and the gradient is linear: the
	 * gradient of the filtered heights IS the filtered gradient, so a coarse level stores the mean slope
	 * of its footprint, not an aliased one. Stored as the X and Z of the unit normal. */
	constexpr auto NormalBakeComputeShader = R"GLSL(#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform sampler2DArray heights;
layout(set = 0, binding = 1, rg16f) uniform writeonly image2DArray normals;

layout(push_constant) uniform Bake
{
	ivec2 origin;
	ivec2 size;
	int layer;
	float texelSize;
	float heightRange;
	int texelCount;
} pc;

float heightAt (ivec2 texel)
{
	const int n = pc.texelCount;

	return texelFetch(heights, ivec3((texel.x + n) % n, (texel.y + n) % n, pc.layer), 0).r;
}

void main ()
{
	const ivec2 local = ivec2(gl_GlobalInvocationID.xy);

	if ( local.x >= pc.size.x || local.y >= pc.size.y )
	{
		return;
	}

	const ivec2 texel = pc.origin + local;

	/* Row index = +Z, column index = +X: n = normalize(-dh/dx, 1, -dh/dz). */
	const float scale = pc.heightRange / (2.0 * pc.texelSize);
	const float slopeX = (heightAt(texel + ivec2(1, 0)) - heightAt(texel - ivec2(1, 0))) * scale;
	const float slopeZ = (heightAt(texel + ivec2(0, 1)) - heightAt(texel - ivec2(0, 1))) * scale;
	const vec3 normal = normalize(vec3(-slopeX, 1.0, -slopeZ));

	imageStore(normals, ivec3(texel, pc.layer), vec4(normal.x, normal.z, 0.0, 0.0));
}
)GLSL";

	/** @brief Push constants of the normal bake (mirror of the GLSL block). */
	struct NormalBakeConstants
	{
		std::array< int32_t, 2 > origin{};
		std::array< int32_t, 2 > size{};
		int32_t layer{0};
		float texelSize{1.0F};
		float heightRange{1.0F};
		int32_t texelCount{1};
	};

	static_assert(sizeof(NormalBakeConstants) == 32);

	/** @brief Floor division for signed lattice indices. */
	[[nodiscard]]
	int64_t
	floorDivide (int64_t value, int64_t divisor) noexcept
	{
		const auto quotient = value / divisor;

		return (value % divisor != 0 && (value < 0) != (divisor < 0)) ? quotient - 1 : quotient;
	}

	/** @brief Positive modulo for signed lattice indices. */
	[[nodiscard]]
	int64_t
	positiveModulo (int64_t value, int64_t divisor) noexcept
	{
		const auto remainder = value % divisor;

		return remainder < 0 ? remainder + divisor : remainder;
	}

	/** @brief One piece of a lattice range once wrapped on the texture: {texel start, length, lattice start}. */
	struct ToroidalSegment
	{
		int64_t texel{0};
		int64_t length{0};
		int64_t lattice{0};
	};

	/** @brief Cuts the lattice range [begin, end) at the texture's wrap points (at most two pieces when end - begin <= texels). */
	[[nodiscard]]
	std::vector< ToroidalSegment >
	toroidalSegments (int64_t begin, int64_t end, int64_t texels) noexcept
	{
		std::vector< ToroidalSegment > segments;

		for ( auto lattice = begin; lattice < end; )
		{
			const auto texel = positiveModulo(lattice, texels);
			const auto length = std::min(end - lattice, texels - texel);

			segments.push_back({texel, length, lattice});

			lattice += length;
		}

		return segments;
	}
}

namespace EmEn::Graphics::Geometry
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Vulkan;

	CDLODTerrainResource::CDLODTerrainResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name) noexcept
		: Interface{serviceProvider, name, EnableHeightfieldSurface}
	{

	}

	CDLODTerrainResource::~CDLODTerrainResource ()
	{
		this->destroyFromHardware(true);
	}

	bool
	CDLODTerrainResource::isCreated () const noexcept
	{
		if ( m_vertexBufferObject == nullptr || !m_vertexBufferObject->isCreated() )
		{
			return false;
		}

		if ( m_indexBufferObject == nullptr || !m_indexBufferObject->isCreated() )
		{
			return false;
		}

		return !m_descriptorSets.empty();
	}

	bool
	CDLODTerrainResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by default!");

		return this->setLoadSuccess(false);
	}

	bool
	CDLODTerrainResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by a JSON file!");

		return this->setLoadSuccess(false);
	}

	size_t
	CDLODTerrainResource::memoryOccupied () const noexcept
	{
		size_t bytes = 0;

		for ( const auto & level : m_pyramid )
		{
			bytes += level.size() * sizeof(uint16_t);
		}

		for ( const auto & ranges : m_nodeHeightRanges )
		{
			bytes += ranges.size() * sizeof(std::array< float, 2 >);
		}

		if ( m_vertexBufferObject != nullptr )
		{
			bytes += m_vertexBufferObject->bytes();
		}

		if ( m_indexBufferObject != nullptr )
		{
			bytes += m_indexBufferObject->bytes();
		}

		return bytes;
	}

	bool
	CDLODTerrainResource::load (const std::shared_ptr< const Grid< float > > & source, const CDLODTerrainParameters & parameters) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		const auto start = std::chrono::steady_clock::now();

		if ( source == nullptr || !source->isValid() )
		{
			Tracer::error(ClassId, "The height grid is invalid !");

			return this->setLoadSuccess(false);
		}

		const auto cellCount = source->squaredQuadCount();
		const auto patchQuads = parameters.patchQuads;

		if ( patchQuads < 2 || patchQuads % 2 != 0 || !isPowerOfTwo(cellCount) || cellCount % patchQuads != 0 || !isPowerOfTwo(cellCount / patchQuads) )
		{
			TraceError{ClassId} << "Terrain '" << this->name() << "': the grid's " << cellCount << " cells must be a power of two, and a power-of-two multiple of the patch (" << patchQuads << " quads, even).";

			return this->setLoadSuccess(false);
		}

		if ( !isPowerOfTwo(parameters.clipTexels) || parameters.clipTexels < 256 || parameters.clipUpdateTexels == 0 || parameters.clipUpdateTexels * 8 > parameters.clipTexels )
		{
			TraceError{ClassId} << "Terrain '" << this->name() << "': a clip level of " << parameters.clipTexels << " texels (a power of two, at least 256) cannot move by " << parameters.clipUpdateTexels << " texels (at most an eighth of it).";

			return this->setLoadSuccess(false);
		}

		m_source = source;
		m_parameters = parameters;
		m_cellCount = cellCount;
		m_cellSize = source->quadSize();
		m_boundingBox = source->boundingBox();
		m_boundingSphere = source->boundingSphere();
		m_heightMinimum = m_boundingBox.minimum(Y);
		m_heightRange = std::max(m_boundingBox.maximum(Y) - m_heightMinimum, 0.001F);

		/* The quadtree: a leaf covers one patch, the root the whole grid. */
		m_levelOfDetailCount = static_cast< uint32_t >(std::log2(cellCount / patchQuads)) + 1;

		if ( m_levelOfDetailCount > HeightfieldSurface::MaxLevelsOfDetail )
		{
			TraceError{ClassId} << "Terrain '" << this->name() << "': " << m_levelOfDetailCount << " levels of detail, " << HeightfieldSurface::MaxLevelsOfDetail << " at most — use a larger patch.";

			return this->setLoadSuccess(false);
		}

		/* A clip level smaller than the whole grid is wasted memory: a small terrain gets a smaller texture. */
		const auto margin = 2U * (m_parameters.clipUpdateTexels + 2U);

		while ( m_parameters.clipTexels / 2U >= cellCount + margin && m_parameters.clipTexels > 256U )
		{
			m_parameters.clipTexels /= 2U;
		}

		/* The clip levels: as many as it takes for the last one to hold the WHOLE grid with its margin —
		 * then no LOD can ever read outside a level. A vertex-centred grid of N cells has N + 1 points,
		 * so a level of exactly N texels would wrap its first row onto its last. */
		m_clipLevelCount = 1;

		/* NOTE: The same test as levelCenterFor()'s "holds the whole grid", so the last level never moves. */
		while ( (cellCount >> (m_clipLevelCount - 1)) + 1U + margin > m_parameters.clipTexels )
		{
			++m_clipLevelCount;
		}

		if ( m_clipLevelCount > HeightfieldSurface::MaxClipLevels || (cellCount >> m_clipLevelCount) == 0 )
		{
			TraceError{ClassId} << "Terrain '" << this->name() << "': " << m_clipLevelCount << " clip levels of " << m_parameters.clipTexels << " texels would be needed for " << cellCount << " cells, " << HeightfieldSurface::MaxClipLevels << " at most.";

			return this->setLoadSuccess(false);
		}

		/* The pyramid: level l = tent-filtered level l-1, stored at 16 bits over the height range. */
		m_pyramid.clear();
		m_pyramid.reserve(m_clipLevelCount - 1);

		{
			const auto quantize = [this] (const Grid< float > & grid) {
				std::vector< uint16_t > level(grid.pointCount());
				const auto scale = 65535.0F / m_heightRange;
				const auto points = grid.squaredPointCount();

				for ( uint32_t z = 0; z < points; ++z )
				{
					for ( uint32_t x = 0; x < points; ++x )
					{
						const auto value = std::round((grid.getHeightAt(x, z) - m_heightMinimum) * scale);

						level[grid.index(x, z)] = static_cast< uint16_t >(std::clamp(value, 0.0F, 65535.0F));
					}
				}

				return level;
			};

			Grid< float > current;

			for ( uint32_t level = 1; level < m_clipLevelCount; ++level )
			{
				current = level == 1 ? source->halvedTent() : current.halvedTent();

				if ( !current.isValid() )
				{
					TraceError{ClassId} << "Terrain '" << this->name() << "': unable to build clip level " << level << " of the height pyramid !";

					return this->setLoadSuccess(false);
				}

				m_pyramid.emplace_back(quantize(current));
			}
		}

		this->buildNodeHeightRanges();

		if ( !this->buildLevelOfDetailTables() )
		{
			return this->setLoadSuccess(false);
		}

		/* The grid's UVs are linear in the point index: u = u(0) + index / N · U. In world metres, with
		 * the grid centred on the origin, that is u(x) = x · U / size + (u(0) + U / 2). */
		{
			const auto size = static_cast< float >(cellCount) * m_cellSize;
			const auto origin = source->textureCoordinates2D(0U, 0U);

			m_textureCoordinates = {
				source->UMultiplier() / size,
				source->VMultiplier() / size,
				origin[0] + (source->UMultiplier() * 0.5F),
				origin[1] + (source->VMultiplier() * 0.5F)
			};
		}

		size_t pyramidBytes = 0;

		for ( const auto & level : m_pyramid )
		{
			pyramidBytes += level.size() * sizeof(uint16_t);
		}

		TraceInfo{ClassId} <<
			"Terrain '" << this->name() << "': " << cellCount << " cells of " << m_cellSize << " m, " <<
			m_levelOfDetailCount << " levels of detail (patch " << patchQuads << " quads, detail distance " << m_parameters.detailDistance << " m), " <<
			m_clipLevelCount << " clip levels of " << m_parameters.clipTexels << " texels, heights " << m_heightMinimum << " m + [0, " << m_heightRange << "] m, height pyramid " << (pyramidBytes / 1048576) << " MiB, built in " <<
			std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - start).count() << " ms.";

		return this->setLoadSuccess(true);
	}

	void
	CDLODTerrainResource::buildNodeHeightRanges () noexcept
	{
		const auto patchQuads = m_parameters.patchQuads;
		const auto leavesPerAxis = m_cellCount / patchQuads;

		m_nodeHeightRanges.assign(m_levelOfDetailCount, {});

		/* Leaves: every point of the patch, borders included (a neighbour shares them). */
		auto & leaves = m_nodeHeightRanges[0];
		leaves.resize(static_cast< size_t >(leavesPerAxis) * leavesPerAxis);

		for ( uint32_t leafZ = 0; leafZ < leavesPerAxis; ++leafZ )
		{
			for ( uint32_t leafX = 0; leafX < leavesPerAxis; ++leafX )
			{
				auto minimum = std::numeric_limits< float >::max();
				auto maximum = std::numeric_limits< float >::lowest();

				for ( uint32_t z = leafZ * patchQuads; z <= (leafZ + 1) * patchQuads; ++z )
				{
					for ( uint32_t x = leafX * patchQuads; x <= (leafX + 1) * patchQuads; ++x )
					{
						const auto height = m_source->getHeightAt(x, z);

						minimum = std::min(minimum, height);
						maximum = std::max(maximum, height);
					}
				}

				leaves[(static_cast< size_t >(leafZ) * leavesPerAxis) + leafX] = {minimum, maximum};
			}
		}

		/* Parents: the union of their four children. A clip level holds MEAN heights, and a mean lies
		 * between the extremes it averages, so these boxes bound every level's surface too. */
		for ( uint32_t level = 1; level < m_levelOfDetailCount; ++level )
		{
			const auto childrenPerAxis = leavesPerAxis >> (level - 1);
			const auto nodesPerAxis = leavesPerAxis >> level;
			const auto & children = m_nodeHeightRanges[level - 1];
			auto & nodes = m_nodeHeightRanges[level];

			nodes.resize(static_cast< size_t >(nodesPerAxis) * nodesPerAxis);

			for ( uint32_t nodeZ = 0; nodeZ < nodesPerAxis; ++nodeZ )
			{
				for ( uint32_t nodeX = 0; nodeX < nodesPerAxis; ++nodeX )
				{
					auto range = children[(static_cast< size_t >(nodeZ * 2) * childrenPerAxis) + (nodeX * 2)];

					for ( uint32_t child = 1; child < 4; ++child )
					{
						const auto & childRange = children[(static_cast< size_t >((nodeZ * 2) + (child / 2)) * childrenPerAxis) + (nodeX * 2) + (child % 2)];

						range[0] = std::min(range[0], childRange[0]);
						range[1] = std::max(range[1], childRange[1]);
					}

					nodes[(static_cast< size_t >(nodeZ) * nodesPerAxis) + nodeX] = range;
				}
			}
		}
	}

	bool
	CDLODTerrainResource::buildLevelOfDetailTables () noexcept
	{
		const auto patchQuads = static_cast< float >(m_parameters.patchQuads);
		const auto halfTexels = static_cast< float >(m_parameters.clipTexels / 2U);
		const auto slackTexels = static_cast< float >(m_parameters.clipUpdateTexels + 2U);

		/* A node of level k reads clip level k: its farthest vertex (range + node diagonal) must stay
		 * inside that level's valid extent even when the level lags the camera by a strip. Divided by
		 * 2^k, the condition no longer depends on k. */
		const auto coverageLimit = (halfTexels - slackTexels - (patchQuads * std::sqrt(2.0F))) * m_cellSize;

		if ( m_parameters.detailDistance > coverageLimit )
		{
			TraceWarning{ClassId} << "Terrain '" << this->name() << "': a detail distance of " << m_parameters.detailDistance << " m would outrun the clip levels, clamped to " << coverageLimit << " m.";

			m_parameters.detailDistance = coverageLimit;
		}

		/* Strugar's rule of thumb: a level's ring must be wide enough for the nodes of that level, or two
		 * levels meet across one node and the morph cannot hide it. */
		if ( m_parameters.detailDistance < 2.0F * std::sqrt(2.0F) * patchQuads * m_cellSize )
		{
			TraceWarning{ClassId} << "Terrain '" << this->name() << "': a detail distance of " << m_parameters.detailDistance << " m is short for patches of " << (patchQuads * m_cellSize) << " m — levels may meet two by two.";
		}

		const auto ratio = std::clamp(m_parameters.morphStartRatio, 0.0F, 0.95F);

		for ( uint32_t level = 0; level < HeightfieldSurface::MaxLevelsOfDetail; ++level )
		{
			m_lodRanges[level] = m_parameters.detailDistance * static_cast< float >(1U << std::min(level, 30U));
		}

		/* The root is drawn from anywhere: nothing lies beyond it. */
		m_lodRanges[m_levelOfDetailCount - 1] = std::numeric_limits< float >::max();

		for ( uint32_t level = 0; level < HeightfieldSurface::MaxLevelsOfDetail; ++level )
		{
			if ( level + 1 >= m_levelOfDetailCount )
			{
				/* The coarsest level never morphs: nothing is coarser. (A finite start and a zero slope
				 * keep the shader's product a zero, never an infinity times zero.) */
				m_morphTable[level] = {1.0e30F, 0.0F};

				continue;
			}

			const auto previous = level == 0 ? 0.0F : m_lodRanges[level - 1];
			const auto morphStart = previous + ((m_lodRanges[level] - previous) * ratio);
			/* Fully morphed a hair BEFORE the boundary, so the last vertex of the ring is exactly the next level's. */
			const auto morphEnd = m_lodRanges[level] - ((m_lodRanges[level] - morphStart) * 0.01F);

			m_morphTable[level] = {morphStart, 1.0F / (morphEnd - morphStart)};
		}

		return true;
	}

	uint16_t
	CDLODTerrainResource::levelHeight (uint32_t level, int64_t latticeX, int64_t latticeZ) const noexcept
	{
		/* World lattice index 0 is the world origin, the grid's first point is -N / 2^(l+1). */
		const auto offset = static_cast< int64_t >(m_cellCount >> (level + 1));
		const auto last = static_cast< int64_t >(m_cellCount >> level);
		const auto indexX = static_cast< uint32_t >(std::clamp< int64_t >(latticeX + offset, 0, last));
		const auto indexZ = static_cast< uint32_t >(std::clamp< int64_t >(latticeZ + offset, 0, last));

		if ( level == 0 )
		{
			const auto value = std::round((m_source->getHeightAt(indexX, indexZ) - m_heightMinimum) * (65535.0F / m_heightRange));

			return static_cast< uint16_t >(std::clamp(value, 0.0F, 65535.0F));
		}

		return m_pyramid[level - 1][(static_cast< size_t >(indexZ) * static_cast< size_t >(last + 1)) + indexX];
	}

	std::array< int64_t, 2 >
	CDLODTerrainResource::levelCenterFor (uint32_t level, const Vector< 3, float > & position) const noexcept
	{
		const auto texel = m_cellSize * static_cast< float >(1U << level);
		const auto snap = static_cast< int64_t >(m_parameters.clipUpdateTexels);
		const auto half = static_cast< int64_t >(m_parameters.clipTexels / 2U);
		const auto gridHalf = static_cast< int64_t >(m_cellCount >> (level + 1));

		/* A level that holds the whole grid never moves. */
		if ( (2 * gridHalf) + 1 + static_cast< int64_t >(2U * (m_parameters.clipUpdateTexels + 2U)) <= 2 * half )
		{
			return {0, 0};
		}

		std::array< int64_t, 2 > center{};

		for ( size_t axis = 0; axis < 2; ++axis )
		{
			const auto coordinate = axis == 0 ? position[X] : position[Z];
			const auto lattice = static_cast< int64_t >(std::floor(coordinate / texel));
			const auto snapped = floorDivide(lattice + (snap / 2), snap) * snap;

			/* The window [c - half, c + half) stays inside the grid: against its border the level stops
			 * following the camera, which is still inside it. */
			center[axis] = std::clamp(snapped, -gridHalf + half, gridHalf + 1 - half);
		}

		return center;
	}

	Space3D::AACuboid< float >
	CDLODTerrainResource::nodeBox (uint32_t levelOfDetail, uint32_t nodeX, uint32_t nodeZ) const noexcept
	{
		const auto size = static_cast< float >(m_parameters.patchQuads << levelOfDetail) * m_cellSize;
		const auto halfGrid = static_cast< float >(m_cellCount) * m_cellSize * 0.5F;
		const auto nodesPerAxis = (m_cellCount / m_parameters.patchQuads) >> levelOfDetail;
		const auto & range = m_nodeHeightRanges[levelOfDetail][(static_cast< size_t >(nodeZ) * nodesPerAxis) + nodeX];
		const auto x0 = -halfGrid + (static_cast< float >(nodeX) * size);
		const auto z0 = -halfGrid + (static_cast< float >(nodeZ) * size);

		return {{x0 + size, range[1], z0 + size}, {x0, range[0], z0}};
	}

	bool
	CDLODTerrainResource::selectNode (uint32_t levelOfDetail, uint32_t nodeX, uint32_t nodeZ, const Vector< 3, float > & eye, const Frustum * frustum) const noexcept
	{
		const auto box = this->nodeBox(levelOfDetail, nodeX, nodeZ);
		const auto distance = box.distanceTo(eye);

		/* Beyond this level's range: the parent draws this area, coarser. */
		if ( distance > m_lodRanges[levelOfDetail] )
		{
			return false;
		}

		/* Culled: handled, nothing to draw. */
		if ( frustum != nullptr && !frustum->isSeeing(box) )
		{
			return true;
		}

		const auto size = static_cast< float >(m_parameters.patchQuads << levelOfDetail) * m_cellSize;
		const std::array< float, 4 > node{box.minimum(X), box.minimum(Z), size, static_cast< float >(levelOfDetail)};

		/* The finest level, or entirely beyond the next finer level's range: the whole node, at this level. */
		if ( levelOfDetail == 0 || distance > m_lodRanges[levelOfDetail - 1] )
		{
			m_selection.push_back({{0, m_patchIndexCount}, node});

			return true;
		}

		/* Some part is close enough for the finer level: let the children decide, and draw at THIS level
		 * the quarters none of them took. The patch index buffer is sorted by quarter. */
		const auto quarterIndexCount = m_patchIndexCount / 4U;

		for ( uint32_t quarter = 0; quarter < 4; ++quarter )
		{
			const auto childX = (nodeX * 2U) + (quarter % 2U);
			const auto childZ = (nodeZ * 2U) + (quarter / 2U);

			if ( !this->selectNode(levelOfDetail - 1, childX, childZ, eye, frustum) )
			{
				m_selection.push_back({{quarter * quarterIndexCount, quarterIndexCount}, node});
			}
		}

		return true;
	}

	void
	CDLODTerrainResource::prepareAdaptiveRendering (const Vector< 3, float > & lodViewPosition, const Frustum * cullingFrustum, const CartesianFrame< float > * /*worldCoordinates*/) const noexcept
	{
		m_selection.clear();

		if ( m_nodeHeightRanges.empty() || m_patchIndexCount == 0 )
		{
			return;
		}

		/* Object space IS the world (see the class note): the camera and the frustum are used as given. */
		const auto top = m_levelOfDetailCount - 1;
		const auto rootsPerAxis = (m_cellCount / m_parameters.patchQuads) >> top;

		for ( uint32_t rootZ = 0; rootZ < rootsPerAxis; ++rootZ )
		{
			for ( uint32_t rootX = 0; rootX < rootsPerAxis; ++rootX )
			{
				this->selectNode(top, rootX, rootZ, lodViewPosition, cullingFrustum);
			}
		}

	}

	bool
	CDLODTerrainResource::createOnHardware (TransferManager & transferManager) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::warning(ClassId, "The terrain is already in video memory !");

			return true;
		}

		if ( m_source == nullptr )
		{
			Tracer::error(ClassId, "The terrain has no height grid, load() it first !");

			return false;
		}

		const auto & device = transferManager.device();
		const auto patchQuads = m_parameters.patchQuads;
		const auto patchPoints = patchQuads + 1;

		/* The shared patch: integer grid coordinates in X and Z, the vertex stage does the rest. */
		{
			std::vector< float > positions;
			positions.reserve(static_cast< size_t >(patchPoints) * patchPoints * 3);

			for ( uint32_t z = 0; z < patchPoints; ++z )
			{
				for ( uint32_t x = 0; x < patchPoints; ++x )
				{
					positions.push_back(static_cast< float >(x));
					positions.push_back(0.0F);
					positions.push_back(static_cast< float >(z));
				}
			}

			m_vertexBufferObject = std::make_unique< VertexBufferObject >(device, patchPoints * patchPoints, getElementCountFromFlags(this->flags()), false);
			m_vertexBufferObject->setIdentifier(ClassId, this->name(), "PatchVertexBufferObject");

			if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->transferData(transferManager, positions) )
			{
				Tracer::error(ClassId, "Unable to create the patch vertex buffer object (VBO) !");

				m_vertexBufferObject.reset();

				return false;
			}
		}

		/* The patch triangles, sorted by QUARTER so a node can draw one quarter of itself (Strugar
		 * § 2.3), all split along the same diagonal: collapsing the odd vertices onto the even ones
		 * (the geomorph at 1) then yields exactly the next level's patch, the other triangles
		 * degenerating. The winding is the grid's (Grid proxies and the former adaptive grid). */
		{
			std::vector< uint32_t > indices;
			indices.reserve(static_cast< size_t >(patchQuads) * patchQuads * 6);

			const auto half = patchQuads / 2;
			const auto index = [patchPoints] (uint32_t x, uint32_t z) {
				return (z * patchPoints) + x;
			};

			for ( uint32_t quarter = 0; quarter < 4; ++quarter )
			{
				const auto startX = (quarter % 2) * half;
				const auto startZ = (quarter / 2) * half;

				for ( uint32_t z = startZ; z < startZ + half; ++z )
				{
					for ( uint32_t x = startX; x < startX + half; ++x )
					{
						indices.push_back(index(x, z));
						indices.push_back(index(x, z + 1));
						indices.push_back(index(x + 1, z + 1));

						indices.push_back(index(x, z));
						indices.push_back(index(x + 1, z + 1));
						indices.push_back(index(x + 1, z));
					}
				}
			}

			m_patchIndexCount = static_cast< uint32_t >(indices.size());

			m_indexBufferObject = std::make_unique< IndexBufferObject >(device, m_patchIndexCount);
			m_indexBufferObject->setIdentifier(ClassId, this->name(), "PatchIndexBufferObject");

			if ( !m_indexBufferObject->createOnHardware() || !m_indexBufferObject->transferData(transferManager, indices) )
			{
				Tracer::error(ClassId, "Unable to create the patch index buffer object (IBO) !");

				m_vertexBufferObject.reset();
				m_indexBufferObject.reset();

				return false;
			}
		}

		if ( !this->createSurfaceResources() )
		{
			this->destroyFromHardware(false);

			return false;
		}

		/* The first ray-tracing proxy, around the origin, before the BLAS is built from it
		 * (Interface::onDependenciesLoaded() builds it right after this). */
		{
			const auto proxyCells = std::min(m_cellCount, static_cast< uint32_t >(std::lround(m_parameters.rayTracingProxySize / m_cellSize)));
			const auto center = m_source->subGridCenter({0.0F, 0.0F}, proxyCells);
			PendingRayTracingProxy proxy;

			if ( this->generateRayTracingProxy(center, proxy) )
			{
				m_rtVertexBufferObject = std::move(proxy.vertexBufferObject);
				m_rtIndexBufferObjectProxy = std::move(proxy.indexBufferObject);
				m_rtProxyCenter = center;
			}
		}

		/* The renderer updates the clipmap every frame from now on. */
		this->serviceProvider().graphicsRenderer().registerSurfaceGeometry(std::static_pointer_cast< Interface >(this->shared_from_this()));

		return true;
	}

	bool
	CDLODTerrainResource::createSurfaceResources () noexcept
	{
		auto & renderer = this->serviceProvider().graphicsRenderer();
		const auto & device = renderer.device();
		const auto framesInFlight = std::max(1U, renderer.framesInFlight());
		const auto texels = m_parameters.clipTexels;
		const auto levels = m_clipLevelCount;

		/* The formats must be filterable (bilinear heights between lattice points) and the normals writable. */
		{
			VkFormatProperties heightProperties{};
			VkFormatProperties normalProperties{};

			vkGetPhysicalDeviceFormatProperties(device->physicalDevice()->handle(), HeightfieldSurface::HeightFormat, &heightProperties);
			vkGetPhysicalDeviceFormatProperties(device->physicalDevice()->handle(), HeightfieldSurface::NormalFormat, &normalProperties);

			const auto heightNeeds = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
			const auto normalNeeds = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

			if ( (heightProperties.optimalTilingFeatures & heightNeeds) != heightNeeds || (normalProperties.optimalTilingFeatures & normalNeeds) != normalNeeds || device->physicalDevice()->featuresVK10().shaderStorageImageExtendedFormats == VK_FALSE )
			{
				TraceError{ClassId} << "Terrain '" << this->name() << "': the device cannot filter R16_UNORM heights, or cannot write R16G16_SFLOAT normals from a compute shader (shaderStorageImageExtendedFormats).";

				return false;
			}
		}

		constexpr VkImageSubresourceRange allLevels{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, VK_REMAINING_ARRAY_LAYERS};

		m_heightImage = std::make_shared< Image >(device, VK_IMAGE_TYPE_2D, HeightfieldSurface::HeightFormat, VkExtent3D{texels, texels, 1}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, 1, levels);
		m_heightImage->setIdentifier(ClassId, this->name(), "HeightClipmap");
		m_normalImage = std::make_shared< Image >(device, VK_IMAGE_TYPE_2D, HeightfieldSurface::NormalFormat, VkExtent3D{texels, texels, 1}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 0, 1, levels);
		m_normalImage->setIdentifier(ClassId, this->name(), "NormalClipmap");

		if ( !m_heightImage->createOnHardware() || !m_normalImage->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the clipmap images !");

			return false;
		}

		m_heightView = std::make_shared< ImageView >(m_heightImage, VK_IMAGE_VIEW_TYPE_2D_ARRAY, allLevels);
		m_heightView->setIdentifier(ClassId, this->name(), "HeightClipmapView");
		m_normalView = std::make_shared< ImageView >(m_normalImage, VK_IMAGE_VIEW_TYPE_2D_ARRAY, allLevels);
		m_normalView->setIdentifier(ClassId, this->name(), "NormalClipmapView");

		if ( !m_heightView->createOnHardware() || !m_normalView->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the clipmap image views !");

			return false;
		}

		/* REPEAT is the toroidal addressing: a world lattice point i lives in texel i mod N. */
		{
			VkSamplerCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.maxAnisotropy = 1.0F;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 0.0F;
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			createInfo.unnormalizedCoordinates = VK_FALSE;

			m_sampler = std::make_shared< Sampler >(device, createInfo);
			m_sampler->setIdentifier(ClassId, this->name(), "ClipmapSampler");

			if ( !m_sampler->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the clipmap sampler !");

				return false;
			}
		}

		/* One uniform section per frame in flight: the level centres change with the camera. */
		{
			const auto alignment = std::max< VkDeviceSize >(1, device->physicalDevice()->propertiesVK10().limits.minUniformBufferOffsetAlignment);

			m_uniformSectionSize = ((sizeof(HeightfieldSurface::Uniforms) + alignment - 1) / alignment) * alignment;
			m_uniformBuffer = std::make_unique< UniformBufferObject >(device, m_uniformSectionSize * framesInFlight);
			m_uniformBuffer->setIdentifier(ClassId, this->name(), "SurfaceUniforms");

			if ( !m_uniformBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the surface uniform buffer !");

				return false;
			}
		}

		m_descriptorPool = std::make_shared< DescriptorPool >(
			device,
			std::vector< VkDescriptorPoolSize >{
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (2U * framesInFlight) + 1U},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1U}
			},
			framesInFlight + 1U,
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		);
		m_descriptorPool->setIdentifier(ClassId, this->name(), "DescriptorPool");

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the surface descriptor pool !");

			return false;
		}

		const auto surfaceLayout = Saphir::Generator::getHeightfieldSurfaceDescriptorSetLayout(renderer.layoutManager());

		if ( surfaceLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the heightfield surface descriptor set layout !");

			return false;
		}

		m_descriptorSets.clear();

		for ( uint32_t frame = 0; frame < framesInFlight; ++frame )
		{
			auto descriptorSet = std::make_unique< DescriptorSet >(m_descriptorPool, surfaceLayout);

			const VkDescriptorBufferInfo uniforms{
				.buffer = m_uniformBuffer->handle(),
				.offset = m_uniformSectionSize * frame,
				.range = sizeof(HeightfieldSurface::Uniforms)
			};

			if (
				!descriptorSet->create() ||
				!descriptorSet->writeCombinedImageSampler(HeightfieldSurface::HeightsBinding, *m_heightView, *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ||
				!descriptorSet->writeCombinedImageSampler(HeightfieldSurface::NormalsBinding, *m_normalView, *m_sampler, VK_IMAGE_LAYOUT_GENERAL) ||
				!descriptorSet->writeUniformBuffer(HeightfieldSurface::UniformsBinding, uniforms)
			)
			{
				Tracer::error(ClassId, "Unable to create a surface descriptor set !");

				m_descriptorSets.clear();

				return false;
			}

			m_descriptorSets.emplace_back(std::move(descriptorSet));
		}

		/* The normal bake: heights sampled, normals written. */
		{
			m_bakeSetLayout = std::make_shared< DescriptorSetLayout >(device, "HeightfieldNormalBakeDSLayout");
			m_bakeSetLayout->declare(VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
			m_bakeSetLayout->declare(VkDescriptorSetLayoutBinding{1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});

			if ( !m_bakeSetLayout->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the normal bake descriptor set layout !");

				return false;
			}

			m_bakePipelineLayout = std::make_shared< PipelineLayout >(
				device, "HeightfieldNormalBakePipelineLayout",
				StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_bakeSetLayout},
				StaticVector< VkPushConstantRange, 4 >{VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(NormalBakeConstants)}}
			);

			if ( !m_bakePipelineLayout->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the normal bake pipeline layout !");

				return false;
			}

			const auto shaderModule = renderer.shaderManager().getShaderModuleFromSourceCode(device, "HeightfieldNormalBake_CS", Saphir::ShaderType::ComputeShader, NormalBakeComputeShader);

			if ( shaderModule == nullptr )
			{
				Tracer::error(ClassId, "Unable to compile the normal bake compute shader !");

				return false;
			}

			m_bakePipeline = std::make_shared< ComputePipeline >(m_bakePipelineLayout);
			m_bakePipeline->setShaderModule(shaderModule->handle());

			if ( !m_bakePipeline->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the normal bake compute pipeline !");

				return false;
			}

			m_bakeDescriptorSet = std::make_unique< DescriptorSet >(m_descriptorPool, m_bakeSetLayout);

			if (
				!m_bakeDescriptorSet->create() ||
				!m_bakeDescriptorSet->writeCombinedImageSampler(0, *m_heightView, *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ||
				!m_bakeDescriptorSet->writeStorageImage(1, *m_normalView, VK_IMAGE_LAYOUT_GENERAL)
			)
			{
				Tracer::error(ClassId, "Unable to create the normal bake descriptor set !");

				return false;
			}
		}

		/* The per-frame command buffers: graphics queue, so queue order puts the clipmap update ahead of
		 * every pass of the frame. */
		m_commandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), false, true, false);
		m_commandPool->setIdentifier(ClassId, this->name(), "CommandPool");

		if ( !m_commandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the clipmap command pool !");

			return false;
		}

		m_commandBuffers.clear();
		m_stagingBuffers.clear();

		for ( uint32_t frame = 0; frame < framesInFlight; ++frame )
		{
			auto commandBuffer = std::make_shared< CommandBuffer >(m_commandPool, true);

			if ( !commandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "Unable to create a clipmap command buffer !");

				return false;
			}

			commandBuffer->setIdentifier(ClassId, this->name(), "ClipmapCommandBuffer");

			m_commandBuffers.emplace_back(std::move(commandBuffer));
			m_stagingBuffers.emplace_back(nullptr);
		}

		m_surfaceUploaded = false;

		return true;
	}

	void
	CDLODTerrainResource::stageLevelRects (uint32_t level, const std::vector< LatticeRect > & rects, std::vector< uint16_t > & stagingData, std::vector< VkBufferImageCopy > & copies, std::vector< std::array< int32_t, 5 > > & bakes) const noexcept
	{
		const auto texels = static_cast< int64_t >(m_parameters.clipTexels);

		for ( const auto & rect : rects )
		{
			for ( const auto & segmentZ : toroidalSegments(rect.z0, rect.z1, texels) )
			{
				for ( const auto & segmentX : toroidalSegments(rect.x0, rect.x1, texels) )
				{
					/* A copy's buffer offset must be a multiple of 4. */
					if ( stagingData.size() % 2 != 0 )
					{
						stagingData.push_back(0);
					}

					VkBufferImageCopy copy{};
					copy.bufferOffset = stagingData.size() * sizeof(uint16_t);
					copy.bufferRowLength = 0;
					copy.bufferImageHeight = 0;
					copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, level, 1};
					copy.imageOffset = {static_cast< int32_t >(segmentX.texel), static_cast< int32_t >(segmentZ.texel), 0};
					copy.imageExtent = {static_cast< uint32_t >(segmentX.length), static_cast< uint32_t >(segmentZ.length), 1};

					for ( int64_t z = 0; z < segmentZ.length; ++z )
					{
						for ( int64_t x = 0; x < segmentX.length; ++x )
						{
							stagingData.push_back(this->levelHeight(level, segmentX.lattice + x, segmentZ.lattice + z));
						}
					}

					copies.push_back(copy);
				}
			}

			/* The bake reads the four neighbours: the normals one texel around the rewritten heights change
			 * too. A rectangle already as wide as the level stays so (growing it would wrap onto itself). */
			auto x0 = rect.x0;
			auto x1 = rect.x1;
			auto z0 = rect.z0;
			auto z1 = rect.z1;

			if ( x1 - x0 + 2 <= texels )
			{
				--x0;
				++x1;
			}

			if ( z1 - z0 + 2 <= texels )
			{
				--z0;
				++z1;
			}

			for ( const auto & segmentZ : toroidalSegments(z0, z1, texels) )
			{
				for ( const auto & segmentX : toroidalSegments(x0, x1, texels) )
				{
					bakes.push_back({static_cast< int32_t >(segmentX.texel), static_cast< int32_t >(segmentZ.texel), static_cast< int32_t >(segmentX.length), static_cast< int32_t >(segmentZ.length), static_cast< int32_t >(level)});
				}
			}
		}
	}

	void
	CDLODTerrainResource::writeUniforms (uint32_t frameIndex) const noexcept
	{
		HeightfieldSurface::Uniforms uniforms{};

		uniforms.grid = {m_cellSize, static_cast< float >(m_parameters.clipTexels), static_cast< float >(m_clipLevelCount), static_cast< float >(m_parameters.patchQuads)};
		uniforms.height = {m_heightRange, m_heightMinimum, 0.0F, 0.0F};
		uniforms.textureCoordinates = m_textureCoordinates;

		/* A level is valid over its window minus the strip it may lag by and the bake's border. */
		const auto validHalfTexels = static_cast< float >((m_parameters.clipTexels / 2U) - m_parameters.clipUpdateTexels - 2U);

		for ( uint32_t level = 0; level < m_clipLevelCount; ++level )
		{
			const auto texel = m_cellSize * static_cast< float >(1U << level);

			uniforms.levels[level] = {
				static_cast< float >(m_levelCenters[level][0]) * texel,
				static_cast< float >(m_levelCenters[level][1]) * texel,
				validHalfTexels * texel,
				texel
			};
		}

		for ( uint32_t level = 0; level < HeightfieldSurface::MaxLevelsOfDetail; ++level )
		{
			uniforms.levelsOfDetail[level] = {m_morphTable[level][0], m_morphTable[level][1], m_lodRanges[level], 0.0F};
		}

		if ( !m_uniformBuffer->writeData(MemoryRegion{&uniforms, sizeof(uniforms), static_cast< size_t >(m_uniformSectionSize * frameIndex)}) )
		{
			TraceError{ClassId} << "Unable to write the surface uniforms of '" << this->name() << "' !";
		}
	}

	bool
	CDLODTerrainResource::updateSurfaceVideoMemory (const Vector< 3, float > & lodViewPosition, uint32_t frameIndex) noexcept
	{
		if ( m_descriptorSets.empty() )
		{
			return false;
		}

		const auto start = std::chrono::steady_clock::now();

		frameIndex %= static_cast< uint32_t >(m_descriptorSets.size());
		m_currentFrameIndex = frameIndex;

		const auto half = static_cast< int64_t >(m_parameters.clipTexels / 2U);
		const auto texels = static_cast< int64_t >(m_parameters.clipTexels);
		const bool full = !m_surfaceUploaded;

		std::vector< uint16_t > stagingData;
		std::vector< VkBufferImageCopy > copies;
		std::vector< std::array< int32_t, 5 > > bakes;

		/* Each level follows the camera by whole strips: what entered its window is rewritten, the rest
		 * stays where it is (a lattice point never moves in the texture). */
		for ( uint32_t level = 0; level < m_clipLevelCount; ++level )
		{
			const auto center = this->levelCenterFor(level, lodViewPosition);
			const auto previous = m_levelCenters[level];

			if ( !full && center == previous )
			{
				continue;
			}

			std::vector< LatticeRect > rects;
			const auto deltaX = center[0] - previous[0];
			const auto deltaZ = center[1] - previous[1];

			if ( full || std::abs(deltaX) >= texels || std::abs(deltaZ) >= texels )
			{
				rects.push_back({center[0] - half, center[1] - half, center[0] + half, center[1] + half});
			}
			else
			{
				if ( deltaX > 0 )
				{
					rects.push_back({previous[0] + half, center[1] - half, center[0] + half, center[1] + half});
				}
				else if ( deltaX < 0 )
				{
					rects.push_back({center[0] - half, center[1] - half, previous[0] - half, center[1] + half});
				}

				if ( deltaZ > 0 )
				{
					rects.push_back({center[0] - half, previous[1] + half, center[0] + half, center[1] + half});
				}
				else if ( deltaZ < 0 )
				{
					rects.push_back({center[0] - half, center[1] - half, center[0] + half, previous[1] - half});
				}
			}

			this->stageLevelRects(level, rects, stagingData, copies, bakes);

			m_levelCenters[level] = center;
		}

		this->writeUniforms(frameIndex);

		if ( copies.empty() )
		{
			return true;
		}

		auto & renderer = this->serviceProvider().graphicsRenderer();
		const auto & device = renderer.device();
		const auto bytes = stagingData.size() * sizeof(uint16_t);
		auto & staging = m_stagingBuffers[frameIndex];

		/* This frame's staging buffer was last read by this frame index's previous cycle, which the
		 * in-flight fence already waited for. */
		if ( staging == nullptr || staging->bytes() < bytes )
		{
			staging = std::make_unique< Buffer >(device, 0, bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
			staging->setIdentifier(ClassId, this->name(), "ClipmapStaging");

			if ( !staging->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the clipmap staging buffer !");

				staging.reset();

				return false;
			}
		}

		if ( !staging->writeData(MemoryRegion{stagingData.data(), bytes}) )
		{
			Tracer::error(ClassId, "Unable to write the clipmap staging buffer !");

			return false;
		}

		const auto & commandBuffer = m_commandBuffers[frameIndex];

		if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			Tracer::error(ClassId, "Unable to begin the clipmap command buffer !");

			return false;
		}

		const auto handle = commandBuffer->handle();
		constexpr VkImageSubresourceRange allLevels{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, VK_REMAINING_ARRAY_LAYERS};
		constexpr VkPipelineStageFlags readers = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		/* 1. Heights to transfer. The barrier's first scope is EVERY earlier command of the graphics
		 * queue: the frames still in flight have finished reading the texels this frame rewrites. */
		{
			std::array< VkImageMemoryBarrier, 2 > barriers{};

			barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barriers[0].srcAccessMask = full ? 0 : VK_ACCESS_SHADER_READ_BIT;
			barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barriers[0].oldLayout = full ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barriers[0].image = m_heightImage->handle();
			barriers[0].subresourceRange = allLevels;

			/* The normals live in GENERAL: the bake writes them, the stages sample them. */
			barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barriers[1].srcAccessMask = full ? 0 : VK_ACCESS_SHADER_READ_BIT;
			barriers[1].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barriers[1].oldLayout = full ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
			barriers[1].newLayout = VK_IMAGE_LAYOUT_GENERAL;
			barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barriers[1].image = m_normalImage->handle();
			barriers[1].subresourceRange = allLevels;

			vkCmdPipelineBarrier(handle, full ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : readers, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, static_cast< uint32_t >(barriers.size()), barriers.data());
		}

		/* 2. The strips. */
		vkCmdCopyBufferToImage(handle, staging->handle(), m_heightImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast< uint32_t >(copies.size()), copies.data());

		/* 3. Heights readable by the bake and by the frame. */
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_heightImage->handle();
			barrier.subresourceRange = allLevels;

			vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, readers, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		/* 4. The normals of what changed. */
		{
			const auto descriptorSet = m_bakeDescriptorSet->handle();

			vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, m_bakePipeline->handle());
			vkCmdBindDescriptorSets(handle, VK_PIPELINE_BIND_POINT_COMPUTE, m_bakePipelineLayout->handle(), 0, 1, &descriptorSet, 0, nullptr);

			for ( const auto & bake : bakes )
			{
				NormalBakeConstants constants{};
				constants.origin = {bake[0], bake[1]};
				constants.size = {bake[2], bake[3]};
				constants.layer = bake[4];
				constants.texelSize = m_cellSize * static_cast< float >(1U << static_cast< uint32_t >(bake[4]));
				constants.heightRange = m_heightRange;
				constants.texelCount = static_cast< int32_t >(texels);

				vkCmdPushConstants(handle, m_bakePipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
				vkCmdDispatch(handle, (static_cast< uint32_t >(bake[2]) + 7U) / 8U, (static_cast< uint32_t >(bake[3]) + 7U) / 8U, 1);
			}
		}

		/* 5. Normals readable by the frame. */
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_normalImage->handle();
			barrier.subresourceRange = allLevels;

			vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, readers, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		if ( !commandBuffer->end() )
		{
			Tracer::error(ClassId, "Unable to end the clipmap command buffer !");

			return false;
		}

		/* No semaphore, no fence: queue order runs this before every pass of the frame, and the frame's
		 * own fence signal covers every command submitted to the queue before it. */
		if ( !renderer.graphicsQueue()->submit(*commandBuffer) )
		{
			Tracer::error(ClassId, "Unable to submit the clipmap update !");

			return false;
		}

		m_heightImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_normalImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_GENERAL);

		if ( full )
		{
			m_surfaceUploaded = true;

			TraceInfo{ClassId} <<
				"Terrain '" << this->name() << "': clipmap uploaded (" << (bytes / 1048576) << " MiB, " << copies.size() << " copies, " << bakes.size() << " bakes) in " <<
				std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::steady_clock::now() - start).count() << " us.";

			/* The full upload's staging buffer is tens of megabytes: retired, the next strips allocate a small one. */
			renderer.deferredDestructor().retireObject(std::move(staging));
		}

		return true;
	}

	const DescriptorSet *
	CDLODTerrainResource::surfaceDescriptorSet () const noexcept
	{
		/* Nothing is sampled before the first upload: the draw skips the frame (the upload happens at the
		 * start of the first frame after creation). */
		if ( !m_surfaceUploaded || m_descriptorSets.empty() )
		{
			return nullptr;
		}

		return m_descriptorSets[m_currentFrameIndex].get();
	}

	bool
	CDLODTerrainResource::generateRayTracingProxy (const Vector< 2, float > & center, PendingRayTracingProxy & proxy) const noexcept
	{
		const auto start = std::chrono::steady_clock::now();

		/* A BLAS has no level of detail: one fixed surface around the camera, at the finest step whose
		 * triangle count fits the budget (the full resolution of a 4 km window is 33.5 M triangles, and
		 * +2.5 GiB of VRAM, measured on the former adaptive grid). */
		auto & settings = this->serviceProvider().primaryServices().settings();
		const auto triangleBudget = std::max(2U, settings.getOrSetDefault< uint32_t >(GraphicsRayTracingTerrainBLASMaxTrianglesKey, DefaultGraphicsRayTracingTerrainBLASMaxTriangles));
		const auto proxyCells = std::min(m_cellCount, static_cast< uint32_t >(std::lround(m_parameters.rayTracingProxySize / m_cellSize)));

		uint32_t step = 1;

		while ( step < proxyCells && 2ULL * (proxyCells / step) * (proxyCells / step) > triangleBudget )
		{
			step *= 2;
		}

		const auto cells = std::max(step, (proxyCells / step) * step);
		auto window = m_source->subGrid(center, cells);

		if ( step > 1 )
		{
			window = window.coarsened(step);
		}

		if ( !window.isValid() )
		{
			TraceError{ClassId} << "Terrain '" << this->name() << "': unable to cut the ray-tracing proxy window !";

			return false;
		}

		/* Position, tangent, binormal, normal, UV: the layout of RayTracingProxyFlags, read by the hit shaders. */
		const auto pointCount = window.pointCount();
		const auto elementCount = getElementCountFromFlags(RayTracingProxyFlags);
		std::vector< float > attributes(static_cast< size_t >(pointCount) * elementCount);

		for ( uint32_t index = 0; index < pointCount; ++index )
		{
			const auto position = window.position(index);
			const auto normal = window.normal(index, position);
			const auto tangent = window.tangent(index, position, window.textureCoordinates3D(index));
			const auto binormal = Vector< 3, float >::crossProduct(normal, tangent);
			const auto uv = window.textureCoordinates2D(index);
			auto * out = attributes.data() + (static_cast< size_t >(index) * elementCount);

			*out++ = position[X];
			*out++ = position[Y];
			*out++ = position[Z];
			*out++ = tangent[X];
			*out++ = tangent[Y];
			*out++ = tangent[Z];
			*out++ = binormal[X];
			*out++ = binormal[Y];
			*out++ = binormal[Z];
			*out++ = normal[X];
			*out++ = normal[Y];
			*out++ = normal[Z];
			*out++ = uv[X];
			*out = uv[Y];
		}

		const auto quads = window.squaredQuadCount();
		std::vector< uint32_t > indices;
		indices.reserve(static_cast< size_t >(quads) * quads * 6);

		for ( uint32_t z = 0; z < quads; ++z )
		{
			for ( uint32_t x = 0; x < quads; ++x )
			{
				const auto topLeft = window.index(x, z);
				const auto topRight = window.index(x + 1, z);
				const auto bottomLeft = window.index(x, z + 1);
				const auto bottomRight = window.index(x + 1, z + 1);

				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(topRight);

				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);
				indices.push_back(topRight);
			}
		}

		auto & transferManager = this->serviceProvider().graphicsRenderer().transferManager();

		proxy.vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), pointCount, elementCount, false);
		proxy.vertexBufferObject->setIdentifier(ClassId, this->name(), "RT_ProxyVertexBufferObject");
		proxy.indexBufferObject = std::make_unique< IndexBufferObject >(transferManager.device(), static_cast< uint32_t >(indices.size()));
		proxy.indexBufferObject->setIdentifier(ClassId, this->name(), "RT_ProxyIndexBufferObject");

		if (
			!proxy.vertexBufferObject->createOnHardware() || !proxy.vertexBufferObject->transferData(transferManager, attributes) ||
			!proxy.indexBufferObject->createOnHardware() || !proxy.indexBufferObject->transferData(transferManager, indices)
		)
		{
			TraceError{ClassId} << "Terrain '" << this->name() << "': unable to upload the ray-tracing proxy !";

			proxy.vertexBufferObject.reset();
			proxy.indexBufferObject.reset();

			return false;
		}

		TraceInfo{ClassId} <<
			"Terrain '" << this->name() << "': ray-tracing proxy of " << (window.squaredSize()) << " m around (" << center[X] << ", " << center[Y] << "), step " << step <<
			" (" << (indices.size() / 3) << " triangles, budget " << triangleBudget << "), built in " <<
			std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - start).count() << " ms.";

		return true;
	}

	void
	CDLODTerrainResource::updateRayTracingProxy (const Vector< 3, float > & worldPosition) noexcept
	{
		/* Logic thread. One proxy in flight at a time. */
		if ( m_source == nullptr || !this->isCreated() || m_rtProxyUpdating.load(std::memory_order_acquire) )
		{
			return;
		}

		const auto half = m_parameters.rayTracingProxySize * 0.5F;
		const auto margin = std::min(m_parameters.rayTracingProxyMargin, half * 0.75F);
		const Vector< 2, float > camera{worldPosition[X], worldPosition[Z]};

		if ( Vector< 2, float >::distance(m_rtProxyCenter, camera) <= half - margin )
		{
			return;
		}

		/* Where the proxy CAN go (snapped and clamped like the extraction): against the grid border the
		 * camera stays past the threshold for ever, and only a real move is worth a rebuild. */
		const auto proxyCells = std::min(m_cellCount, static_cast< uint32_t >(std::lround(m_parameters.rayTracingProxySize / m_cellSize)));
		const auto wanted = m_source->subGridCenter(camera, proxyCells);

		if ( Vector< 2, float >::distance(wanted, m_rtProxyCenter) <= half - margin )
		{
			return;
		}

		const auto threadPool = this->serviceProvider().primaryServices().threadPool();

		if ( threadPool == nullptr )
		{
			return;
		}

		m_rtProxyUpdating.store(true, std::memory_order_release);
		m_rtProxyCenter = wanted;

		const auto enqueued = threadPool->enqueue([self = std::static_pointer_cast< CDLODTerrainResource >(this->shared_from_this()), wanted] () {
			PendingRayTracingProxy proxy;

			if ( !self->generateRayTracingProxy(wanted, proxy) )
			{
				self->m_rtProxyUpdating.store(false, std::memory_order_release);

				return;
			}

			{
				const std::lock_guard< std::mutex > lock{self->m_pendingAccess};

				self->m_pendingProxy = std::move(proxy);
				self->m_hasPendingProxy = true;
			}

			self->serviceProvider().graphicsRenderer().requestGeometryVideoMemoryUpdate(std::static_pointer_cast< Interface >(self));
		});

		if ( !enqueued )
		{
			TraceError{ClassId} << "Unable to hand the ray-tracing proxy of '" << this->name() << "' to the thread pool !";

			m_rtProxyUpdating.store(false, std::memory_order_release);
		}
	}

	bool
	CDLODTerrainResource::updateVideoMemory () noexcept
	{
		/* Render thread, behind the frame fence: the ONE place the traced buffers may change. */
		PendingRayTracingProxy proxy;

		{
			const std::lock_guard< std::mutex > lock{m_pendingAccess};

			if ( !m_hasPendingProxy )
			{
				return true;
			}

			proxy = std::move(m_pendingProxy);
			m_hasPendingProxy = false;
		}

		/* The previous proxy may still be read by a frame in flight (BLAS, hit shading): retired. */
		auto & deferredDestructor = this->serviceProvider().graphicsRenderer().deferredDestructor();

		deferredDestructor.retireObject(std::move(m_rtVertexBufferObject));
		deferredDestructor.retireObject(std::move(m_rtIndexBufferObjectProxy));

		m_rtVertexBufferObject = std::move(proxy.vertexBufferObject);
		m_rtIndexBufferObjectProxy = std::move(proxy.indexBufferObject);

		m_rtProxyUpdating.store(false, std::memory_order_release);

		/* ⚠️ LAST: the flag is what hands the new proxy to the frame path that rebuilds the BLAS
		 * (Scenes::SceneMetaData::rebuild()). */
		this->markAccelerationStructureStale();

		return true;
	}

	void
	CDLODTerrainResource::destroyFromHardware (bool clearLocalData) noexcept
	{
		{
			const std::lock_guard< std::mutex > lock{m_pendingAccess};

			m_pendingProxy = {};
			m_hasPendingProxy = false;
		}

		m_rtProxyUpdating.store(false, std::memory_order_release);

		m_stagingBuffers.clear();
		m_commandBuffers.clear();
		m_commandPool.reset();
		m_bakeDescriptorSet.reset();
		m_bakePipeline.reset();
		m_bakePipelineLayout.reset();
		m_bakeSetLayout.reset();
		m_descriptorSets.clear();
		m_descriptorPool.reset();
		m_uniformBuffer.reset();
		m_sampler.reset();
		m_normalView.reset();
		m_normalImage.reset();
		m_heightView.reset();
		m_heightImage.reset();
		m_rtVertexBufferObject.reset();
		m_rtIndexBufferObjectProxy.reset();
		m_indexBufferObject.reset();
		m_vertexBufferObject.reset();
		m_selection.clear();
		m_surfaceUploaded = false;
		m_patchIndexCount = 0;

		if ( clearLocalData )
		{
			m_source.reset();
			m_pyramid.clear();
			m_nodeHeightRanges.clear();
		}
	}
}
