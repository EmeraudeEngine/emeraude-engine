/*
 * src/Graphics/Effects/Framebuffer/SSR.hpp
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
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn
{
	namespace Graphics
	{
		class TextureCubemap;
	}

	namespace Vulkan::TextureResource
	{
		class TextureCubemap;
	}
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Screen-Space Reflections (SSR) post-processing effect.
	 * @note Traces reflection rays in screen space using the depth and normals buffers,
	 * applies bilateral blur to reduce noise, then composites reflections with the scene color.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API SSR final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SSREffect"};

			/**
			 * @brief User-facing SSR parameters.
			 */
			struct EMEN_API Parameters
			{
				float maxDistance{50.0F};
				float stride{0.1F};
				float thickness{0.2F};
				float intensity{0.8F};
				float fadeScreenEdge{0.15F};
				float envFallbackIntensity{0.3F};
				uint32_t maxSteps{128};
				uint32_t binarySteps{8};
			};

			/**
			 * @brief Push constants for the SSR trace pass.
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
				float fadeScreenEdge;
				uint32_t maxSteps;
				uint32_t hiZMaxLevel;
				uint32_t padding;
			};

			/**
			 * @brief Push constants for the Hi-Z pyramid build dispatches.
			 */
			struct EMEN_API HiZPushConstants
			{
				int32_t destWidth;
				int32_t destHeight;
				int32_t sourceMaxX;
				int32_t sourceMaxY;
			};

			/**
			 * @brief Push constants for the blur pass.
			 */
			struct EMEN_API BlurPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float directionX;
				float directionY;
				float depthSigma;
				float normalSigma;
				int32_t blurRadius;
				float padding;
			};

			/**
			 * @brief Push constants for the resolve pass (cone lookup + cubemap fallback).
			 */
			struct EMEN_API ResolvePushConstants
			{
				float invViewCol0[4];
				float invViewCol1[4];
				float invViewCol2[4];
				float texelSizeX;
				float texelSizeY;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				float aspectRatio;
				float envFallbackIntensity;
				float intensity;
				/** @brief log2(pyramid base texel / trace texel): maps a cone width measured
				 * in TRACE texels onto the color pyramid LOD (the pyramid base is half-res). */
				float pyramidLodOffset;
				float pyramidMaxLod;
			};

			/**
			 * @brief Push constants for the composite pass.
			 */
			struct EMEN_API CompositePushConstants
			{
				float intensity;
				float padding1;
				float padding2;
				float padding3;
			};

			/**
			 * @brief Constructs a screen-space reflexion effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			SSR (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a screen-space reflexion effect.
			 * @note The ray-miss environment fallback reads the scene's GGX-prefiltered
			 * cubemap through the bindless table (reserved cube slot 2, always current).
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			SSR (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::providesReflections() */
			[[nodiscard]]
			bool
			providesReflections () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the SSR parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current SSR parameters.
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
			/* IRTs: trace (half-res), resolve (half-res), blur H (half-res), blur V (half-res), composite (full-res). */
			IntermediateRenderTarget m_traceTarget;
			IntermediateRenderTarget m_resolveTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tracePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_resolvePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_traceLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_resolveLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_blurLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;
			/* Descriptor sets (fixed — never updated after creation). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurHPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurVPerFrame;
			/* Quality knobs, read from the Core/Graphics/ScreenSpace/Reflection/? settings at create(). */
			uint32_t m_blurRadius{2U};
			float m_depthSigma{0.5F};
			float m_normalSigma{0.3F};
			/* Hi-Z depth pyramid (min-reduction mip chain, R32F) for the hierarchical trace
			 * (Uludag, GPU Pro 5 — the UE-class SSR traversal). Rebuilt every frame from the
			 * scene depth by compute dispatches recorded ahead of the trace pass. */
			std::shared_ptr< Vulkan::Image > m_hiZImage;
			std::vector< std::shared_ptr< Vulkan::ImageView > > m_hiZMipViews;
			std::shared_ptr< Vulkan::ImageView > m_hiZFullView;
			std::shared_ptr< Vulkan::Sampler > m_hiZSampler;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_hiZDSLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_hiZPipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_hiZCopyPipeline;
			std::unique_ptr< Vulkan::ComputePipeline > m_hiZReducePipeline;
			std::shared_ptr< Vulkan::DescriptorPool > m_hiZDescriptorPool;
			/* Per frame-in-flight: the copy set (scene depth -> mip 0). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_hiZCopyPerFrame;
			/* Fixed: one reduce set per destination mip (mip N-1 -> mip N). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_hiZReduceSets;
			uint32_t m_hiZMipCount{0U};
			/* Pre-convolved COLOR pyramid (Uludag cone tracing — the second half of the Hi-Z
			 * chapter): half-res base, tent-downsampled chain, rebuilt every frame by compute.
			 * The resolve reads it at the cone-width LOD; mirror-sharp fetches (cone < 1
			 * texel) stay on the full-res input color — no sharpness regression, half the
			 * memory. Shares the Hi-Z DS/pipeline layouts (same {sampler, storage} shape). */
			std::shared_ptr< Vulkan::Image > m_colorPyramidImage;
			std::vector< std::shared_ptr< Vulkan::ImageView > > m_colorPyramidMipViews;
			std::shared_ptr< Vulkan::ImageView > m_colorPyramidFullView;
			std::shared_ptr< Vulkan::Sampler > m_colorPyramidSampler;
			std::unique_ptr< Vulkan::ComputePipeline > m_colorDownsamplePipeline;
			/* Per frame-in-flight: input color -> pyramid mip 0. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_colorCopyPerFrame;
			/* Fixed: one downsample set per destination mip. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_colorReduceSets;
			uint32_t m_colorPyramidMipCount{0U};
			/* Per-frame-in-flight descriptor sets (updated every frame). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_resolvePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
	};
}
