/*
 * src/Graphics/Effects/Lighting/SSGI.hpp
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
#include "Graphics/GIDenoiser.hpp"
#include "Graphics/IntermediateRenderTarget.hpp"

namespace EmEn::Graphics::Effects::Lighting
{
	/**
	 * @brief Screen-Space Global Illumination (SSGI) post-processing effect.
	 * @note The COMPLETE indirect-diffuse estimator of the screen-space lane: the sky and the
	 * bounce, exactly like RTGI in the traced lane.
	 * - The BOUNCE: cosine-weighted hemisphere rays marched through the depth buffer; on hit, the
	 *   scene color at the hit point is the indirect radiance (color bleeding).
	 * - The SKY (Sep 2026): a GTAO horizon search measures how much of the sky each pixel actually
	 *   sees, and the baked irradiance cubemap sampled along the resulting BENT NORMAL gives the
	 *   irradiance that visibility lets through. Without it this effect claimed no sky, the scene
	 *   kept handing the raster its full unoccluded irradiance cubemap, and an enclosed space came
	 *   out 4.6x brighter than the traced lane (measured on Sponza, 2026-09-13).
	 * @note That sky term is what makes this effect an indirect-diffuse PROVIDER
	 * (providesIndirectDiffuse()): the scene drops the raster's own diffuse IBL leg to 0 and the
	 * sky is composited ONCE, by whoever measures its visibility.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API SSGI final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SSGIEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note The screen-space alternative of the same concept — never live at the same time as RTGI, which the slot now enforces. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::IndirectDiffuse;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing SSGI parameters.
			 */
			struct EMEN_API Parameters
			{
				float maxDistance{5.0F};
				float intensity{0.8F};
				float thickness{0.5F};
				uint32_t sampleCount{8};
				uint32_t stepCount{16};
				/* Sky visibility (GTAO horizon search). The radius is the SKY occluder range and
				 * has nothing to do with maxDistance, which is the bounce range — see
				 * SettingKeys.hpp. */
				float skyVisibilityRadius{16.0F};
				float skyVisibilityFalloffRange{0.2F};
				uint32_t skyVisibilitySliceCount{3};
				uint32_t skyVisibilityStepCount{6};
				/* À-trous edge-stopping sigmas + iteration count (GIDenoiser::Parameters). */
				float depthSigma{1.0F};
				float normalSigma{0.5F};
				float luminanceSigma{4.0F};
				uint32_t atrousIterations{4};
				/* GIDenoiser temporal resolve (SSGI's first temporal accumulation). */
				float temporalAlpha{0.1F};
				float temporalDepthTolerance{0.05F};
				float temporalNormalThreshold{0.8F};
				float temporalVarianceGamma{1.0F};
				uint32_t denoiserMaxAccumulation{64};
				/* Denoiser debug view (combine draws it INSTEAD of the GI): 0 = off,
				 * 1 = temporal variance, 2 = accumulation age. */
				uint32_t denoiserDebugView{0};
				bool skyVisibilityEnabled{true};
				bool denoiserAccumulationCounter{true};
				bool temporalEnabled{true};
				bool temporalNeighborhoodClamp{false};
				bool temporalAnimatedNoise{true};
			};

			/**
			 * @brief Push constants for the SSGI trace pass.
			 */
			struct EMEN_API TracePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				float aspectRatio;
				float maxDistance;
				float thickness;
				uint32_t sampleCount;
				uint32_t stepCount;
				/* Animated-noise frame index of the R2 sequence (< 0 = frozen pattern). */
				float noiseFrameIndex;
				/* Luminance of the sky in nits (0 = no sky): the scale of the irradiance cubemap,
				 * the SAME value the raster leg this effect takes over applied
				 * (ViewUB EnvironmentLuminance). Read by the sky-visibility variant only. */
				float skyLuminance;
			};

			/**
			 * @brief Push constants for the sky-visibility pass (GTAO horizon search).
			 * @note The three inverse-view columns carry the view -> world rotation the bent normal
			 * needs to sample the irradiance cubemap; their unused w components carry the scalars
			 * that would otherwise pad the block.
			 */
			struct EMEN_API HorizonPushConstants
			{
				/* xyz = inverse view rotation column 0, w = search radius in world units. */
				float invViewCol0[4];
				/* xyz = inverse view rotation column 1, w = falloff range, as a fraction of the radius. */
				float invViewCol1[4];
				/* xyz = inverse view rotation column 2, w = animated-noise frame index (< 0 = frozen). */
				float invViewCol2[4];
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				float aspectRatio;
				uint32_t sliceCount;
				uint32_t stepCount;
			};

			/**
			 * @brief Constructs a screen-space global illumination effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			SSGI (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer},
				m_denoiser{renderer, ClassId}
			{

			}

			/**
			 * @brief Constructs a screen-space global illumination effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			SSGI (Renderer & renderer, const Parameters & parameters) noexcept
				: IndirectPostProcessEffect{renderer},
				m_denoiser{renderer, ClassId},
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
				/* The trace pass gathers the indirect bounce radiance from the chain color. */
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordOverlayPasses()
			 * @note SSGI left the shared H/V DenoisePass with the SVGF chain: the whole
			 * internal chain (trace → temporal resolve on the RAW trace → moments →
			 * variance-guided à-trous) records here, through the owned GIDenoiser —
			 * SSGI's FIRST temporal accumulation. */
			void recordOverlayPasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::providesIndirectDiffuse()
			 * @note True as soon as the sky-visibility pass is live: the effect then composites the
			 * sky irradiance itself, with the visibility it measures, and the scene switches its
			 * ambient pass' diffuse IBL leg off (Scene::updateIBLDiffuseOwnership()).
			 * ⚠️ Gated on the SAME conditions the pass actually runs on — created, and with the
			 * bindless irradiance cubemap available — for the reason RTGI documents: claiming the
			 * indirect diffuse while nothing composites it leaves the frame with NO sky at all. */
			[[nodiscard]]
			bool providesIndirectDiffuse () const noexcept override;

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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties() */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresAlbedo() */
			[[nodiscard]]
			bool
			requiresAlbedo () const noexcept override
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresVelocity()
			 * @note The GIDenoiser temporal resolve reprojects through the velocity buffer. */
			[[nodiscard]]
			bool
			requiresVelocity () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the SSGI parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current SSGI parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/* The temporal denoiser component (history ping-pong, temporal resolve,
			 * moments, à-trous) — shared code with the other GI producers. */
			GIDenoiser m_denoiser;
			Parameters m_parameters;
			/* IRT: trace (half-res). The denoiser owns everything downstream. */
			IntermediateRenderTarget m_traceTarget;
			/* IRT: sky visibility (half-res, RGBA16F) — xyz = WORLD-space bent normal, w = the
			 * cosine-weighted visibility of the hemisphere. Written before the trace, read by it. */
			IntermediateRenderTarget m_horizonTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tracePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_horizonPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_traceLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_horizonLayout;
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_horizonPerFrame;
			/* Texture consumed by this frame's combine snippet: the denoiser output when
			 * the temporal chain is active, the raw trace otherwise. Set by
			 * recordOverlayPasses() every frame. */
			const Vulkan::TextureInterface * m_combineSource{nullptr};
			/* Whether the sky-visibility half of the estimator is live: the user setting AND the
			 * bindless irradiance cubemap this effect samples. Decided ONCE, by create(), because
			 * it selects the trace shader variant and the pipeline layout.
			 * ⚠️ Read by providesIndirectDiffuse() from the LOGIC thread while create() writes it
			 * on the render thread — the same benign race as the effects' enabled flags, and with
			 * the same worst case: one frame of the previous ownership weight. */
			bool m_skyVisibilityActive{false};
	};
}
