/*
 * src/Graphics/Geometry/VertexGridResource.cpp
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

#include "VertexGridResource.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Libs::PixelFactory;
	using namespace Vulkan;

	[[nodiscard]]
	bool
	VertexGridResource::generateGPUBuffers (std::vector< float > & vertexAttributes, uint32_t vertexElementCount, std::vector< uint32_t > & indices) const noexcept
	{
		if ( !m_localData.isValid() )
		{
			TraceError{ClassId} <<
				"Resource '" << this->name() << "' has invalid local data ! "
				"Loading into video memory cancelled.";

			return false;
		}

		/* Create the vertex and the index buffers local data. */
		const auto rowCount = m_localData.squaredQuadCount(); /* 4 */

		/* 25 * element count per vertex */
		vertexAttributes.reserve(m_localData.pointCount() * vertexElementCount);

		/* This holds the number of indices requested to draw a
		 * full row of quads, including the primitive restart. */
		const auto indexPerRowCount = (m_localData.squaredPointCount() * 2) + 1; /* 11 */
		const auto indexCount = indexPerRowCount * rowCount; /* 44 */

		indices.reserve(indexCount);

		for ( auto quadYIndex = 0U; quadYIndex < rowCount; quadYIndex++ )
		{
			const auto sharedIndexesOffset = quadYIndex > 1 ? (quadYIndex * indexPerRowCount) - indexPerRowCount : 0;

			for ( auto quadXIndex = 0U; quadXIndex < rowCount; quadXIndex++ )
			{
				const auto currentQuad = m_localData.quad(quadXIndex, quadYIndex);

				/* NOTE: Only once by row of quads at executing because of GL_TRIANGLE_STRIP technic. */
				if ( quadXIndex == 0 )
				{
					/* NOTE: Shared index if above row 0. */
					if ( quadYIndex == 0 )
					{
						/* Top left vertex. */
						indices.emplace_back(this->addVertexToBuffer(currentQuad.topLeftIndex(), vertexAttributes, vertexElementCount));
					}
					else
					{
						/* Adds a previously registered vertex index. */
						indices.emplace_back(indices[sharedIndexesOffset + 1]);
					}

					/* Bottom left vertex. */
					indices.emplace_back(this->addVertexToBuffer(currentQuad.bottomLeftIndex(), vertexAttributes, vertexElementCount));
				}

				/* NOTE: Shared index if above row 0. */
				if ( quadYIndex == 0 )
				{
					/* Top right vertex. */
					indices.emplace_back(this->addVertexToBuffer(currentQuad.topRightIndex(), vertexAttributes, vertexElementCount));
				}
				else
				{
					/* Adds a previously registered vertex index. */
					indices.emplace_back(indices[sharedIndexesOffset + ((quadXIndex + 1) * 2) + 1]);
				}

				/* Bottom right vertex. */
				indices.emplace_back(this->addVertexToBuffer(currentQuad.bottomRightIndex(), vertexAttributes, vertexElementCount));
			}

			/* Add a primitive restart hint. This will break the triangle strip. */
			indices.emplace_back(std::numeric_limits< uint32_t >::max());
		}

		if ( vertexAttributes.empty() || indices.empty() || vertexElementCount == 0 )
		{
			Tracer::error(ClassId, "Buffers creation fails !");

			return false;
		}

		return true;
	}

	bool
	VertexGridResource::createOnHardware (TransferManager & transferManager) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::warning(ClassId, "The buffers are already in video memory ! Use processLogics() instead.");

			return true;
		}

		/* NOTE: Prepare vectors in the desired format for the GPU. */
		const auto vertexElementCount = getElementCountFromFlags(this->flags());

		std::vector< float > vertexAttributes;
		std::vector< uint32_t > indices;

		if ( !this->generateGPUBuffers(vertexAttributes, vertexElementCount, indices) )
		{
			return false;
		}

		/* Create the VBO. */
		{
			m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), m_localData.pointCount(), vertexElementCount, false);
			m_vertexBufferObject->setIdentifier(ClassId, this->name(), "VertexBufferObject");

			if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->transferData(transferManager, vertexAttributes) )
			{
				Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

				m_vertexBufferObject.reset();

				return false;
			}
		}

		/* Create the IBO. */
		{
			m_indexBufferObject = std::make_unique< IndexBufferObject >(transferManager.device(), static_cast< uint32_t >(indices.size()));
			m_indexBufferObject->setIdentifier(ClassId, this->name(), "IndexBufferObject");

			if ( !m_indexBufferObject->createOnHardware() || !m_indexBufferObject->transferData(transferManager, indices) )
			{
				Tracer::error(ClassId, "Unable to get an index buffer object (IBO) !");

				m_indexBufferObject.reset();

				return false;
			}
		}

		return true;
	}

	bool
	VertexGridResource::updateVideoMemory () noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::warning(ClassId, "No buffer in video memory to update !");

			return false;
		}

		Tracer::warning(ClassId, "Updating geometry in video memory is not handled yet !");

		return true;
	}

	void
	VertexGridResource::destroyFromHardware (bool clearLocalData) noexcept
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
		}
	}

	void
	VertexGridResource::enableVertexColor (const Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::error(ClassId, "Vertex color must be enabled before loading the data !");

			return;
		}

		m_vertexColorGenMode = VertexColorGenMode::UseGlobalColor;
		m_globalVertexColor = color;
		m_vertexColorMap.reset();
	}

	void
	VertexGridResource::enableVertexColor (const std::shared_ptr< ImageResource > & colorMap) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::error(ClassId, "Vertex color must be enabled before loading the data !");

			return;
		}

		m_vertexColorGenMode = VertexColorGenMode::UseColorMap;
		m_vertexColorMap = colorMap;
	}

	void
	VertexGridResource::enableVertexColorRandom () noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::error(ClassId, "Vertex color must be enabled before loading the data !");

			return;
		}

		m_vertexColorGenMode = VertexColorGenMode::UseRandom;
		m_vertexColorMap.reset();
	}

	void
	VertexGridResource::enableVertexColorFromCoords () noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::error(ClassId, "Vertex color must be enabled before loading the data !");

			return;
		}

		m_vertexColorGenMode = VertexColorGenMode::GenerateFromCoords;
		m_vertexColorMap.reset();
	}

	bool
	VertexGridResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		return this->load(DefaultGridSize, DefaultGridDivision, DefaultUVMultiplier);
	}

	bool
	VertexGridResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & data) noexcept
	{
		return this->load(
			FastJSON::getValue< float >(data, JKSize).value_or(DefaultGridSize),
			FastJSON::getValue< uint32_t >(data, JKDivision).value_or(DefaultGridDivision),
			FastJSON::getValue< float >(data, JKUVMultiplier).value_or(DefaultUVMultiplier)
		);
	}

	bool
	VertexGridResource::load (float gridSize, uint32_t gridDivision, float UVMultiplier) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !m_localData.initializeByGridSize(gridSize, gridDivision) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		m_localData.setUVMultiplier(UVMultiplier);

		return this->setLoadSuccess(true);
	}

	bool
	VertexGridResource::load (const Grid< float > & grid) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !grid.isValid() )
		{
			Tracer::error(ClassId, "The grid geometry is invalid!");

			return this->setLoadSuccess(false);
		}

		m_localData = grid;

		return this->setLoadSuccess(true);
	}

	uint32_t
	VertexGridResource::addVertexToBuffer (uint32_t pointIndex, std::vector< float > & vertexAttributes, uint32_t vertexElementCount) const noexcept
	{
		const auto position = m_localData.position(pointIndex);

		/* Vertex position */
		vertexAttributes.emplace_back(position[X]);
		vertexAttributes.emplace_back(position[Y]);
		vertexAttributes.emplace_back(position[Z]);

		if ( this->isFlagEnabled(EnableTangentSpace) )
		{
			const auto normal = m_localData.normal(pointIndex, position);
			const auto tangent = m_localData.tangent(pointIndex, position, m_localData.textureCoordinates3D(pointIndex));
			const auto binormal = Vector< 3, float >::crossProduct(normal, tangent);

			/* Tangent */
			vertexAttributes.emplace_back(tangent[X]);
			vertexAttributes.emplace_back(tangent[Y]);
			vertexAttributes.emplace_back(tangent[Z]);

			/* Binormal */
			vertexAttributes.emplace_back(binormal[X]);
			vertexAttributes.emplace_back(binormal[Y]);
			vertexAttributes.emplace_back(binormal[Z]);

			/* Normal */
			vertexAttributes.emplace_back(normal[X]);
			vertexAttributes.emplace_back(normal[Y]);
			vertexAttributes.emplace_back(normal[Z]);
		}
		else if ( this->isFlagEnabled(EnableNormal) )
		{
			const auto normal = m_localData.normal(pointIndex, position);

			/* Normal */
			vertexAttributes.emplace_back(normal[X]);
			vertexAttributes.emplace_back(normal[Y]);
			vertexAttributes.emplace_back(normal[Z]);
		}

		if ( this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DPrimaryTextureCoordinates) )
			{
				const auto UVWCoords = m_localData.textureCoordinates3D(pointIndex);

				/* 3D texture coordinates */
				vertexAttributes.emplace_back(UVWCoords[X]);
				vertexAttributes.emplace_back(UVWCoords[Y]);
				vertexAttributes.emplace_back(UVWCoords[Z]);
			}
			else
			{
				const auto UVCoords = m_localData.textureCoordinates2D(pointIndex);

				/* 2D texture coordinates */
				vertexAttributes.emplace_back(UVCoords[X]);
				vertexAttributes.emplace_back(UVCoords[Y]);
			}
		}

		/* FIXME: For now the secondary texture are the same as primary. */
		if ( this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DSecondaryTextureCoordinates) )
			{
				const auto UVWCoords = m_localData.textureCoordinates3D(pointIndex);

				/* 3D texture coordinates */
				vertexAttributes.emplace_back(UVWCoords[X]);
				vertexAttributes.emplace_back(UVWCoords[Y]);
				vertexAttributes.emplace_back(UVWCoords[Z]);
			}
			else
			{
				const auto UVCoords = m_localData.textureCoordinates2D(pointIndex);

				/* 2D texture coordinates */
				vertexAttributes.emplace_back(UVCoords[X]);
				vertexAttributes.emplace_back(UVCoords[Y]);
			}
		}

		/* Vertex color. */
		if ( this->isFlagEnabled(EnableVertexColor) )
		{
			switch ( m_vertexColorGenMode )
			{
				case VertexColorGenMode::UseGlobalColor :
					vertexAttributes.emplace_back(m_globalVertexColor.red());
					vertexAttributes.emplace_back(m_globalVertexColor.green());
					vertexAttributes.emplace_back(m_globalVertexColor.blue());
					vertexAttributes.emplace_back(1.0F);
					break;

				case VertexColorGenMode::UseColorMap:
					// TODO
					break;

				case VertexColorGenMode::UseRandom :
				{
					const auto randomColor = Color< float >::quickRandom();

					vertexAttributes.emplace_back(randomColor.red());
					vertexAttributes.emplace_back(randomColor.green());
					vertexAttributes.emplace_back(randomColor.blue());
					vertexAttributes.emplace_back(1.0F);
				}
					break;

				case VertexColorGenMode::GenerateFromCoords :
				{
					const auto UVCoords = m_localData.textureCoordinates2D(pointIndex);
					const auto level = 1.0F - ((position[Y] - m_localData.boundingBox().minimum(Y)) / m_localData.boundingBox().height());

					vertexAttributes.emplace_back(UVCoords[X] / m_localData.UMultiplier());
					vertexAttributes.emplace_back(UVCoords[Y] / m_localData.VMultiplier());
					vertexAttributes.emplace_back(level);
				}
					break;
			}
		}

		/* Vertex weight. */
		if ( this->isFlagEnabled(EnableWeight) )
		{
			vertexAttributes.emplace_back(1.0F);
			vertexAttributes.emplace_back(1.0F);
			vertexAttributes.emplace_back(1.0F);
			vertexAttributes.emplace_back(1.0F);
		}

		return static_cast< uint32_t >(vertexAttributes.size() / vertexElementCount) - 1;
	}
}
