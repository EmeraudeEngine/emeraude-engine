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
				float stride;
				float thickness;
				float fadeScreenEdge;
				uint32_t maxSteps;
				uint32_t binarySteps;
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
			};

			/**
			 * @brief Push constants for the resolve pass (cubemap fallback).
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
			std::unique_ptr< Vulkan::DescriptorSet > m_blurHDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_blurVDescSet;
			/* Per-frame-in-flight descriptor sets (updated every frame). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_resolvePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
	};
}
