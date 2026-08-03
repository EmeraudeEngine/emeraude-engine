/*
 * src/Scenes/BindlessTextureSet.hpp
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
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

/* Local inclusions for usages. */
#include "Graphics/BindlessTextureManager.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class TextureInterface;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Per-scene description of the bindless textures used by a scene.
	 * @note This is the bindless equivalent of LightSet: a passive state structure owned and
	 * maintained by the scene (its RT materials, its lights, its environment cubemap register
	 * here). The global Graphics::BindlessTextureManager READS this structure to populate its
	 * descriptor table for the active scene — the scene NEVER writes the manager directly.
	 *
	 * Slots are allocated per-scene from Graphics::BindlessTextureManager::FirstDynamicSlot.
	 * Because only one scene is active at a time and the manager mirrors only the active scene's
	 * set, two scenes may legitimately reuse the same indices: table capacity is the largest
	 * scene, not the sum of all scenes.
	 */
	class EMEN_API BindlessTextureSet final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BindlessTextureSet"};

			/**
			 * @brief A single registered texture: the texture kept alive and its global bindless index.
			 */
			struct EMEN_API Entry
			{
				std::shared_ptr< Vulkan::TextureInterface > texture;
				uint32_t globalIndex;
			};

			/**
			 * @brief An immutable copy of the set used by the manager to upload descriptors
			 * without holding the set's lock during Vulkan calls.
			 */
			struct EMEN_API Snapshot
			{
				std::vector< Entry > textures2D;
				std::vector< Entry > texturesCube;
				std::vector< Entry > texturesCubeArray;
				std::shared_ptr< Vulkan::TextureInterface > environmentCubemap;
				std::shared_ptr< Vulkan::TextureInterface > irradianceCubemap;
				std::shared_ptr< Vulkan::TextureInterface > prefilteredCubemap;
			};

			/**
			 * @brief Constructs an empty bindless texture set.
			 */
			BindlessTextureSet () noexcept = default;

			/**
			 * @brief Sets the slot capacities the set is allowed to allocate from.
			 * @note MUST mirror the effective capacities of the GPU descriptor table
			 * (Graphics::BindlessTextureManager::maxTextures2D() and friends), which are resolved at
			 * renderer initialization from the device's update-after-bind budget and are NOT the
			 * compile-time desired values on every device (MoltenVK caps them). Handing out a slot
			 * beyond the table size would make the manager silently reject the descriptor write and
			 * the texture would never appear. Called by the owning scene at construction.
			 * @param maxTextures2D The capacity of the 2D array.
			 * @param maxTexturesCube The capacity of the cubemap array.
			 * @param maxTexturesCubeArray The capacity of the cube array array.
			 * @return void
			 */
			void setCapacities (uint32_t maxTextures2D, uint32_t maxTexturesCube, uint32_t maxTexturesCubeArray) noexcept;

			/**
			 * @brief Registers a 2D texture (deduplicated by texture instance).
			 * @param texture A reference to the texture smart pointer.
			 * @return The global bindless index, or UINT32_MAX if the table is full.
			 */
			uint32_t registerTexture2D (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept;

			/**
			 * @brief Registers a cube texture (deduplicated by texture instance).
			 * @param texture A reference to the texture smart pointer.
			 * @return The global bindless index, or UINT32_MAX if the table is full.
			 */
			uint32_t registerTextureCube (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept;

			/**
			 * @brief Registers a cube array texture (deduplicated by texture instance).
			 * @param texture A reference to the texture smart pointer.
			 * @return The global bindless index, or UINT32_MAX if the table is full.
			 */
			uint32_t registerTextureCubeArray (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept;

			/**
			 * @brief Unregisters a 2D texture by instance.
			 * @param texture A raw pointer to the texture instance.
			 * @return void
			 */
			void unregisterTexture2D (const Vulkan::TextureInterface * texture) noexcept;

			/**
			 * @brief Unregisters a cube texture by instance.
			 * @param texture A raw pointer to the texture instance.
			 * @return void
			 */
			void unregisterTextureCube (const Vulkan::TextureInterface * texture) noexcept;

			/**
			 * @brief Unregisters a cube array texture by instance.
			 * @param texture A raw pointer to the texture instance.
			 * @return void
			 */
			void unregisterTextureCubeArray (const Vulkan::TextureInterface * texture) noexcept;

			/**
			 * @brief Sets the scene environment cubemap (written to the reserved env slot by the manager).
			 * @param cubemap A reference to the cubemap smart pointer (may be null to clear).
			 * @return void
			 */
			void setEnvironmentCubemap (const std::shared_ptr< Vulkan::TextureInterface > & cubemap) noexcept;

			/**
			 * @brief Returns the scene environment cubemap (thread-safe copy).
			 * @return std::shared_ptr< Vulkan::TextureInterface >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::TextureInterface > environmentCubemap () const noexcept;

			/**
			 * @brief Sets the baked IBL irradiance cubemap (reserved slot 1, written by the manager).
			 * @param cubemap A reference to the cubemap smart pointer (may be null to clear).
			 * @return void
			 */
			void setIrradianceCubemap (const std::shared_ptr< Vulkan::TextureInterface > & cubemap) noexcept;

			/**
			 * @brief Sets the baked IBL prefiltered cubemap (reserved slot 2, written by the manager).
			 * @param cubemap A reference to the cubemap smart pointer (may be null to clear).
			 * @return void
			 */
			void setPrefilteredCubemap (const std::shared_ptr< Vulkan::TextureInterface > & cubemap) noexcept;

			/**
			 * @brief Produces a thread-safe copy of the set for descriptor upload.
			 * @return Snapshot
			 */
			[[nodiscard]]
			Snapshot snapshot () const noexcept;

			/**
			 * @brief Removes every registration from the set.
			 * @return void
			 */
			void clear () noexcept;

		private:

			/**
			 * @brief Registers a texture in one binding bucket (deduplicated).
			 */
			uint32_t registerInBucket (std::vector< Entry > & entries, std::unordered_map< const Vulkan::TextureInterface *, uint32_t > & lookup, std::queue< uint32_t > & freeIndices, uint32_t & nextIndex, uint32_t maxIndex, const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept;

			/**
			 * @brief Unregisters a texture from one binding bucket.
			 */
			static void unregisterFromBucket (std::vector< Entry > & entries, std::unordered_map< const Vulkan::TextureInterface *, uint32_t > & lookup, std::queue< uint32_t > & freeIndices, const Vulkan::TextureInterface * texture) noexcept;

			mutable std::mutex m_access;

			std::vector< Entry > m_textures2D;
			std::vector< Entry > m_texturesCube;
			std::vector< Entry > m_texturesCubeArray;

			std::unordered_map< const Vulkan::TextureInterface *, uint32_t > m_lookup2D;
			std::unordered_map< const Vulkan::TextureInterface *, uint32_t > m_lookupCube;
			std::unordered_map< const Vulkan::TextureInterface *, uint32_t > m_lookupCubeArray;

			std::queue< uint32_t > m_free2D;
			std::queue< uint32_t > m_freeCube;
			std::queue< uint32_t > m_freeCubeArray;

			uint32_t m_next2D{Graphics::BindlessTextureManager::FirstDynamicSlot};
			uint32_t m_nextCube{Graphics::BindlessTextureManager::FirstDynamicSlot};
			uint32_t m_nextCubeArray{Graphics::BindlessTextureManager::FirstDynamicSlot};

			/* Effective table capacities, pushed by the owning scene — see setCapacities(). The
			 * desired values are only a fallback for a set used before the scene wires them. */
			uint32_t m_maxTextures2D{Graphics::BindlessTextureManager::DesiredMaxTextures2D};
			uint32_t m_maxTexturesCube{Graphics::BindlessTextureManager::DesiredMaxTexturesCube};
			uint32_t m_maxTexturesCubeArray{Graphics::BindlessTextureManager::DesiredMaxTexturesCubeArray};

			std::shared_ptr< Vulkan::TextureInterface > m_environmentCubemap;
			std::shared_ptr< Vulkan::TextureInterface > m_irradianceCubemap;
			std::shared_ptr< Vulkan::TextureInterface > m_prefilteredCubemap;
	};
}
