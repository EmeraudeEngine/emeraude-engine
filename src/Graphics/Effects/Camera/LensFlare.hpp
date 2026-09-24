/*
 * src/Graphics/Effects/Camera/LensFlare.hpp
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
#include <vector>

/* Local inclusions for inheritances. */
#include "Graphics/IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "Graphics/IntermediateRenderTarget.hpp"

namespace EmEn::Graphics::Effects::Camera
{
	/**
	 * @brief The ghosts and the halo reflected between the lens elements (Sep 2026 rewrite).
	 * @note Two half-resolution passes. The SOURCE pass extracts what is bright enough to leave a visible
	 * ghost — the chain colour above a threshold in DISPLAY units (after the camera's exposure) — and adds
	 * the ANALYTIC SUN: a soft disc at the main directional light's projected position carrying the light's
	 * illuminance spread over the disc, times its visibility (a 16-tap depth probe, times the clouds' view
	 * transmittance). The GHOST pass is John Chapman's "pseudo lens flare" (2013): every bright source is
	 * reflected through the centre of the screen as a row of ghosts, plus a halo ring, with a chromatic
	 * distortion; `Parameters::intensity` plays the lens reflectance.
	 * @note ⚠️ Before the rewrite the "ghosts" were radial streaks: each pixel read the bright image at a
	 * fixed distance from the light IN ITS OWN DIRECTION, so a point-like sun made nothing and a bright
	 * sky seen through leaves made rainbow bands. And the painted sun of an HDRI stays under the threshold
	 * once exposed: without the analytic sun, a daylight scene has no flare at all.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API LensFlare final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LensFlareEffect"};

			/** @brief The area of the sun disc's soft profile, `1 - smoothstep(0.6, 1, r)`, relative to the full disc. */
			static constexpr auto SunDiscProfileArea{0.648F};

			/** @brief The ceiling of the injected sun disc, in display units: under a half float's 65 504. */
			static constexpr auto MaxSunDisplayLuminance{50000.0F};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note Reads the chain colour for its bright pass: everything that can be bright must have run. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::LensFlare;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @brief User-facing lens flare parameters.
			 */
			struct EMEN_API Parameters
			{
				/**
				 * @brief The brightness a pixel must exceed to feed the ghosts, in DISPLAY units (after the
				 * camera's exposure: 1 = the sensor's white).
				 * @note ⚠️ Owner decision 2026-09-24: 4. It was 0.8 compared with NITS (the camera phase runs
				 * before the tone mapping), so outdoors the whole sky fed the flare. Measured on `forest`:
				 * 0.8 display still streaks through the leaves, 2 faintly, 4 and 8 are clean.
				 */
				float threshold{4.0F};
				/** @brief The soft knee of the threshold, as a fraction of it. */
				float softKnee{0.5F};
				/** @brief The number of ghosts per bright source. */
				int32_t ghostCount{6};
				/** @brief The spacing of the ghosts along the axis through the screen centre (Chapman's dispersal). */
				float ghostDispersal{0.35F};
				/** @brief The radius of the halo ring around the screen centre, in UV of the screen height. */
				float haloWidth{0.45F};
				/**
				 * @brief The halo's weight relative to the ghosts.
				 * @note Flux conservation for the sun: the ring spreads its disc over a circumference, a ring
				 * area of about 4 · haloWidth / sunDiscRadius ≈ 120 discs — so ~0.008. At 1 (Chapman's sum)
				 * the ring outshone the sun.
				 */
				float haloIntensity{0.01F};
				/** @brief The chromatic separation of the ghosts and the halo, in UV. */
				float chromaticDistortion{0.004F};
				/**
				 * @brief The lens reflectance: the fraction of a source's luminance its ghosts carry.
				 * @note A ghost is light reflected twice between coated elements — a small fraction. At this
				 * value only a source far above the exposure (the sun, a lamp at night) leaves a visible ghost.
				 */
				float intensity{5.0e-4F};
				/**
				 * @brief The radius of the injected sun disc, as a fraction of the screen HEIGHT.
				 * @note Wider than the real sun (0.27°): it stands for the defocused image of the source the
				 * ghosts are copies of. The disc carries the light's whole illuminance, spread over it.
				 */
				float sunDiscRadius{0.015F};
				/**
				 * @brief Radius, as a fraction of the screen HEIGHT, of the depth-buffer disk probed
				 * around the light's projected position to decide how much of it the geometry hides.
				 * @note 16 taps; the fraction of taps that read the far plane is the visibility. A
				 * sun behind a wall injects nothing; a sun half behind an edge, half of it.
				 */
				float occlusionRadius{0.012F};
			};

			/**
			 * @brief Push constants of the source pass (bright extraction + the analytic sun).
			 */
			struct EMEN_API SourcePushConstants
			{
				float texelSizeX;
				float texelSizeY;
				float threshold;
				float softKnee;
				/* nit -> display value (FrameContext::displayExposure), 1 when the chain has no tone mapper. */
				float exposure;
				/* The light's projected position, in UV. */
				float lightScreenX;
				float lightScreenY;
				/* The sun disc radius in UV, per axis; 0 = no sun injected this frame. */
				float sunRadiusX;
				float sunRadiusY;
				/* The sun disc luminance, in DISPLAY units (the illuminance spread over the disc × the exposure, the
				 * edge fade applied): the source target holds display units, a sun in nits overflows a half float. */
				float sunLuminanceR;
				float sunLuminanceG;
				float sunLuminanceB;
				/* Occlusion probe radius in UV, per axis (the screen is not square). */
				float occlusionRadiusX;
				float occlusionRadiusY;
				/* 1 when the clouds' view transmittance is bound at binding 2 (the cloud transmittance pairing). */
				float cloudTransmittanceEnabled;
			};

			/**
			 * @brief Push constants of the ghost pass.
			 */
			struct EMEN_API GhostPushConstants
			{
				float ghostDispersal;
				float haloWidth;
				float chromaticDistortion;
				float intensity;
				int32_t ghostCount;
				/* Width over height of the screen: the halo is a circle, not an ellipse. */
				float aspect;
				/* display value -> nit: the ghosts go back to the chain's unit. */
				float inverseExposure;
				float haloIntensity;
			};

			/**
			 * @brief Constructs a lens flare effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			LensFlare (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a lens flare effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			LensFlare (Renderer & renderer, const Parameters & parameters) noexcept
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::readsChainColorUpstream() */
			[[nodiscard]]
			bool
			readsChainColorUpstream (const FrameContext & /*context*/) const noexcept override
			{
				/* The source pass samples the chain color to extract the bright spots. */
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordOverlayPasses() */
			void recordOverlayPasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresHDR() */
			[[nodiscard]]
			bool
			requiresHDR () const noexcept override
			{
				return true;
			}

			/**
			 * @copydoc EmEn::Graphics::IndirectPostProcessEffect::consumesCloudTransmittance()
			 * @note The sun probe counts the taps that read the sky; behind a cloud a tap is only worth
			 * the cloud's transmittance, so a cloud over the sun dims its ghosts as it covers it.
			 */
			[[nodiscard]]
			bool
			consumesCloudTransmittance () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::setCloudTransmittanceSource() */
			void
			setCloudTransmittanceSource (const Vulkan::TextureInterface * texture) noexcept override
			{
				m_cloudTransmittance = texture;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth()
			 * @note The source pass probes the scene depth around the projected sun: its occlusion. */
			[[nodiscard]]
			bool
			requiresDepth () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the lens flare parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current lens flare parameters.
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
			/* The clouds' view transmittance for THIS frame, or nullptr (set every frame by the stack). */
			const Vulkan::TextureInterface * m_cloudTransmittance{nullptr};
			/* Intermediate render targets (half resolution). */
			IntermediateRenderTarget m_sourceTarget;
			IntermediateRenderTarget m_ghostTarget;
			/* Pipelines. */
			std::shared_ptr< Vulkan::GraphicsPipeline > m_sourcePipeline;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_ghostPipeline;
			/* Pipeline layouts. */
			std::shared_ptr< Vulkan::PipelineLayout > m_sourceLayout;
			std::shared_ptr< Vulkan::PipelineLayout > m_ghostLayout;
			/* Source: binding 0 = chain colour, 1 = scene depth, 2 = the clouds' transmittance or the depth again (all per frame). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_sourcePerFrame;
			/* Ghost: binding 0 = the source target (fixed). */
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_ghostPerFrame;
	};
}
