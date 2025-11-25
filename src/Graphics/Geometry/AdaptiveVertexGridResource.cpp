/*
 * src/Graphics/Geometry/AdaptiveVertexGridResource.cpp
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

#include "AdaptiveVertexGridResource.hpp"

/* Local inclusions. */
#include "Vulkan/TransferManager.hpp"
#include "Constants.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Libs::PixelFactory;
	using namespace Vulkan;

	bool
	AdaptiveVertexGridResource::createOnHardware (TransferManager & transferManager) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::warning(ClassId, "The buffers are already in video memory ! Use processLogics() instead.");

			return true;
		}

		/* Checking local data ... */
		if ( m_localData.empty() || m_indices.empty() )
		{
			TraceError{ClassId} <<
				"Resource '" << this->name() << "' has invalid local data ! "
				"Loading into video memory cancelled.";

			return false;
		}

		/* NOTE: Requesting a VBO using data. */
		const auto vertexElementCount = getElementCountFromFlags(this->flags());

		m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), static_cast< uint32_t >(m_localData.size()), vertexElementCount, false);
		m_vertexBufferObject->setIdentifier(ClassId, this->name(), "VertexBufferObject");

		if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->transferData(transferManager, m_localData) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		/* NOTE: Requesting an IBO using data. */
		m_indexBufferObject = std::make_unique< IndexBufferObject >(transferManager.device(), static_cast< uint32_t >(m_indices.size()));
		m_indexBufferObject->setIdentifier(ClassId, this->name(), "IndexBufferObject");

		if ( !m_indexBufferObject->createOnHardware() || !m_indexBufferObject->transferData(transferManager, m_indices) )
		{
			Tracer::error(ClassId, "Unable to get an index buffer object (IBO) !");

			m_indexBufferObject.reset();

			return false;
		}

		/* Creates drawing batches. */
		return true;
	}

	bool
	AdaptiveVertexGridResource::updateVideoMemory () noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::warning(ClassId, "No buffer in video update to update !");

			return false;
		}

		Tracer::warning(ClassId, "Updating geometry in video memory is not handled yet !");

		return true;
	}

	void
	AdaptiveVertexGridResource::destroyFromHardware (bool clearLocalData) noexcept
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
			this->setFlags(EnablePrimitiveRestart);
			m_localData.clear();
			m_indices.clear();

			m_minimalUpdateDistance = DefaultMinimalUpdateDistance;
			m_squareQuadCount = 0;
			m_quadCount = 0;
			m_squarePointCount = 0;
			m_pointCount = 0;
			m_vertexColorMap.reset();
		}
	}

	bool
	AdaptiveVertexGridResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not available yet !");

		return this->setLoadSuccess(false);
	}

	bool
	AdaptiveVertexGridResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not available yet !");

		return this->setLoadSuccess(false);
	}

	bool
	AdaptiveVertexGridResource::load (const Grid< float > & baseGrid, uint32_t quadCount, const Vector< 3, float > & position) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !baseGrid.isValid() )
		{
			Tracer::warning(ClassId, "Grid data is not yet loaded, so it can't be moved to video memory.");

			return this->setLoadSuccess(false);
		}

		if ( quadCount >= baseGrid.quadCount() )
		{
			TraceError{ClassId} << "Grid data must be smaller than AdaptiveGridGeometry !";

			return this->setLoadSuccess(false);
		}

		m_squareQuadCount = quadCount;
		m_quadCount = m_squareQuadCount * m_squareQuadCount;
		m_squarePointCount = m_squareQuadCount + 1;
		m_pointCount = m_squarePointCount * m_squarePointCount;
		m_minimalUpdateDistance = baseGrid.quadSize() * static_cast< float >(m_squareQuadCount) * Half< float >;

		/* Generate vertices buffer. */
		if ( !this->updateLocalData(baseGrid, position) )
		{
			return this->setLoadSuccess(false);
		}

		/* Generate indices buffer. */
		if ( !this->generateIndicesBuffer() )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	uint32_t
	AdaptiveVertexGridResource::findStartingOffset (const Grid< float > & baseGrid, const Vector< 3, float > & position) const noexcept
	{
		/* Convert from 3D to 2D coordinates and shift the position to the bottom-left
		 * position of the future adaptive grid to get the right first quad. */
		const auto xCoord = position[X] - m_minimalUpdateDistance;
		const auto yCoord = position[Z] - m_minimalUpdateDistance;

		const auto offset = baseGrid.nearestQuad(xCoord, yCoord).topLeftIndex();
		const auto max = baseGrid.pointCount() - m_pointCount;

		if ( offset >= max )
		{
			return max - 1;
		}

		return offset;
	}

	bool
	AdaptiveVertexGridResource::generateIndicesBuffer () noexcept
	{
		for ( uint32_t yIndex = 0; yIndex < m_squareQuadCount; yIndex++ )
		{
			const auto rowOffset = yIndex * m_squarePointCount;

			for ( uint32_t xIndex = 0; xIndex < m_squareQuadCount; xIndex++ )
			{
				const auto topLeft = rowOffset + xIndex;
				const auto bottomLeft = topLeft + m_squarePointCount;
				const auto topRight = topLeft + 1;
				const auto bottomRight = bottomLeft + 1;

				if ( xIndex == 0 )
				{
					m_indices.emplace_back(topLeft);
					m_indices.emplace_back(bottomLeft);
				}

				m_indices.emplace_back(topRight);
				m_indices.emplace_back(bottomRight);
			}

			/* Primitive restart. This will break the triangle strip. */
			m_indices.emplace_back(std::numeric_limits< uint32_t >::max());
		}

		return !m_indices.empty();
	}

	bool
	AdaptiveVertexGridResource::updateLocalData (const Grid< float > & baseGrid, const Vector< 3, float > & position) noexcept
	{
		if ( !baseGrid.isValid() )
		{
			Tracer::warning(ClassId, "Grid data is no more valid... Unable to use it for updating the adaptive grid.");

			return false;
		}

		m_localData.clear();

		const auto startingOffset = this->findStartingOffset(baseGrid, position);

		for ( uint32_t yIndex = 0; yIndex < m_squarePointCount; yIndex++ )
		{
			const auto rowOffset = startingOffset + (yIndex * baseGrid.squaredPointCount());

			for ( uint32_t xIndex = 0; xIndex < m_squarePointCount; xIndex++ )
			{
				const auto index = rowOffset + xIndex;

				AdaptiveVertexGridResource::addGridPointToVertexAttributes(baseGrid, index);
			}
		}

		return true;
	}

	void
	AdaptiveVertexGridResource::addGridPointToVertexAttributes (const Grid< float > & grid, uint32_t index) noexcept
	{
		const auto position = grid.position(index);

		/* Position */
		m_localData.emplace_back(position[X]);
		m_localData.emplace_back(position[Y]);
		m_localData.emplace_back(position[Z]);

		if ( this->isFlagEnabled(EnableTangentSpace) )
		{
			const auto normal = grid.normal(index, position);
			const auto tangent = grid.tangent(index, position, grid.textureCoordinates3D(index));
			const auto binormal = Vector< 3, float >::crossProduct(normal, tangent);

			/* Tangent */
			m_localData.emplace_back(tangent[X]);
			m_localData.emplace_back(tangent[Y]);
			m_localData.emplace_back(tangent[Z]);

			/* Binormal */
			m_localData.emplace_back(binormal[X]);
			m_localData.emplace_back(binormal[Y]);
			m_localData.emplace_back(binormal[Z]);

			/* Normal */
			m_localData.emplace_back(normal[X]);
			m_localData.emplace_back(normal[Y]);
			m_localData.emplace_back(normal[Z]);
		}
		else if ( this->isFlagEnabled(EnableNormal) )
		{
			const auto normal = grid.normal(index, position);

			/* Normal */
			m_localData.emplace_back(normal[X]);
			m_localData.emplace_back(normal[Y]);
			m_localData.emplace_back(normal[Z]);
		}

		if ( this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DPrimaryTextureCoordinates) )
			{
				const auto UVWCoords = grid.textureCoordinates3D(index);

				/* 3D texture coordinates */
				m_localData.emplace_back(UVWCoords[X]);
				m_localData.emplace_back(UVWCoords[Y]);
				m_localData.emplace_back(UVWCoords[Z]);
			}
			else
			{
				const auto UVCoords = grid.textureCoordinates2D(index);

				/* 2D texture coordinates */
				m_localData.emplace_back(UVCoords[X]);
				m_localData.emplace_back(UVCoords[Y]);
			}
		}

		/* FIXME: For now the secondary texture is the same as the primary. */
		if ( this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DSecondaryTextureCoordinates) )
			{
				const auto UVWCoords = grid.textureCoordinates3D(index);

				/* 3D texture coordinates */
				m_localData.emplace_back(UVWCoords[X]);
				m_localData.emplace_back(UVWCoords[Y]);
				m_localData.emplace_back(UVWCoords[Z]);
			}
			else
			{
				const auto UVCoords = grid.textureCoordinates2D(index);

				/* 2D Texture2Ds CartesianFrame */
				m_localData.emplace_back(UVCoords[X]);
				m_localData.emplace_back(UVCoords[Y]);
			}
		}

		/* Vertex color. */
		if ( this->isFlagEnabled(EnableVertexColor) )
		{
			const auto color = Color< float >::quickRandom();
			
			m_localData.emplace_back(color.red());
			m_localData.emplace_back(color.green());
			m_localData.emplace_back(color.blue());
			m_localData.emplace_back(1.0);
		}

		/* Vertex weight. */
		if ( this->isFlagEnabled(EnableWeight) )
		{
			m_localData.emplace_back(1.0);
			m_localData.emplace_back(1.0);
			m_localData.emplace_back(1.0);
			m_localData.emplace_back(1.0);
		}
	}

	void
	AdaptiveVertexGridResource::updateVisibility (const CartesianFrame< float > & /*view*/) noexcept
	{
		/* FIXME: Empty function. */
	}

	void
	AdaptiveVertexGridResource::enableVertexColor (const std::shared_ptr< ImageResource > & vertexColorMap) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::error(ClassId, "Vertex color must be enabled before loading the data !");

			return;
		}

		m_vertexColorMap = vertexColorMap;
	}
}
