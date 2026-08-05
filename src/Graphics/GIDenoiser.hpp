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

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <memory>
#include <string>
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
				/* alpha, depthTolerance, normalThreshold, flags (bit 0 = variance clip, bit 1 = animated noise). */
				std::array< float, 4 > temporalParams;
				/* strength, clamp, variance-clip gamma, unused. */
				std::array< float, 4 > bounceParams;
				/* sky luminance in nits (0 = no sky), sky ray distance, unused, unused. */
				std::array< float, 4 > skyParams;
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
			 * @brief Writes this frame's data into the UBO and advances the noise sequence.
			 * @param frameIndex The frame-in-flight index.
			 * @param data The frame data (assembled by the owner — it holds the trace scalars).
			 * @return bool
			 */
			[[nodiscard]]
			bool updateFrameData (uint32_t frameIndex, const FrameUBOData & data) noexcept;

			/**
			 * @brief Records the temporal resolve and the normal-history retention passes.
			 * @note Called outside any active render pass, after the owner's noisy estimate is
			 * complete. Flips the history ping-pong. When the temporal chain is off, records
			 * nothing and returns the noisy input unchanged.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param noisyInput The owner's denoised-so-far estimate (blur output today).
			 * @param context The per-frame chain context.
			 * @return const Vulkan::TextureInterface * The texture the owner's combine must consume.
			 */
			[[nodiscard]]
			const Vulkan::TextureInterface * recordResolve (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & noisyInput, const FrameContext & context) noexcept;

		private:

			/** @brief The owning effect's ClassId (GPU object name prefix). */
			const char * m_ownerLabel;
			/* Temporal history (owner resolution, ping-pong): RGB = resolved indirect
			 * irradiance, A = camera distance of the pixel (0 = invalid/sky). Plus the
			 * world-space normal history used for disocclusion rejection. */
			std::array< IntermediateRenderTarget, 2 > m_historyTargets;
			std::array< IntermediateRenderTarget, 2 > m_normalHistoryTargets;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_temporalPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_normalCopyPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_temporalLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_normalCopyLayout;
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_temporalPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_normalCopyPerFrame;
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
