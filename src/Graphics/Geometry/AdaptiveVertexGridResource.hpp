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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <array>
#include <atomic>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Graphics/Frustum.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Graphics::Geometry
{
	/** @brief Maximum number of LOD levels per sector. */
	static constexpr uint32_t MaxLODLevels{8};

	/**
	 * @brief Represents a draw call parameters for a sector at a specific LOD level.
	 */
	struct SectorDrawCall final
	{
		uint32_t indexOffset{0};
		uint32_t indexCount{0};
	};

	/**
	 * @brief Edge direction for stitching.
	 */
	enum class SectorEdge : uint8_t
	{
		North = 0, /**< Top edge (Z-) */
		South = 1, /**< Bottom edge (Z+) */
		West = 2,  /**< Left edge (X-) */
		East = 3   /**< Right edge (X+) */
	};

	/**
	 * @brief Contains all LOD draw calls for a single sector.
	 */
	struct SectorLODData final
	{
		uint32_t sectorX{0};
		uint32_t sectorY{0};
		Libs::Math::Space3D::AACuboid< float > bounds{};
		std::array< SectorDrawCall, MaxLODLevels > lodDrawCalls{};
		/**
		 * @brief Stitching draw calls for edge transitions to neighbor with +1 LOD.
		 * Indexed by [myLOD][edge]. Only valid for myLOD < MaxLODLevels-1.
		 * Connects this sector's edge at myLOD to neighbor's edge at myLOD+1.
		 */
		std::array< std::array< SectorDrawCall, 4 >, MaxLODLevels > edgeStitching{};
	};

	/**
	 * @brief Defines a geometry using a VBO and an IBO to produce a grid with LOD adapted from the point of view.
	 * @extends EmEn::Graphics::Geometry::Interface The common base for all geometry types.
	 *
	 * This class implements a section-based LOD system where:
	 * - The grid is divided into NxN sections
	 * - Each section has multiple pre-computed LOD levels
	 * - LOD selection is based on distance from camera
	 * - Triangle caps fill gaps between sections at different LOD levels
	 * - Frustum culling skips invisible sections
	 */
	class AdaptiveVertexGridResource final : public Interface
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
			 * @param name A reference to a string for the resource name.
			 * @param geometryFlagBits The geometry resource flag bits, See EmEn::Graphics::Geometry::GeometryFlagBits. Default EnablePrimitiveRestart.
			 */
			explicit
			AdaptiveVertexGridResource (const std::string & name, uint32_t geometryFlagBits = EnablePrimitiveRestart) noexcept
				: Interface{name, geometryFlagBits}
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
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
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
			const Libs::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_localData.boundingBox();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
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

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				// TODO ...
				return 0;
			}

			/**
			 * @brief Enables vertex color from a global color.
			 * @note Should be called before the load() function.
			 * @param color A reference to a color.
			 * @return void.
			 */
			void enableVertexColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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
			bool load (const Libs::VertexFactory::Grid< float > & grid, uint32_t sectorCountPerAxis = 8) noexcept;

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
			 * @brief Updates the geometry with new grid data.
			 * @note The grid must have the same point count as the current local data.
			 * @param grid A reference to the new grid data.
			 * @return bool True if update succeeded.
			 */
			bool updateData (const Libs::VertexFactory::Grid< float > & grid) noexcept;

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

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallCount() */
			[[nodiscard]]
			uint32_t getAdaptiveDrawCallCount (const Libs::Math::Vector< 3, float > & viewPosition) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallRange() */
			[[nodiscard]]
			std::array< uint32_t, 2 > getAdaptiveDrawCallRange (uint32_t drawCallIndex, const Libs::Math::Vector< 3, float > & viewPosition) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::prepareAdaptiveRendering() */
			void prepareAdaptiveRendering (const Libs::Math::Vector< 3, float > & viewPosition) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getStitchingDrawCallCount() */
			[[nodiscard]]
			uint32_t getStitchingDrawCallCount () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getStitchingDrawCallRange() */
			[[nodiscard]]
			std::array< uint32_t, 2 > getStitchingDrawCallRange (uint32_t drawCallIndex) const noexcept override;

			/**
			 * @brief Computes the LOD level for a specific sector based on view distance.
			 * @param sectorIndex The sector index.
			 * @param viewPosition The view/camera position.
			 * @return uint32_t The LOD level (0 = highest detail).
			 */
			[[nodiscard]]
			uint32_t getSectorLOD (uint32_t sectorIndex, const Libs::Math::Vector< 3, float > & viewPosition) const noexcept;

			/**
			 * @brief Computes LOD levels for all sectors.
			 * @param viewPosition The view/camera position.
			 * @param outLODs Output vector to store LOD levels (resized to sector count).
			 * @return void
			 */
			void computeAllSectorLODs (const Libs::Math::Vector< 3, float > & viewPosition, std::vector< uint32_t > & outLODs) const noexcept;

			/**
			 * @brief Gets stitching draw calls for current LOD configuration.
			 * @param sectorLODs The LOD level of each sector.
			 * @param outDrawCalls Output vector of [indexOffset, indexCount] pairs.
			 * @return void
			 */
			void getStitchingDrawCalls (const std::vector< uint32_t > & sectorLODs, std::vector< std::array< uint32_t, 2 > > & outDrawCalls) const noexcept;

		private:

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
			 * @brief Adds a vertex to the local buffer.
			 * @param pointIndex The point index in the local grid.
			 * @param vertexAttributes A writable reference to a vector of vertex attributes.
			 * @param vertexElementCount The number of elements which compose one vertex.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t addVertexToBuffer (uint32_t pointIndex, std::vector< float > & vertexAttributes, uint32_t vertexElementCount) const noexcept;

			/* Vulkan buffers. */
			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			std::unique_ptr< Vulkan::IndexBufferObject > m_indexBufferObject;
			/* Local data. */
			Libs::VertexFactory::Grid< float > m_localData;
			/* Sector and LOD configuration. */
			uint32_t m_sectorCountPerAxis{4};
			uint32_t m_lodLevelCount{0};
			std::vector< SectorLODData > m_sectorsData{};
			/* VBO generation options. */
			VertexColorGenMode m_vertexColorGenMode{VertexColorGenMode::UseRandom};
			Libs::PixelFactory::Color< float > m_globalVertexColor;
			std::shared_ptr< ImageResource > m_vertexColorMap{};
			/* Debug options. */
			uint32_t m_forcedLODLevel{NoForcedLOD};
			/* LOD distance configuration. */
			float m_lodBaseMultiplier{0.125F};
			float m_lodThresholdGrowth{2.0F};
			/* Thread safety for VBO updates. */
			std::atomic< bool > m_isUpdating{false};
			/* Deferred VBO destruction (to avoid Vulkan use-after-free). */
			std::unique_ptr< Vulkan::VertexBufferObject > m_pendingDestructionVBO;
			/* Cached stitching data (updated by prepareAdaptiveRendering()). */
			mutable std::vector< uint32_t > m_cachedSectorLODs{};
			mutable std::vector< std::array< uint32_t, 2 > > m_cachedStitchingDrawCalls{};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using AdaptiveVertexGridGeometries = Container< Graphics::Geometry::AdaptiveVertexGridResource >;
}
