/*
 * src/Scenes/SceneMetaData.hpp
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
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Local inclusions. */
#include "GPUMeshMetaData.hpp"
#include "RenderBatch.hpp"
#include "Graphics/Material/GPURTMaterialData.hpp"
#include "Vulkan/AccelerationStructure.hpp"
#include "Vulkan/AccelerationStructureBuilder.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Device;
	class TextureInterface;
}

namespace EmEn::Graphics
{
	class BindlessTextureManager;

	namespace Material
	{
		class Interface;
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief Manages scene-level ray tracing metadata: TLAS, mesh metadata, and material data.
	 * @note When RT is not available on the device, this class is inert (rebuild() is a no-op,
	 *       TLAS() returns nullptr). This centralizes all RT resource management that was
	 *       previously scattered in Scene.
	 */
	class SceneMetaData final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneMetaData"};

			/** @brief Number of render list categories passed to rebuild(). */
			static constexpr size_t RenderListCount{7};

			/** @brief Maximum number of RT instances (meshes) supported. */
			static constexpr size_t MaxRTInstances{4096};

			/** @brief Maximum number of unique RT materials supported. */
			static constexpr size_t MaxRTMaterials{1024};

			/**
			 * @brief Constructs the scene metadata manager.
			 * @note Initializes the acceleration structure builder if the device supports ray tracing
			 *       AND the user setting enables it. Registers the builder with Geometry::Interface
			 *       for BLAS creation.
			 * @param device A reference to the Vulkan device smart pointer.
			 * @param enableRayTracing Whether the user setting allows ray tracing.
			 */
			explicit SceneMetaData (const std::shared_ptr< Vulkan::Device > & device, bool enableRayTracing = true) noexcept;

			SceneMetaData (const SceneMetaData & copy) noexcept = delete;
			SceneMetaData (SceneMetaData && copy) noexcept = delete;
			SceneMetaData & operator= (const SceneMetaData & copy) noexcept = delete;
			SceneMetaData & operator= (SceneMetaData && copy) noexcept = delete;

			/**
			 * @brief Destructs the scene metadata manager.
			 * @note Unregisters the acceleration structure builder from Geometry::Interface.
			 */
			~SceneMetaData ();

			/**
			 * @brief Returns whether ray tracing is available for this scene.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRayTracingEnabled () const noexcept
			{
				return m_accelerationStructureBuilder != nullptr;
			}

			/**
			 * @brief Initializes per-frame SSBO storage for frames-in-flight synchronization.
			 * @param frameCount Number of frames in flight.
			 * @return bool
			 */
			[[nodiscard]]
			bool initializePerFrameBuffers (uint32_t frameCount) noexcept;

			/**
			 * @brief Rebuilds all RT metadata from render batches.
			 * @note The TLAS must include ALL scene geometry regardless of frustum culling.
			 * RT effects (GI, AO, reflections) cast rays in world space and must be able
			 * to hit geometry outside the camera's field of view.
			 * @param opaqueList The opaque render batch list (not frustum-culled).
			 * @param opaqueLightedList The opaque lighted render batch list (not frustum-culled).
			 * @param bindlessTextureManager A pointer to the bindless texture manager for RT texture registration.
			 * @param frameIndex The current frame-in-flight index for SSBO double-buffering.
			 */
			void rebuild (const RenderBatch::List & opaqueList, const RenderBatch::List & opaqueLightedList, Graphics::BindlessTextureManager * bindlessTextureManager, uint32_t frameIndex) noexcept;

			/**
			 * @brief Records the pending TLAS build into an external command buffer.
			 * @note Must be called after rebuild() and before render passes that use RT.
			 * No-op if there is no pending build (RT disabled or empty scene).
			 * @param cmdBuf The Vulkan command buffer to record into (must be in recording state).
			 */
			void recordTLASBuild (VkCommandBuffer cmdBuf) noexcept;

			/**
			 * @brief Returns whether a TLAS build is pending and needs to be recorded.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPendingTLASBuild () const noexcept
			{
				return m_pendingTLASBuild != nullptr;
			}

			/**
			 * @brief Returns the top-level acceleration structure for ray tracing.
			 * @return const Vulkan::AccelerationStructure *
			 */
			[[nodiscard]]
			const Vulkan::AccelerationStructure *
			TLAS () const noexcept
			{
				return m_TLAS.get();
			}

			/**
			 * @brief Returns the mesh metadata SSBO for the given frame index.
			 * @param frameIndex The current frame-in-flight index.
			 * @return const Vulkan::ShaderStorageBufferObject *
			 */
			[[nodiscard]]
			const Vulkan::ShaderStorageBufferObject *
			meshMetaDataSSBO (uint32_t frameIndex) const noexcept
			{
				if ( frameIndex >= m_meshMetaDataSSBOs.size() )
				{
					return nullptr;
				}

				return m_meshMetaDataSSBOs[frameIndex].get();
			}

			/**
			 * @brief Returns the material data SSBO for the given frame index.
			 * @param frameIndex The current frame-in-flight index.
			 * @return const Vulkan::ShaderStorageBufferObject *
			 */
			[[nodiscard]]
			const Vulkan::ShaderStorageBufferObject *
			materialDataSSBO (uint32_t frameIndex) const noexcept
			{
				if ( frameIndex >= m_materialDataSSBOs.size() )
				{
					return nullptr;
				}

				return m_materialDataSSBOs[frameIndex].get();
			}

			/**
			 * @brief Returns the number of RT instances in the current frame.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			instanceCount () const noexcept
			{
				return m_instanceCount;
			}

			/**
			 * @brief Returns the number of unique RT materials in the current frame.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			materialCount () const noexcept
			{
				return m_materialCount;
			}

		private:

			/** @brief Reference to the Vulkan device for SSBO creation. */
			std::shared_ptr< Vulkan::Device > m_device;
			/** @brief Acceleration structure builder for ray tracing. Null when RT is disabled. */
			std::unique_ptr< Vulkan::AccelerationStructureBuilder > m_accelerationStructureBuilder;
			/** @brief Top-level acceleration structure for the scene. Rebuilt each frame. */
			std::unique_ptr< Vulkan::AccelerationStructure > m_TLAS;
			/** @brief Pending TLAS build request (prepared by rebuild, consumed by recordTLASBuild). */
			std::unique_ptr< Vulkan::TLASBuildRequest > m_pendingTLASBuild;
			/** @brief Retired TLAS objects kept alive until frames-in-flight have completed. */
			std::deque< std::unique_ptr< Vulkan::AccelerationStructure > > m_retiredTLAS;
			/** @brief Per-frame mesh metadata SSBOs (one per frame-in-flight). */
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_meshMetaDataSSBOs;
			/** @brief Per-frame material data SSBOs (one per frame-in-flight). */
			std::vector< std::unique_ptr< Vulkan::ShaderStorageBufferObject > > m_materialDataSSBOs;
			/** @brief Cache of registered bindless texture indices keyed by texture pointer. */
			std::unordered_map< const Vulkan::TextureInterface *, uint32_t > m_textureRegistrationCache;
			/** @brief Persistent per-frame collection structures (reused to avoid heap allocations). */
			std::vector< Vulkan::TLASInstanceInput > m_rebuildInstances;
			std::vector< GPUMeshMetaData > m_rebuildMeshEntries;
			std::unordered_map< const Graphics::Material::Interface *, uint32_t > m_rebuildMaterialMap;
			std::vector< Graphics::Material::GPURTMaterialData > m_rebuildMaterialEntries;
			std::unordered_set< const Vulkan::TextureInterface * > m_rebuildActiveTextures;
			/** @brief Number of active RT instances this frame. */
			size_t m_instanceCount{0};
			/** @brief Number of unique RT materials this frame. */
			size_t m_materialCount{0};
	};
}
