/*
 * src/Graphics/Effects/Framebuffer/ContactShadows.hpp
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

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Ray-traced contact shadows post-processing effect.
	 * @note Uses VK_KHR_ray_query to trace shadow rays from each pixel toward
	 * the dominant light source. This produces pixel-accurate contact shadows
	 * that complement shadow maps for fine-scale occlusion detail.
	 * Requires hardware ray tracing support (automatically skipped when unavailable).
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class ContactShadows final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ContactShadowsEffect"};

			/**
			 * @brief User-facing contact shadows parameters.
			 */
			struct Parameters
			{
				float maxDistance{2.0F};
				float normalBias{0.01F};
				float intensity{0.8F};
				float maxBlurRadius{10.0F};
			};

			/**
			 * @brief Push constants for the RT shadow pass.
			 */
			struct ShadowPushConstants
			{
				float inverseProjViewMatrix[16];
				float lightDirWorldX;
				float lightDirWorldY;
				float lightDirWorldZ;
				float maxDistance;
				float normalBias;
				float viewPosX;
				float viewPosY;
				float viewPosZ;
			};

			/**
			 * @brief Push constants for the apply pass.
			 */
			struct ApplyPushConstants
			{
				float intensity;
			};

			/**
			 * @brief Push constants for the blur passes (PCSS-lite).
			 */
			struct BlurPushConstants
			{
				float directionX;
				float directionY;
				float texelSizeX;
				float texelSizeY;
				float maxBlurRadius;
			};

			/**
			 * @brief Constructs a contact shadows effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			ContactShadows (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a contact shadows effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			ContactShadows (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresRayTracing() */
			[[nodiscard]]
			bool
			requiresRayTracing () const noexcept override
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresLightSet() */
			[[nodiscard]]
			bool
			requiresLightSet () const noexcept override
			{
				return true;
			}


			/**
			 * @brief Sets the contact shadows parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current contact shadows parameters.
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
			/* Intermediate render targets. */
			IntermediateRenderTarget m_shadowTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			IntermediateRenderTarget m_outputTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_shadowPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurHPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_blurVPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_applyPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_shadowLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_blurLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_applyLayout;
			/* Descriptor set layouts. */
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_shadowDescLayout;
			/* Descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_shadowPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurHPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_blurVPerFrame;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_applyPerFrame;
	};
}
