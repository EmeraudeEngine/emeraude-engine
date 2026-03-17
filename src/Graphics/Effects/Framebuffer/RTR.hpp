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
 * https://github.com/londnoir/emeraude-engine
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
#include "Vulkan/DescriptorSet.hpp"


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
	class RTR final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RTREffect"};

			/**
			 * @brief User-facing RTR parameters.
			 */
			struct Parameters
			{
				float maxDistance{100.0F};
				float intensity{0.8F};
				float fadeScreenEdge{0.15F};
			};

			/**
			 * @brief Push constants for the RTR trace pass.
			 */
			struct TracePushConstants
			{
				float invViewProj[16];
				float invViewCol0[3];
				float viewPosX;
				float invViewCol1[3];
				float viewPosY;
				float invViewCol2[3];
				float viewPosZ;
				float maxDistance;
				float intensity;
				float fadeScreenEdge;
				uint32_t lightCount;
			};

			/**
			 * @brief Push constants for the blur pass.
			 */
			struct BlurPushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float directionX;
				float directionY;
			};

			/**
			 * @brief Push constants for the composite pass.
			 */
			struct CompositePushConstants
			{
				float intensity;
				float padding1;
				float padding2;
				float padding3;
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
			 */
			RTR (Renderer & renderer, const Parameters & parameters) noexcept
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
			const Vulkan::TextureInterface & execute (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const Vulkan::TextureInterface * inputDepth, const Vulkan::TextureInterface * inputNormals, const Vulkan::TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept override;

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
			/* Descriptor sets. */
			std::unique_ptr< Vulkan::DescriptorSet > m_blurHDescSet;
			std::unique_ptr< Vulkan::DescriptorSet > m_blurVDescSet;
			/* Per-frame descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_tracePerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;
	};
}
