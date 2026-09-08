/*
 * src/Graphics/Effects/Framebuffer/VolumetricScattering.hpp
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

namespace EmEn::Graphics::Effects::Framebuffer
{
	/**
	 * @brief World-space single-scattering pass through the scene's participating medium.
	 *
	 * @note This is the physical sibling of AtmosphericFog, and they share
	 * @ref EmEn::Graphics::EffectSlot::Fog **on purpose**: a slot holds one enabled effect at a
	 * time (`PostProcessStack::disableSlotSiblings()`, only EffectSlot::Custom is multi-occupant),
	 * so enabling this one disables the other. That exclusivity is not a convenience — it is what
	 * makes it STRUCTURALLY impossible to integrate the same medium's extinction twice, which is
	 * the defect family that has cost this engine the most (RTGI owning the sky, the two albedo
	 * lanes, the transmission pass composition). Pick one integrator per medium; the architecture
	 * enforces it rather than trusting discipline.
	 *
	 * @note What distinguishes it from AtmosphericFog is not accuracy in the abstract, it is
	 * **shadowing inside the volume**. AtmosphericFog evaluates a closed-form height-fog integral
	 * and is therefore structurally unable to put a shadow INSIDE a light shaft: nothing in a
	 * closed form knows what occludes the volume. This pass marches the view ray and samples the
	 * cascaded shadow map at every step, so a palm's shadow appears as a dark lane inside the
	 * shaft. That is the acceptance test, and it depends on no tunable threshold.
	 * ⚠️ Consequently it **requires a CSM directional light**. Without one it passes the chain
	 * colour through untouched and says so once — the honest answer, since an unshadowed march is
	 * exactly what its slot sibling already does more cheaply.
	 *
	 * @note The medium is NOT a parameter of this effect. Density, height falloff, base height,
	 * max distance, scattering albedo, phase anisotropy and luminance belong to
	 * Scenes::ParticipatingMedium, owned by the scene and reached through FrameContext — a medium
	 * is a property of the WORLD, not of whichever effect integrates it.
	 *
	 * @note Photometry: the integral yields **nits directly**. The source term is
	 * `sigmaS * phase(theta) * illuminance`, whose units are lm/(m³·sr); integrated over the march
	 * length that is lm/(m²·sr) = cd/m². There is therefore **no exposure or gain knob** to
	 * calibrate, deliberately — the screen-space god rays needed one only because their `exposure`
	 * was converting the light's LUX into the nits buffer by hand.
	 *
	 * References, technique only: B. Wronski, *Volumetric Fog: Unified Compute Shader Based
	 * Solution to Atmospheric Scattering*, SIGGRAPH 2014 (Assassin's Creed 4); S. Hillaire,
	 * *Physically-based & Unified Volumetric Rendering in Frostbite*, SIGGRAPH 2015;
	 * L. G. Henyey & J. L. Greenstein, *Diffuse radiation in the galaxy*, 1941 (the phase function).
	 *
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class EMEN_API VolumetricScattering final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VolumetricScatteringEffect"};

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note The participating medium, marched. Exclusive with AtmosphericFog by
			 * construction — see the class note. */
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
			 * @brief The TECHNIQUE knobs of the march.
			 * @note ⚠️ Every physical quantity is absent on purpose: it belongs to the medium or to
			 * the light. What is left here is the quality/cost trade of THIS integrator.
			 */
			struct EMEN_API Parameters
			{
				/**
				 * @brief Number of march steps along the view ray.
				 * @note Cost is linear in this and it is the only quality knob that matters.
				 * ⚠️ It is NOT a fidelity knob you can lower freely: the march origin is dithered
				 * by a fraction of one step, so fewer steps means a longer step and coarser
				 * sub-step speckle, not smooth blur.
				 */
				uint32_t sampleCount{32};
			};

			/**
			 * @brief Push constants for the scattering pass (120 bytes).
			 */
			struct EMEN_API ScatteringPushConstants
			{
				/* Camera basis — extracted from the view matrix in execute(). */
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
				/* The medium, straight from Scenes::ParticipatingMedium. */
				float mediumDensity;
				float mediumHeightFalloff;
				float mediumBaseHeight;
				float mediumMaxDistance;
				float scatteringAlbedoR;
				float scatteringAlbedoG;
				float scatteringAlbedoB;
				float phaseAnisotropy;
				/* The light: direction of propagation, and colour times illuminance in lux. */
				float lightDirX;
				float lightDirY;
				float lightDirZ;
				float lightIlluminanceR;
				float lightIlluminanceG;
				float lightIlluminanceB;
				/* March and shadow lookup. */
				float sampleCount;
				float cascadeCount;
				float shadowBias;
			};

			/* The Vulkan spec only guarantees 128 bytes for maxPushConstantsSize, and part of the
			 * AMD/Intel fleet exposes exactly that. A block over the floor makes
			 * PipelineLayout::create() FAIL on those devices, so the effect is never created and
			 * its contribution silently disappears -- invisible on NVIDIA, which exposes 256. That
			 * is exactly how RTR was dead on min-spec until ae61e368. This is also why the four
			 * cascade matrices (256 bytes on their own) travel in a per-frame UBO instead.
			 * 120 bytes: 8 bytes (2 floats) of headroom left. */
			static_assert(sizeof(ScatteringPushConstants) <= 128, "Push constant block over the 128-byte Vulkan minimum guarantee: move it to a per-frame UBO.");

			/**
			 * @brief Constructs a volumetric scattering effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			VolumetricScattering (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Constructs a volumetric scattering effect.
			 * @param renderer A reference to the graphics renderer.
			 * @param parameters The initial parameters.
			 */
			VolumetricScattering (Renderer & renderer, const Parameters & parameters) noexcept
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
			 * @brief Sets the scattering parameters.
			 * @param parameters The new parameters.
			 * @return void
			 */
			void
			setParameters (const Parameters & parameters) noexcept
			{
				m_parameters = parameters;
			}

			/**
			 * @brief Returns the current scattering parameters.
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
			std::shared_ptr< Vulkan::GraphicsPipeline > m_scatteringPipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_scatteringLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_scatteringPerFrame;
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_cascadeUBOs;
			/* One warning per effect instance, not one per frame. */
			bool m_missingMediumReported{false};
			bool m_missingCSMLightReported{false};
	};
}
