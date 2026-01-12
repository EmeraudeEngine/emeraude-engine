/*
 * src/Graphics/Geometry/AdaptiveVertexGridResource.cpp
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

#include "AdaptiveVertexGridResource.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Libs::PixelFactory;
	using namespace Vulkan;

	bool
	AdaptiveVertexGridResource::generateGPUBuffers (std::vector< float > & vertexAttributes, uint32_t vertexElementCount, std::vector< uint32_t > & indices) noexcept
	{
		if ( !m_localData.isValid() )
		{
			TraceError{ClassId} <<
				"Resource '" << this->name() << "' has invalid local data ! "
				"Loading into video memory cancelled.";

			return false;
		}

		/* === STEP 1: Create VBO with all vertices === */
		const auto totalPoints = m_localData.pointCount();

		vertexAttributes.reserve(totalPoints * vertexElementCount);

		for ( uint32_t pointIndex = 0; pointIndex < totalPoints; ++pointIndex )
		{
			(void) this->addVertexToBuffer(pointIndex, vertexAttributes, vertexElementCount);
		}

		/* === STEP 2: Prepare sector data === */
		const auto gridQuadCount = m_localData.squaredQuadCount();
		const auto quadsPerSector = gridQuadCount / m_sectorCountPerAxis;

		m_sectorsData.clear();
		m_sectorsData.reserve(this->sectorCount());

		/* Estimate index count for reservation. */
		uint32_t estimatedIndexCount = 0;

		for ( uint32_t lodLevel = 0; lodLevel < m_lodLevelCount; ++lodLevel )
		{
			const auto step = 1U << lodLevel;
			const auto rowsPerSector = (quadsPerSector + step - 1) / step;
			const auto colsPerSector = rowsPerSector;
			const auto indicesPerSectorLOD = rowsPerSector * ((colsPerSector + 1) * 2 + 1);

			estimatedIndexCount += indicesPerSectorLOD * this->sectorCount();
		}

		indices.reserve(estimatedIndexCount);

		/* === STEP 3: Generate indices for each sector and each LOD === */
		for ( uint32_t sectorY = 0; sectorY < m_sectorCountPerAxis; ++sectorY )
		{
			for ( uint32_t sectorX = 0; sectorX < m_sectorCountPerAxis; ++sectorX )
			{
				SectorLODData sectorData;
				sectorData.sectorX = sectorX;
				sectorData.sectorY = sectorY;

				/* Calculate sector quad boundaries. */
				const auto sectorQuadStartX = sectorX * quadsPerSector;
				const auto sectorQuadStartY = sectorY * quadsPerSector;
				const auto sectorQuadEndX = sectorQuadStartX + quadsPerSector;
				const auto sectorQuadEndY = sectorQuadStartY + quadsPerSector;

				/* Calculate sector bounding box from corner positions. */
				const auto topLeft = m_localData.position(sectorQuadStartX, sectorQuadStartY);
				const auto bottomRight = m_localData.position(sectorQuadEndX, sectorQuadEndY);

				sectorData.bounds.set(
					{topLeft[X], m_localData.boundingBox().maximum(Y), topLeft[Z]},
					{bottomRight[X], m_localData.boundingBox().minimum(Y), bottomRight[Z]}
				);

				/* Generate indices for each LOD level (or just the forced one). */
				const auto startLOD = (m_forcedLODLevel < m_lodLevelCount) ? m_forcedLODLevel : 0U;
				const auto endLOD = (m_forcedLODLevel < m_lodLevelCount) ? (m_forcedLODLevel + 1U) : m_lodLevelCount;

				for ( uint32_t lodLevel = startLOD; lodLevel < endLOD; ++lodLevel )
				{
					const auto step = 1U << lodLevel;

					SectorDrawCall drawCall;
					drawCall.indexOffset = static_cast< uint32_t >(indices.size());

					/* Generate triangle strip rows for this sector at this LOD. */
					for ( auto pointY = sectorQuadStartY; pointY < sectorQuadEndY; pointY += step )
					{
						const auto nextPointY = std::min(pointY + step, sectorQuadEndY);

						/* For each column of points in the row. */
						for ( auto pointX = sectorQuadStartX; pointX <= sectorQuadEndX; pointX += step )
						{
							/* Clamp to sector boundary. */
							const auto clampedX = std::min(pointX, sectorQuadEndX);

							/* Top vertex. */
							indices.emplace_back(m_localData.index(clampedX, pointY));

							/* Bottom vertex. */
							indices.emplace_back(m_localData.index(clampedX, nextPointY));
						}

						/* Add primitive restart to end this strip. */
						indices.emplace_back(std::numeric_limits< uint32_t >::max());
					}

					drawCall.indexCount = static_cast< uint32_t >(indices.size()) - drawCall.indexOffset;
					sectorData.lodDrawCalls[lodLevel] = drawCall;
				}

				/* === Generate edge stitching for LOD transitions === */
				for ( uint32_t lodLevel = startLOD; lodLevel < endLOD && lodLevel < m_lodLevelCount - 1; ++lodLevel )
				{
					const auto stepHigh = 1U << lodLevel;	   /* Higher detail (more vertices) */
					const auto stepLow = 1U << (lodLevel + 1);  /* Lower detail (fewer vertices) */

					/* North edge (Z-): connects points at sectorQuadStartY */
					{
						SectorDrawCall & drawCall = sectorData.edgeStitching[lodLevel][static_cast< size_t >(SectorEdge::North)];
						drawCall.indexOffset = static_cast< uint32_t >(indices.size());

						for ( auto pointX = sectorQuadStartX; pointX < sectorQuadEndX; pointX += stepLow )
						{
							const auto nextLowX = std::min(pointX + stepLow, sectorQuadEndX);

							/* High detail vertices along this segment */
							for ( auto highX = pointX; highX < nextLowX; highX += stepHigh )
							{
								const auto nextHighX = std::min(highX + stepHigh, nextLowX);

								/* Triangle: lowLeft, highCurrent, highNext */
								indices.emplace_back(m_localData.index(pointX, sectorQuadStartY));
								indices.emplace_back(m_localData.index(highX, sectorQuadStartY));
								indices.emplace_back(m_localData.index(nextHighX, sectorQuadStartY));
								indices.emplace_back(std::numeric_limits< uint32_t >::max());
							}
						}

						drawCall.indexCount = static_cast< uint32_t >(indices.size()) - drawCall.indexOffset;
					}

					/* South edge (Z+): connects points at sectorQuadEndY */
					{
						SectorDrawCall & drawCall = sectorData.edgeStitching[lodLevel][static_cast< size_t >(SectorEdge::South)];
						drawCall.indexOffset = static_cast< uint32_t >(indices.size());

						for ( auto pointX = sectorQuadStartX; pointX < sectorQuadEndX; pointX += stepLow )
						{
							const auto nextLowX = std::min(pointX + stepLow, sectorQuadEndX);

							for ( auto highX = pointX; highX < nextLowX; highX += stepHigh )
							{
								const auto nextHighX = std::min(highX + stepHigh, nextLowX);

								indices.emplace_back(m_localData.index(pointX, sectorQuadEndY));
								indices.emplace_back(m_localData.index(nextHighX, sectorQuadEndY));
								indices.emplace_back(m_localData.index(highX, sectorQuadEndY));
								indices.emplace_back(std::numeric_limits< uint32_t >::max());
							}
						}

						drawCall.indexCount = static_cast< uint32_t >(indices.size()) - drawCall.indexOffset;
					}

					/* West edge (X-): connects points at sectorQuadStartX */
					{
						SectorDrawCall & drawCall = sectorData.edgeStitching[lodLevel][static_cast< size_t >(SectorEdge::West)];
						drawCall.indexOffset = static_cast< uint32_t >(indices.size());

						for ( auto pointY = sectorQuadStartY; pointY < sectorQuadEndY; pointY += stepLow )
						{
							const auto nextLowY = std::min(pointY + stepLow, sectorQuadEndY);

							for ( auto highY = pointY; highY < nextLowY; highY += stepHigh )
							{
								const auto nextHighY = std::min(highY + stepHigh, nextLowY);

								indices.emplace_back(m_localData.index(sectorQuadStartX, pointY));
								indices.emplace_back(m_localData.index(sectorQuadStartX, nextHighY));
								indices.emplace_back(m_localData.index(sectorQuadStartX, highY));
								indices.emplace_back(std::numeric_limits< uint32_t >::max());
							}
						}

						drawCall.indexCount = static_cast< uint32_t >(indices.size()) - drawCall.indexOffset;
					}

					/* East edge (X+): connects points at sectorQuadEndX */
					{
						SectorDrawCall & drawCall = sectorData.edgeStitching[lodLevel][static_cast< size_t >(SectorEdge::East)];
						drawCall.indexOffset = static_cast< uint32_t >(indices.size());

						for ( auto pointY = sectorQuadStartY; pointY < sectorQuadEndY; pointY += stepLow )
						{
							const auto nextLowY = std::min(pointY + stepLow, sectorQuadEndY);

							for ( auto highY = pointY; highY < nextLowY; highY += stepHigh )
							{
								const auto nextHighY = std::min(highY + stepHigh, nextLowY);

								indices.emplace_back(m_localData.index(sectorQuadEndX, pointY));
								indices.emplace_back(m_localData.index(sectorQuadEndX, highY));
								indices.emplace_back(m_localData.index(sectorQuadEndX, nextHighY));
								indices.emplace_back(std::numeric_limits< uint32_t >::max());
							}
						}

						drawCall.indexCount = static_cast< uint32_t >(indices.size()) - drawCall.indexOffset;
					}
				}

				m_sectorsData.emplace_back(std::move(sectorData));
			}
		}

		if ( vertexAttributes.empty() || indices.empty() || vertexElementCount == 0 )
		{
			Tracer::error(ClassId, "Buffers creation failed !");

			return false;
		}

		TraceInfo{ClassId} <<
			"Generated GPU buffers: " << totalPoints << " vertices, " << indices.size() << " indices, " <<
			m_sectorsData.size() << " sectors with " << m_lodLevelCount << " LOD levels each.";

		return true;
	}

	bool
	AdaptiveVertexGridResource::createOnHardware (TransferManager & transferManager) noexcept
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
				Tracer::error(ClassId, "Unable to create the index buffer object (IBO) !");

				m_vertexBufferObject.reset();
				m_indexBufferObject.reset();

				return false;
			}
		}

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

		if ( m_pendingDestructionVBO != nullptr )
		{
			m_pendingDestructionVBO->destroyFromHardware();
			m_pendingDestructionVBO.reset();
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
			m_vertexColorMap.reset();
		}
	}

	void
	AdaptiveVertexGridResource::enableVertexColor (const Color< float > & color) noexcept
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
	AdaptiveVertexGridResource::enableVertexColor (const std::shared_ptr< ImageResource > & colorMap) noexcept
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
	AdaptiveVertexGridResource::enableVertexColorRandom () noexcept
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
	AdaptiveVertexGridResource::enableVertexColorFromCoords () noexcept
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
	AdaptiveVertexGridResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by default!");

		return this->setLoadSuccess(false);
	}

	bool
	AdaptiveVertexGridResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by a JSON file!");

		return this->setLoadSuccess(false);
	}

	bool
	AdaptiveVertexGridResource::load (const Grid< float > & grid, uint32_t sectorCountPerAxis) noexcept
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

		/* Validate sector count. */
		if ( sectorCountPerAxis == 0 )
		{
			Tracer::warning(ClassId, "Sector count per axis cannot be 0, defaulting to 1.");

			sectorCountPerAxis = 1;
		}

		const auto gridDivisions = grid.squaredQuadCount();

		if ( sectorCountPerAxis > gridDivisions )
		{
			TraceWarning{ClassId} <<
				"Sector count per axis (" << sectorCountPerAxis << ") exceeds grid divisions (" << gridDivisions << "). "
				"Clamping to grid divisions.";

			sectorCountPerAxis = gridDivisions;
		}

		/* Validate that grid divisions are evenly divisible by sector count. */
		if ( gridDivisions % sectorCountPerAxis != 0 )
		{
			TraceError{ClassId} <<
				"Grid divisions (" << gridDivisions << ") must be evenly divisible by sector count (" << sectorCountPerAxis << ").";

			return this->setLoadSuccess(false);
		}

		const auto divisionsPerSector = gridDivisions / sectorCountPerAxis;

		/* Calculate the number of LOD levels based on divisions per sector. */
		uint32_t lodLevelCount = 1;

		if ( divisionsPerSector >= 2 )
		{
			/* log2(divisions) gives us how many times we can halve the resolution. */
			const auto maxLod = static_cast< uint32_t >(std::floor(std::log2(divisionsPerSector)));

			/* Clamp to MaxLODLevels (1, 1/2, 1/4, 1/8, 1/16). */
			lodLevelCount = std::min(maxLod, MaxLODLevels);
		}

		/* Validate that divisions per sector is divisible by the maximum step (2^(lodLevelCount-1)). */
		const auto maxStep = 1U << (lodLevelCount - 1);

		if ( divisionsPerSector % maxStep != 0 )
		{
			TraceError{ClassId} <<
				"Divisions per sector (" << divisionsPerSector << ") must be divisible by " << maxStep <<
				" for " << lodLevelCount << " LOD levels. Use a power-of-2 division count or reduce sectors.";

			return this->setLoadSuccess(false);
		}

		m_localData = grid;
		m_sectorCountPerAxis = sectorCountPerAxis;
		m_lodLevelCount = lodLevelCount;

		TraceDebug{ClassId} <<
			"Loaded adaptive grid: " << gridDivisions << "x" << gridDivisions << " divisions, " <<
			m_sectorCountPerAxis << "x" << m_sectorCountPerAxis << " sectors (" << this->sectorCount() << " total), " <<
			m_lodLevelCount << " LOD levels, " <<
			divisionsPerSector << " divisions per sector.";

		return this->setLoadSuccess(true);
	}

	uint32_t
	AdaptiveVertexGridResource::addVertexToBuffer (uint32_t pointIndex, std::vector< float > & vertexAttributes, uint32_t vertexElementCount) const noexcept
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

	bool
	AdaptiveVertexGridResource::updateData (const Grid< float > & grid) noexcept
	{
		/* Validate grid compatibility. */
		if ( !grid.isValid() )
		{
			Tracer::error(ClassId, "Cannot update: grid is invalid!");

			return false;
		}

		if ( grid.pointCount() != m_localData.pointCount() )
		{
			TraceError{ClassId} <<
				"Cannot update: point count mismatch. Expected " << m_localData.pointCount() <<
				", got " << grid.pointCount() << ".";

			return false;
		}

		/* Check if static renderer is available. */
		if ( s_graphicsRenderer == nullptr )
		{
			Tracer::error(ClassId, "Cannot update: no renderer available!");

			return false;
		}

		/* Mark as updating. */
		m_isUpdating.store(true, std::memory_order_release);

		/* Destroy the previously pending VBO (safe now, it's been at least one frame). */
		if ( m_pendingDestructionVBO != nullptr )
		{
			m_pendingDestructionVBO->destroyFromHardware();
			m_pendingDestructionVBO.reset();
		}

		/* Overwrite local data. */
		m_localData = grid;

		/* Update sector bounds to match new grid world position. */
		const auto gridQuadCount = m_localData.squaredQuadCount();
		const auto quadsPerSector = gridQuadCount / m_sectorCountPerAxis;

		for ( auto & sectorData : m_sectorsData )
		{
			const auto sectorQuadStartX = sectorData.sectorX * quadsPerSector;
			const auto sectorQuadStartY = sectorData.sectorY * quadsPerSector;
			const auto sectorQuadEndX = sectorQuadStartX + quadsPerSector;
			const auto sectorQuadEndY = sectorQuadStartY + quadsPerSector;

			const auto topLeft = m_localData.position(sectorQuadStartX, sectorQuadStartY);
			const auto bottomRight = m_localData.position(sectorQuadEndX, sectorQuadEndY);

			sectorData.bounds.set(
				{topLeft[X], m_localData.boundingBox().maximum(Y), topLeft[Z]},
				{bottomRight[X], m_localData.boundingBox().minimum(Y), bottomRight[Z]}
			);
		}

		/* Generate vertex attributes. */
		const auto vertexElementCount = getElementCountFromFlags(this->flags());
		const auto totalPoints = m_localData.pointCount();

		std::vector< float > vertexAttributes;
		vertexAttributes.reserve(totalPoints * vertexElementCount);

		for ( uint32_t pointIndex = 0; pointIndex < totalPoints; ++pointIndex )
		{
			(void) this->addVertexToBuffer(pointIndex, vertexAttributes, vertexElementCount);
		}

		/* Create new VBO. */
		auto newVBO = std::make_unique< VertexBufferObject >(s_graphicsRenderer->transferManager().device(), totalPoints, vertexElementCount, false);
		newVBO->setIdentifier(ClassId, this->name(), "VertexBufferObject");

		if ( !newVBO->createOnHardware() || !newVBO->transferData(s_graphicsRenderer->transferManager(), vertexAttributes) )
		{
			Tracer::error(ClassId, "Failed to create new VBO!");

			m_isUpdating.store(false, std::memory_order_release);

			return false;
		}

		/* Swap VBOs: new one becomes active, old one is queued for deferred destruction. */
		std::swap(m_vertexBufferObject, newVBO);
		m_pendingDestructionVBO = std::move(newVBO);

		m_isUpdating.store(false, std::memory_order_release);

		return true;
	}

	uint32_t
	AdaptiveVertexGridResource::getAdaptiveDrawCallCount (const Vector< 3, float > & /*viewPosition*/) const noexcept
	{
		return static_cast< uint32_t >(m_sectorsData.size());
	}

	std::array< uint32_t, 2 >
	AdaptiveVertexGridResource::getAdaptiveDrawCallRange (uint32_t drawCallIndex, const Vector< 3, float > & viewPosition) const noexcept
	{
		if ( drawCallIndex >= m_sectorsData.size() )
		{
			return {0, 0};
		}

		const auto & sector = m_sectorsData[drawCallIndex];

		/* Use cached LOD if available (from prepareAdaptiveRendering). */
		uint32_t lodLevel = 0;

		if ( drawCallIndex < m_cachedSectorLODs.size() )
		{
			lodLevel = m_cachedSectorLODs[drawCallIndex];
		}
		else
		{
			/* Fallback: compute LOD directly. */
			lodLevel = this->getSectorLOD(drawCallIndex, viewPosition);
		}

		const auto & drawCall = sector.lodDrawCalls[lodLevel];

		return {drawCall.indexOffset, drawCall.indexCount};
	}

	uint32_t
	AdaptiveVertexGridResource::getSectorLOD (uint32_t sectorIndex, const Vector< 3, float > & viewPosition) const noexcept
	{
		if ( sectorIndex >= m_sectorsData.size() )
		{
			return 0;
		}

		const auto & sector = m_sectorsData[sectorIndex];

		if ( m_forcedLODLevel < m_lodLevelCount )
		{
			return m_forcedLODLevel;
		}

		const auto sectorCenter = sector.bounds.centroid();
		const auto distance = Vector< 3, float >::distance(viewPosition, sectorCenter);
		const auto sectorSize = sector.bounds.width();
		auto threshold = sectorSize * m_lodBaseMultiplier;

		for ( uint32_t lod = 0; lod < m_lodLevelCount - 1; ++lod )
		{
			if ( distance <= threshold )
			{
				return lod;
			}

			threshold *= m_lodThresholdGrowth;
		}

		return m_lodLevelCount - 1;
	}

	void
	AdaptiveVertexGridResource::computeAllSectorLODs (const Vector< 3, float > & viewPosition, std::vector< uint32_t > & outLODs) const noexcept
	{
		const auto sectorCount = static_cast< uint32_t >(m_sectorsData.size());
		outLODs.resize(sectorCount);

		/* First pass: compute raw LOD for each sector. */
		for ( uint32_t i = 0; i < sectorCount; ++i )
		{
			outLODs[i] = this->getSectorLOD(i, viewPosition);
		}

		/* Second pass: constrain adjacent sectors to differ by at most 1 LOD.
		 * Iterate until no changes are made (propagate constraints). */
		bool changed = true;

		while ( changed )
		{
			changed = false;

			for ( uint32_t sectorY = 0; sectorY < m_sectorCountPerAxis; ++sectorY )
			{
				for ( uint32_t sectorX = 0; sectorX < m_sectorCountPerAxis; ++sectorX )
				{
					const auto idx = sectorY * m_sectorCountPerAxis + sectorX;
					const auto myLOD = outLODs[idx];

					/* Check North neighbor */
					if ( sectorY > 0 )
					{
						const auto neighborIdx = (sectorY - 1) * m_sectorCountPerAxis + sectorX;

						if ( outLODs[neighborIdx] > myLOD + 1 )
						{
							outLODs[neighborIdx] = myLOD + 1;
							changed = true;
						}
						else if ( myLOD > outLODs[neighborIdx] + 1 )
						{
							outLODs[idx] = outLODs[neighborIdx] + 1;
							changed = true;
						}
					}

					/* Check West neighbor */
					if ( sectorX > 0 )
					{
						const auto neighborIdx = sectorY * m_sectorCountPerAxis + (sectorX - 1);

						if ( outLODs[neighborIdx] > myLOD + 1 )
						{
							outLODs[neighborIdx] = myLOD + 1;
							changed = true;
						}
						else if ( myLOD > outLODs[neighborIdx] + 1 )
						{
							outLODs[idx] = outLODs[neighborIdx] + 1;
							changed = true;
						}
					}
				}
			}
		}
	}

	void
	AdaptiveVertexGridResource::getStitchingDrawCalls (const std::vector< uint32_t > & sectorLODs, std::vector< std::array< uint32_t, 2 > > & outDrawCalls) const noexcept
	{
		outDrawCalls.clear();

		for ( uint32_t sectorY = 0; sectorY < m_sectorCountPerAxis; ++sectorY )
		{
			for ( uint32_t sectorX = 0; sectorX < m_sectorCountPerAxis; ++sectorX )
			{
				const auto idx = sectorY * m_sectorCountPerAxis + sectorX;
				const auto myLOD = sectorLODs[idx];
				const auto & sector = m_sectorsData[idx];

				/* Check South neighbor (sectorY + 1) */
				if ( sectorY < m_sectorCountPerAxis - 1 )
				{
					const auto neighborIdx = (sectorY + 1) * m_sectorCountPerAxis + sectorX;
					const auto neighborLOD = sectorLODs[neighborIdx];

					if ( neighborLOD > myLOD && myLOD < m_lodLevelCount - 1 )
					{
						/* This sector has higher detail, draw stitching on South edge. */
						const auto & stitch = sector.edgeStitching[myLOD][static_cast< size_t >(SectorEdge::South)];

						if ( stitch.indexCount > 0 )
						{
							outDrawCalls.push_back({stitch.indexOffset, stitch.indexCount});
						}
					}
				}

				/* Check East neighbor (sectorX + 1) */
				if ( sectorX < m_sectorCountPerAxis - 1 )
				{
					const auto neighborIdx = sectorY * m_sectorCountPerAxis + (sectorX + 1);
					const auto neighborLOD = sectorLODs[neighborIdx];

					if ( neighborLOD > myLOD && myLOD < m_lodLevelCount - 1 )
					{
						/* This sector has higher detail, draw stitching on East edge. */
						const auto & stitch = sector.edgeStitching[myLOD][static_cast< size_t >(SectorEdge::East)];

						if ( stitch.indexCount > 0 )
						{
							outDrawCalls.push_back({stitch.indexOffset, stitch.indexCount});
						}
					}
				}

				/* Check North neighbor (sectorY - 1) */
				if ( sectorY > 0 )
				{
					const auto neighborIdx = (sectorY - 1) * m_sectorCountPerAxis + sectorX;
					const auto neighborLOD = sectorLODs[neighborIdx];

					if ( neighborLOD > myLOD && myLOD < m_lodLevelCount - 1 )
					{
						const auto & stitch = sector.edgeStitching[myLOD][static_cast< size_t >(SectorEdge::North)];

						if ( stitch.indexCount > 0 )
						{
							outDrawCalls.push_back({stitch.indexOffset, stitch.indexCount});
						}
					}
				}

				/* Check West neighbor (sectorX - 1) */
				if ( sectorX > 0 )
				{
					const auto neighborIdx = sectorY * m_sectorCountPerAxis + (sectorX - 1);
					const auto neighborLOD = sectorLODs[neighborIdx];

					if ( neighborLOD > myLOD && myLOD < m_lodLevelCount - 1 )
					{
						const auto & stitch = sector.edgeStitching[myLOD][static_cast< size_t >(SectorEdge::West)];

						if ( stitch.indexCount > 0 )
						{
							outDrawCalls.push_back({stitch.indexOffset, stitch.indexCount});
						}
					}
				}
			}
		}
	}

	void
	AdaptiveVertexGridResource::prepareAdaptiveRendering (const Vector< 3, float > & viewPosition) const noexcept
	{
		/* Compute constrained LODs for all sectors. */
		this->computeAllSectorLODs(viewPosition, m_cachedSectorLODs);

		/* Compute stitching draw calls based on LOD differences. */
		this->getStitchingDrawCalls(m_cachedSectorLODs, m_cachedStitchingDrawCalls);
	}

	uint32_t
	AdaptiveVertexGridResource::getStitchingDrawCallCount () const noexcept
	{
		return static_cast< uint32_t >(m_cachedStitchingDrawCalls.size());
	}

	std::array< uint32_t, 2 >
	AdaptiveVertexGridResource::getStitchingDrawCallRange (uint32_t drawCallIndex) const noexcept
	{
		if ( drawCallIndex >= m_cachedStitchingDrawCalls.size() )
		{
			return {0, 0};
		}

		return m_cachedStitchingDrawCalls[drawCallIndex];
	}
}
