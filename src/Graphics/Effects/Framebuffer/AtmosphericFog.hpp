/*
 * src/Graphics/Effects/Framebuffer/AtmosphericFog.hpp
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

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Analytical atmospheric fog post-processing effect.
	 * @note Reads the depth buffer to reconstruct world-space positions and applies
	 * exponential height fog with directional inscattering (closed-form integral,
	 * single fullscreen pass). Style inspired by UE5 exponential height fog.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class AtmosphericFog final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AtmosphericFogEffect"};

			/**
			 * @brief User-facing atmospheric fog parameters.
			 */
			struct Parameters
			{
				float density{0.02F};
				float heightFalloff{0.2F};
				float baseHeight{0.0F};
				float maxDistance{10000.0F};
				float fogColorR{0.5F};
				float fogColorG{0.6F};
				float fogColorB{0.7F};
				float inscatterExponent{8.0F};
				float inscatterIntensity{1.0F};
				bool skyFogEnabled{false};
			};

			/**
			 * @brief Push constants for the atmospheric fog pass (116 bytes).
			 */
			struct FogPushConstants
			{
				/* Camera basis — extracted from view matrix in execute(). */
				float cameraPosX;
				float cameraPosY;
				float cameraPosZ;
				float cameraRightX;
				float cameraRightY;
				float cameraRightZ;
				float cameraForwardX;
				float cameraForwardY;
				float cameraForwardZ;
				/* Depth reconstruction. */
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				float aspectRatio;
				/* Fog parameters. */
				float fogDensity;
				float fogHeightFalloff;
				float fogBaseHeight;
				float fogMaxDistance;
				float fogColorR;
				float fogColorG;
				float fogColorB;
				/* Directional inscattering. */
				float lightDirX;
				float lightDirY;
				float lightDirZ;
				float inscatterExponent;
				float inscatterColorR;
				float inscatterColorG;
				float inscatterColorB;
				float inscatterIntensity;
				/* Sky fog option (0.0 = off, 1.0 = on). */
				float skyFogEnabled;
			};

			/**
			 * @brief Constructs an atmospheric fog effect.
			 */
			AtmosphericFog () noexcept = default;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::create() */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::execute() */
			[[nodiscard]]
			const Vulkan::TextureInterface & execute (
				const Vulkan::CommandBuffer & commandBuffer,
				const Vulkan::TextureInterface & inputColor,
				const Vulkan::TextureInterface * inputDepth,
				const Vulkan::TextureInterface * inputNormals,
				const PostProcessor::PushConstants & constants
			) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth() */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
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
			 * @brief Sets the light direction for inscattering (emission direction).
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
			 * @brief Sets the inscattering color.
			 * @param r Red component (0-1).
			 * @param g Green component (0-1).
			 * @param b Blue component (0-1).
			 * @return void
			 */
			void
			setInscatterColor (float r, float g, float b) noexcept
			{
				m_inscatterColorR = r;
				m_inscatterColorG = g;
				m_inscatterColorB = b;
			}

			/**
			 * @brief Sets the atmospheric fog parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current atmospheric fog parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

		private:

			/* Intermediate render target. */
			IntermediateRenderTarget m_outputTarget;

			/* Pipeline. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_fogPipeline;

			/* Pipeline layout. */
			std::shared_ptr< Vulkan::PipelineLayout > m_fogLayout;

			/* Descriptor sets (per-frame). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_fogPerFrame;

			/* Light direction for inscattering. */
			float m_lightDirX{0.0F};
			float m_lightDirY{-1.0F};
			float m_lightDirZ{0.0F};

			/* Inscattering color. */
			float m_inscatterColorR{1.0F};
			float m_inscatterColorG{0.9F};
			float m_inscatterColorB{0.7F};

			Parameters m_parameters;
			Renderer * m_renderer{nullptr};
	};
}
