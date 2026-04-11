/*
 * src/Graphics/Geometry/RawIndexedVertexResource.cpp
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

#include "RawIndexedVertexResource.hpp"

/* STL inclusions. */
#include <cmath>
#include <vector>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;
	using namespace Vulkan;

	/* ============================================================
	 * Interface overrides
	 * ============================================================ */

	bool
	RawIndexedVertexResource::createOnHardware (TransferManager & /*transferManager*/) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		Tracer::error(ClassId, "No data available ! Use load() with raw buffers first.");

		return false;
	}

	bool
	RawIndexedVertexResource::updateVideoMemory () noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::warning(ClassId, "No buffer in video memory to update !");

			return false;
		}

		Tracer::error(ClassId, "Updating geometry in video memory is not handled yet !");

		return false;
	}

	void
	RawIndexedVertexResource::destroyFromHardware (bool clearLocalData) noexcept
	{
		if ( m_vertexBufferObject != nullptr )
		{
			m_vertexBufferObject->destroyFromHardware();
			m_vertexBufferObject.reset();
		}

		if ( m_indexBufferObject != nullptr )
		{
			m_indexBufferObject->destroyFromHardware();
			m_indexBufferObject.reset();
		}

		if ( clearLocalData )
		{
			this->resetFlags();
			m_boundingBox = {};
			m_boundingSphere = {};
		}
	}

	/* ============================================================
	 * ResourceTrait load overrides (unsupported paths)
	 * ============================================================ */

	bool
	RawIndexedVertexResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "Default loading is not supported for raw geometry resources !");

		return this->setLoadSuccess(false);
	}

	bool
	RawIndexedVertexResource::load (const std::filesystem::path & /*filepath*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "File loading is not supported for raw geometry resources !");

		return this->setLoadSuccess(false);
	}

	bool
	RawIndexedVertexResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "JSON loading is not supported for raw geometry resources !");

		return this->setLoadSuccess(false);
	}

	/* ============================================================
	 * Pre-interleaved load
	 * ============================================================ */

	bool
	RawIndexedVertexResource::load (std::span< const float > vertexAttributes, uint32_t vertexElementCount, std::span< const uint32_t > indices, const RawGeometryOptions & options) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( vertexElementCount < 3 )
		{
			TraceError{ClassId} << "Vertex element count must be at least 3 (position xyz), got " << vertexElementCount << " !";

			return this->setLoadSuccess(false);
		}

		if ( vertexAttributes.empty() || (vertexAttributes.size() % vertexElementCount) != 0 )
		{
			TraceError{ClassId} << "Vertex attributes size (" << vertexAttributes.size() << ") is not a multiple of vertexElementCount (" << vertexElementCount << ") !";

			return this->setLoadSuccess(false);
		}

		if ( indices.empty() )
		{
			Tracer::error(ClassId, "Index buffer is empty !");

			return this->setLoadSuccess(false);
		}

		const auto vertexCount = static_cast< uint32_t >(vertexAttributes.size() / vertexElementCount);

		if constexpr ( IsDebug )
		{
			for ( const auto index : indices )
			{
				if ( index >= vertexCount )
				{
					TraceError{ClassId} << "Index " << index << " is out of range (vertexCount=" << vertexCount << ") !";

					return this->setLoadSuccess(false);
				}
			}
		}

		return this->setLoadSuccess(this->uploadToGPU(vertexAttributes.data(), vertexCount, vertexElementCount, indices.data(), static_cast< uint32_t >(indices.size()), options));
	}

	/* ============================================================
	 * Separate attributes load (engine interleaves)
	 * ============================================================ */

	bool
	RawIndexedVertexResource::load (std::span< const float > positions, std::span< const uint32_t > indices, std::span< const float > normals, std::span< const float > textureCoordinates, std::span< const float > colors, const RawGeometryOptions & options) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( positions.empty() || (positions.size() % 3) != 0 )
		{
			TraceError{ClassId} << "Positions size (" << positions.size() << ") must be a non-zero multiple of 3 !";

			return this->setLoadSuccess(false);
		}

		const auto vertexCount = static_cast< uint32_t >(positions.size() / 3);

		if ( indices.empty() )
		{
			Tracer::error(ClassId, "Index buffer is empty !");

			return this->setLoadSuccess(false);
		}

		/* Validate optional attribute sizes. */
		if ( !normals.empty() && normals.size() != static_cast< size_t >(vertexCount) * 3 )
		{
			TraceError{ClassId} << "Normals size (" << normals.size() << ") does not match vertexCount * 3 (" << (vertexCount * 3) << ") !";

			return this->setLoadSuccess(false);
		}

		if ( !textureCoordinates.empty() && textureCoordinates.size() != static_cast< size_t >(vertexCount) * 2 )
		{
			TraceError{ClassId} << "Texture coordinates size (" << textureCoordinates.size() << ") does not match vertexCount * 2 (" << (vertexCount * 2) << ") !";

			return this->setLoadSuccess(false);
		}

		if ( !colors.empty() && colors.size() != static_cast< size_t >(vertexCount) * 4 )
		{
			TraceError{ClassId} << "Colors size (" << colors.size() << ") does not match vertexCount * 4 (" << (vertexCount * 4) << ") !";

			return this->setLoadSuccess(false);
		}

		/* Auto-set geometry flags from provided attributes. */
		if ( !normals.empty() )
		{
			this->enableFlag(EnableNormal);
		}

		if ( !textureCoordinates.empty() )
		{
			this->enableFlag(EnablePrimaryTextureCoordinates);
		}

		if ( !colors.empty() )
		{
			this->enableFlag(EnableVertexColor);
		}

		/* Compute stride and interleave. */
		const uint32_t vertexElementCount = 3
			+ (!normals.empty() ? 3 : 0)
			+ (!textureCoordinates.empty() ? 2 : 0)
			+ (!colors.empty() ? 4 : 0);

		std::vector< float > interleaved(static_cast< size_t >(vertexCount) * vertexElementCount);

		for ( uint32_t i = 0; i < vertexCount; ++i )
		{
			auto * dst = interleaved.data() + static_cast< size_t >(i) * vertexElementCount;
			const auto i3 = static_cast< size_t >(i) * 3;

			/* Position (always present). */
			dst[0] = positions[i3];
			dst[1] = positions[i3 + 1];
			dst[2] = positions[i3 + 2];
			dst += 3;

			/* Normals. */
			if ( !normals.empty() )
			{
				dst[0] = normals[i3];
				dst[1] = normals[i3 + 1];
				dst[2] = normals[i3 + 2];
				dst += 3;
			}

			/* Texture coordinates. */
			if ( !textureCoordinates.empty() )
			{
				const auto i2 = static_cast< size_t >(i) * 2;

				dst[0] = textureCoordinates[i2];
				dst[1] = textureCoordinates[i2 + 1];
				dst += 2;
			}

			/* Colors (RGBA). */
			if ( !colors.empty() )
			{
				const auto i4 = static_cast< size_t >(i) * 4;

				dst[0] = colors[i4];
				dst[1] = colors[i4 + 1];
				dst[2] = colors[i4 + 2];
				dst[3] = colors[i4 + 3];
			}
		}

		return this->setLoadSuccess(this->uploadToGPU(interleaved.data(), vertexCount, vertexElementCount, indices.data(), static_cast< uint32_t >(indices.size()), options));
	}

	/* ============================================================
	 * Private helpers
	 * ============================================================ */

	bool
	RawIndexedVertexResource::uploadToGPU (const float * vertexAttributes, uint32_t vertexCount, uint32_t vertexElementCount, const uint32_t * indices, uint32_t indexCount, const RawGeometryOptions & options) noexcept
	{
		m_topology = options.topology;

		/* Compute bounding volumes. */
		if ( options.boundingBoxMin.has_value() && options.boundingBoxMax.has_value() )
		{
			computeBoundingVolumes(m_boundingBox, m_boundingSphere, *options.boundingBoxMin, *options.boundingBoxMax);
		}
		else
		{
			computeBoundingVolumes(m_boundingBox, m_boundingSphere, vertexAttributes, vertexCount, vertexElementCount);
		}

		/* Upload VBO. */
		auto & transferManager = this->serviceProvider().graphicsRenderer().transferManager();

		m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), vertexCount, vertexElementCount, false);
		m_vertexBufferObject->setIdentifier(ClassId, this->name(), "VertexBufferObject");

		if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->transferData(transferManager, MemoryRegion{vertexAttributes, static_cast< size_t >(vertexCount) * vertexElementCount * sizeof(float)}) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		/* Upload IBO. */
		m_indexBufferObject = std::make_unique< IndexBufferObject >(transferManager.device(), indexCount);
		m_indexBufferObject->setIdentifier(ClassId, this->name(), "IndexBufferObject");

		if ( !m_indexBufferObject->createOnHardware() || !m_indexBufferObject->transferData(transferManager, MemoryRegion{indices, static_cast< size_t >(indexCount) * sizeof(uint32_t)}) )
		{
			Tracer::error(ClassId, "Unable to create the index buffer object (IBO) !");

			m_indexBufferObject.reset();

			return false;
		}

		return true;
	}

	void
	RawIndexedVertexResource::computeBoundingVolumes (AACuboid< float > & boundingBox, Sphere< float > & boundingSphere, const Point< float > & minimum, const Point< float > & maximum) noexcept
	{
		boundingBox.set(maximum, minimum);

		const auto centroid = boundingBox.centroid();
		const auto diff = maximum - centroid;

		boundingSphere = Sphere< float >(diff.length(), centroid);
	}

	void
	RawIndexedVertexResource::computeBoundingVolumes (AACuboid< float > & boundingBox, Sphere< float > & boundingSphere, const float * positions, uint32_t vertexCount, uint32_t stride) noexcept
	{
		boundingBox.reset();

		for ( uint32_t i = 0; i < vertexCount; ++i )
		{
			const auto offset = static_cast< size_t >(i) * stride;

			boundingBox.merge(Point< float >{
				positions[offset],
				positions[offset + 1],
				positions[offset + 2]
			});
		}

		const auto centroid = boundingBox.centroid();

		auto maxDistSq = 0.0F;

		for ( uint32_t i = 0; i < vertexCount; ++i )
		{
			const auto offset = static_cast< size_t >(i) * stride;
			const auto dx = positions[offset] - centroid[X];
			const auto dy = positions[offset + 1] - centroid[Y];
			const auto dz = positions[offset + 2] - centroid[Z];
			const auto distSq = dx * dx + dy * dy + dz * dz;

			if ( distSq > maxDistSq )
			{
				maxDistSq = distSq;
			}
		}

		boundingSphere = Sphere< float >(std::sqrt(maxDistSq), centroid);
	}
}
