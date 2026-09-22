/*
 * src/Graphics/Geometry/CDLODTerrainResource.hpp
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
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Frustum.hpp"
#include "HeightfieldSurface.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		template< typename resource_t >
		class Container;
	}

	namespace Vulkan
	{
		class Buffer;
		class CommandBuffer;
		class CommandPool;
		class ComputePipeline;
		class DescriptorPool;
		class DescriptorSet;
		class DescriptorSetLayout;
		class Image;
		class ImageView;
		class PipelineLayout;
		class Sampler;
		class UniformBufferObject;
	}
}

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief The knobs of a CDLOD terrain. The defaults are the measured ones of the `terrain` demo.
	 */
	struct CDLODTerrainParameters
	{
		/** @brief Quads per side of the shared patch (G): a leaf node covers G cells. Even. */
		uint32_t patchQuads{64};
		/** @brief Texels per side of a clip level (T). A power of two, at least 256. */
		uint32_t clipTexels{2048};
		/** @brief A clip level re-centres by this many of its texels at a time (the strip width). */
		uint32_t clipUpdateTexels{32};
		/** @brief Distance (m) up to which the finest level of detail is drawn; every next level doubles it. */
		float detailDistance{384.0F};
		/** @brief Where, in a level's distance ring, the geomorph toward the next level starts (Strugar: 0.66). */
		float morphStartRatio{0.66F};
		/** @brief Side (m) of the ray-tracing proxy, centred on the camera. */
		float rayTracingProxySize{4096.0F};
		/** @brief The proxy is rebuilt when less than this (m) is left to its edge. */
		float rayTracingProxyMargin{1500.0F};
	};

	/**
	 * @brief A heightfield terrain drawn by Continuous Distance-Dependent LOD over a height CLIPMAP.
	 * @details One shared patch of (G+1)² flat vertices (positions only) is drawn once per selected
	 * quadtree node, placed by push constants and displaced in the vertex stage by a clipmap of
	 * tent-filtered heights (HeightfieldSurface): no vertex of the terrain is ever stored, no LOD is
	 * stitched, no window slides. The fragment stage lights each pixel with the normal of the clip level
	 * matching its footprint, baked on the GPU from that level's heights.
	 *
	 * Threads:
	 * - load() (loading thread) builds the CPU side: the height pyramid (16-bit, levels 1..L-1; level 0
	 *   is the source grid itself), the per-node height ranges, the patch.
	 * - updateSurfaceVideoMemory() (render thread, once per frame, before any pass) re-centres the clip
	 *   levels on the camera — strips copied and their normals baked in a command buffer submitted to the
	 *   graphics queue ahead of the frame — and writes the frame's uniform section.
	 * - prepareAdaptiveRendering() (render thread, once per pass) selects the nodes.
	 * - updateRayTracingProxy() (logic thread) re-centres the ray-tracing proxy on a worker; the render
	 *   thread publishes it in updateVideoMemory().
	 *
	 * References: F. Strugar, "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps",
	 * JGT 2009 (https://github.com/fstrugar/CDLOD); F. Losasso, H. Hoppe, "Geometry Clipmaps", SIGGRAPH 2004.
	 * @warning ⚠️ The object space of this geometry IS the world: a terrain answers ground levels in world
	 * coordinates, and the nodes and the camera are pushed in world coordinates.
	 */
	class EMEN_API CDLODTerrainResource final : public Interface
	{
		friend class Resources::Container< CDLODTerrainResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CDLODTerrainResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs a CDLOD terrain geometry.
			 * @param serviceProvider A reference to the service provider.
			 * @param name A reference to a string for the resource name.
			 */
			CDLODTerrainResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name) noexcept;

			CDLODTerrainResource (const CDLODTerrainResource & copy) noexcept = delete;

			CDLODTerrainResource (CDLODTerrainResource && copy) noexcept = delete;

			CDLODTerrainResource & operator= (const CDLODTerrainResource & copy) noexcept = delete;

			CDLODTerrainResource & operator= (CDLODTerrainResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the CDLOD terrain geometry.
			 */
			~CDLODTerrainResource () override;

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

			/** @copydoc EmEn::Resources::ResourceTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::isCreated() const */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::topology() const */
			[[nodiscard]]
			Topology
			topology () const noexcept override
			{
				return Topology::TriangleList;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryCount() const */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::subGeometryRange() const */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			subGeometryRange (uint32_t /*subGeometryIndex*/) const noexcept override
			{
				return {0, m_patchIndexCount};
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::boundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_boundingSphere;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::vertexBufferObject() const */
			[[nodiscard]]
			const Vulkan::VertexBufferObject *
			vertexBufferObject () const noexcept override
			{
				return m_vertexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::indexBufferObject() const */
			[[nodiscard]]
			const Vulkan::IndexBufferObject *
			indexBufferObject () const noexcept override
			{
				return m_indexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::useIndexBuffer() const */
			[[nodiscard]]
			bool
			useIndexBuffer () const noexcept override
			{
				return true;
			}

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

			/** @copydoc EmEn::Graphics::Geometry::Interface::dedicatedRTVertexBufferObject() const */
			[[nodiscard]]
			const Vulkan::VertexBufferObject *
			dedicatedRTVertexBufferObject () const noexcept override
			{
				return m_rtVertexBufferObject.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::dedicatedRTIndexBufferObject() const */
			[[nodiscard]]
			const Vulkan::IndexBufferObject *
			dedicatedRTIndexBufferObject () const noexcept override
			{
				return m_rtIndexBufferObjectProxy.get();
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::dedicatedRTVertexFlags() const */
			[[nodiscard]]
			uint32_t
			dedicatedRTVertexFlags () const noexcept override
			{
				return RayTracingProxyFlags;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::isAdaptiveLOD() const */
			[[nodiscard]]
			bool
			isAdaptiveLOD () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::prepareAdaptiveRendering() const */
			void prepareAdaptiveRendering (const Base::Math::Vector< 3, float > & lodViewPosition, const Frustum * cullingFrustum, const Base::Math::CartesianFrame< float > * worldCoordinates) const noexcept override;

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallCount() const */
			[[nodiscard]]
			uint32_t
			getAdaptiveDrawCallCount () const noexcept override
			{
				return static_cast< uint32_t >(m_selection.size());
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallRange() const */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			getAdaptiveDrawCallRange (uint32_t drawCallIndex) const noexcept override
			{
				return m_selection[drawCallIndex].range;
			}

			/** @copydoc EmEn::Graphics::Geometry::Interface::getAdaptiveDrawCallConstants() const */
			[[nodiscard]]
			std::array< float, 4 >
			getAdaptiveDrawCallConstants (uint32_t drawCallIndex) const noexcept override
			{
				return m_selection[drawCallIndex].node;
			}

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
			size_t memoryOccupied () const noexcept override;

			/**
			 * @brief Builds the terrain from a height grid.
			 * @note Loading thread. The grid is SHARED, never copied: it is also what the terrain answers the
			 * physics with, and it must not change after this call. Its cell count must be a power of two, a
			 * multiple of the patch, and its world offset zero (the terrain is centred on the origin).
			 * @param source The height grid (level 0 of the pyramid).
			 * @param parameters The terrain knobs.
			 * @return bool
			 */
			bool load (const std::shared_ptr< const Base::VertexFactory::Grid< float > > & source, const CDLODTerrainParameters & parameters) noexcept;

			/**
			 * @brief Re-centres the ray-tracing proxy on the camera when it gets too close to its edge.
			 * @note Logic thread, once per cycle. The proxy is generated on the thread pool and published by
			 * the render thread (updateVideoMemory()), which then declares the BLAS stale.
			 * @param worldPosition The camera position in world coordinates.
			 * @return void
			 */
			void updateRayTracingProxy (const Base::Math::Vector< 3, float > & worldPosition) noexcept;

			/**
			 * @brief Returns the number of levels of detail (quadtree depth).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			levelOfDetailCount () const noexcept
			{
				return m_levelOfDetailCount;
			}

			/**
			 * @brief Returns the number of clip levels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			clipLevelCount () const noexcept
			{
				return m_clipLevelCount;
			}

		private:

			/** @brief The layout of the ray-tracing proxy vertex: position, tangent frame, UV (14 floats). */
			static constexpr uint32_t RayTracingProxyFlags{EnableTangentSpace | EnablePrimaryTextureCoordinates};

			/** @brief One draw of a pass: an index range of the patch (whole or one quarter) and its node. */
			struct Selection
			{
				std::array< uint32_t, 2 > range{};
				std::array< float, 4 > node{};
			};

			/** @brief A rectangle of world lattice indices of one clip level, [x0, x1) × [z0, z1). */
			struct LatticeRect
			{
				int64_t x0{0};
				int64_t z0{0};
				int64_t x1{0};
				int64_t z1{0};
			};

			/** @brief The ray-tracing proxy generated by a worker, waiting for the render thread. */
			struct PendingRayTracingProxy
			{
				std::unique_ptr< Vulkan::VertexBufferObject > vertexBufferObject;
				std::unique_ptr< Vulkan::IndexBufferObject > indexBufferObject;
			};

			/**
			 * @brief Returns the 16-bit height of a world lattice point of a clip level, edge-clamped.
			 * @param level The clip level.
			 * @param latticeX The world lattice index on X (0 at the world origin).
			 * @param latticeZ The world lattice index on Z.
			 * @return uint16_t
			 */
			[[nodiscard]]
			uint16_t levelHeight (uint32_t level, int64_t latticeX, int64_t latticeZ) const noexcept;

			/**
			 * @brief Returns the world lattice index a clip level should be centred on for a camera.
			 * @param level The clip level.
			 * @param position The camera, world XZ.
			 * @return std::array< int64_t, 2 >
			 */
			[[nodiscard]]
			std::array< int64_t, 2 > levelCenterFor (uint32_t level, const Base::Math::Vector< 3, float > & position) const noexcept;

			/**
			 * @brief Builds the per-node height ranges of every level of detail from the source grid.
			 * @return void
			 */
			void buildNodeHeightRanges () noexcept;

			/**
			 * @brief Builds the level-of-detail distance and morph tables.
			 * @return bool False if the parameters cannot keep a node within its clip level.
			 */
			[[nodiscard]]
			bool buildLevelOfDetailTables () noexcept;

			/**
			 * @brief Creates the clipmap images, the sampler, the uniform buffer, the descriptor sets and the
			 * normal bake pipeline.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSurfaceResources () noexcept;

			/**
			 * @brief Stages lattice rectangles of a clip level: their heights appended to the staging data,
			 * one copy region per toroidal texel rectangle, and the texel rectangles whose normals must be
			 * re-baked (grown by one texel, the bake reads the neighbours).
			 * @param level The clip level.
			 * @param rects The world lattice rectangles to rewrite.
			 * @param stagingData Receives the heights (the byte offset of each copy region points into it).
			 * @param copies Receives the copy regions.
			 * @param bakes Receives the bake regions {texel x, texel z, width, height, layer}.
			 * @return void
			 */
			void stageLevelRects (uint32_t level, const std::vector< LatticeRect > & rects, std::vector< uint16_t > & stagingData, std::vector< VkBufferImageCopy > & copies, std::vector< std::array< int32_t, 5 > > & bakes) const noexcept;

			/**
			 * @brief Writes the frame's uniform section.
			 * @param frameIndex The frame-in-flight index.
			 * @return void
			 */
			void writeUniforms (uint32_t frameIndex) const noexcept;

			/**
			 * @brief Selects the nodes of one quadtree node's subtree (Strugar, § 2.3).
			 * @return bool True when this node's area is handled (drawn or culled), false when it is beyond
			 * the range of its level and the parent must draw it.
			 */
			bool selectNode (uint32_t levelOfDetail, uint32_t nodeX, uint32_t nodeZ, const Base::Math::Vector< 3, float > & eye, const Frustum * frustum) const noexcept;

			/**
			 * @brief Returns the world box of a quadtree node.
			 * @return Base::Math::Space3D::AACuboid< float >
			 */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float > nodeBox (uint32_t levelOfDetail, uint32_t nodeX, uint32_t nodeZ) const noexcept;

			/**
			 * @brief Generates the ray-tracing proxy around a centre (worker thread).
			 * @param center The proxy centre, world XZ (snapped).
			 * @param proxy Receives the buffers, created and uploaded.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateRayTracingProxy (const Base::Math::Vector< 2, float > & center, PendingRayTracingProxy & proxy) const noexcept;

			/* CPU side (loading thread, read-only afterwards). */
			std::shared_ptr< const Base::VertexFactory::Grid< float > > m_source;
			CDLODTerrainParameters m_parameters;
			std::vector< std::vector< uint16_t > > m_pyramid; ///< Clip levels 1..L-1, row-major, (N / 2^l + 1)² each.
			std::vector< std::vector< std::array< float, 2 > > > m_nodeHeightRanges; ///< Per level of detail: {min, max} of each node.
			std::array< std::array< float, 2 >, HeightfieldSurface::MaxLevelsOfDetail > m_morphTable{}; ///< Per level of detail: {morph start, 1 / morph length}.
			std::array< float, HeightfieldSurface::MaxLevelsOfDetail > m_lodRanges{}; ///< Per level of detail: the distance it is drawn to.
			std::array< float, 4 > m_textureCoordinates{}; ///< U per metre, V per metre, U and V at the world origin.
			Base::Math::Space3D::AACuboid< float > m_boundingBox;
			Base::Math::Space3D::Sphere< float > m_boundingSphere;
			float m_cellSize{1.0F};
			float m_heightMinimum{0.0F};
			float m_heightRange{1.0F};
			uint32_t m_cellCount{0}; ///< N, cells per side of the source.
			uint32_t m_levelOfDetailCount{0}; ///< K.
			uint32_t m_clipLevelCount{0}; ///< L.
			uint32_t m_patchIndexCount{0};

			/* Patch. */
			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			std::unique_ptr< Vulkan::IndexBufferObject > m_indexBufferObject;

			/* Surface (render thread after creation). */
			std::shared_ptr< Vulkan::Image > m_heightImage;
			std::shared_ptr< Vulkan::ImageView > m_heightView;
			std::shared_ptr< Vulkan::Image > m_normalImage;
			std::shared_ptr< Vulkan::ImageView > m_normalView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::unique_ptr< Vulkan::UniformBufferObject > m_uniformBuffer;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets; ///< One per frame in flight (its uniform section).
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_bakeSetLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_bakePipelineLayout;
			std::shared_ptr< Vulkan::ComputePipeline > m_bakePipeline;
			std::unique_ptr< Vulkan::DescriptorSet > m_bakeDescriptorSet;
			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::vector< std::shared_ptr< Vulkan::CommandBuffer > > m_commandBuffers; ///< One per frame in flight.
			std::vector< std::unique_ptr< Vulkan::Buffer > > m_stagingBuffers; ///< One per frame in flight.
			std::array< std::array< int64_t, 2 >, HeightfieldSurface::MaxClipLevels > m_levelCenters{}; ///< World lattice index of each level's centre.
			VkDeviceSize m_uniformSectionSize{0};
			uint32_t m_currentFrameIndex{0};

			/* Selection (render thread, one pass at a time). */
			mutable std::vector< Selection > m_selection;

			/* Ray-tracing proxy. */
			std::unique_ptr< Vulkan::VertexBufferObject > m_rtVertexBufferObject;
			std::unique_ptr< Vulkan::IndexBufferObject > m_rtIndexBufferObjectProxy;
			mutable std::mutex m_pendingAccess;
			PendingRayTracingProxy m_pendingProxy;
			Base::Math::Vector< 2, float > m_rtProxyCenter; ///< Logic thread.
			std::atomic< bool > m_rtProxyUpdating{false};
			bool m_hasPendingProxy{false};
			bool m_surfaceUploaded{false};
	};
}
