/*
 * src/Graphics/Effects/Framebuffer/ToneMapping.hpp
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
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Graphics/Photometry.hpp"
#include "Vulkan/Buffer.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Tone mapping post-processing effect that converts HDR to LDR.
	 * @note Supports multiple tone mapping operators: ACES Filmic, Reinhard, and Uncharted 2.
	 * Includes optional auto-exposure (eye adaptation) via luminance measurement and
	 * temporal exponential moving average.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API ToneMapping final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ToneMappingEffect"};

			/**
			 * @brief Available tone mapping operators.
			 */
			enum class EMEN_API Operator : std::uint8_t
			{
				ACESFilmic = 0,
				Reinhard = 1,
				Uncharted2 = 2
			};

			/**
			 * @brief User-facing tone mapping parameters.
			 */
			struct EMEN_API Parameters
			{
				Operator tonemapOperator{Operator::ACESFilmic};
				float exposure{1.0F};
				float gamma{2.2F};
				/* NOTE: Defaults calibrated for sRGB-correct textures (lower linear values
				 * need a higher exposure target) — the values validated in the demo benches.
				 * These are the values used by the camera-materialized instance (enableHDR()). */
				/* MIDDLE GREY. The auto-exposure maps the scene's log-average luminance onto this
				 * value (`autoExposure = keyValue / avgLuminance`), so it IS the photographic key.
				 * It keys on `Photometry::MeteredMiddleGrey` (K=12.5 / (1.2 · 100) ≈ 0.104) — the
				 * value the MANUAL APEX triad lands a correctly metered scene on — so the auto and
				 * manual paths agree and the ISO bounds keep their meaning. History: 0.5 placed the
				 * average ~1.5 stops hot (washed out once photometric); the next value, Reinhard's
				 * 0.18, is a DISPLAY-side grey-card convention, NOT what a K=12.5 reflected-light
				 * meter produces through this engine's own exposure function — it kept the auto
				 * mode 0.79 EV hotter than the same scene shot manually. */
				float keyValue{Photometry::MeteredMiddleGrey};
				float adaptSpeedUp{1.5F};
				float adaptSpeedDown{2.0F};
				/* PHOTOMETRIC RANGE. These clamp the auto-exposure multiplier that maps scene
				 * luminance to the display range, so they must span the physical range of the
				 * content: a sunlit white surface is ~30000 nits and needs ~1e-5, while a moonlit
				 * interior is a fraction of a nit and needs to be lifted. The previous 0.1 floor
				 * was calibrated for arbitrary light units, and it made every photometric scene
				 * saturate against it — the metering asked for 0.001 and got 0.1, i.e. 100x too
				 * bright, with 44% of the frame blown to white. */
				float minExposure{1.0e-5F};
				float maxExposure{100.0F};
				bool autoExposureEnabled{true};
			};

			/**
			 * @brief Push constants sent to the tone mapping shader (no auto-exposure).
			 */
			struct EMEN_API ToneMappingPushConstants
			{
				float exposure;
				float gamma;
				uint32_t tonemapOperator;
				float padding;
			};

			/**
			 * @brief Constructs a tone mapping effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			ToneMapping (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a tone mapping effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			ToneMapping (Renderer & renderer, const Parameters & parameters) noexcept
				: IndirectPostProcessEffect{renderer},
				m_parameters{parameters}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/**
			 * @brief Returns whether this effect needs a high dynamic range input. It does, by
			 * definition: converting linear HDR radiance into a display-referred image IS what
			 * this effect is for.
			 * @note This MUST be declared. The scene render target's colour format is chosen from
			 * the stack's aggregate requirement (`Renderer::recreateSceneTarget()` via
			 * `PostProcessor::cachedRequiresHDR()`), and the requirement is re-evaluated whenever
			 * the camera materializes or retires an effect. While this returned the inherited
			 * `false`, the HDR buffer was only held up by whichever OTHER effect happened to be
			 * enabled (bloom, motion blur, TAA, SSR...). Turning the last of them off dropped the
			 * scene target to the 8-bit swap-chain format underneath a still-active tone mapper,
			 * so photometric radiance of a few thousand nits clamped to 1.0 everywhere — a
			 * uniformly grey/white frame, which is exactly the failure the on-demand chain in
			 * `Renderer::render()` was written to prevent.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the tone mapping parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current tone mapping parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Returns the sensitivity (ISO) the auto-exposure metering landed on.
			 * @note Read back from the GPU adaptation history with `framesInFlight` frames of
			 * latency (one tiny host-visible slot per frame in flight, zero stall — the slot is
			 * only read once its fence has passed). 0 until a measurement completed, and reset
			 * when the auto-exposure is off (there is no metering to report then).
			 * @warning RENDER THREAD contract: written by execute(), intended for the overlay
			 * panel which draws inside the same frame scope.
			 * @return float
			 */
			[[nodiscard]]
			float
			meteredSensitivity () const noexcept
			{
				return m_meteredSensitivity;
			}

			/**
			 * @brief Returns the metered scene average luminance, in nits (cd/m²).
			 * @note Same readback and thread contract as meteredSensitivity(); 0 until valid.
			 * @return float
			 */
			[[nodiscard]]
			float
			meteredLuminance () const noexcept
			{
				return m_meteredLuminance;
			}

			/**
			 * @brief Returns how many metered frames the adaptation pass REJECTED as implausible.
			 * @note The adaptation pass validates its measurement against a physical
			 * log-luminance window and holds the previous adapted value instead of feeding an
			 * out-of-range one into the EMA — the filter is infinite-impulse, so a single
			 * non-finite sample used to poison it permanently and pin the auto-exposure against
			 * the ISO ceiling (frame blown white until the effect was recreated).
			 * @note A count that keeps GROWING is a diagnostic, not a cosmetic detail: it means
			 * the luminance chain is sampling implausible data every frame. On macOS that is the
			 * fingerprint of the video-memory corruption that also manifests as green blocks;
			 * this counter is the cheapest detector for it short of a GPU capture. Same readback
			 * latency and RENDER THREAD contract as meteredSensitivity(). Reset on (re)creation.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			meteredRejectedCount () const noexcept
			{
				return m_meteredRejectedCount;
			}

		private:

			/** @brief One host-visible readback slot per frame in flight (metered exposure). */
			struct MeteredReadbackSlot
			{
				std::unique_ptr< Vulkan::Buffer > buffer;
				const uint8_t * mappedPtr{nullptr};
				bool pending{false};
			};

			/**
			 * @brief Creates all graphics pipelines.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipelines () noexcept;

			/**
			 * @brief Creates all descriptor sets.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSets () noexcept;

			Parameters m_parameters;
			/* Standard tone mapping output (LDR). */
			IntermediateRenderTarget m_outputTarget;
			/* Standard tone mapping pipeline (no auto-exposure). */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			/* Auto-exposure: luminance downsample chain (half-res -> 1x1, R16G16B16A16_SFLOAT). */
			std::vector< std::unique_ptr< IntermediateRenderTarget > > m_lumTargets;
			/* Auto-exposure: adaptation ping-pong (two 1x1 R16G16B16A16_SFLOAT targets). */
			std::array< IntermediateRenderTarget, 2 > m_adaptTargets;
			uint32_t m_currentAdaptIndex{0};
			/* Auto-exposure: pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_lumExtractPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_lumDownsamplePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_adaptPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_autoExposurePipeline;
			/* Auto-exposure: pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_adaptPipelineLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_autoExpPipelineLayout;
			/* Auto-exposure: descriptor sets.
			 * m_lumDownDescSets are fixed (internal chain).
			 * Per-frame sets are updated each frame to handle external input and ping-pong. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_lumDownDescSets;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_adaptPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_autoExpDescPerFrame;
			/* Metered-exposure readback ring (auto-ISO display): slot N is written by the GPU
			 * during frame N and read back when slot N comes around again — framesInFlight
			 * frames of latency, zero stall. Persistently mapped. RENDER THREAD ONLY. */
			std::vector< MeteredReadbackSlot > m_meteredReadback;
			float m_meteredSensitivity{0.0F};
			float m_meteredLuminance{0.0F};
			/* Number of metered frames whose measurement was rejected as implausible — a growing
			 * value means the luminance chain is reading corrupt data (see meteredRejectedCount()). */
			uint32_t m_meteredRejectedCount{0};
			/* Auto-exposure: true until the first adaptation pass ran — drives the shader-side
			 * history reset (the frame delta itself comes from PushConstants::deltaTime). */
			bool m_firstFrame{true};
	};
}
