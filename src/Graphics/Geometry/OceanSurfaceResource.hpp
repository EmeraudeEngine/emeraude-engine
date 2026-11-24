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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/OceanWaves.hpp"
#include "Graphics/SeaSurfaceWaves.hpp"
#include "Hash/FNV1a.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Sphere.hpp"
#include "Math/Vector.hpp"
#include "Mutex.hpp"
#include "OceanSurface.hpp"
#include "Resources/ResourceTrait.hpp"
#include "ThreadSafety.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Frustum;
	struct SeaShelterMap;
}

namespace EmEn::Vulkan
{
	class Buffer;
	class CommandBuffer;
	class CommandPool;
	class DescriptorPool;
	class DescriptorSet;
	class Image;
	class ImageView;
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
		/** @brief The narrowest passage the swell enters, in metres: a basin joined to the open sea by a narrower one
		 * stays sheltered (owner decision 2026-11-24: 30 m to start with). */
		float shelterMinimumPassage{30.0F};
		/** @brief The distance, in metres, over which the sea calms down once it has gone as far as it can. */
		float shelterFade{25.0F};
		/** @brief The share of each cascade's waves a sheltered water keeps, largest cascade first: the swell goes, the
		 * smallest waves stay, much attenuated (owner decision 2026-11-24). ⚠️ First values: the look is the owner's
		 * to tune on screen. */
		std::array< float, OceanWaves::CascadeCount > shelteredWaveShares{0.0F, 0.04F, 0.35F};
		/** @brief Quads per side of the shared patch. Even. */
		uint32_t patchQuads{64};
		/** @brief Levels of detail (quadtree depth). */
		uint32_t levelCount{8};
		/** @brief Root nodes per side of the square around the camera. Even. */
		uint32_t rootsPerSide{4};
		/** @brief Texels per side of the shelter map (one byte each): the scene's square is cut into that many cells. */
		uint32_t shelterMapSide{1024};
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

			/**
			 * @brief Gives the surface its shelter map: where the land shelters the water, the waves are weighed down
			 * to their sheltered shares (OceanSurfaceParameters::shelteredWaveShares).
			 * @note ANY THREAD: the map is taken by the render thread at the next surface update, which uploads it
			 * ahead of the frame. It is kept, and uploaded again after the surface's GPU resources are created anew.
			 * @param map The map, of OceanSurfaceParameters::shelterMapSide texels a side; null for none (the open
			 * sea everywhere, the default). A map of another size, or of a square without extent, is refused with an
			 * error and the previous one kept.
			 */
			void setShelterMap (std::shared_ptr< const SeaShelterMap > map) noexcept EMEN_EXCLUDES(m_shelterAccess);

			/**
			 * @brief Returns what a pass binds to know where the drawn surface stands (SeaSurfaceWaves), or nullptr
			 * until the surface has been updated once: its cascades hold nothing before.
			 * @note ⚠️ RENDER THREAD, valid for the frame it is asked in.
			 * @return const SeaSurfaceWaves *
			 */
			[[nodiscard]]
			const SeaSurfaceWaves *
			surfaceWaves () const noexcept
			{
				return m_surfaceUpdated ? &m_surfaceWaves : nullptr;
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

			/**
			 * @brief Creates the shelter map's image, view and sampler.
			 * @return bool
			 */
			[[nodiscard]]
			bool createShelterResources () noexcept;

			/**
			 * @brief Writes a shelter map — or the open sea everywhere — into a frame's staging buffer, and takes its
			 * rectangle for the uniforms.
			 * @param frameIndex The frame in flight.
			 * @param map The map, null for none.
			 * @return bool
			 */
			[[nodiscard]]
			bool stageShelterMap (uint32_t frameIndex, const SeaShelterMap * map) noexcept;

			/**
			 * @brief Records the copy of a frame's staging buffer into the shelter image, which ends ready to be read.
			 * @param commandBuffer The frame's command buffer, begun.
			 * @param frameIndex The frame in flight.
			 */
			void recordShelterUpload (const Vulkan::CommandBuffer & commandBuffer, uint32_t frameIndex) const noexcept;

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
			/** @brief What the underwater view binds, filled with the surface's resources (createSurfaceResources()). */
			SeaSurfaceWaves m_surfaceWaves{};
			std::shared_ptr< Vulkan::Image > m_shelterImage;
			std::shared_ptr< Vulkan::ImageView > m_shelterView;
			std::shared_ptr< Vulkan::Sampler > m_shelterSampler;
			/** @brief One staging buffer per frame in flight, created when that frame first uploads a map. */
			std::vector< std::unique_ptr< Vulkan::Buffer > > m_shelterStaging;
			mutable Base::Mutex m_shelterAccess;
			/** @brief The map to show, null for none (setShelterMap()). */
			std::shared_ptr< const SeaShelterMap > m_shelterMap EMEN_GUARDED_BY(m_shelterAccess);
			/** @brief Counts the maps given: the render thread uploads when it differs from the one it uploaded. */
			uint64_t m_shelterGeneration EMEN_GUARDED_BY(m_shelterAccess){1};
			/** @brief The generation the image holds; 0 while it holds nothing (render thread). */
			uint64_t m_uploadedShelterGeneration{0};
			/** @brief The rectangle of the map the image holds, as the uniforms state it (render thread). */
			std::array< float, 4 > m_shelterRectangle{};
			mutable std::vector< Selection > m_selection; ///< Render thread, one pass at a time.
			bool m_surfaceUpdated{false};
	};
}
