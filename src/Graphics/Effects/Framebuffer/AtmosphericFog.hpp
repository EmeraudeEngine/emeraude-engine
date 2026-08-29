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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief Analytical atmospheric fog post-processing effect.
	 * @note Reads the depth buffer to reconstruct world-space positions and applies
	 * exponential height fog with directional inscattering (closed-form integral,
	 * single fullscreen pass). Style inspired by UE5 exponential height fog.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class EMEN_API AtmosphericFog final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AtmosphericFogEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note The participating medium, applied to the fully lit scene. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::Fog;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing atmospheric fog parameters.
			 */
			/**
			 * @brief The TECHNIQUE knobs of the fog effect.
			 * @note ⚠️ The MEDIUM is no longer here. Density, height falloff, base height, max
			 * distance, chromaticity and luminance moved to Scenes::ParticipatingMedium, owned by
			 * the scene and reached through FrameContext — because a medium is a property of the
			 * WORLD, not of whichever effect integrates it, and keeping it private to one effect
			 * instance is what made it unshareable with the volumetric pass. What remains here is
			 * what belongs to this particular technique.
			 */
			struct EMEN_API Parameters
			{
				/* Henyey-Greenstein-ish inscattering lobe of the sun halo: how tight it is. */
				float inscatterExponent{8.0F};
				/* Gain of that halo over the medium's own luminance. */
				float inscatterIntensity{1.0F};
				/* Whether far-plane fragments are fogged too. ⚠️ Their fog amount saturates (the
				 * fictive ray length is the medium's maxDistance), so enabling this REPLACES the
				 * sky with the fog rather than tinting it. */
				bool skyFogEnabled{false};
			};

			/**
			 * @brief Push constants for the atmospheric fog pass (116 bytes).
			 */
			struct EMEN_API FogPushConstants
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
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			AtmosphericFog (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs an atmospheric fog effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			AtmosphericFog (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties() */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Overrides the inscattering color (instead of reading from LightSet).
			 * @param color The override color.
			 * @return void
			 */
			void
			setInscatterColorOverride (const Base::PixelFactory::Color<> & color) noexcept
			{
				m_inscatterColorOverride = color;
			}

			/**
			 * @brief Clears the inscatter color override (reverts to LightSet value).
			 * @return void
			 */
			void
			clearInscatterColorOverride () noexcept
			{
				m_inscatterColorOverride.reset();
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

			Parameters m_parameters;
			IntermediateRenderTarget m_outputTarget;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_fogPipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_fogLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_fogPerFrame;
			std::optional< Base::PixelFactory::Color<> > m_inscatterColorOverride;
			/* One warning per effect instance, not one per frame. */
			bool m_missingMediumReported{false};
	};
}
