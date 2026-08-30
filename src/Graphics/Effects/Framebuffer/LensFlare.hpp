/*
 * src/Graphics/Effects/Framebuffer/LensFlare.hpp
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

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Light-aware lens flare post-processing effect.
	 * @note Projects a dominant light source to screen-space, then generates
	 * ghost copies along the light-to-center axis with chromatic distortion,
	 * adds a halo ring around the light position, and composites additively
	 * with the scene.
	 * @note The source's VISIBILITY is probed in the depth buffer (Aug 2026): 16 taps on a small
	 * disk around the projected light; a tap that does not read the far plane is geometry hiding
	 * the source. Until then the flare only knew whether the light was inside the frustum and shone
	 * through every wall (owner report on Sponza: "il passe à travers la géométrie").
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API LensFlare final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LensFlareEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note Reads the chain colour for its bright pass: everything that can be bright must have run. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::LensFlare;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing lens flare parameters.
			 */
			struct EMEN_API Parameters
			{
				float threshold{0.8F};
				float softKnee{0.5F};
				int32_t ghostCount{4};
				float ghostSpacing{0.3F};
				float haloRadius{0.6F};
				float haloThickness{0.1F};
				float chromaticDistortion{0.02F};
				float intensity{1.0F};
				/**
				 * @brief Radius, as a fraction of the screen HEIGHT, of the depth-buffer disk probed
				 * around the light's projected position to decide how much of it the geometry hides.
				 * @note 16 taps; the fraction of taps that read the far plane is the visibility. A
				 * source behind a wall produces NO flare; a source half behind an edge, half of it.
				 */
				float occlusionRadius{0.012F};
			};

			/**
			 * @brief Push constants for the threshold pass.
			 */
			struct EMEN_API ThresholdPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float threshold;
				float softKnee;
			};

			/**
			 * @brief Push constants for the ghost + halo pass.
			 */
			struct EMEN_API GhostHaloPushConstants
			{
				float lightScreenX;
				float lightScreenY;
				float ghostSpacing;
				float haloRadius;
				float haloThickness;
				float chromaticDistortion;
				float intensity;
				int32_t ghostCount;
				/* Occlusion probe radius in UV, per axis (the screen is not square). */
				float occlusionRadiusX;
				float occlusionRadiusY;
			};

			/**
			 * @brief Constructs a lens flare effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			LensFlare (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a lens flare effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			LensFlare (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::readsChainColorUpstream() */
			[[nodiscard]]
			bool
			readsChainColorUpstream (const FrameContext & /*context*/) const noexcept override
			{
				/* The threshold pass samples the chain color to extract bright spots. */
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordOverlayPasses() */
			void recordOverlayPasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
						[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth()
			 * @note The ghost + halo pass probes the scene depth around the projected light: the source occlusion. */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
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
			 * @brief Sets the lens flare parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current lens flare parameters.
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
			/* Intermediate render targets. */
			IntermediateRenderTarget m_thresholdTarget;
			IntermediateRenderTarget m_ghostHaloTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_thresholdPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_ghostHaloPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_thresholdLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_ghostHaloLayout;
			/* Descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_thresholdPerFrame;
			/* Ghost + halo: binding 0 = threshold target (fixed), binding 1 = scene depth (per frame,
			 * the occlusion probe). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_ghostHaloPerFrame;
			/* Light visibility factor computed by recordOverlayPasses(), consumed by
			 * combineContribution() as the flare modulation (dynamics0.x). */
			float m_lastLightOnScreen{0.0F};
	};
}
