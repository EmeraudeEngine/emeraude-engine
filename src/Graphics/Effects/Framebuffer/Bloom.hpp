/*
 * src/Graphics/Effects/Framebuffer/Bloom.hpp
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
#include <array>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief HDR Bloom post-processing effect using a multi-pass Dual Kawase approach.
	 * @note The algorithm performs a progressive downsample (13-tap) with brightness threshold,
	 * followed by an upsample (tent filter) with additive blending, and a final composite.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API Bloom final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BloomEffect"};

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/** @brief Number of mip levels in the downsample/upsample chain. */
			static constexpr int MipLevels = 5;

			/**
			 * @brief User-facing bloom parameters.
			 */
			struct EMEN_API Parameters
			{
				/** @brief Brightness above which a pixel glows, in NITS (cd/m²).
				 * @note This effect runs on the HDR scene BEFORE tone mapping, so the threshold is
				 * an absolute scene luminance, not a display value. Now that scenes are
				 * photometric that matters enormously: the old default of 1.0 meant "anything
				 * above one nit", i.e. every lit surface in the frame, so the whole image bloomed.
				 * Reference points to choose one: a wall under a lit interior sits around 15-30
				 * nits, an overcast sky 8000, a bare lamp or the sun far above. Pick it per scene
				 * — a night scene glows from a 10-nit torch-lit wall, a daylight one must not. */
				float threshold{1000.0F};
				float softKnee{0.5F};
				/** @brief Fraction of the above-threshold energy the lens scatters (physical:
				 * a clean modern lens 2-5%). Mirrors the Camera default. */
				float intensity{0.03F};
				float spread{1.0F};
			};

			/**
			 * @brief Push constants sent to all bloom shader passes.
			 */
			struct EMEN_API BloomPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float threshold;
				float softKnee;
				float intensity;
				float spread;
				/** @brief Anti-firefly ceiling, in nits: max(threshold, 1) * 64 — the six stops of
				 * differentiation headroom the LDR-era constant 64 provided above a threshold of 1.
				 * The SAME ceiling is sent to every downsample mip: a fixed 64 there re-crushed the
				 * whole photometric range (a 1200-nit lamp and the sun bloomed identically). */
				float fireflyClamp;
				/** @brief 1 on the first downsample pass only: Karis anti-firefly weighting +
				 * threshold extraction + material-properties modulation. An EXPLICIT flag — the
				 * old discriminator was `threshold > 0`, which silently disabled all three on a
				 * legal threshold of zero ("everything glares"). */
				uint32_t firstPass;
			};

			/**
			 * @brief Constructs a Bloom effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			Bloom (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a Bloom effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			Bloom (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties() */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
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

			/**
			 * @brief Sets the bloom parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current bloom parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Bypasses the full-resolution composite pass.
			 * @note Set by PostProcessStack::syncCameraEffects() when the camera ToneMapping
			 * consumes bloomTexture() directly (it adds the glare inside its own pass): the
			 * chain color then passes through this effect untouched and the composite pass
			 * is not paid. A standalone Bloom (no tone mapping downstream) keeps compositing.
			 * @param state The bypass state.
			 * @return void
			 */
			void
			setCompositeBypassed (bool state) noexcept
			{
				m_compositeBypassed = state;
			}

			/**
			 * @brief Returns whether the composite pass is bypassed by a downstream consumer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCompositeBypassed () const noexcept
			{
				return m_compositeBypassed;
			}

			/**
			 * @brief Returns the final blurred glare texture (top of the upsample chain, half-res).
			 * @note Only meaningful after create() and an execute() this frame; consumed by the
			 * camera ToneMapping when the composite is bypassed.
			 * @return const Vulkan::TextureInterface &
			 */
			[[nodiscard]]
			const Vulkan::TextureInterface &
			bloomTexture () const noexcept
			{
				return m_upTargets[MipLevels - 2];
			}

		private:

			/**
			 * @brief Creates all graphics pipelines for the bloom passes.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipelines () noexcept;

			/**
			 * @brief Creates all descriptor sets for the bloom passes.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDescriptorSets () noexcept;

			Parameters m_parameters;
			/* When true, a downstream consumer (camera ToneMapping) samples bloomTexture()
			 * itself and the full-res composite pass is skipped. */
			bool m_compositeBypassed{false};
			/* Intermediate render targets. */
			std::array< IntermediateRenderTarget, MipLevels > m_downTargets;
			std::array< IntermediateRenderTarget, MipLevels - 1 > m_upTargets;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_downsamplePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_upsamplePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_downsampleLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_upsampleLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;
			/* Descriptor sets. The passes reading EXTERNAL per-frame textures (first downsample,
			 * composite) use per-frame-in-flight copies — rewriting a set still referenced by a
			 * pending command buffer is illegal; static sets only reference the effect's own
			 * targets. m_downDescSets[0] stays empty (mip 0 lives in m_downFirstPerFrame). */
			std::array< std::unique_ptr< Vulkan::DescriptorSet >, MipLevels > m_downDescSets;
			std::array< std::unique_ptr< Vulkan::DescriptorSet >, MipLevels - 1 > m_upDescSets;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_downFirstPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
	};
}
