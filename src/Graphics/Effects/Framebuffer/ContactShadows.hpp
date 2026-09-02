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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Vulkan/UniformBufferObject.hpp"

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
	class EMEN_API ContactShadows final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ContactShadowsEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note Fine-detail depth-derived shadowing, after the indirect terms it darkens. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::ContactShadows;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing contact shadows parameters.
			 */
			struct EMEN_API Parameters
			{
				float maxDistance{2.0F};
				float normalBias{0.01F};
				float intensity{0.8F};
				float maxBlurRadius{10.0F};
			};

			/**
			 * @brief Per-frame data for the RT shadow pass.
			 * @note A UBO, not push constants: the pass needs the inverse view ROTATION on top
			 * of the inverse view-projection (it offsets the ray origin along the G-buffer
			 * normal, which is view-space), and that totals 132 bytes — above the 128-byte
			 * Vulkan push constant minimum guarantee (maxPushConstantsSize). Layout is
			 * std140-compatible: mat4 and vec4 members only, scalars packed in the w slots.
			 */
			struct EMEN_API ShadowFrameUBOData
			{
				/** @brief Inverse view-projection matrix, for world position reconstruction. */
				std::array< float, 16 > inverseProjViewMatrix;
				/** @brief xyz = inverse view rotation column 0, w = camera world position X. */
				std::array< float, 4 > invViewCol0;
				/** @brief xyz = inverse view rotation column 1, w = camera world position Y. */
				std::array< float, 4 > invViewCol1;
				/** @brief xyz = inverse view rotation column 2, w = camera world position Z. */
				std::array< float, 4 > invViewCol2;
				/** @brief xyz = the directional light EMISSION direction (world), w = maxDistance. */
				std::array< float, 4 > lightParameters;
				/** @brief x = normalBias, yzw = unused (std140 padding). */
				std::array< float, 4 > shadowParameters;
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::producesOverlay() */
			[[nodiscard]]
			bool
			producesOverlay () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::usesSharedDenoise() */
			[[nodiscard]]
			bool
			usesSharedDenoise () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordPreDenoisePasses() */
			void recordPreDenoisePasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::denoiseContribution() */
			[[nodiscard]]
			DenoiseContribution denoiseContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

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
			/* Intermediate render targets (half-res, gated by the RT AO pixel doubling
			 * setting — the shared denoise group must share one extent). */
			IntermediateRenderTarget m_shadowTarget;
			IntermediateRenderTarget m_blurHTarget;
			IntermediateRenderTarget m_blurVTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_shadowPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_shadowLayout;
			/* Descriptor set layouts. */
			/** @brief Set 1 of the shadow pass: depth + normals. Set 0 is the Renderer's RT set
			 * (TLAS + scene SSBOs), set 2 the bindless textures — both owned elsewhere. */
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_shadowInputLayout;
			/* Descriptor sets. */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_shadowPerFrame;
			/** @brief Per-frame shadow parameters (set 1, binding 2). */
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_shadowFrameUBOs;
	};
}
