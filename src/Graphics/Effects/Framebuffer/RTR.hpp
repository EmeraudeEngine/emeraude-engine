/*
 * src/Graphics/Effects/Framebuffer/RTR.hpp
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
#include "Graphics/TextureResource/TextureCubemap.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Ray-Traced Reflections (RTR) post-processing effect.
	 * @note Uses VK_KHR_ray_query / GL_EXT_ray_query in a fragment shader to trace
	 * hardware-accelerated reflection rays against the TLAS. Fetches material data
	 * from the RT SSBOs (mesh metadata, material properties, lights).
	 * Falls back to SSR when ray tracing hardware is not available.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API RTR final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RTREffect"};

			/**
			 * @brief User-facing RTR parameters.
			 */
			struct EMEN_API Parameters
			{
				float maxDistance{100.0F};
				float intensity{0.8F};
				float fadeScreenEdge{0.15F};
				/** @brief MAXIMUM bilateral blur radius in texels, reached near roughness 0.7
				 * (the per-pixel radius scales with roughness² — the GGX cone footprint).
				 * Polished surfaces pay ~1 texel whatever this value. */
				uint32_t blurRadius{12};
				float depthSigma{0.5F};
				float normalSigma{0.3F};
			};

			/**
			 * @brief Push constants for the RTR trace pass.
			 */
			struct EMEN_API TracePushConstants
			{
				std::array< float, 16 > invViewProj;
				std::array< float, 3 > invViewCol0;
				float viewPosX;
				std::array< float, 3 > invViewCol1;
				float viewPosY;
				std::array< float, 3 > invViewCol2;
				float viewPosZ;
				float maxDistance;
				float intensity;
				float fadeScreenEdge;
				uint32_t lightCount;
				/* Scene ambient term (color × intensity), added at reflection hit points so
				 * the reflected surfaces match the raster look (which includes ambient). */
				float ambientR;
				float ambientG;
				float ambientB;
				/** @brief Sky luminance (nits): scales the NORMALIZED bindless environment
				 * sources sampled at hit/miss (irradiance, prefiltered) into absolute light. */
				float skyLuminance;
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
			 * @brief Push constants for the composite pass (cone lookup included).
			 */
			struct EMEN_API CompositePushConstants
			{
				float intensity;
				/** @brief Cone width in TRACE texels per unit of GGX alpha (roughness²):
				 * 2 x assumedHitFraction x trace height. v1 approximation — the per-pixel
				 * hit distance is not available (alpha carries the confidence), the cone
				 * assumes a representative hit distance. */
				float coneWidthScale;
				/** @brief log2(pyramid base texel / trace texel). */
				float pyramidLodOffset;
				float pyramidMaxLod;
			};

			/**
			 * @brief Constructs a ray-tracing reflexion effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			RTR (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a ray-tracing reflexion effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 * @param environmentCubemap A cubemap sampled when a reflection ray escapes the scene (sky, distant environment). Default none (renderer default cubemap).
			 */
			RTR (Renderer & renderer, const Parameters & parameters, const std::shared_ptr< TextureResource::TextureCubemap > & environmentCubemap = nullptr) noexcept
				: IndirectPostProcessEffect{renderer},
				m_parameters{parameters},
				m_environmentCubemap{environmentCubemap}
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresRayTracing() */
			[[nodiscard]]
			bool
			requiresRayTracing () const noexcept override
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
			 * @brief Sets the RTR parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current RTR parameters.
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
			std::shared_ptr< TextureResource::TextureCubemap > m_environmentCubemap;
			/* IRTs: trace (half-res), blur H (half-res), blur V (half-res), composite (full-res). */
			IntermediateRenderTarget m_traceTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_tracePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_traceLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_blurLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurHPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurVPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
			/* Pre-convolved REFLECTION pyramid (glossy cone approximation): half-res base,
			 * tent-downsampled chain of the PREMULTIPLIED trace output rebuilt every frame.
			 * The composite reads it at the roughness²-driven LOD (the /confidence division
			 * renormalizes edge bleed) — an O(1) blur whatever the cone width, where the
			 * separable bilateral tops out at a few texels. */
			std::shared_ptr< Vulkan::Image > m_pyramidImage;
			std::vector< std::shared_ptr< Vulkan::ImageView > > m_pyramidMipViews;
			std::shared_ptr< Vulkan::ImageView > m_pyramidFullView;
			std::shared_ptr< Vulkan::Sampler > m_pyramidSampler;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_pyramidDSLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_pyramidPipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_pyramidDownsamplePipeline;
			std::shared_ptr< Vulkan::DescriptorPool > m_pyramidDescriptorPool;
			/* Fixed sets: trace target -> mip 0, then mip k-1 -> mip k. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_pyramidSets;
			uint32_t m_pyramidMipCount{0U};
	};
}
