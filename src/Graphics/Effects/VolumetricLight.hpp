/*
 * src/Graphics/Effects/VolumetricLight.hpp
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
#include "Graphics/PostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorSet;
		class DescriptorSetLayout;
		class GraphicsPipeline;
		class PipelineLayout;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics::Effects
{
	/**
	 * @brief Screen-space volumetric light (god rays) post-processing effect.
	 * @note Extracts an occlusion mask from the depth buffer, applies a radial blur
	 * centered on the projected sun position, and additively composites the result.
	 * @extends EmEn::Graphics::PostProcessEffect This is a multi-pass post-process effect.
	 */
	class VolumetricLight final : public PostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VolumetricLightEffect"};

			/**
			 * @brief User-facing volumetric light parameters.
			 */
			struct Parameters
			{
				float density{1.0F};
				float decay{0.975F};
				float exposure{0.25F};
				uint32_t numSamples{64};
				float depthThreshold{0.9999F};
			};

			/**
			 * @brief Push constants for the occlusion and radial blur passes.
			 */
			struct ScatterPushConstants
			{
				float lightScreenX;
				float lightScreenY;
				float texelSizeX;
				float texelSizeY;
				float nearPlane;
				float farPlane;
				float lightColorR;
				float lightColorG;
				float lightColorB;
				float lightIntensity;
				float density;
				float decay;
				float exposure;
				float depthThreshold;
				uint32_t numSamples;
				float lightOnScreen;
			};

			/**
			 * @brief Push constants for the composite pass.
			 */
			struct CompositePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float padding1;
				float padding2;
			};

			/**
			 * @brief Constructs a volumetric light effect.
			 */
			VolumetricLight () noexcept = default;

			/** @copydoc EmEn::Graphics::PostProcessEffect::create() */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::resize() */
			[[nodiscard]]
			bool resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (
				const Vulkan::CommandBuffer & commandBuffer,
				const Vulkan::TextureInterface & inputColor,
				const Vulkan::TextureInterface * inputDepth,
				const Vulkan::TextureInterface * inputNormals,
				const PostProcessor::PushConstants & constants
			) noexcept override;

			/** @copydoc EmEn::Graphics::PostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the light direction (emission direction, not source direction).
			 * @param x The X component of the light direction.
			 * @param y The Y component of the light direction.
			 * @param z The Z component of the light direction.
			 * @return void
			 */
			void
			setLightDirection (float x, float y, float z) noexcept
			{
				m_lightDirX = x;
				m_lightDirY = y;
				m_lightDirZ = z;
			}

			/**
			 * @brief Sets the light color.
			 * @param r Red component (0-1).
			 * @param g Green component (0-1).
			 * @param b Blue component (0-1).
			 * @return void
			 */
			void
			setLightColor (float r, float g, float b) noexcept
			{
				m_lightColorR = r;
				m_lightColorG = g;
				m_lightColorB = b;
			}

			/**
			 * @brief Sets the light intensity multiplier.
			 * @param intensity The intensity value.
			 * @return void
			 */
			void
			setLightIntensity (float intensity) noexcept
			{
				m_lightIntensity = intensity;
			}

			/**
			 * @brief Sets the volumetric light parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current volumetric light parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/**
			 * @brief Records a single fullscreen pass into a target IRT.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param target A reference to the target intermediate render target.
			 * @param pipeline A reference to the graphics pipeline.
			 * @param pipelineLayout A reference to the pipeline layout.
			 * @param descriptorSet A reference to the descriptor set.
			 * @param pushConstants A pointer to the push constants data.
			 * @param pushConstantsSize The size of the push constants in bytes.
			 * @return void
			 */
			void recordFullscreenPass (
				const Vulkan::CommandBuffer & commandBuffer,
				IntermediateRenderTarget & target,
				const Vulkan::GraphicsPipeline & pipeline,
				const Vulkan::PipelineLayout & pipelineLayout,
				const Vulkan::DescriptorSet & descriptorSet,
				const void * pushConstants,
				uint32_t pushConstantsSize
			) const noexcept;

			/* Intermediate render targets. */
			IntermediateRenderTarget m_occlusionTarget;
			IntermediateRenderTarget m_radialTarget;
			IntermediateRenderTarget m_outputTarget;

			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_occlusionPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_radialPipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_compositePipeline;

			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_occlusionLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_radialLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_compositeLayout;

			/* Descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_occlusionPerFrame;
			std::unique_ptr< Vulkan::DescriptorSet > m_radialDescSet;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_compositePerFrame;

			/* Light parameters. */
			float m_lightDirX{0.0F};
			float m_lightDirY{-1.0F};
			float m_lightDirZ{0.0F};
			float m_lightColorR{1.0F};
			float m_lightColorG{0.9F};
			float m_lightColorB{0.7F};
			float m_lightIntensity{1.0F};
			Parameters m_parameters;
			Renderer * m_renderer{nullptr};
	};
}
