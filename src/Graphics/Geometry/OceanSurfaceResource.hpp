/*
 * src/Graphics/Geometry/OceanSurfaceResource.hpp
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

/* STL inclusions. */
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/OceanWaves.hpp"
#include "OceanSurface.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
	class CommandPool;
	class DescriptorPool;
	class DescriptorSet;
	class Sampler;
	class UniformBufferObject;
}

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief The knobs of an ocean surface.
	 */
	struct OceanSurfaceParameters
	{
		/** @brief The sea state (FFT spectrum, cascades, choppiness). */
		OceanWaveParameters waves{};
		/** @brief The sea level, in metres. */
		float seaLevel{0.0F};
		/** @brief Side of one patch quad at the finest level, in metres. */
		float finestCell{0.5F};
		/** @brief Distance (m) up to which the finest level is drawn; every next level doubles it. */
		float detailDistance{48.0F};
		/** @brief Where, in a level's distance ring, the geomorph toward the next level starts (Strugar: 0.66). */
		float morphStartRatio{0.66F};
		/** @brief Bound of the waves' reach from the sea level, in metres: the vertical extent of a node's box. */
		float waveReach{8.0F};
		/** @brief Texture repeats per metre (the material's UV). */
		float textureRepeatsPerMetre{0.125F};
		/** @brief Quads per side of the shared patch. Even. */
		uint32_t patchQuads{64};
		/** @brief Levels of detail (quadtree depth). */
		uint32_t levelCount{8};
		/** @brief Root nodes per side of the square around the camera. Even. */
		uint32_t rootsPerSide{4};
	};

	/**
	 * @brief An animated sea: the terrain's CDLOD on an INFINITE plane, displaced by FFT wave cascades.
	 * @note Engine item ocean-fft-surface, owner decisions 2026-09-28: FFT spectral waves (Graphics::OceanWaves), on a
	 * camera-following level-of-detail grid displaced in the VERTEX stage — one path for every device, MoltenVK included.
	 * The grid is a world-aligned quadtree (F. Strugar, "Continuous Distance-Dependent Level of Detail for Rendering
	 * Heightmaps", JGT 2009) selected around the camera from a square of root nodes: its nodes never move in the world, so
	 * nothing swims; one shared patch is drawn per node with the heightfield node + camera push constants, and the
	 * geomorph closes every level border. The vertex stage (AbstractVertexStage ocean branch) displaces it by the cascades.
	 * @note Flags EnableHeightfieldSurface + EnableOceanSurface: the program's PerModel set is this geometry's surface set
	 * (the heightfield layout: displacement, slopes, uniforms — Geometry::OceanSurface).
	 * @note The FFT runs every frame from updateSurfaceVideoMemory() (Renderer::registerSurfaceGeometry()), on the graphics
	 * queue ahead of the frame, like the terrain's clipmap.
	 */
	class EMEN_API OceanSurfaceResource final : public Interface
	{
		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OceanSurfaceResource"};

			/**
			 * @brief Constructs an ocean surface.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 */
			OceanSurfaceResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name) noexcept;

			/** @brief Destructs the ocean surface. */
			~OceanSurfaceResource () override;

			OceanSurfaceResource (const OceanSurfaceResource & copy) noexcept = delete;
			OceanSurfaceResource (OceanSurfaceResource && copy) noexcept = delete;
			OceanSurfaceResource & operator= (const OceanSurfaceResource & copy) noexcept = delete;
			OceanSurfaceResource & operator= (OceanSurfaceResource && copy) noexcept = delete;

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
			size_t classUID () const noexcept override;

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool is (size_t classUID) const noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char * classLabel () const noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/**
			 * @brief Loads the ocean from its parameters (the tables and the bounds; the GPU side comes with createOnHardware()).
			 * @param parameters The knobs.
			 * @return bool
			 */
			bool load (const OceanSurfaceParameters & parameters) noexcept;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const */
			[[nodiscard]]
			size_t memoryOccupied () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::isCreated() const */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::topology() const */
			[[nodiscard]]
			Topology topology () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryCount() const */
			[[nodiscard]]
			uint32_t subGeometryCount () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryRange() const */
			[[nodiscard]]
			std::array< uint32_t, 2 > subGeometryRange (uint32_t subGeometryIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > & boundingBox () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > & boundingSphere () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::vertexBufferObject() const */
			[[nodiscard]]
			const Vulkan::VertexBufferObject * vertexBufferObject () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::indexBufferObject() const */
			[[nodiscard]]
			const Vulkan::IndexBufferObject * indexBufferObject () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::useIndexBuffer() const */
			[[nodiscard]]
			bool useIndexBuffer () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::createOnHardware() */
			bool createOnHardware (Vulkan::TransferManager & transferManager) noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::updateVideoMemory() */
			bool updateVideoMemory () noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::destroyFromHardware() */
			void destroyFromHardware (bool clearLocalData) noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::surfaceDescriptorSet() const */
			[[nodiscard]]
			const Vulkan::DescriptorSet * surfaceDescriptorSet () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::updateSurfaceVideoMemory() */
			bool updateSurfaceVideoMemory (const Base::Math::Vector< 3, float > & lodViewPosition, uint32_t frameIndex) noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::isAdaptiveLOD() const */
			[[nodiscard]]
			bool isAdaptiveLOD () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::prepareAdaptiveRendering() const */
			void prepareAdaptiveRendering (const Base::Math::Vector< 3, float > & lodViewPosition, const Frustum * cullingFrustum, const Base::Math::CartesianFrame< float > * worldCoordinates) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallCount() const */
			[[nodiscard]]
			uint32_t getAdaptiveDrawCallCount () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallRange() const */
			[[nodiscard]]
			std::array< uint32_t, 2 > getAdaptiveDrawCallRange (uint32_t drawCallIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallConstants() const */
			[[nodiscard]]
			std::array< float, 4 > getAdaptiveDrawCallConstants (uint32_t drawCallIndex) const noexcept override;

			/**
			 * @brief Returns the knobs.
			 * @return const OceanSurfaceParameters &
			 */
			[[nodiscard]]
			const OceanSurfaceParameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/** @brief One selected node: its index range in the patch and its placement (origin x, z, size, level). */
			struct Selection
			{
				std::array< uint32_t, 2 > range{};
				std::array< float, 4 > node{};
			};

			/**
			 * @brief Selects the nodes of one node's subtree (Strugar, § 2.3), as CDLODTerrainResource::selectNode().
			 * @return bool True when this node's area is handled (drawn or culled), false when the parent must draw it.
			 */
			bool selectNode (uint32_t levelOfDetail, double originX, double originZ, const Base::Math::Vector< 3, float > & eye, const Frustum * frustum) const noexcept;

			/**
			 * @brief Creates the uniform buffer, the sampler, the descriptor sets and the command buffers.
			 * @return bool
			 */
			bool createSurfaceResources () noexcept;

			OceanSurfaceParameters m_parameters{};
			std::array< float, OceanSurface::MaxLevelsOfDetail > m_lodRanges{};
			std::array< std::array< float, 2 >, OceanSurface::MaxLevelsOfDetail > m_morphTable{};
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			std::unique_ptr< Vulkan::IndexBufferObject > m_indexBufferObject;
			std::unique_ptr< OceanWaves > m_waves;
			std::unique_ptr< Vulkan::UniformBufferObject > m_uniformBuffer;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::vector< std::shared_ptr< Vulkan::CommandBuffer > > m_commandBuffers;
			std::chrono::steady_clock::time_point m_startTime;
			VkDeviceSize m_uniformSectionSize{0};
			uint32_t m_patchIndexCount{0};
			uint32_t m_currentFrameIndex{0};
			/** @brief The previous FFT's time (the whitecaps decay over the step); negative before the first one. */
			double m_previousTime{-1.0};
			mutable std::vector< Selection > m_selection; ///< Render thread, one pass at a time.
			bool m_surfaceUpdated{false};
	};
}
