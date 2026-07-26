/*
 * src/Graphics/PostProcessStack.hpp
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
#include <cstdint>
#include <memory>
#include <vector>

namespace EmEn::Graphics
{
	class IndirectPostProcessEffect;
	class Renderer;
}

namespace EmEn::Scenes::Component
{
	class Camera;
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	class DepthOfField;
	class ToneMapping;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Ordered container of multi-pass post-process effects with GPU lifecycle management.
	 * @note Owned by a Scene to provide per-scene post-processing configuration.
	 */
	class EMEN_API PostProcessStack final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessStack"};

			PostProcessStack () noexcept = default;

			~PostProcessStack () noexcept;

			/* Non-copiable, movable. */
			PostProcessStack (const PostProcessStack &) = delete;

			PostProcessStack & operator= (const PostProcessStack &) = delete;

			PostProcessStack (PostProcessStack &&) noexcept = default;

			PostProcessStack & operator= (PostProcessStack &&) noexcept = default;

			/**
			 * @brief Appends an effect to the chain.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void addEffect (std::shared_ptr< IndirectPostProcessEffect > effect) noexcept;

			/**
			 * @brief Removes an effect from the chain.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void removeEffect (const std::shared_ptr< IndirectPostProcessEffect > & effect) noexcept;

			/**
			 * @brief Synchronizes the CAMERA-DRIVEN photographic effects with the active camera.
			 * @note Physical camera contract: the camera declares its photographic behaviour
			 * (enableDepthOfField()/enableHDR()); this call (de)materializes the matching effects
			 * at the END of the chain, in canonical order (DepthOfField, then Bloom, then ToneMapping last).
			 * Scene effects (GI, AO, fog...) added by the application are left untouched.
			 * Called by the Renderer once per frame, on the render thread; removed effects are
			 * retired through the deferred destructor (frames-in-flight safety).
			 * @param camera The scene's active camera (nullptr = no photographic effects).
			 * @param renderer A reference to the graphics renderer.
			 * @return bool Whether the effect set changed (the pipeline must be reconfigured).
			 */
			[[nodiscard]]
			bool syncCameraEffects (const Scenes::Component::Camera * camera, Renderer & renderer) noexcept;

			/**
			 * @brief Returns the camera-materialized tone mapping effect, or nullptr.
			 * @note For readers of its metered values (the overlay panel): RENDER THREAD only,
			 * inside the frame scope — the instance is (de)materialized by syncCameraEffects()
			 * on that same thread, once per frame.
			 * @return std::shared_ptr< Effects::Framebuffer::ToneMapping >
			 */
			[[nodiscard]]
			std::shared_ptr< Effects::Framebuffer::ToneMapping > cameraToneMapping () const noexcept;

			/**
			 * @brief Returns the camera-materialized depth of field effect, or nullptr.
			 * @note Same RENDER THREAD / frame scope contract as cameraToneMapping(); the panel
			 * reads its metered focus distance through this.
			 * @return std::shared_ptr< Effects::Framebuffer::DepthOfField >
			 */
			[[nodiscard]]
			std::shared_ptr< Effects::Framebuffer::DepthOfField > cameraDepthOfField () const noexcept;

			/**
			 * @brief Clears the entire effect chain.
			 * @return void
			 */
			void clearEffects () noexcept;

			/**
			 * @brief Returns the effect chain.
			 * @return const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			effects () const noexcept
			{
				return m_effects;
			}

			/**
			 * @brief Returns whether the stack has any effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasEffects () const noexcept
			{
				return !m_effects.empty();
			}

			/**
			 * @brief Creates GPU resources for all effects.
			 * @param width The framebuffer width.
			 * @param height The framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			bool createAll (uint32_t width, uint32_t height) const noexcept;

			/**
			 * @brief Destroys GPU resources for all effects.
			 * @return void
			 */
			void destroyAll () const noexcept;

			/**
			 * @brief Recreates GPU resources after a resize.
			 * @param width The new framebuffer width.
			 * @param height The new framebuffer height.
			 * @return bool
			 */
			[[nodiscard]]
			bool resizeAll (uint32_t width, uint32_t height) const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires HDR input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresHDR () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires depth input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresDepth () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires normals input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresNormals () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires material properties input.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresMaterialProperties () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires albedo input.
			 * @note Albedo implies normals and material properties (fixed MRT order).
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresAlbedo () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires the velocity buffer (motion vectors).
			 * @note Velocity implies the full MRT chain before it (fixed MRT order).
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresVelocity () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires the scene light set.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresLightSet () const noexcept;

			/**
			 * @brief Returns whether any effect in the stack requires the sub-pixel projection jitter (TAA).
			 * @note The Renderer polls this once per rendered frame to drive the Halton jitter sequence.
			 * @return bool
			 */
			[[nodiscard]]
			bool requiresJitter () const noexcept;

		private:

			std::vector< std::shared_ptr< IndirectPostProcessEffect > > m_effects;
			/* Camera-driven photographic effects (physical camera contract). Kept aside to
			 * distinguish them from the application/scene effects inside m_effects. */
			std::shared_ptr< IndirectPostProcessEffect > m_cameraDepthOfField;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraMotionBlur;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraBloom;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraToneMapping;
	};
}
