/*
 * src/Graphics/Effects/Framebuffer/VolumetricLight.hpp
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
#include <optional>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Screen-space volumetric light (god rays) post-processing effect.
	 * @note Extracts an occlusion mask from the depth buffer, applies a radial blur
	 * centered on the projected sun position, and additively composites the result.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API VolumetricLight final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VolumetricLightEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note Order-insensitive by construction (a pure additive combine, no chain sampling); the slot is a convention. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::VolumetricLight;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing volumetric light parameters.
			 */
			struct EMEN_API Parameters
			{
				float density{1.0F};
				float decay{0.975F};
				float exposure{0.25F};
				uint32_t numSamples{64};
				float depthThreshold{0.9999F};
				/* Temporal EMA weight of the occlusion mask (1 = no accumulation).
				 * A source narrower than a pixel (a door slit) RASTERIZES differently at
				 * every TAA jitter offset: its flux in the depth buffer genuinely
				 * oscillates with the Halton phase and the radial march integrates that
				 * into a streak vibration. Stable sampling cannot fix a source that
				 * really changes — averaging over the jitter cycle can. 0.2 ≈ 8 frames. */
				float temporalAlpha{0.2F};
			};

			/**
			 * @brief Push constants for the occlusion and radial blur passes.
			 */
			struct EMEN_API ScatterPushConstants
			{
				float lightScreenX;
				float lightScreenY;
				float texelSizeX;
				float texelSizeY;
				float nearPlane;
				float farPlane;
				float lightColorR;
				float lightColorG;
				float lightColorB;
				float lightIntensity;
				float density;
				float decay;
				float exposure;
				float depthThreshold;
				uint32_t numSamples;
				float lightOnScreen;
				/* TAA sub-pixel jitter of the frame, in UV units (NDC jitter x 0.5).
				 * The occlusion pass samples the depth at vUV + jitterUV so the mask
				 * silhouette stays phase-stable (the buffer content is shifted BY the
				 * jitter — frame-history contract, same convention as the TAA resolve). */
				float jitterUVX;
				float jitterUVY;
				/* EMA weight of the occlusion mask (forced to 1 on the first frame). */
				float temporalAlpha;
			};

			/**
			 * @brief Constructs a volumetric light effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			VolumetricLight (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a volumetric light effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			VolumetricLight (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordOverlayPasses() */
			void recordOverlayPasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
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
			 * @brief Overrides the light color (instead of reading from LightSet).
			 * @param color The override color.
			 * @return void
			 */
			void
			setLightColorOverride (const Base::PixelFactory::Color<> & color) noexcept
			{
				m_lightColorOverride = color;
			}

			/**
			 * @brief Clears the light color override (reverts to LightSet value).
			 * @return void
			 */
			void
			clearLightColorOverride () noexcept
			{
				m_lightColorOverride.reset();
			}

			/**
			 * @brief Overrides the light intensity (instead of reading from LightSet).
			 * @param intensity The override intensity.
			 * @return void
			 */
			void
			setLightIntensityOverride (float intensity) noexcept
			{
				m_lightIntensityOverride = intensity;
			}

			/**
			 * @brief Clears the light intensity override (reverts to LightSet value).
			 * @return void
			 */
			void
			clearLightIntensityOverride () noexcept
			{
				m_lightIntensityOverride.reset();
			}

			/**
			 * @brief Sets the volumetric light parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current volumetric light parameters.
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
			/* Intermediate render targets. The occlusion mask is a PING-PONG pair: the
			 * pass blends the current binary test with the previous frame's mask (EMA)
			 * to average the real flux oscillation of sub-pixel sources under the TAA
			 * jitter (see Parameters::temporalAlpha). */
			std::array< IntermediateRenderTarget, 2 > m_occlusionTargets;
			IntermediateRenderTarget m_radialTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_occlusionPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_radialPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_occlusionLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_radialLayout;
			/* Descriptor sets (all per-frame: the ping-pong bindings rotate). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_occlusionPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_radialPerFrame;
			/* Optional overrides (nullopt = read from LightSet at execute time). */
			std::optional< Base::PixelFactory::Color<> > m_lightColorOverride;
			std::optional< float > m_lightIntensityOverride;
			/* Ping-pong index of the occlusion mask written THIS frame. */
			uint32_t m_occlusionWriteIndex{0};
			/* False until a first frame filled the mask history (forces alpha = 1). */
			bool m_historyValid{false};
	};
}
