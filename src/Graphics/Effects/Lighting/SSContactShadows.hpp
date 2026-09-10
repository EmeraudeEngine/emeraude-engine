/*
 * src/Graphics/Effects/Lighting/SSContactShadows.hpp
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

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Vulkan/UniformBufferObject.hpp"

namespace EmEn::Graphics::Effects::Lighting
{
	/**
	 * @brief Screen-space contact shadows: the depth buffer marched toward the light.
	 * @note The SCREEN-SPACE lane of EffectSlot::ContactShadows, sibling of RTContactShadows.
	 * For each pixel a ray steps toward the main directional light in VIEW space, by a fixed
	 * world-space step, and each step is projected to the screen to read the depth buffer: the
	 * pixel is shadowed when the ray passes behind a depth sample by less than the configured
	 * thickness. This is the original technique — UE (`r.ContactShadows`), Unity HDRP Contact
	 * Shadows, Frostbite — and the ray-queried sibling is the newer of the two.
	 *
	 * @note ⚠️ THE STEP IS A WORLD-SPACE LENGTH (`maxDistance / stepCount`, in metres), not a
	 * pixel count, and that is what makes the lane switch measurable: `maxDistance` means the
	 * same thing here and in RTContactShadows, so an A/B compares the two TECHNIQUES rather than
	 * two different distances. A pixel-uniform march would be cheaper and would never miss a thin
	 * occluder, but its "length" would vary with depth. It is the documented refinement once this
	 * version has been measured — never before.
	 *
	 * @note ⚠️ Two structural limits, inherent and not worth "fixing" by adding steps:
	 * - **An occluder that is not on screen casts nothing.** Same ceiling as SSR, where 43.3 % of
	 *   the rays were measured leaving the screen; `maxSteps` bought nothing there and will buy
	 *   nothing here.
	 * - **The depth buffer is a heightfield with no thickness**, hence the `thickness` parameter.
	 *   Too thin leaks light through thin geometry, too thick trails a halo behind the occluder.
	 *
	 * @note It produces the SAME signal as its sibling — R = shadow factor, G = normalized contact
	 * distance — so it joins the same shared denoise group and emits the same combine snippet. A
	 * slot's two occupants must be interchangeable downstream or the chain would have to know
	 * which one is running.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API SSContactShadows final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SSContactShadowsEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot() */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::ContactShadows;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing screen-space contact shadows parameters.
			 * @note The first four are the SAME quantities, in the same units, as
			 * RTContactShadows::Parameters — see the class note on why that matters.
			 */
			struct EMEN_API Parameters
			{
				float maxDistance{2.0F};
				float normalBias{0.01F};
				float intensity{0.8F};
				float maxBlurRadius{10.0F};
				/** @brief How far behind a depth sample the ray may pass and still be occluded, in metres. */
				float thickness{0.25F};
				/** @brief Steps along the march; the step length is maxDistance / stepCount. */
				uint32_t stepCount{16};
			};

			/**
			 * @brief Per-frame data for the screen-space march.
			 * @note A UBO and not push constants: two mat4 alone are 128 bytes, which is the
			 * Vulkan push-constant MINIMUM guarantee, and the parameters push it to 160. Layout is
			 * std140-compatible: mat4 and vec4 members only.
			 * @note ⚠️ Both matrices are the PROJECTION, not the view-projection. The march lives
			 * in view space — the G-buffer normal is already view-space, and a world-space march
			 * would need a view matrix the fragment stage does not have.
			 */
			struct EMEN_API MarchFrameUBOData
			{
				/** @brief Inverse projection: clip to view, for position reconstruction. */
				std::array< float, 16 > inverseProjectionMatrix;
				/** @brief Projection: view to clip, to place each step back on the screen. */
				std::array< float, 16 > projectionMatrix;
				/** @brief xyz = the directional light EMISSION direction in VIEW space, w = maxDistance. */
				std::array< float, 4 > lightParameters;
				/** @brief x = normalBias, y = thickness, z = step count, w = unused. */
				std::array< float, 4 > marchParameters;
			};

			/**
			 * @brief Constructs a screen-space contact shadows effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			SSContactShadows (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a screen-space contact shadows effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			SSContactShadows (Renderer & renderer, const Parameters & parameters) noexcept
				: IndirectPostProcessEffect{renderer},
				m_parameters{parameters}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::producesOverlay() */
			[[nodiscard]]
			bool
			producesOverlay () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::usesSharedDenoise() */
			[[nodiscard]]
			bool
			usesSharedDenoise () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordPreDenoisePasses() */
			void recordPreDenoisePasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::denoiseContribution() */
			[[nodiscard]]
			DenoiseContribution denoiseContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresNormals() */
			[[nodiscard]]
			bool
			requiresNormals () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties()
			 * @note The march itself does not need them; the COMBINE does, to read the material
			 * shadowResponse — exactly like the ray-traced sibling. */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresLightSet() */
			[[nodiscard]]
			bool
			requiresLightSet () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the contact shadows parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current contact shadows parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			Parameters m_parameters;
			/* Intermediate render targets. ⚠️ FULL resolution, unlike the ray-traced sibling: a
			 * contact shadow exists for the fine detail at the point of contact, which is exactly
			 * what a half-res march destroys. The sibling is half-res to amortise the cost of ray
			 * traversal, which a depth march does not pay. The shared denoise pass partitions its
			 * group by extent, so cohabiting with a half-res SSGI/SSAO is already handled. */
			IntermediateRenderTarget m_shadowTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_shadowPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_shadowLayout;
			/** @brief The single set: depth (0), normals (1), per-frame parameters (2). No RT set
			 * and no bindless set — this lane reads nothing but the G-buffer. */
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_shadowInputLayout;
			/* Descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_shadowPerFrame;
			/** @brief Per-frame march parameters (set 0, binding 2). */
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_shadowFrameUBOs;
	};
}
