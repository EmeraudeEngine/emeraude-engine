/*
 * src/Scenes/CloudSet.hpp
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
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class CloudShadowMap;
		class Renderer;
	}

	namespace Vulkan
	{
		class CommandBuffer;
	}

	namespace Scenes
	{
		class BindlessTextureSet;
		class Scene;

		namespace Component
		{
			class CloudVolume;
			class DirectionalLight;
		}
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief The volumetric clouds of a scene — what Graphics::Effects::Atmosphere::VolumetricClouds draws.
	 * @note Filled by the scene from its entities' notifications (AbstractEntity::CloudVolumeCreated /
	 * CloudVolumeDestroyed), the way the LightSet is. It is also the switch of the whole feature: the
	 * post-process stack files and materializes the cloud pass only while this set is not empty
	 * (PostProcessStack::syncSceneEffects()), so a scene with no cloud pays nothing.
	 * @note [THREAD] Mutated on the logic thread, walked on the render thread: every access goes
	 * through the mutex, and forEach() holds it for the whole walk — the callback must take no lock
	 * that the logic thread could hold while adding a cloud.
	 */
	class EMEN_API CloudSet final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CloudSet"};

			/**
			 * @brief The automatic coverage: the side of the shadow map as a multiple of the clouds' mean width.
			 * @note Never below `Core/Graphics/PostProcessing/Clouds/ShadowCoverage` (1024 m by default), which keeps
			 * a scene of small clouds exactly where it was (`forest`: 70-130 m clouds, 1024 m) and lets a landscape of
			 * cumulus cast over kilometres (`terrain`: 300-1000 m clouds, ~5 km).
			 */
			static constexpr float AutomaticCoverageFactor{8.0F};

			/** @brief The bindless slot of a shadow map that does not exist (yet). */
			static constexpr uint32_t NoShadowMap{UINT32_MAX};

			/**
			 * @brief Constructs an empty cloud set.
			 */
			CloudSet () noexcept;

			/**
			 * @brief Destructs the cloud set, and releases the shadow map's bindless slot.
			 */
			~CloudSet ();

			CloudSet (const CloudSet & copy) noexcept = delete;
			CloudSet (CloudSet && copy) noexcept = delete;
			CloudSet & operator= (const CloudSet & copy) noexcept = delete;
			CloudSet & operator= (CloudSet && copy) noexcept = delete;

			/**
			 * @brief Records this frame's Beer shadow map of the clouds, seen from the main sun [RENDER THREAD].
			 * @note Created the first time it is needed (settings `Core/Graphics/PostProcessing/Clouds/Shadow*`,
			 * read once), registered in the scene's bindless 2D array, and recorded only on a frame whose
			 * published sun block already reads it — the matrix the pass uses must be the one the lit
			 * shaders use. Called by Scene::recordCloudShadowMap(), before the scene pass.
			 * The map SIZES ITSELF on the clouds: its side is AutomaticCoverageFactor times their mean width (the
			 * `ShadowCoverage` setting is the floor), re-evaluated when the number of drawn clouds changes — never
			 * every frame, a texel size that breathes would make the shadows crawl; and the range it searches
			 * along the light encloses every cloud, every frame (CloudShadowMap::MinimumDepthRange is the floor).
			 * @param commandBuffer A reference to the frame's command buffer, outside any render pass.
			 * @param renderer A reference to the graphics renderer.
			 * @param bindlessTextureSet A reference to the scene's bindless set.
			 * @param sun A reference to the scene's main directional light.
			 * @param readStateIndex The render state slot latched by the frame.
			 * @return void
			 */
			void recordShadowMap (const Vulkan::CommandBuffer & commandBuffer, Graphics::Renderer & renderer, BindlessTextureSet & bindlessTextureSet, const Component::DirectionalLight & sun, uint32_t readStateIndex) noexcept;

			/**
			 * @brief Returns the shadow map's bindless 2D slot, or NoShadowMap [THREAD-SAFE].
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			shadowMapBindlessIndex () const noexcept
			{
				return m_shadowMapBindlessIndex.load(std::memory_order_acquire);
			}

			/**
			 * @brief Returns the side of the shadow map, in metres [THREAD-SAFE].
			 * @return float
			 */
			[[nodiscard]]
			float
			shadowMapCoverage () const noexcept
			{
				return m_shadowMapCoverage.load(std::memory_order_acquire);
			}

			/**
			 * @brief Returns the side of the shadow map, in texels [THREAD-SAFE].
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			shadowMapResolution () const noexcept
			{
				return m_shadowMapResolution.load(std::memory_order_acquire);
			}

			/**
			 * @brief Adds a cloud and joins it to the scene (bindless shape registration).
			 * @param scene A reference to the scene.
			 * @param cloud A reference to the cloud smart pointer.
			 * @return void
			 */
			void add (Scene & scene, const std::shared_ptr< Component::CloudVolume > & cloud) noexcept;

			/**
			 * @brief Removes a cloud and frees its bindless slot.
			 * @param cloud A reference to the cloud smart pointer.
			 * @return void
			 */
			void remove (const std::shared_ptr< Component::CloudVolume > & cloud) noexcept;

			/**
			 * @brief Removes every cloud.
			 * @return void
			 */
			void clear () noexcept;

			/**
			 * @brief Returns whether the scene holds no cloud [THREAD-SAFE, lock-free].
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_count.load(std::memory_order_acquire) == 0;
			}

			/**
			 * @brief Returns the number of clouds [THREAD-SAFE, lock-free].
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			count () const noexcept
			{
				return m_count.load(std::memory_order_acquire);
			}

			/**
			 * @brief Calls a function on every cloud, under the set mutex.
			 * @tparam function_t The type of the callable, `void (const Component::CloudVolume &)`.
			 * @param function The callable.
			 * @return void
			 */
			template< typename function_t >
			void
			forEach (function_t && function) const noexcept requires (std::is_invocable_v< function_t, const Component::CloudVolume & >)
			{
				const std::lock_guard< std::mutex > lock{m_access};

				for ( const auto & cloud : m_clouds )
				{
					function(*cloud);
				}
			}

		private:

			mutable std::mutex m_access;
			std::vector< std::shared_ptr< Component::CloudVolume > > m_clouds;
			std::atomic< size_t > m_count{0};
			/* The shadow map — RENDER THREAD only, created on first use. */
			std::unique_ptr< Graphics::CloudShadowMap > m_shadowMap;
			BindlessTextureSet * m_shadowMapBindlessSet{nullptr};
			std::atomic< uint32_t > m_shadowMapBindlessIndex{NoShadowMap};
			std::atomic< float > m_shadowMapCoverage{0.0F};
			std::atomic< uint32_t > m_shadowMapResolution{0};
			/* RENDER THREAD only: the floor of the automatic coverage (the setting), and the drawn cloud count the
			 * current coverage was computed for. */
			float m_shadowMapMinimumCoverage{0.0F};
			uint32_t m_shadowMapCoverageCloudCount{0};
			/* The creation was attempted (or declined by the settings): never retried every frame. */
			bool m_shadowMapResolved{false};
	};
}
