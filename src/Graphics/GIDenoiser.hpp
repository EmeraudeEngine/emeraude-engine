/*
 * src/Graphics/GIDenoiser.hpp
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
#include "IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "IntermediateRenderTarget.hpp"
#include "Vulkan/UniformBufferObject.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The shared temporal denoiser of the diffuse GI producers (SVGF work site).
	 * @note One instance is OWNED by each GI effect (RTGI, later SSGI): the code is shared,
	 * the histories are not — two producers reprojecting into one history would corrupt each
	 * other. The component owns the temporal resolve (velocity reprojection + dilation,
	 * camera-distance/world-normal disocclusion, variance clipping, EMA), the history
	 * ping-pong pair, the world-normal history and the per-frame frame-data UBO the owner
	 * also binds into its trace pass. The owner records its noisy estimate, then delegates
	 * the resolve; the returned texture feeds its combine snippet.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect Reuses the shared fullscreen
	 * pass infrastructure — it is never inserted into a stack itself.
	 */
	class EMEN_API GIDenoiser final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GIDenoiser"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note An INTERNAL component: owned by the effect that uses it (its pipeline helpers come from this base), never filed into a chain — PostProcessStack::addEffect() refuses this slot. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::Internal;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief Per-frame UBO shared by the owner's trace pass and the denoiser passes.
			 * @note With the previous-frame matrices the data exceeds the 128-byte Vulkan
			 * push constant minimum guarantee (maxPushConstantsSize), hence a UBO.
			 * Layout is std140-compatible (mat4 and vec4 members only).
			 */
			struct EMEN_API FrameUBOData
			{
				std::array< float, 16 > invViewProj;
				std::array< float, 16 > prevViewProj;
				std::array< float, 3 > invViewCol0;
				float viewPosX;
				std::array< float, 3 > invViewCol1;
				float viewPosY;
				std::array< float, 3 > invViewCol2;
				float viewPosZ;
				std::array< float, 4 > prevCamPos;
				/* maxDistance, bias, sampleCount (as float), animated-noise frame index (R2). */
				std::array< float, 4 > traceParams;
				/* alpha, depthTolerance, normalThreshold, flags (bit 0 = variance clip,
				 * bit 1 = animated noise, bit 2 = 1/N accumulation counter). */
				std::array< float, 4 > temporalParams;
				/* strength, clamp, variance-clip gamma, accumulation cap N. */
				std::array< float, 4 > bounceParams;
				/* sky luminance in nits (0 = no sky), sky ray distance, unused, unused. */
				std::array< float, 4 > skyParams;
			};

			/**
			 * @brief Denoiser parameters (SVGF temporal resolve + à-trous filter).
			 * @note Set by the owner BEFORE create() (VRAM gating) — the scalar parameters
			 * travel to the shaders through the frame UBO / push constants, so
			 * setParameters() also works at runtime.
			 */
			struct EMEN_API Parameters
			{
				/* Depth edge-stopping sigma (gaussian on the raw depth difference). */
				float depthSigma{1.0F};
				/* Normal edge-stopping sigma (pow(dot, 1/sigma)). */
				float normalSigma{0.5F};
				/* Luminance edge-stopping sigma, normalised by the LOCAL standard deviation
				 * (SVGF auto-dosage: noisy pixel → wide tolerance → smooth hard; converged
				 * pixel → tight tolerance → preserve detail). SVGF paper default: 4. */
				float luminanceSigma{4.0F};
				/* À-trous iterations (5x5 kernel, footprint doubles each pass: 1,2,4,8,16).
				 * 0 = no spatial filtering (temporal resolve only — A/B lever). */
				uint32_t atrousIterations{4};
				/* Fixed temporal blend weight — only rules when accumulationCounter is off. */
				float temporalAlpha{0.1F};
				/* Relative camera-distance tolerance of the disocclusion test. */
				float temporalDepthTolerance{0.05F};
				/* Minimum world-normal dot of the disocclusion test. */
				float temporalNormalThreshold{0.8F};
				/* Variance-clipping width in standard deviations (neighborhoodClamp). */
				float temporalVarianceGamma{1.0F};
				/* SVGF 1/N accumulation cap (steady-state blend weight floor = 1/N). */
				uint32_t maxAccumulation{64};
				/* Rectify the history against the raw 3x3 statistics. Default OFF: on the
				 * RAW input it pulls the history toward the noisy local distribution
				 * (measured ~5% GI energy loss for no stability gain). */
				bool temporalNeighborhoodClamp{false};
				/* Advance the producer's noise seed along the R2 sequence every frame. */
				bool temporalAnimatedNoise{true};
				/* Per-pixel 1/N blend weight instead of the fixed temporalAlpha. */
				bool accumulationCounter{true};
			};

			/**
			 * @brief Producer-provided scalars of the frame.
			 * @note The denoiser assembles the matrices and its own temporal parameters;
			 * the owner only supplies what its TRACE shader consumes. Producers without a
			 * feedback loop or a sky term leave the corresponding fields at zero.
			 */
			struct EMEN_API FrameInputs
			{
				float traceMaxDistance{0.0F};
				float traceBias{0.0F};
				float traceSampleCount{0.0F};
				/* Multi-bounce feedback (gated internally on history validity). */
				float bounceStrength{0.0F};
				float bounceClamp{0.0F};
				/* Sky luminance in nits (0 = no sky term) and sky ray distance. */
				float skyLuminance{0.0F};
				float skyDistance{0.0F};
			};

			/**
			 * @brief Push constants of the à-trous filter pass.
			 */
			struct EMEN_API AtrousPushConstants
			{
				/* Texel stride of this iteration (1, 2, 4, 8, 16). */
				float stepSize;
				float depthSigma;
				float normalSigma;
				float luminanceSigma;
				/* > 0.5: first iteration — variance comes from the moments texture (with the
				 * young-history spatial fallback), the input alpha is the camera distance. */
				float firstIteration;
				float padding0;
				float padding1;
				float padding2;
			};

			/**
			 * @brief Constructs a GI denoiser component.
			 * @param renderer A reference to the graphics renderer.
			 * @param ownerLabel The owning effect's ClassId, prefixed to every GPU object name.
			 */
			GIDenoiser (Renderer & renderer, const char * ownerLabel) noexcept
				: IndirectPostProcessEffect{renderer},
				m_ownerLabel{ownerLabel}
			{

			}

			/**
			 * @brief Sets the denoiser parameters.
			 * @note Call BEFORE create() so the à-trous targets are (not) allocated to match
			 * atrousIterations; the sigmas alone may change at runtime.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current denoiser parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Enables or disables the temporal chain BEFORE create().
			 * @note When disabled, create() allocates the frame UBOs only (no history VRAM,
			 * no pipelines) and recordResolve() passes the noisy input through unchanged.
			 * @param state The desired state.
			 * @return void
			 */
			void
			setTemporalEnabled (bool state) noexcept
			{
				m_temporalEnabled = state;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create()
			 * @note Width/height are the OWNER's working resolution (half-res unless the
			 * owner is full-res gated). */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/**
			 * @brief Returns whether the temporal chain is enabled AND its GPU objects exist.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			temporalActive () const noexcept
			{
				return m_temporalEnabled && m_temporalPipeline != nullptr;
			}

			/**
			 * @brief Returns whether the history holds a valid resolved frame.
			 * @note False until the first resolve after (re)creation: the owner must force
			 * alpha to 1 and disable any history feedback for that frame.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			historyUsable () const noexcept
			{
				return this->temporalActive() && m_historyValid;
			}

			/**
			 * @brief Returns the current frame index of the animated-noise R2 sequence.
			 * @note Advanced once per updateFrameData() call, wraps at 4096 (exact in float32).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			noiseFrameIndex () const noexcept
			{
				return m_noiseFrameIndex;
			}

			/**
			 * @brief Returns the history texture READ this frame (previous resolved frame).
			 * @note Only meaningful when temporalActive(); the owner's trace binds it for
			 * the multi-bounce feedback. Stable until the flip inside recordResolve().
			 * @return const Vulkan::TextureInterface &
			 */
			[[nodiscard]]
			const Vulkan::TextureInterface &
			historyReadTexture () const noexcept
			{
				return m_historyTargets[1U - m_historyWriteIndex];
			}

			/**
			 * @brief Returns the moments texture WRITTEN this frame (m1/m2 of the raw estimate's
			 * luminance, accumulation age, camera distance).
			 * @note Only meaningful when temporalActive(), after recordResolve(). The variance
			 * is derived at the read site: max(m2 - m1*m1, 0). Feeds the variance-guided
			 * à-trous weights (SVGF stage 2) and the debug views.
			 * @return const Vulkan::TextureInterface &
			 */
			[[nodiscard]]
			const Vulkan::TextureInterface &
			momentsTexture () const noexcept
			{
				/* recordResolve() flipped m_historyWriteIndex AFTER writing: the freshly
				 * written moments now sit at [1 - writeIdx]. */
				return m_momentsTargets[1U - m_historyWriteIndex];
			}

			/**
			 * @brief Returns the per-frame frame-data UBO, for the owner's own descriptor sets.
			 * @param frameIndex The frame-in-flight index.
			 * @return const Vulkan::UniformBufferObject &
			 */
			[[nodiscard]]
			const Vulkan::UniformBufferObject &
			frameUBO (uint32_t frameIndex) const noexcept
			{
				return *m_frameUBOs[frameIndex];
			}

			/**
			 * @brief Assembles this frame's UBO (matrices from the renderer's view state,
			 * temporal parameters from Parameters, trace scalars from the owner), writes it
			 * and advances the animated-noise sequence.
			 * @param frameIndex The frame-in-flight index.
			 * @param context The per-frame chain context.
			 * @param inputs The owner's trace scalars.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateFrameData (uint32_t frameIndex, const FrameContext & context, const FrameInputs & inputs) noexcept;

			/**
			 * @brief Returns the combine contribution of the denoiser DEBUG views, drawn
			 * INSTEAD of the owner's GI contribution.
			 * @note Binary-amplified — a linear scale is unreadable under the photometric
			 * exposure (the tone mapper is a camera sensor, not a data scope). Only
			 * meaningful when temporalActive().
			 * @param prefix The owner's combine prefix (e.g. "rtgi").
			 * @param mode 1 = temporal variance (x1e6, bounded), 2 = accumulation age.
			 * @return IndirectPostProcessEffect::CombineContribution
			 */
			[[nodiscard]]
			CombineContribution debugCombineContribution (const char * prefix, uint32_t mode) const noexcept;

			/**
			 * @brief Records the whole denoise chain: temporal resolve + moments accumulation
			 * of the RAW estimate, normal-history retention, then the variance-guided à-trous
			 * iterations (SVGF order — temporal integration FIRST, spatial filtering on the
			 * integrated signal).
			 * @note Called outside any active render pass, after the owner's raw estimate is
			 * complete. Flips the history ping-pong. The colour history fed back to the owner
			 * (multi-bounce) is the TEMPORAL output, before the à-trous (v1 decision — the
			 * SVGF first-iteration feedback is a later candidate). When the temporal chain is
			 * off, records nothing and returns the raw input unchanged (diagnostic mode: the
			 * à-trous variance guide REQUIRES the temporal moments).
			 * @param commandBuffer A reference to the active command buffer.
			 * @param rawInput The owner's RAW estimate (trace output, before any filtering).
			 * @param context The per-frame chain context.
			 * @return const Vulkan::TextureInterface * The texture the owner's combine must consume.
			 */
			[[nodiscard]]
			const Vulkan::TextureInterface * recordResolve (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & rawInput, const FrameContext & context) noexcept;

		private:

			/** @brief The owning effect's ClassId (GPU object name prefix). */
			const char * m_ownerLabel;
			Parameters m_parameters;
			/* Temporal history (owner resolution, ping-pong): RGB = resolved indirect
			 * irradiance, A = camera distance of the pixel (0 = invalid/sky). Plus the
			 * world-space normal history used for disocclusion rejection. */
			std::array< IntermediateRenderTarget, 2 > m_historyTargets;
			std::array< IntermediateRenderTarget, 2 > m_normalHistoryTargets;
			/* Per-pixel moments of the RAW estimate's luminance (ping-pong, RGBA16F):
			 * R = m1 (mean), G = m2 (mean of squares) — temporal variance = m2 - m1²,
			 * B = accumulation age in frames (saturates at 64; reset on disocclusion —
			 * the SVGF 1/N accumulation counter reuses this channel),
			 * A = camera distance of the pixel (0 = invalid/sky), like the colour history. */
			std::array< IntermediateRenderTarget, 2 > m_momentsTargets;
			/* À-trous working pair (ping-pong across iterations, RGBA16F): RGB = filtered
			 * irradiance, A = filtered variance (w² propagation rule). */
			std::array< IntermediateRenderTarget, 2 > m_atrousTargets;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_temporalPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_momentsPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_normalCopyPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_atrousPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_temporalLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_momentsLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_normalCopyLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_atrousLayout;
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_temporalPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_momentsPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_normalCopyPerFrame;
			/* À-trous sets, one flavour per INPUT: [0] reads the freshly resolved history
			 * (first iteration), [1] reads atrous[0], [2] reads atrous[1]. */
			std::array< std::vector< std::unique_ptr< Vulkan::DescriptorSet > >, 3 > m_atrousPerFrame;
			/* Per-frame UBOs shared by the owner's trace and the denoiser passes. */
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_frameUBOs;
			/* Ping-pong index of the history buffer written THIS frame. */
			uint32_t m_historyWriteIndex{0};
			/* Frame index of the animated-noise R2 sequence (advances once per recorded
			 * frame, wraps at 4096 to stay exact in float32). */
			uint32_t m_noiseFrameIndex{0};
			/* Set by setTemporalEnabled() BEFORE create(). */
			bool m_temporalEnabled{true};
			/* False until a first frame filled the history (forces alpha=1, no feedback). */
			bool m_historyValid{false};
	};
}
