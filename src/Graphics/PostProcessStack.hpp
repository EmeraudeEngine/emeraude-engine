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
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for usages. */
#include "DirectPostProcessEffect.hpp"
#include "EffectSlot.hpp"

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
	 * @brief The frame's post-process chain, as a fixed sequence of CONCEPTS rather than a list.
	 * @note ⚠️⚠️ THE CHAIN ORDER IS A PROPERTY OF THIS CLASS, NOT OF THE CALL SEQUENCE. Effects
	 * are filed into the slot they declare (`IndirectPostProcessEffect::slot()`) and the chain is
	 * walked in `EffectSlot` order, so an application may add its effects in any order at all and
	 * still get the canonical one. This replaced an insertion-ordered vector in which twelve
	 * scenes each restated the order by hand — three of them differently, and a wrong order was
	 * silent: an ambient occlusion added before the indirect diffuse occluded the direct light
	 * and left the GI untouched, and a screen-space reflection added first reflected a world
	 * with no indirect light in it at all.
	 * @note A slot holds AS MANY alternatives as the application builds — several RTGI and
	 * several SSGI with different Parameters, all resident so a runtime switch can compare them
	 * on the very same framing — of which the stack keeps AT MOST ONE ENABLED. Enabling one is
	 * selecting it; its siblings are disabled mechanically (see disableSlotSiblings()).
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
			 * @brief Files an effect into the slot it declares.
			 * @note ⚠️ The ORDER OF THE CALLS IS IRRELEVANT — the effect's own `slot()` decides
			 * where it runs. Adding several occupants to one slot is legal and is how a runtime
			 * A/B works (all resident, one enabled); the newcomer becomes the enabled one if it
			 * is enabled, which every effect is at construction.
			 * @warning A camera slot (DepthOfField, MotionBlur, Glare, ToneMapping) is REFUSED
			 * with a trace error: those belong to syncCameraEffects(), which owns their lifetime.
			 * @param effect A shared pointer to the effect.
			 * @return void
			 */
			void addEffect (std::shared_ptr< IndirectPostProcessEffect > effect) noexcept;

			/**
			 * @brief Disables every OTHER occupant of an effect's slot.
			 * @warning ⚠️ INTERNAL — called by IndirectPostProcessEffect::enable() to make the
			 * exclusivity of a concept mechanical. Never call it directly: enable the effect you
			 * want and the siblings follow.
			 * @param effect A reference to the effect being enabled.
			 * @return void
			 */
			void disableSlotSiblings (const IndirectPostProcessEffect & effect) noexcept;

			/**
			 * @brief Returns the occupants of a slot, in their order of addition.
			 * @param slot The slot.
			 * @return const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< IndirectPostProcessEffect > > &
			slotEffects (EffectSlot slot) const noexcept
			{
				return m_slots[static_cast< size_t >(slot)];
			}

			/**
			 * @brief Returns the ENABLED occupant of a slot, or nullptr.
			 * @param slot The slot.
			 * @return std::shared_ptr< IndirectPostProcessEffect >
			 */
			[[nodiscard]]
			std::shared_ptr< IndirectPostProcessEffect > enabledEffect (EffectSlot slot) const noexcept;

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
			 * @brief Appends a DISPLAY effect, compiled into the final fullscreen pass.
			 * @note Display effects (anti-aliasing, sharpening) are display-referred and
			 * single-pass by nature: instead of paying a render pass + render target each,
			 * they generate GLSL into the final post-process shader, BEFORE the camera's
			 * lens effects (film grain or scanlines must not be sharpened). At most ONE
			 * fetch-overriding effect (FXAA, FXAASharpen) per stack.
			 * @param effect A shared pointer to the display effect.
			 * @return void
			 */
			void
			addDisplayEffect (std::shared_ptr< DirectPostProcessEffect > effect) noexcept
			{
				if ( effect != nullptr )
				{
					m_displayEffects.emplace_back(std::move(effect));
				}
			}

			/**
			 * @brief Returns the display effect list (final-pass compiled effects).
			 * @return const DirectEffectList &
			 */
			[[nodiscard]]
			const DirectEffectList &
			displayEffects () const noexcept
			{
				return m_displayEffects;
			}

			/**
			 * @brief Returns whether the stack has any display effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasDisplayEffects () const noexcept
			{
				return !m_displayEffects.empty();
			}

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
				return m_orderedEffects;
			}

			/**
			 * @brief Returns whether the stack has any effects.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasEffects () const noexcept
			{
				return !m_orderedEffects.empty();
			}

			/**
			 * @brief Returns whether an ENABLED scene-reflection provider (SSR, RTR) is in
			 * the chain.
			 * @note Read by the Renderer to suspend the continuous reflection probes while a
			 * traced reflection covers the same job (reflection cost ladder).
			 * @return bool
			 */
			[[nodiscard]]
			bool hasEnabledReflectionProvider () const noexcept;

			/**
			 * @brief Returns whether an ENABLED indirect-diffuse provider (RTGI) is in the chain.
			 * @note Read by the scene to hand the diffuse IBL leg over to that effect — see
			 * PostProcessEffect::providesIndirectDiffuse() for the ownership contract.
			 * @return bool
			 */
			[[nodiscard]]
			bool hasEnabledIndirectDiffuseProvider () const noexcept;

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

			/**
			 * @brief Rebuilds the flat, slot-ordered view the chain executor walks.
			 * @note Rebuilt on every mutation rather than assembled on demand: it is read once
			 * per frame and mutated a handful of times per scene.
			 * @return void
			 */
			void rebuildOrderedEffects () noexcept;

			/** @brief The occupants of each concept, indexed by EffectSlot. */
			std::array< std::vector< std::shared_ptr< IndirectPostProcessEffect > >, EffectSlotCount > m_slots;
			/** @brief The slot table flattened in EffectSlot order — THE chain order. */
			std::vector< std::shared_ptr< IndirectPostProcessEffect > > m_orderedEffects;
			/* Display effects (AA, sharpening): no GPU resources of their own, they are
			 * compiled into the final fullscreen pass shader before the camera lens effects. */
			DirectEffectList m_displayEffects;
			/* Camera-driven photographic effects (physical camera contract). Kept aside to
			 * distinguish them from the application/scene effects inside m_effects. */
			std::shared_ptr< IndirectPostProcessEffect > m_cameraDepthOfField;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraMotionBlur;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraBloom;
			std::shared_ptr< IndirectPostProcessEffect > m_cameraToneMapping;
	};
}
