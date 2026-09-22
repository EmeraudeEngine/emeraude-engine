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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "AdaptiveVertexGridResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <utility>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "ThreadPool.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Base::PixelFactory;
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

		vertexAttributes = this->generateVertexAttributes(m_localData, vertexElementCount);

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

				/* The sector's own box: its footprint, and the Y range of ITS points. */
				sectorData.bounds = computeSectorBounds(m_localData, sectorQuadStartX, sectorQuadStartY, sectorQuadEndX, sectorQuadEndY);

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

	std::vector< uint32_t >
	AdaptiveVertexGridResource::generateTriangleListIndicesForRT () const noexcept
	{
		if ( !m_localData.isValid() )
		{
			return {};
		}

		/* NOTE: The IBO of this geometry CANNOT be converted as a whole. It holds, for every sector,
		 * one strip range PER LOD LEVEL describing the SAME quads, plus the edge-stitching ranges:
		 * converting all of it would stack every LOD of the terrain in the BLAS, one surface on top
		 * of another. A BLAS has no view-dependent LOD selection either, so the right content is one
		 * fixed proxy of the whole grid, regenerated here at a single step.
		 * The VBO holds one vertex per grid point, in grid point order (createOnHardware() walks
		 * [0, pointCount) through addVertexToBuffer()), so a grid index IS a VBO index. */
		const auto quadCount = m_localData.squaredQuadCount();

		if ( quadCount == 0 )
		{
			return {};
		}

		/* The full-resolution surface is quadratic in the division count and unbounded (a 4096-division
		 * grid is 33.5 M triangles, MEASURED at +2471 MiB of VRAM), so the step is the finest one whose
		 * triangle count fits the budget. A small grid keeps its exact surface. */
		auto & settings = this->serviceProvider().primaryServices().settings();
		const auto triangleBudget = std::max(2U, settings.getOrSetDefault< uint32_t >(GraphicsRayTracingTerrainBLASMaxTrianglesKey, DefaultGraphicsRayTracingTerrainBLASMaxTriangles));

		uint32_t step = 1;

		while ( step < quadCount )
		{
			const auto cellsPerAxis = (quadCount + step - 1) / step;

			if ( 2U * cellsPerAxis * cellsPerAxis <= triangleBudget )
			{
				break;
			}

			step *= 2;
		}

		const auto cellsPerAxis = (quadCount + step - 1) / step;

		TraceInfo{ClassId} <<
			"Generating the RT proxy of '" << this->name() << "': step " << step << " over " << quadCount <<
			" divisions, " << (2U * cellsPerAxis * cellsPerAxis) << " triangles (budget " << triangleBudget << ").";

		std::vector< uint32_t > triangleList;
		triangleList.reserve(static_cast< size_t >(cellsPerAxis) * cellsPerAxis * 6);

		for ( uint32_t quadY = 0; quadY < quadCount; quadY += step )
		{
			const auto nextY = std::min(quadY + step, quadCount);

			for ( uint32_t quadX = 0; quadX < quadCount; quadX += step )
			{
				const auto nextX = std::min(quadX + step, quadCount);

				const auto topLeft = m_localData.index(quadX, quadY);
				const auto topRight = m_localData.index(nextX, quadY);
				const auto bottomLeft = m_localData.index(quadX, nextY);
				const auto bottomRight = m_localData.index(nextX, nextY);

				/* Same winding as the rasterized strip: a row emits top/bottom pairs, so the two
				 * triangles of a quad come out as (T0, B0, T1) then (B0, B1, T1) once the strip
				 * alternation is resolved. */
				triangleList.emplace_back(topLeft);
				triangleList.emplace_back(bottomLeft);
				triangleList.emplace_back(topRight);

				triangleList.emplace_back(bottomLeft);
				triangleList.emplace_back(bottomRight);
				triangleList.emplace_back(topRight);
			}
		}

		return triangleList;
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
		/* Render thread, behind the frame fence: the ONE place the buffer draw calls read may change. */
		std::unique_ptr< VertexBufferObject > vertexBufferObject;
		Grid< float > localData;
		std::vector< Space3D::AACuboid< float > > sectorBounds;

		{
			const std::lock_guard< std::mutex > lock{m_pendingAccess};

			if ( !m_hasPendingUpdate )
			{
				return true;
			}

			vertexBufferObject = std::move(m_pendingVertexBufferObject);
			localData = std::move(m_pendingLocalData);
			sectorBounds = std::move(m_pendingSectorBounds);
			m_hasPendingUpdate = false;
		}

		if ( !this->isCreated() || vertexBufferObject == nullptr || sectorBounds.size() != m_sectorsData.size() )
		{
			TraceError{ClassId} << "The staged window of '" << this->name() << "' cannot be published: the geometry is not created or the window does not match it.";

			m_isUpdating.store(false, std::memory_order_release);

			return false;
		}

		/* The previous buffer may still be read by a frame in flight: retired, never freed here. */
		this->serviceProvider().graphicsRenderer().deferredDestructor().retireObject(std::move(m_vertexBufferObject));

		m_vertexBufferObject = std::move(vertexBufferObject);
		m_localData = std::move(localData);

		for ( size_t index = 0; index < m_sectorsData.size(); ++index )
		{
			m_sectorsData[index].bounds = sectorBounds[index];
		}

		m_isUpdating.store(false, std::memory_order_release);

		/* The BLAS holds the surface of the PREVIOUS window: the sub-grid just slid, so every vertex
		 * moved in XZ and its height changed with it. Until this is answered, the traced lane occludes
		 * against a terrain that is no longer where it is drawn — and nothing says so, neither an
		 * error nor a validation message.
		 * ⚠️ This MUST be the last statement: the flag is what publishes the new grid and the new VBO
		 * to the frame path that rebuilds (Scenes::SceneMetaData::rebuild(), which re-reads
		 * m_localData through generateTriangleListIndicesForRT()). A rebuild is the right answer
		 * rather than a refit because a slide is rare — visibleSize / 3 of travel — and a refit would
		 * freeze a BVH partition built for another window (owner decision, 2026-09-22). */
		this->markAccelerationStructureStale();

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

		{
			const std::lock_guard< std::mutex > lock{m_pendingAccess};

			m_pendingVertexBufferObject.reset();
			m_pendingLocalData.clear();
			m_pendingSectorBounds.clear();
			m_hasPendingUpdate = false;
		}

		m_isUpdating.store(false, std::memory_order_release);

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
	AdaptiveVertexGridResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by default!");

		return this->setLoadSuccess(false);
	}

	bool
	AdaptiveVertexGridResource::load (const Json::Value & /*data*/) noexcept
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

	void
	AdaptiveVertexGridResource::writeVertex (const Grid< float > & grid, uint32_t pointIndex, float * destination) const noexcept
	{
		const auto position = grid.position(pointIndex);
		auto * out = destination;

		/* Vertex position */
		*out++ = position[X];
		*out++ = position[Y];
		*out++ = position[Z];

		if ( this->isFlagEnabled(EnableTangentSpace) )
		{
			const auto normal = grid.normal(pointIndex, position);
			const auto tangent = grid.tangent(pointIndex, position, grid.textureCoordinates3D(pointIndex));
			const auto binormal = Vector< 3, float >::crossProduct(normal, tangent);

			/* Tangent */
			*out++ = tangent[X];
			*out++ = tangent[Y];
			*out++ = tangent[Z];

			/* Binormal */
			*out++ = binormal[X];
			*out++ = binormal[Y];
			*out++ = binormal[Z];

			/* Normal */
			*out++ = normal[X];
			*out++ = normal[Y];
			*out++ = normal[Z];
		}
		else if ( this->isFlagEnabled(EnableNormal) )
		{
			const auto normal = grid.normal(pointIndex, position);

			/* Normal */
			*out++ = normal[X];
			*out++ = normal[Y];
			*out++ = normal[Z];
		}

		if ( this->isFlagEnabled(EnablePrimaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DPrimaryTextureCoordinates) )
			{
				const auto UVWCoords = grid.textureCoordinates3D(pointIndex);

				/* 3D texture coordinates */
				*out++ = UVWCoords[X];
				*out++ = UVWCoords[Y];
				*out++ = UVWCoords[Z];
			}
			else
			{
				const auto UVCoords = grid.textureCoordinates2D(pointIndex);

				/* 2D texture coordinates */
				*out++ = UVCoords[X];
				*out++ = UVCoords[Y];
			}
		}

		/* FIXME: For now the secondary texture are the same as primary. */
		if ( this->isFlagEnabled(EnableSecondaryTextureCoordinates) )
		{
			if ( this->isFlagEnabled(Enable3DSecondaryTextureCoordinates) )
			{
				const auto UVWCoords = grid.textureCoordinates3D(pointIndex);

				/* 3D texture coordinates */
				*out++ = UVWCoords[X];
				*out++ = UVWCoords[Y];
				*out++ = UVWCoords[Z];
			}
			else
			{
				const auto UVCoords = grid.textureCoordinates2D(pointIndex);

				/* 2D texture coordinates */
				*out++ = UVCoords[X];
				*out++ = UVCoords[Y];
			}
		}

		/* Vertex color. */
		if ( this->isFlagEnabled(EnableVertexColor) )
		{
			switch ( m_vertexColorGenMode )
			{
				case VertexColorGenMode::UseGlobalColor :
					*out++ = m_globalVertexColor.red();
					*out++ = m_globalVertexColor.green();
					*out++ = m_globalVertexColor.blue();
					*out++ = 1.0F;
					break;

				case VertexColorGenMode::UseColorMap:
					// TODO
					break;

				case VertexColorGenMode::UseRandom :
				{
					const auto randomColor = Color< float >::quickRandom();

					*out++ = randomColor.red();
					*out++ = randomColor.green();
					*out++ = randomColor.blue();
					*out++ = 1.0F;
				}
					break;

				case VertexColorGenMode::GenerateFromCoords :
				{
					const auto UVCoords = grid.textureCoordinates2D(pointIndex);
					const auto level = 1.0F - ((position[Y] - grid.boundingBox().minimum(Y)) / grid.boundingBox().height());

					*out++ = UVCoords[X] / grid.UMultiplier();
					*out++ = UVCoords[Y] / grid.VMultiplier();
					*out++ = level;
				}
					break;
			}
		}

		/* Vertex weight. */
		if ( this->isFlagEnabled(EnableWeight) )
		{
			*out++ = 1.0F;
			*out++ = 1.0F;
			*out++ = 1.0F;
			*out++ = 1.0F;
		}

	}

	std::vector< float >
	AdaptiveVertexGridResource::generateVertexAttributes (const Grid< float > & grid, uint32_t vertexElementCount) const noexcept
	{
		const auto totalPoints = grid.pointCount();

		std::vector< float > vertexAttributes(static_cast< size_t >(totalPoints) * vertexElementCount);

		const auto writeRange = [this, &grid, &vertexAttributes, vertexElementCount] (uint32_t begin, uint32_t end) {
			for ( auto pointIndex = begin; pointIndex < end; ++pointIndex )
			{
				this->writeVertex(grid, pointIndex, &vertexAttributes[static_cast< size_t >(pointIndex) * vertexElementCount]);
			}
		};

		/* The random vertex colour draws from a generator that is not reentrant: one thread then. */
		const auto sequential = this->isFlagEnabled(EnableVertexColor) && m_vertexColorGenMode == VertexColorGenMode::UseRandom;
		const auto threadPool = sequential ? nullptr : this->serviceProvider().primaryServices().threadPool();

		if ( threadPool == nullptr )
		{
			writeRange(0U, totalPoints);
		}
		else
		{
			/* One grid row per grain: the neighbours a normal reads stay in cache. Safe from a pool
			 * worker — the calling thread takes its share and helpers that find no chunk exit. */
			threadPool->parallelFor(0U, totalPoints, writeRange, static_cast< size_t >(grid.squaredPointCount()));
		}

		return vertexAttributes;
	}

	bool
	AdaptiveVertexGridResource::updateData (Grid< float > grid) noexcept
	{
		/* Worker-thread side of a slide. NOTHING the render thread reads is written here: the new
		 * buffer is built beside the live one and parked, and the swap is updateVideoMemory()'s. */
		if ( !grid.isValid() )
		{
			Tracer::error(ClassId, "Cannot update: grid is invalid!");

			m_isUpdating.store(false, std::memory_order_release);

			return false;
		}

		if ( grid.pointCount() != m_localData.pointCount() )
		{
			TraceError{ClassId} <<
				"Cannot update: point count mismatch. Expected " << m_localData.pointCount() <<
				", got " << grid.pointCount() << ".";

			m_isUpdating.store(false, std::memory_order_release);

			return false;
		}

		const auto stagingStart = std::chrono::steady_clock::now();

		/* Sector boxes of the new window, so the render thread has nothing to compute at swap time. */
		auto sectorBounds = computeAllSectorBounds(grid, m_sectorCountPerAxis);

		/* Generate vertex attributes from the NEW grid, on every core the pool has. */
		const auto vertexElementCount = getElementCountFromFlags(this->flags());
		const auto totalPoints = grid.pointCount();

		auto vertexAttributes = this->generateVertexAttributes(grid, vertexElementCount);

		auto & renderer = this->serviceProvider().graphicsRenderer();
		auto & transferManager = renderer.transferManager();

		const auto generationEnd = std::chrono::steady_clock::now();

		/* Create and fill the new VBO. */
		auto newVBO = std::make_unique< VertexBufferObject >(transferManager.device(), totalPoints, vertexElementCount, false);
		newVBO->setIdentifier(ClassId, this->name(), "VertexBufferObject");

		if ( !newVBO->createOnHardware() || !newVBO->transferData(transferManager, vertexAttributes) )
		{
			Tracer::error(ClassId, "Failed to create new VBO!");

			m_isUpdating.store(false, std::memory_order_release);

			return false;
		}

		{
			using Milliseconds = std::chrono::duration< double, std::milli >;

			const auto uploadEnd = std::chrono::steady_clock::now();

			/* The two halves of a slide, measured: what the CPU spends rebuilding the window, and what
			 * the transfer costs. A window the camera outruns is a number here before it is a picture. */
			TraceInfo{ClassId} <<
				"Window of '" << this->name() << "' staged: " << totalPoints << " vertices generated in " <<
				Milliseconds{generationEnd - stagingStart}.count() << " ms, uploaded (" << (vertexAttributes.size() * sizeof(float) / 1048576) << " MiB) in " <<
				Milliseconds{uploadEnd - generationEnd}.count() << " ms.";
		}

		/* Stage it. A window staged while a previous one waits replaces it: the newest wins. */
		{
			const std::lock_guard< std::mutex > lock{m_pendingAccess};

			m_pendingVertexBufferObject = std::move(newVBO);
			m_pendingLocalData = std::move(grid);
			m_pendingSectorBounds = std::move(sectorBounds);
			m_hasPendingUpdate = true;
		}

		/* The render thread publishes it through updateVideoMemory(), before the frame's uploads. */
		renderer.requestGeometryVideoMemoryUpdate(std::static_pointer_cast< Interface >(this->weak_from_this().lock()));

		return true;
	}

	Space3D::AACuboid< float >
	AdaptiveVertexGridResource::computeSectorBounds (const Grid< float > & grid, uint32_t quadStartX, uint32_t quadStartY, uint32_t quadEndX, uint32_t quadEndY) noexcept
	{
		/* The XZ footprint from the corner points, the Y range from EVERY point of the sector. The
		 * level of detail is a 3D distance to this box; with the whole grid's Y range every sector
		 * stood as tall as the highest peak, and a camera 500 m above a valley measured 0 m to it. */
		const auto topLeft = grid.position(quadStartX, quadStartY);
		const auto bottomRight = grid.position(quadEndX, quadEndY);

		auto minimumY = std::numeric_limits< float >::max();
		auto maximumY = std::numeric_limits< float >::lowest();

		for ( auto pointY = quadStartY; pointY <= quadEndY; ++pointY )
		{
			for ( auto pointX = quadStartX; pointX <= quadEndX; ++pointX )
			{
				const auto height = grid.getHeightAt(pointX, pointY);

				minimumY = std::min(minimumY, height);
				maximumY = std::max(maximumY, height);
			}
		}

		return {{bottomRight[X], maximumY, bottomRight[Z]}, {topLeft[X], minimumY, topLeft[Z]}};
	}

	std::vector< Space3D::AACuboid< float > >
	AdaptiveVertexGridResource::computeAllSectorBounds (const Grid< float > & grid, uint32_t sectorCountPerAxis) noexcept
	{
		std::vector< Space3D::AACuboid< float > > bounds;
		bounds.reserve(static_cast< size_t >(sectorCountPerAxis) * sectorCountPerAxis);

		const auto quadsPerSector = grid.squaredQuadCount() / sectorCountPerAxis;

		for ( uint32_t sectorY = 0; sectorY < sectorCountPerAxis; ++sectorY )
		{
			for ( uint32_t sectorX = 0; sectorX < sectorCountPerAxis; ++sectorX )
			{
				const auto quadStartX = sectorX * quadsPerSector;
				const auto quadStartY = sectorY * quadsPerSector;

				bounds.emplace_back(computeSectorBounds(grid, quadStartX, quadStartY, quadStartX + quadsPerSector, quadStartY + quadsPerSector));
			}
		}

		return bounds;
	}

	uint32_t
	AdaptiveVertexGridResource::getAdaptiveDrawCallCount () const noexcept
	{
		return static_cast< uint32_t >(m_cachedDrawCalls.size());
	}

	std::array< uint32_t, 2 >
	AdaptiveVertexGridResource::getAdaptiveDrawCallRange (uint32_t drawCallIndex) const noexcept
	{
		if ( drawCallIndex >= m_cachedDrawCalls.size() )
		{
			return {0, 0};
		}

		return m_cachedDrawCalls[drawCallIndex];
	}

	uint32_t
	AdaptiveVertexGridResource::sectorLevelOfDetail (const Space3D::AACuboid< float > & bounds, float sectorSize, const Vector< 3, float > & viewPosition) const noexcept
	{
		if ( m_forcedLODLevel < m_lodLevelCount )
		{
			return m_forcedLODLevel;
		}

		/* The distance to the SURFACE of the sector, in three dimensions: 0 for the sector under the
		 * camera, the height above a valley floor for a camera in the air. Each level's range is
		 * twice the previous one's, so the on-screen triangle density stays about constant. */
		const auto distance = bounds.distanceTo(viewPosition);
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

	uint32_t
	AdaptiveVertexGridResource::getSectorLOD (uint32_t sectorIndex, const Vector< 3, float > & viewPosition) const noexcept
	{
		if ( sectorIndex >= m_sectorsData.size() )
		{
			return 0;
		}

		const auto & sector = m_sectorsData[sectorIndex];

		return this->sectorLevelOfDetail(sector.bounds, sector.bounds.width(), viewPosition);
	}

	void
	AdaptiveVertexGridResource::computeAllSectorLODs (const Vector< 3, float > & viewPosition, const std::vector< Space3D::AACuboid< float > > & bounds, std::vector< uint32_t > & outLODs) const noexcept
	{
		const auto sectorCount = static_cast< uint32_t >(m_sectorsData.size());
		outLODs.resize(sectorCount);

		/* First pass: the raw level of each sector, from ITS box (the sector's own width is the unit
		 * of the thresholds; a world box may be wider once rotated). */
		for ( uint32_t index = 0; index < sectorCount; ++index )
		{
			outLODs[index] = this->sectorLevelOfDetail(bounds[index], m_sectorsData[index].bounds.width(), viewPosition);
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
	AdaptiveVertexGridResource::computeAllSectorLODs (const Vector< 3, float > & viewPosition, std::vector< uint32_t > & outLODs) const noexcept
	{
		std::vector< Space3D::AACuboid< float > > bounds;
		bounds.reserve(m_sectorsData.size());

		for ( const auto & sector : m_sectorsData )
		{
			bounds.emplace_back(sector.bounds);
		}

		this->computeAllSectorLODs(viewPosition, bounds, outLODs);
	}

	void
	AdaptiveVertexGridResource::getStitchingDrawCalls (const std::vector< uint32_t > & sectorLODs, const std::vector< uint8_t > & sectorVisibility, std::vector< std::array< uint32_t, 2 > > & outDrawCalls) const noexcept
	{
		outDrawCalls.clear();

		for ( uint32_t sectorY = 0; sectorY < m_sectorCountPerAxis; ++sectorY )
		{
			for ( uint32_t sectorX = 0; sectorX < m_sectorCountPerAxis; ++sectorX )
			{
				const auto idx = sectorY * m_sectorCountPerAxis + sectorX;

				/* A culled sector's edges are not seen either: the stitching belongs to the finer side. */
				if ( idx < sectorVisibility.size() && sectorVisibility[idx] == 0 )
				{
					continue;
				}

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
	AdaptiveVertexGridResource::prepareAdaptiveRendering (const Vector< 3, float > & lodViewPosition, const Frustum * cullingFrustum, const CartesianFrame< float > * worldCoordinates) const noexcept
	{
		const auto sectorCount = static_cast< uint32_t >(m_sectorsData.size());

		/* 1. The sector boxes in WORLD space, where the camera and the frustum live. The scene ground
		 * sits on the root node, so this is the identity for it; any other frame gets the box of its
		 * eight transformed corners (conservative under a rotation, which only makes a level finer). */
		m_cachedWorldBounds.resize(sectorCount);

		if ( worldCoordinates == nullptr )
		{
			for ( uint32_t index = 0; index < sectorCount; ++index )
			{
				m_cachedWorldBounds[index] = m_sectorsData[index].bounds;
			}
		}
		else
		{
			const auto modelMatrix = worldCoordinates->getModelMatrix();

			for ( uint32_t index = 0; index < sectorCount; ++index )
			{
				auto & worldBounds = m_cachedWorldBounds[index];
				worldBounds.reset();

				for ( const auto & corner : m_sectorsData[index].bounds.points() )
				{
					worldBounds.merge(modelMatrix * Vector< 4, float >{corner, 1.0F});
				}
			}
		}

		/* 2. The levels, from the LOD camera, constrained to one step between neighbours. Every sector
		 * takes part — a culled sector still constrains the level of its visible neighbour, or the
		 * stitching on their shared edge would not match. */
		this->computeAllSectorLODs(lodViewPosition, m_cachedWorldBounds, m_cachedSectorLODs);

		/* 3. The visibility, from THIS pass's frustum. */
		m_cachedSectorVisibility.assign(sectorCount, 1);

		if ( cullingFrustum != nullptr )
		{
			for ( uint32_t index = 0; index < sectorCount; ++index )
			{
				m_cachedSectorVisibility[index] = cullingFrustum->isSeeing(m_cachedWorldBounds[index]) ? 1 : 0;
			}
		}

		/* 4. The draw list: one range per visible sector, at its level. */
		m_cachedDrawCalls.clear();

		for ( uint32_t index = 0; index < sectorCount; ++index )
		{
			if ( m_cachedSectorVisibility[index] == 0 )
			{
				continue;
			}

			const auto & drawCall = m_sectorsData[index].lodDrawCalls[m_cachedSectorLODs[index]];

			if ( drawCall.indexCount > 0 )
			{
				m_cachedDrawCalls.push_back({drawCall.indexOffset, drawCall.indexCount});
			}
		}

		/* 5. The stitching between levels, for the visible sectors. */
		this->getStitchingDrawCalls(m_cachedSectorLODs, m_cachedSectorVisibility, m_cachedStitchingDrawCalls);
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
