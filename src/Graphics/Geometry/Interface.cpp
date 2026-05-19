/*
 * src/Graphics/Geometry/Interface.cpp
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

#include "Interface.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iterator>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/AccelerationStructureBuilder.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Vulkan;

	constexpr auto TracerTag{"GeometryInterface"};

	/* TODO: Remove this ! */
	AccelerationStructureBuilder * Interface::s_accelerationStructureBuilder = nullptr;

	bool
	Interface::buildSubGeometries (std::vector< SubGeometry > & subGeometries, uint32_t length) noexcept
	{
		if ( length == 0 )
		{
			return false;
		}

		subGeometries.clear();
		subGeometries.emplace_back(0, length);

		return true;
	}

	bool
	Interface::buildSubGeometries (std::vector< SubGeometry > & subGeometries, const Shape< float > & shape) noexcept
	{
		if ( shape.empty() )
		{
			return false;
		}

		subGeometries.clear();

		if ( shape.hasGroups() )
		{
			/* FIXME: Check topology/primitive */
			std::ranges::transform(shape.groups(), std::back_inserter(subGeometries), [] (const auto & group) {
				const auto offset = static_cast< uint32_t >(group.first * 3);
				const auto length = static_cast< uint32_t >(group.second * 3);

				return SubGeometry{offset, length};
			});
		}

		return true;
	}

	bool
	Interface::buildSubGeometries (std::vector< SubGeometry > & subGeometries, const Grid< float > & grid) noexcept
	{
		if ( grid.empty() )
		{
			return false;
		}

		subGeometries.clear();

		// TODO ...

		return true;
	}

	void
	Interface::buildAccelerationStructure () noexcept
	{
		/* Skip if RT is not available. */
		if ( s_accelerationStructureBuilder == nullptr )
		{
			return;
		}

		const auto topo = this->topology();

		/* Only triangle-based topologies can produce a BLAS. */
		if ( topo != Topology::TriangleList && topo != Topology::TriangleStrip )
		{
			return;
		}

		const auto * vbo = this->vertexBufferObject();

		if ( vbo == nullptr || !vbo->isCreated() )
		{
			return;
		}

		/* Shared header: same VB/IB across all sub-geometries of this Geometry::Interface. */
		BLASGeometryInput sharedHeader{};
		sharedHeader.vertexBuffer = vbo->handle();
		sharedHeader.vertexCount = vbo->vertexCount();
		sharedHeader.vertexStride = vbo->vertexElementCount() * sizeof(float);

		/* For TriangleStrip topologies, convert indices to TriangleList via the virtual override. */
		std::vector< uint32_t > convertedIndices;
		uint32_t totalIndexCount = 0;

		if ( topo == Topology::TriangleStrip )
		{
			convertedIndices = this->generateTriangleListIndicesForRT();

			if ( convertedIndices.empty() )
			{
				TraceError{TracerTag} << "Geometry '" << this->name() << "' uses TriangleStrip but generateTriangleListIndicesForRT() returned empty indices !";

				return;
			}

			/* Create a persistent RT index buffer for shader access (UV/normal lookup).
			 * The BLAS build also uses these indices via cpuIndices. */
			auto & transferManager = this->serviceProvider().graphicsRenderer().transferManager();
			const auto indexCount = static_cast< uint32_t >(convertedIndices.size());

			m_rtIndexBufferObject = std::make_unique< Vulkan::IndexBufferObject >(transferManager.device(), indexCount);
			m_rtIndexBufferObject->setIdentifier("Geometry", this->name(), "RT_IndexBufferObject");

			if ( m_rtIndexBufferObject->createOnHardware() && m_rtIndexBufferObject->transferData(transferManager, convertedIndices) )
			{
				sharedHeader.indexBuffer = m_rtIndexBufferObject->handle();
				sharedHeader.indexType = VK_INDEX_TYPE_UINT32;
				totalIndexCount = indexCount;
			}
			else
			{
				TraceError{TracerTag} << "Failed to create RT index buffer for geometry '" << this->name() << "' ! Falling back to CPU indices.";

				m_rtIndexBufferObject.reset();

				/* Fall back to CPU index upload in the builder. */
				sharedHeader.cpuIndices = convertedIndices.data();
				sharedHeader.cpuIndexCount = static_cast< uint32_t >(convertedIndices.size());
				totalIndexCount = sharedHeader.cpuIndexCount;
			}
		}
		else
		{
			/* TriangleList: use existing GPU index buffer directly. */
			const auto * ibo = this->indexBufferObject();

			if ( ibo != nullptr && ibo->isCreated() )
			{
				sharedHeader.indexBuffer = ibo->handle();
				sharedHeader.indexType = VK_INDEX_TYPE_UINT32;
				totalIndexCount = ibo->indexCount();
			}
		}

		/* Partition by sub-geometry: each sub-geometry becomes its own
		 * VkAccelerationStructureGeometryKHR inside the BLAS. The ray query then
		 * uses rayQueryGetIntersectionGeometryIndexEXT to identify which sub-geometry
		 * was hit, so the trace shader can look up the right material per hit (vs the
		 * old "one BLAS, one material per instance" scheme that aliased materials
		 * across the same BLAS).
		 *
		 * Caveat: the TriangleStrip→TriangleList CPU-indices fallback path produces a
		 * single flat triangle list; we DON'T partition that case (the converted
		 * indices don't map cleanly to original sub-geometry ranges). Procedural
		 * shapes (cuboid, sphere) currently are single-sub-geometry anyway, so this
		 * is fine in practice. */
		std::vector< BLASGeometryInput > geometries;
		const auto subGeoCount = this->subGeometryCount();

		if ( subGeoCount <= 1 || sharedHeader.cpuIndices != nullptr )
		{
			BLASGeometryInput single = sharedHeader;
			single.firstIndex = 0;
			single.indexCount = totalIndexCount;
			geometries.emplace_back(single);
		}
		else
		{
			geometries.reserve(subGeoCount);

			for ( uint32_t i = 0; i < subGeoCount; ++i )
			{
				const auto range = this->subGeometryRange(i); /* {firstIndex, indexCount} */
				BLASGeometryInput sub = sharedHeader;
				sub.firstIndex = range[0];
				sub.indexCount = range[1];
				geometries.emplace_back(sub);
			}
		}

		m_accelerationStructure = s_accelerationStructureBuilder->buildBLAS(geometries);

		if ( m_accelerationStructure == nullptr )
		{
			TraceError{TracerTag} << "Failed to build BLAS for geometry '" << this->name() << "' !";
		}
	}

	bool
	Interface::onDependenciesLoaded () noexcept
	{
		if ( !this->isCreated() && !this->createOnHardware(this->serviceProvider().graphicsRenderer().transferManager()) )
		{
			TraceError{TracerTag} << "Unable to send the geometry resource '" << this->name() << "' (" << this->classLabel() << ") into the video memory !";

			return false;
		}

		/* Build BLAS for ray tracing (all geometry types). */
		this->buildAccelerationStructure();

		return true;
	}
}
