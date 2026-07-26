/*
 * src/Graphics/Effects/Framebuffer/DepthOfField.hpp
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
#include "Vulkan/Buffer.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Depth of Field post-processing effect (physical camera model).
	 * @note Production-grade gather DoF: signed circle of confusion (thin lens model),
	 * near/far field separation with foreground silhouette bleeding (dilated near CoC),
	 * golden-angle spiral disc gather (circular bokeh), and a smoothed auto-focus
	 * (1x1 ping-pong history, exponential rack focus).
	 * The OPTICAL parameters (aperture, focal length, focus distance, auto-focus) come
	 * from the scene's ACTIVE CAMERA when one is present in the frame context — the
	 * camera is the single source of truth of the photographic behaviour. The local
	 * Parameters only act as a fallback without a camera, plus the effect-quality knobs.
	 * Technique references: "Next Generation Post Processing in Call of Duty: Advanced
	 * Warfare", J. Jimenez, SIGGRAPH 2014 (scatter-as-gather, near-field dilation).
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API DepthOfField final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DepthOfFieldEffect"};

			/**
			 * @brief User-facing depth of field parameters.
			 * @note The first block mirrors the physical camera options and is ONLY used
			 * when no active camera is provided by the frame context. The second block
			 * holds the effect-quality knobs (overridden by settings keys at creation).
			 */
			struct EMEN_API Parameters
			{
				/* Optics fallback (the active camera overrides these). */
				float focusDistance{10.0F};
				float aperture{2.8F};
				float focalLength{50.0F};
				float sensorWidth{36.0F};
				/* Effect-quality knobs (settings-driven). maxCoCRadius is a pure
				 * performance/quality clamp in half-res pixels: the blur amount itself is
				 * the thin-lens CoC converted from sensor fraction to pixels, unscaled. */
				float maxCoCRadius{32.0F};
				float autoFocusSpeed{3.0F};
				uint32_t sampleCount{48};
				bool autoFocus{true};
				bool nearFieldEnabled{true};
			};

			/**
			 * @brief Push constants for the auto-focus pass (1x1).
			 */
			struct EMEN_API FocusPushConstants
			{
				float nearPlane;
				float farPlane;
				float focusDistance;
				float autoFocusSpeed;
				float time;
				float texelSizeX;
				float texelSizeY;
				/* Bit 0 = auto-focus enabled, bit 1 = reset history (first frame). */
				uint32_t flags;
			};

			/**
			 * @brief Push constants for the CoC setup pass.
			 */
			struct EMEN_API SetupPushConstants
			{
				float nearPlane;
				float farPlane;
				float aperture;
				float focalLength;
				float sensorWidth;
				/* Width of the setup target in pixels: converts the CoC — a fraction of the
				 * image width — into a half-res pixel radius, the unit of every later pass. */
				float targetWidth;
				/* Blur ceiling in half-res pixels (performance clamp, applied at the source). */
				float maxCoCRadius;
				float padding;
			};

			/**
			 * @brief Push constants for the near-CoC dilation passes.
			 */
			struct EMEN_API DilatePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float directionX;
				float directionY;
				int32_t radius;
				/* 1 = read the signed CoC from the source alpha (first pass), 0 = read R (second pass). */
				uint32_t extractFromAlpha;
			};

			/**
			 * @brief Push constants for the gather passes (far and near fields).
			 */
			struct EMEN_API GatherPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				uint32_t sampleCount;
				uint32_t padding;
			};

			/**
			 * @brief Push constants for the composite pass.
			 */
			struct EMEN_API CompositePushConstants
			{
				uint32_t nearFieldEnabled;
				uint32_t padding;
			};

			/**
			 * @brief Constructs a depth of field effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			DepthOfField (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a depth of field effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			DepthOfField (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
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

			/**
			 * @brief Sets the depth of field parameters.
			 * @note The optical block only applies without an active camera in the frame
			 * context; the effect-quality knobs apply at the next (re)creation.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current depth of field parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Returns the focus distance the rack-focus EMA is currently at, in meters.
			 * @note Read back from the GPU focus history (1x1 RG32F, R = distance) with
			 * `framesInFlight` frames of latency — one tiny host-visible slot per frame in
			 * flight, read only once its fence has passed, zero stall. Meaningful in AUTO focus
			 * (what the measurement landed on) and during a manual focus pull (the smoothed
			 * position of the ring). 0 until a measurement completed.
			 * @warning RENDER THREAD contract: written by execute(), read by the overlay panel
			 * inside the same frame scope.
			 * @return float
			 */
			[[nodiscard]]
			float
			meteredFocusDistance () const noexcept
			{
				return m_meteredFocusDistance;
			}

		private:

			/** @brief One host-visible readback slot per frame in flight (focus distance). */
			struct FocusReadbackSlot
			{
				std::unique_ptr< Vulkan::Buffer > buffer;
				const uint8_t * mappedPtr{nullptr};
				bool pending{false};
			};

			Parameters m_parameters;
			/* IRTs. Half-res working set; auto-focus history is a 1x1 RG32F ping-pong
			 * (R = focus distance, G = timestamp for the rack-focus EMA); output full-res. */
			std::array< IntermediateRenderTarget, 2 > m_focusTargets;
			IntermediateRenderTarget m_setupTarget;
			IntermediateRenderTarget m_dilateHTarget;
			IntermediateRenderTarget m_dilateVTarget;
			IntermediateRenderTarget m_farGatherTarget;
			IntermediateRenderTarget m_nearGatherTarget;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_focusPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_setupPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_dilatePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_farGatherPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_nearGatherPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_focusLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_setupLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_dilateLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_farGatherLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_nearGatherLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;
			/* Descriptor sets. Per-frame sets have their bindings rewritten in execute()
			 * (input textures and the focus ping-pong), static sets are wired at creation. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_focusPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_setupPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
			std::unique_ptr< Vulkan::DescriptorSet > m_dilateHDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_dilateVDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_farGatherDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_nearGatherDescSet;
			/* Focus-distance readback ring: slot N is written by the GPU during frame N and
			 * read back when slot N comes around again — framesInFlight frames of latency,
			 * zero stall. Persistently mapped. RENDER THREAD ONLY. */
			std::vector< FocusReadbackSlot > m_focusReadback;
			float m_meteredFocusDistance{0.0F};
			/* Ping-pong index of the focus history written THIS frame. */
			uint32_t m_focusWriteIndex{0};
			/* False until the focus history holds a valid value (forces a reset). */
			bool m_focusValid{false};
	};
}
