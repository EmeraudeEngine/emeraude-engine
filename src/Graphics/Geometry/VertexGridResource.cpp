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

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::Geometry::VertexGridResource >::ClassId{"VertexGridContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::Geometry::VertexGridResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::Geometry
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::VertexFactory;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Vulkan;

	const size_t VertexGridResource::ClassUID{getClassUID(ClassId)};

	bool
	VertexGridResource::createOnHardware (TransferManager & transferManager) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::warning(ClassId, "The buffers are already in video memory ! Use processLogics() instead.");

			return true;
		}

		/* Checking local data ... */
		if ( !m_localData.isValid() )
		{
			TraceError{ClassId} <<
				"Resource '" << this->name() << "' has invalid local data ! "
				"Loading into video memory cancelled.";

			return false;
		}

		/* NOTE: Example for a 4 divisions square. */

		/* Create the vertex and the index buffers local data. */
		const auto vertexElementCount = getElementCountFromFlags(this->flags());
		const auto rowCount = m_localData.squaredQuadCount(); /* 4 */
		/* This holds the number of indices requested to draw a
		 * full row of quads, including the primitive restart. */
		const auto indexPerRowCount = (m_localData.squaredPointCount() * 2) + 1; /* 11 */
		const auto indexCount = indexPerRowCount * rowCount; /* 44 */

		std::vector< float > vertexAttributes;
		vertexAttributes.reserve(m_localData.pointCount() * vertexElementCount); /* 25 * element count per vertex */

		std::vector< uint32_t > indices;
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

		/* Create hardware buffers from local data. */
		return this->createVideoMemoryBuffers(transferManager, vertexAttributes, m_localData.pointCount(), vertexElementCount, indices);
	}

	bool
	VertexGridResource::createVideoMemoryBuffers (TransferManager & transferManager, const std::vector< float > & vertexAttributes, uint32_t vertexCount, uint32_t vertexElementCount, const std::vector< uint32_t > & indices) noexcept
	{
		m_vertexBufferObject = std::make_unique< VertexBufferObject >(transferManager.device(), vertexCount, vertexElementCount);
		m_vertexBufferObject->setIdentifier(ClassId, this->name(), "VertexBufferObject");

		if ( !m_vertexBufferObject->create(transferManager, vertexAttributes) )
		{
			Tracer::error(ClassId, "Unable to create the vertex buffer object (VBO) !");

			m_vertexBufferObject.reset();

			return false;
		}

		m_indexBufferObject = std::make_unique< IndexBufferObject >(transferManager.device(), static_cast< uint32_t >(indices.size()));
		m_indexBufferObject->setIdentifier(ClassId, this->name(), "IndexBufferObject");

		if ( !m_indexBufferObject->create(transferManager, indices) )
		{
			Tracer::error(ClassId, "Unable to get an index buffer object (IBO) !");

			m_indexBufferObject.reset();

			return false;
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

	bool
	VertexGridResource::load () noexcept
	{
		return this->load(DefaultSize, DefaultDivision, DefaultUVMultiplier);
	}

	bool
	VertexGridResource::load (const Json::Value & data) noexcept
	{
		return this->load(
			FastJSON::getValue< float >(data, JKSize).value_or(DefaultSize),
			FastJSON::getValue< uint32_t >(data, JKDivision).value_or(DefaultDivision),
			FastJSON::getValue< float >(data, JKUVMultiplier).value_or(DefaultUVMultiplier)
		);
	}

	bool
	VertexGridResource::load (float size, uint32_t division, float UVMultiplier, const VertexColorGenMode & vertexColorGenMode, const Color< float > & globalVertexColor) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( size < 0.0F )
		{
			Tracer::error(ClassId, "Size parameter must be positive !");

			return this->setLoadSuccess(false);
		}

		if ( division < 1 )
		{
			Tracer::error(ClassId, "Division parameter must be a least 1 !");

			return this->setLoadSuccess(false);
		}

		if ( !m_localData.initializeData(size, division) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		m_localData.setUVMultiplier(UVMultiplier);
		m_vertexColorGenMode = vertexColorGenMode;
		m_globalVertexColor = globalVertexColor;

		return this->setLoadSuccess(true);
	}

	bool
	VertexGridResource::load (const Grid< float > & grid, const VertexColorGenMode & vertexColorGenMode, const Color< float > & globalVertexColor) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !grid.isValid() )
		{
			Tracer::error(ClassId, "The base geometry is not usable !");

			return this->setLoadSuccess(false);
		}

		m_localData = grid;
		m_vertexColorGenMode = vertexColorGenMode;
		m_globalVertexColor = globalVertexColor;

		return this->setLoadSuccess(true);
	}

	uint32_t
	VertexGridResource::addVertexToBuffer (uint32_t index, std::vector< float > & buffer, uint32_t vertexElementCount) const noexcept
	{
		const auto position = m_localData.position(index);

		/* Vertex position */
		buffer.emplace_back(position[X]);
		buffer.emplace_back(position[Y]);
		buffer.emplace_back(position[Z]);

		if ( this->isFlagEnabled(EnableTangentSpace) )
		{
			const auto normal = m_localData.normal(index, position);
			const auto tangent = m_localData.tangent(index, position, m_localData.textureCoordinates3D(index));
			const auto binormal = Vector< 3, float >::crossProduct(normal, tangent);

			/* Tangent */
			buffer.emplace_back(tangent[X]);
			buffer.emplace_back(tangent[Y]);
			buffer.emplace_back(tangent[Z]);

			/* Binormal */
			buffer.emplace_back(binormal[X]);
			buffer.emplace_back(binormal[Y]);
			buffer.emplace_back(binormal[Z]);

			/* Normal */
			buffer.emplace_back(normal[X]);
			buffer.emplace_back(normal[Y]);
			buffer.emplace_back(normal[Z]);
		}
		else if ( this->isFlagEnabled(EnableNormal) )
		{
			const auto normal = m_localData.normal(index, position);

			/* Normal */
			buffer.emplace_back(normal[X]);
			buffer.emplace_back(normal[Y]);
			buffer.emplace_back(normal[Z]);
		}

		if ( this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DPrimaryTextureCoordinates) )
			{
				const auto UVWCoords = m_localData.textureCoordinates3D(index);

				/* 3D texture coordinates */
				buffer.emplace_back(UVWCoords[X]);
				buffer.emplace_back(UVWCoords[Y]);
				buffer.emplace_back(UVWCoords[Z]);
			}
			else
			{
				const auto UVCoords = m_localData.textureCoordinates2D(index);

				/* 2D texture coordinates */
				buffer.emplace_back(UVCoords[X]);
				buffer.emplace_back(UVCoords[Y]);
			}
		}

		/* FIXME: For now the secondary texture are the same as primary. */
		if ( this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DSecondaryTextureCoordinates) )
			{
				const auto UVWCoords = m_localData.textureCoordinates3D(index);

				/* 3D texture coordinates */
				buffer.emplace_back(UVWCoords[X]);
				buffer.emplace_back(UVWCoords[Y]);
				buffer.emplace_back(UVWCoords[Z]);
			}
			else
			{
				const auto UVCoords = m_localData.textureCoordinates2D(index);

				/* 2D texture coordinates */
				buffer.emplace_back(UVCoords[X]);
				buffer.emplace_back(UVCoords[Y]);
			}
		}

		/* Vertex color. */
		if ( this->isFlagEnabled(EnableVertexColor) )
		{
			switch ( m_vertexColorGenMode )
			{
				case VertexColorGenMode::UseGlobal :
					buffer.emplace_back(m_globalVertexColor.red());
					buffer.emplace_back(m_globalVertexColor.green());
					buffer.emplace_back(m_globalVertexColor.blue());
					buffer.emplace_back(1.0);
					break;

				case VertexColorGenMode::UseRandom :
				{
					const auto randomColor = Color< float >::quickRandom();

					buffer.emplace_back(randomColor.red());
					buffer.emplace_back(randomColor.green());
					buffer.emplace_back(randomColor.blue());
					buffer.emplace_back(1.0);
				}
					break;

				case VertexColorGenMode::GenerateFromCoords :
				{
					const auto UVCoords = m_localData.textureCoordinates2D(index);
					const auto level = 1.0F - ((position[Y] - m_localData.boundingBox().minimum(Y)) / m_localData.boundingBox().height());

					buffer.emplace_back(UVCoords[X] / m_localData.UMultiplier());
					buffer.emplace_back(UVCoords[Y] / m_localData.VMultiplier());
					buffer.emplace_back(level);
				}
					break;
			}
		}

		/* Vertex weight. */
		if ( this->isFlagEnabled(EnableWeight) )
		{
			buffer.emplace_back(1.0);
			buffer.emplace_back(1.0);
			buffer.emplace_back(1.0);
			buffer.emplace_back(1.0);
		}

		return static_cast< uint32_t >(buffer.size() / vertexElementCount) - 1;
	}
	}
