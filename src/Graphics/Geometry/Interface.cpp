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
#include "Vulkan/AccelerationStructureBuilder.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Vulkan;

	constexpr auto TracerTag{"GeometryInterface"};

	Renderer * Interface::s_graphicsRenderer = nullptr;
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

		BLASGeometryInput geometry{};
		geometry.vertexBuffer = vbo->handle();
		geometry.vertexCount = vbo->vertexCount();
		geometry.vertexStride = vbo->vertexElementCount() * sizeof(float);

		/* For TriangleStrip topologies, convert indices to TriangleList via the virtual override. */
		std::vector< uint32_t > convertedIndices;

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
			if ( s_graphicsRenderer != nullptr )
			{
				auto & transferManager = s_graphicsRenderer->transferManager();
				const auto indexCount = static_cast< uint32_t >(convertedIndices.size());

				m_rtIndexBufferObject = std::make_unique< Vulkan::IndexBufferObject >(transferManager.device(), indexCount);
				m_rtIndexBufferObject->setIdentifier("Geometry", this->name(), "RT_IndexBufferObject");

				if ( m_rtIndexBufferObject->createOnHardware() && m_rtIndexBufferObject->transferData(transferManager, convertedIndices) )
				{
					/* Use the persistent RT IBO for the BLAS build. */
					geometry.indexBuffer = m_rtIndexBufferObject->handle();
					geometry.indexCount = indexCount;
					geometry.indexType = VK_INDEX_TYPE_UINT32;
				}
				else
				{
					TraceError{TracerTag} << "Failed to create RT index buffer for geometry '" << this->name() << "' ! Falling back to CPU indices.";

					m_rtIndexBufferObject.reset();

					/* Fall back to CPU index upload in the builder. */
					geometry.cpuIndices = convertedIndices.data();
					geometry.cpuIndexCount = static_cast< uint32_t >(convertedIndices.size());
				}
			}
			else
			{
				geometry.cpuIndices = convertedIndices.data();
				geometry.cpuIndexCount = static_cast< uint32_t >(convertedIndices.size());
			}
		}
		else
		{
			/* TriangleList: use existing GPU index buffer directly. */
			const auto * ibo = this->indexBufferObject();

			if ( ibo != nullptr && ibo->isCreated() )
			{
				geometry.indexBuffer = ibo->handle();
				geometry.indexCount = ibo->indexCount();
				geometry.indexType = VK_INDEX_TYPE_UINT32;
			}
		}

		m_accelerationStructure = s_accelerationStructureBuilder->buildBLAS(geometry);

		if ( m_accelerationStructure == nullptr )
		{
			TraceError{TracerTag} << "Failed to build BLAS for geometry '" << this->name() << "' !";
		}
	}

	bool
	Interface::onDependenciesLoaded () noexcept
	{
		if ( s_graphicsRenderer == nullptr )
		{
			TraceError{TracerTag} << "The static renderer pointer is null !";

			return false;
		}

		if ( !this->isCreated() && !this->createOnHardware(s_graphicsRenderer->transferManager()) )
		{
			TraceError{TracerTag} << "Unable to send the geometry resource '" << this->name() << "' (" << this->classLabel() << ") into the video memory !";

			return false;
		}

		/* Build BLAS for ray tracing (all geometry types). */
		this->buildAccelerationStructure();

		return true;
	}
}
