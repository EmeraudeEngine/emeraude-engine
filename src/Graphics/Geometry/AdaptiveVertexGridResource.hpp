/*
 * src/Graphics/Geometry/AdaptiveVertexGridResource.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Frustum.hpp"
#include "Graphics/ImageResource.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics::Geometry
{
	/** @brief Maximum number of LOD levels per sector. */
	static constexpr uint32_t MaxLODLevels{8};

	/**
	 * @brief Represents a draw call parameters for a sector at a specific LOD level.
	 */
	struct EMEN_API SectorDrawCall final
	{
		uint32_t indexOffset{0};
		uint32_t indexCount{0};
	};

	/**
	 * @brief Edge direction for stitching.
	 */
	enum class EMEN_API SectorEdge : uint8_t
	{
		North = 0, /**< Top edge (Z-) */
		South = 1, /**< Bottom edge (Z+) */
		West = 2,  /**< Left edge (X-) */
		East = 3   /**< Right edge (X+) */
	};

	/**
	 * @brief Contains all LOD draw calls for a single sector.
	 */
	struct EMEN_API SectorLODData final
	{
		uint32_t sectorX{0};
		uint32_t sectorY{0};
		/** @brief The sector's own box, in the geometry's space: its XZ footprint and the Y range of ITS points (not the whole grid's). */
		Base::Math::Space3D::AACuboid< float > bounds;
		std::array< SectorDrawCall, MaxLODLevels > lodDrawCalls{};
		/**
		 * @brief Stitching draw calls for edge transitions to neighbor with +1 LOD.
		 * Indexed by [myLOD][edge]. Only valid for myLOD < MaxLODLevels-1.
		 * Connects this sector's edge at myLOD to neighbor's edge at myLOD+1.
		 */
		std::array< std::array< SectorDrawCall, 4 >, MaxLODLevels > edgeStitching{};
		/**
		 * @brief Stitching draw calls from this sector's OUTWARD edges to the far mesh, indexed by [myLOD][edge].
		 * @note Filled for border sectors only, and only when a far mesh is set: a fan of triangles between
		 * the coarser of the two steps (this level's, or the far mesh's) and the finer, over window vertices
		 * alone — the far mesh's edge vertices coincide with the window's at its step. Empty when the steps match.
		 */
		std::array< std::array< SectorDrawCall, 4 >, MaxLODLevels > outerStitching{};
		/**
		 * @brief The SKIRT hanging from this border sector's outward edges down to the far mesh, indexed by edge.
		 * @note The far mesh sits `depthOffset` below the terrain (owner decision, 2026-09-22: the fine
		 * surface must win any depth contest near the camera). The skirt is a wall of quads between the
		 * window's edge vertices at the far step and their lowered copies (the skirt vertices, appended
		 * after the window's points): its bottom edge is exactly the far mesh's edge. Independent of the
		 * level: the fans above hand the edge over at the far step, the skirt takes it from there.
		 */
		std::array< SectorDrawCall, 4 > farSkirt{};
	};

	/**
	 * @brief One sector of the FAR mesh: the whole terrain at a coarse step, drawn around the window.
	 */
	struct EMEN_API FarSectorData final
	{
		/** @brief The sector's box, in the geometry's space, with the Y range of its own points. */
		Base::Math::Space3D::AACuboid< float > bounds;
		SectorDrawCall drawCall;
	};

	/**
	 * @brief Defines a geometry using a VBO and an IBO to produce a grid with LOD adapted from the point of view.
	 * @extends EmEn::Graphics::Geometry::Interface The common base for all geometry types.
	 *
	 * This class implements a section-based LOD system where:
	 * - The grid is divided into NxN sections (sectors)
	 * - Each section has multiple pre-computed LOD levels (one strip range per level in ONE index buffer)
	 * - LOD selection is the 3D distance from the LOD camera to the SURFACE of the sector's box
	 * - Triangle caps fill gaps between sections at different LOD levels
	 * - Frustum culling skips the sectors the pass does not see
	 *
	 * @note ⚠️ Because the index buffer holds every level of every sector, this geometry must NEVER be
	 * drawn whole: a plain `draw(geometry)` stacks all its levels on top of one another. Every pass
	 * goes through prepareAdaptiveRendering() + the selected ranges — the shadow pass included.
	 * @note A FAR MESH may surround the window (setFarGrid(), owner decision 2026-09-22): the whole terrain at
	 * a coarse step (32 m on `terrain`), cut in its own sectors, appended to the same VBO and IBO and drawn
	 * by the same selection — culled by the pass's frustum and by the HOLE the window occupies, which is
	 * exact because the window's centre snaps to the far sector size (TerrainResource). Its edge vertices
	 * coincide with the window's border vertices at the far step (same source grid, point samples, same
	 * texture coordinates), and the window's border sectors close the remaining crack with outer stitching
	 * fans. Without it the edge of the streamed window is 2048 m away at best — a picture on any terrain
	 * wider than the window.
	 * @note The sub-grid window slides as the camera travels (TerrainResource::updateVisibility()):
	 * a worker thread generates the new vertex data and stages it (updateData()), the RENDER thread
	 * publishes it (updateVideoMemory(), reached through Renderer::requestGeometryVideoMemoryUpdate())
	 * and retires the previous buffer through the DeferredDestructor. Nothing the render thread
	 * reads is ever written by another thread.
	 */
	class EMEN_API AdaptiveVertexGridResource final : public Interface
	{
		friend class Resources::Container< AdaptiveVertexGridResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AdaptiveVertexGridResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs an adaptive grid geometry resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The geometry resource flag bits. See EmEn::Graphics::Geometry::GeometryFlagBits. Default none.
			 */
			AdaptiveVertexGridResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: Interface{serviceProvider, name, resourceFlags}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource (const AdaptiveVertexGridResource & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource (AdaptiveVertexGridResource && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource & operator= (const AdaptiveVertexGridResource & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			AdaptiveVertexGridResource & operator= (AdaptiveVertexGridResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the adaptive grid geometry resource.
			 */
			~AdaptiveVertexGridResource () override
			{
				this->destroyFromHardware(true);
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				if ( m_vertexBufferObject == nullptr || !m_vertexBufferObject->isCreated() )
				{
					return false;
				}

				if ( m_indexBufferObject == nullptr || !m_indexBufferObject->isCreated() )
				{
					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::topology() */
			[[nodiscard]]
			Topology
			topology () const noexcept override
			{
				return Topology::TriangleStrip;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryCount() */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryRange(uint32_t) const */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			subGeometryRange (uint32_t /*subGeometryIndex*/) const noexcept override
			{
				/* FIXME: Incorrect. */
				return {0, m_indexBufferObject->indexCount()};
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingBox() */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_localData.boundingBox();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_localData.boundingSphere();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::vertexBufferObject() */
			[[nodiscard]]
			const Vulkan::VertexBufferObject *
			vertexBufferObject () const noexcept override
			{
				return m_vertexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::indexBufferObject() */
			[[nodiscard]]
			const Vulkan::IndexBufferObject *
			indexBufferObject () const noexcept override
			{
				return m_indexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::useIndexBuffer() */
			[[nodiscard]]
			bool
			useIndexBuffer () const noexcept override
			{
				if constexpr ( IsDebug )
				{
					return m_indexBufferObject != nullptr;
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::generateTriangleListIndicesForRT() */
			[[nodiscard]]
			std::vector< uint32_t > generateTriangleListIndicesForRT () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::createOnHardware() noexcept */
			bool createOnHardware (Vulkan::TransferManager & transferManager) noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::updateVideoMemory() noexcept */
			bool updateVideoMemory () noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::destroyFromHardware(bool) noexcept */
			void destroyFromHardware (bool clearLocalData) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				size_t bytes = (m_localData.pointCount() + m_farGrid.pointCount()) * sizeof(float) + m_farVertexAttributes.size() * sizeof(float);

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

			/**
			 * @brief Enables vertex color from a global color.
			 * @note Should be called before the load() function.
			 * @param color A reference to a color.
			 * @return void.
			 */
			void enableVertexColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Enables vertex color from a color map.
			 * @note Should be called before the load() function.
			 * @param colorMap A reference to an image resource.
			 * @return void.
			 */
			void enableVertexColor (const std::shared_ptr< ImageResource > & colorMap) noexcept;

			/**
			 * @brief Enables vertex color using randomization.
			 * @note Should be called before the load() function.
			 * @todo Set parameters to clamp color.
			 * @return void.
			 */
			void enableVertexColorRandom () noexcept;

			/**
			 * @brief Enables vertex color using coordinates.
			 * @note Should be called before the load() function.
			 * @todo Set parameters for color generation.
			 * @return void.
			 */
			void enableVertexColorFromCoords () noexcept;

			/**
			 * @brief This loads a geometry from a parametric object.
			 * @note This only local data and not pushing it to the video RAM.
			 * @param grid A reference to a geometry from vertex factory library.
			 * @param sectorCountPerAxis The number of sectors per axis (e.g., 4 means 4x4 = 16 sectors). Default 4.
			 * @return bool
			 */
			bool load (const Base::VertexFactory::Grid< float > & grid, uint32_t sectorCountPerAxis = 8) noexcept;

			/**
			 * @brief Sets the far mesh: the whole terrain at a coarse step, drawn around the window.
			 * @note Must be called before load(). The far grid is validated against the window there: its cell
			 * must be a power-of-two multiple of the window's cell that divides the window's sector edge, so
			 * that its edge vertices coincide with the window's at that step. Built from
			 * Grid::coarsened() of the FULL grid — point samples, never an average.
			 * @param farGrid The coarse grid of the whole terrain.
			 * @param farSectorCountPerAxis How many sectors per axis the far mesh is cut in (its divisions must be a multiple).
			 * @param depthOffset How far BELOW the terrain the far mesh sits, in metres (owner decision: the fine
			 * surface wins near the camera; a skirt closes the resulting step at the window's edge). 0 = none.
			 * @return bool
			 */
			bool setFarGrid (const Base::VertexFactory::Grid< float > & farGrid, uint32_t farSectorCountPerAxis, float depthOffset) noexcept;

			/**
			 * @brief Returns whether a far mesh surrounds the window.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasFarGrid () const noexcept
			{
				return m_farStep > 0;
			}

			/**
			 * @brief Returns how many window cells make one far mesh cell (0 without a far mesh).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			farStep () const noexcept
			{
				return m_farStep;
			}

			/**
			 * @brief Returns the far mesh sectors.
			 * @return const std::vector< FarSectorData > &
			 */
			[[nodiscard]]
			const std::vector< FarSectorData > &
			farSectorsData () const noexcept
			{
				return m_farSectorsData;
			}

			/**
			 * @brief Returns the number of sectors per axis.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			sectorCountPerAxis () const noexcept
			{
				return m_sectorCountPerAxis;
			}

			/**
			 * @brief Returns the total number of sectors.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			sectorCount () const noexcept
			{
				return m_sectorCountPerAxis * m_sectorCountPerAxis;
			}

			/**
			 * @brief Returns the number of LOD levels per sector.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			lodLevelCount () const noexcept
			{
				return m_lodLevelCount;
			}

			/**
			 * @brief Returns the sectors LOD data for rendering.
			 * @return const std::vector< SectorLODData > &
			 */
			[[nodiscard]]
			const std::vector< SectorLODData > &
			sectorsData () const noexcept
			{
				return m_sectorsData;
			}

			/**
			 * @brief Forces a specific LOD level to be generated (debug option).
			 * @note Must be called before createOnHardware(). Use NoForcedLOD to disable.
			 * @param level The LOD level to force (0 = highest quality), or NoForcedLOD to generate all levels.
			 * @return void
			 */
			void
			forceLOD (uint32_t level) noexcept
			{
				m_forcedLODLevel = level;
			}

			/** @brief Value indicating no forced LOD level. */
			static constexpr uint32_t NoForcedLOD{std::numeric_limits< uint32_t >::max()};

			/**
			 * @brief Sets the LOD distance parameters.
			 * @param baseMultiplier The base distance threshold as a fraction of sector size (default 0.125).
			 *		Higher values extend the high-detail zone.
			 * @param thresholdGrowth The multiplier applied to threshold between LOD levels (default 2.0).
			 *		Higher values make LOD transitions more gradual.
			 * @return void
			 */
			void
			setLODDistanceParameters (float baseMultiplier, float thresholdGrowth) noexcept
			{
				m_lodBaseMultiplier = baseMultiplier;
				m_lodThresholdGrowth = thresholdGrowth;
			}

			/**
			 * @brief Returns the LOD base distance multiplier.
			 * @return float
			 */
			[[nodiscard]]
			float
			lodBaseMultiplier () const noexcept
			{
				return m_lodBaseMultiplier;
			}

			/**
			 * @brief Returns the LOD threshold growth factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			lodThresholdGrowth () const noexcept
			{
				return m_lodThresholdGrowth;
			}

			/**
			 * @brief Claims the single update slot before generating a new window.
			 * @note Called on the thread that decides a slide (the logic thread), BEFORE the work is
			 * handed to a worker: it is what stops the next cycle from queuing a second one while the
			 * first has not started. Released by updateVideoMemory() once the window is published, or
			 * by cancelUpdate() / a failed updateData().
			 * @return bool False when an update is already in flight.
			 */
			[[nodiscard]]
			bool
			beginUpdate () noexcept
			{
				return !m_isUpdating.exchange(true, std::memory_order_acq_rel);
			}

			/**
			 * @brief Releases the update slot claimed by beginUpdate() when the work could not be handed out.
			 * @return void
			 */
			void
			cancelUpdate () noexcept
			{
				m_isUpdating.store(false, std::memory_order_release);
			}

			/**
			 * @brief Generates the vertex data of a new grid window and STAGES it for the render thread.
			 * @note Worker-thread side of a slide. The grid must have the same point count as the current
			 * local data. Everything the render thread reads (the VBO, the local data, the sector bounds)
			 * stays untouched here: the new buffer is created and filled, then parked with the new grid
			 * and this geometry registers itself with Renderer::requestGeometryVideoMemoryUpdate(). The
			 * swap happens in updateVideoMemory(), on the render thread, behind the frame fence.
			 * @param grid The new grid data [std::move].
			 * @return bool True if the data was staged.
			 */
			bool updateData (Base::VertexFactory::Grid< float > grid) noexcept;

			/**
			 * @brief Checks if an update is currently in progress.
			 * @return bool True if updating.
			 */
			[[nodiscard]]
			bool
			isUpdating () const noexcept
			{
				return m_isUpdating.load(std::memory_order_acquire);
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::isAdaptiveLOD() */
			[[nodiscard]]
			bool
			isAdaptiveLOD () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::prepareAdaptiveRendering() */
			void prepareAdaptiveRendering (const Base::Math::Vector< 3, float > & lodViewPosition, const Frustum * cullingFrustum, const Base::Math::CartesianFrame< float > * worldCoordinates) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallCount() */
			[[nodiscard]]
			uint32_t getAdaptiveDrawCallCount () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallRange() */
			[[nodiscard]]
			std::array< uint32_t, 2 > getAdaptiveDrawCallRange (uint32_t drawCallIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getStitchingDrawCallCount() */
			[[nodiscard]]
			uint32_t getStitchingDrawCallCount () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getStitchingDrawCallRange() */
			[[nodiscard]]
			std::array< uint32_t, 2 > getStitchingDrawCallRange (uint32_t drawCallIndex) const noexcept override;

			/**
			 * @brief Computes the LOD level for a specific sector based on the view distance to the sector's box.
			 * @note ⚠️ The distance is to the SURFACE of the sector's box, never to its centre: a 512 m
			 * sector seen from its own edge is 0 m away. Measured against the centre, the sector under
			 * the camera sat at 362 m and never left step 8, so the finest three levels of a 1 m grid
			 * were carried in the VBO and never drawn (2026-09-22).
			 * @param sectorIndex The sector index.
			 * @param viewPosition The view/camera position, in the geometry's space.
			 * @return uint32_t The LOD level (0 = highest detail).
			 */
			[[nodiscard]]
			uint32_t getSectorLOD (uint32_t sectorIndex, const Base::Math::Vector< 3, float > & viewPosition) const noexcept;

			/**
			 * @brief Computes LOD levels for all sectors.
			 * @param viewPosition The view/camera position.
			 * @param outLODs Output vector to store LOD levels (resized to sector count).
			 * @return void
			 */
			void computeAllSectorLODs (const Base::Math::Vector< 3, float > & viewPosition, std::vector< uint32_t > & outLODs) const noexcept;

			/**
			 * @brief Gets stitching draw calls for current LOD configuration.
			 * @param sectorLODs The LOD level of each sector.
			 * @param sectorVisibility One flag per sector; a culled sector emits no stitching. Empty = every sector visible.
			 * @param outDrawCalls Output vector of [indexOffset, indexCount] pairs.
			 * @return void
			 */
			void getStitchingDrawCalls (const std::vector< uint32_t > & sectorLODs, const std::vector< uint8_t > & sectorVisibility, std::vector< std::array< uint32_t, 2 > > & outDrawCalls) const noexcept;

		private:

			/**
			 * @brief Computes the box of one sector: its XZ footprint and the Y range of its own points.
			 * @param grid The grid the sector belongs to.
			 * @param quadStartX First quad column of the sector.
			 * @param quadStartY First quad row of the sector.
			 * @param quadEndX One past the last quad column (the sector's last point column).
			 * @param quadEndY One past the last quad row (the sector's last point row).
			 * @return Base::Math::Space3D::AACuboid< float >
			 */
			[[nodiscard]]
			static Base::Math::Space3D::AACuboid< float > computeSectorBounds (const Base::VertexFactory::Grid< float > & grid, uint32_t quadStartX, uint32_t quadStartY, uint32_t quadEndX, uint32_t quadEndY) noexcept;

			/**
			 * @brief Computes the boxes of every sector of a grid, in sector order.
			 * @param grid The grid.
			 * @param sectorCountPerAxis The sector count per axis.
			 * @return std::vector< Base::Math::Space3D::AACuboid< float > >
			 */
			[[nodiscard]]
			static std::vector< Base::Math::Space3D::AACuboid< float > > computeAllSectorBounds (const Base::VertexFactory::Grid< float > & grid, uint32_t sectorCountPerAxis) noexcept;

			/**
			 * @brief Picks the level of a sector from the distance between a view position and the sector's box.
			 * @param bounds The sector's box, in the same space as the view position.
			 * @param sectorSize The sector's edge length, the unit of the LOD thresholds.
			 * @param viewPosition The view position.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t sectorLevelOfDetail (const Base::Math::Space3D::AACuboid< float > & bounds, float sectorSize, const Base::Math::Vector< 3, float > & viewPosition) const noexcept;

			/**
			 * @brief Computes the constrained levels of every sector against a set of boxes.
			 * @param viewPosition The view position, in the boxes' space.
			 * @param bounds One box per sector, in sector order.
			 * @param outLODs Output vector (resized to the sector count).
			 * @return void
			 */
			void computeAllSectorLODs (const Base::Math::Vector< 3, float > & viewPosition, const std::vector< Base::Math::Space3D::AACuboid< float > > & bounds, std::vector< uint32_t > & outLODs) const noexcept;

			/**
			 * @brief Prepares data vector to go on GPU.
			 * @param vertexAttributes A writable reference to a vector of vertex attributes.
			 * @param vertexElementCount The number of elements which compose one vertex.
			 * @param indices A writable reference to a vector of indices.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateGPUBuffers (std::vector< float > & vertexAttributes, uint32_t vertexElementCount, std::vector< uint32_t > & indices) noexcept;

			/**
			 * @brief Writes one vertex of a grid at a destination: exactly the element count of the flags.
			 * @note Thread-safe for concurrent points of one grid (it reads the grid and this geometry's
			 * flags, writes only its own slot), which is what lets generateVertexAttributes() fan out.
			 * @param grid The grid the point is read from (the local data, or a window being staged).
			 * @param pointIndex The point index in the grid.
			 * @param destination Where the vertex's elements go.
			 * @return void
			 */
			void writeVertex (const Base::VertexFactory::Grid< float > & grid, uint32_t pointIndex, float * destination, float heightShift = 0.0F) const noexcept;

			/**
			 * @brief Generates the SKIRT vertices of a window: its four edges at the far step, lowered by the far mesh's depth offset.
			 * @note Layout: North edge (y = 0), South (y = last), West (x = 0), East (x = last), each `cells / farStep + 1`
			 * vertices in increasing order along the edge — skirtVertexIndex() addresses them. Empty without a far mesh.
			 * @param grid The window grid.
			 * @param vertexElementCount The number of elements which compose one vertex.
			 * @return std::vector< float >
			 */
			[[nodiscard]]
			std::vector< float > generateSkirtAttributes (const Base::VertexFactory::Grid< float > & grid, uint32_t vertexElementCount) const noexcept;

			/**
			 * @brief Returns the VBO index of a skirt vertex.
			 * @param edge The window edge.
			 * @param along The position along the edge, in far steps (0 to cells / farStep).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			skirtVertexIndex (SectorEdge edge, uint32_t along) const noexcept
			{
				return m_skirtVertexOffset + (static_cast< uint32_t >(edge) * m_skirtVerticesPerEdge) + along;
			}

			/**
			 * @brief Generates the vertex attributes of a whole grid, one vertex per point in point order.
			 * @note Fans out on the engine ThreadPool (`parallelFor`, safe from a pool worker — the slide
			 * runs on one): 16.8 M vertices took 1.29 s on one thread. Falls back to one thread when the
			 * random vertex colour is on (its generator is not reentrant).
			 * @param grid The grid.
			 * @param vertexElementCount The number of elements which compose one vertex.
			 * @return std::vector< float >
			 */
			[[nodiscard]]
			std::vector< float > generateVertexAttributes (const Base::VertexFactory::Grid< float > & grid, uint32_t vertexElementCount) const noexcept;

			/* Vulkan buffers. */
			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			std::unique_ptr< Vulkan::IndexBufferObject > m_indexBufferObject;
			/* Local data. */
			Base::VertexFactory::Grid< float > m_localData;
			/* Sector and LOD configuration. */
			uint32_t m_sectorCountPerAxis{4};
			uint32_t m_lodLevelCount{0};
			std::vector< SectorLODData > m_sectorsData;
			/* The far mesh: the whole terrain at a coarse step, static, appended after the window's vertices. */
			Base::VertexFactory::Grid< float > m_farGrid;
			std::vector< FarSectorData > m_farSectorsData;
			std::vector< float > m_farVertexAttributes; ///< Generated once at load, copied into every staged window (the far mesh never moves).
			uint32_t m_farSectorCountPerAxis{0};
			uint32_t m_farStep{0}; ///< Window cells per far cell; 0 = no far mesh.
			uint32_t m_skirtVertexOffset{0}; ///< First skirt vertex in the VBO (= the window's point count).
			uint32_t m_skirtVerticesPerEdge{0}; ///< cells / farStep + 1.
			uint32_t m_farVertexOffset{0}; ///< First far vertex in the VBO (after the skirt).
			float m_farDepthOffset{0.0F}; ///< How far below the terrain the far mesh sits, in metres.
			/* VBO generation options. */
			VertexColorGenMode m_vertexColorGenMode{VertexColorGenMode::UseRandom};
			Base::PixelFactory::Color< float > m_globalVertexColor;
			std::shared_ptr< ImageResource > m_vertexColorMap;
			/* Debug options. */
			uint32_t m_forcedLODLevel{NoForcedLOD};
			/* LOD distance configuration. */
			float m_lodBaseMultiplier{0.125F};
			float m_lodThresholdGrowth{2.0F};
			/* A window slide in flight: claimed by beginUpdate(), released once published. */
			std::atomic< bool > m_isUpdating{false};
			/* The staged window (written by a worker in updateData(), consumed by updateVideoMemory() on the render thread). */
			std::mutex m_pendingAccess;
			std::unique_ptr< Vulkan::VertexBufferObject > m_pendingVertexBufferObject;
			Base::VertexFactory::Grid< float > m_pendingLocalData;
			std::vector< Base::Math::Space3D::AACuboid< float > > m_pendingSectorBounds;
			bool m_hasPendingUpdate{false};
			/* The selection of the current pass (render thread only, rebuilt by prepareAdaptiveRendering()). */
			mutable std::vector< Base::Math::Space3D::AACuboid< float > > m_cachedWorldBounds;
			mutable std::vector< Base::Math::Space3D::AACuboid< float > > m_cachedFarWorldBounds;
			mutable std::vector< uint32_t > m_cachedSectorLODs;
			mutable std::vector< uint8_t > m_cachedSectorVisibility;
			mutable std::vector< std::array< uint32_t, 2 > > m_cachedDrawCalls;
			mutable std::vector< std::array< uint32_t, 2 > > m_cachedStitchingDrawCalls;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using AdaptiveVertexGridGeometries = Container< Graphics::Geometry::AdaptiveVertexGridResource >;
}
