/*
 * src/Graphics/Effects/Atmosphere/WaterVolume.hpp
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
#include "Graphics/EffectSlot.hpp"
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/UniformBufferObject.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Vulkan
{
	class CommandBuffer;
	class GraphicsPipeline;
	class PipelineLayout;
	class TextureInterface;
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	/**
	 * @brief The WATER between the eye and what it sees, when the eye is under the sea: the underwater view's volume.
	 * @details A pixel whose ray starts under the water (its point on the NEAR plane stands below the DRAWN surface:
	 * the level of a sea at rest, the mesh of a sea whose surface moves — Graphics::SeaSurfaceWaves) becomes `scene · T + L` over its path in the water — from that point to the first surface the depth buffer holds
	 * (the seabed, or the sea surface seen from below), or to where it leaves the water:
	 * - T = exp(−σt · s), per channel, σt the water's extinction;
	 * - L = S · exp(−Kd · z₀) · (1 − exp(−(σt − Kd · ωy) · s)) / (σt − Kd · ωy): the light the water scatters toward
	 *   the eye along the path, its source the downwelling light falling off with the depth (z₀ the depth of the
	 *   path's start, ωy the ray's vertical component — looking up goes toward the light, looking down away from it).
	 * S = σs / 4π · E, E the illuminance that entered the water (the sun through the surface, and the sky); Kd, the
	 * diffuse attenuation with depth, is (σa + B · σs) / μ, B the share of the scattering that goes backward (from
	 * the phase anisotropy) and μ the cosine of the refracted sun. Every pixel above the water is left alone. Where the
	 * lens meets the water, a thin dark line: the meniscus.
	 * @note Three cases a frame, read from FrameContext::seaImmersion (Graphics/SeaImmersion.hpp — decided once a frame
	 * for every effect, from the near plane's corners): Clear — the effect is not executed; Submerged — the variant at
	 * rest, every pixel under water; Astride — the moving variant, which stands each pixel's near-plane point against
	 * the mesh, or the variant at rest against the level when the surface does not move.
	 * @note Where the mesh stands is the OCEAN's own answer: the moving variant binds the resources the ocean's stages
	 * read (Graphics::SeaSurfaceWaves: the displacement, the shelter map, the surface's uniform block) and compiles the
	 * ocean's own GLSL, `ocSurfaceRiseAt()` (Saphir::Generator::getOceanSurfaceStandingSource()). Nothing of the
	 * mesh is restated here (owner decision 2026-11-24): a sheltered water, a faded cascade, are the ocean's to say.
	 * @note The water is the one the scene's sea declares (Scenes::WaterMedium through
	 * Scenes::SeaLevelInterface::waterMedium(), FrameContext::sea — for a renderable sea, the water its surface
	 * material declares): its coefficients give the water its colour, no colour is authored here. The engine item underwater-view holds the owner's decisions (2026-11-23).
	 * @note A SCENE-DRIVEN effect (EffectSlot::WaterVolume): filed by PostProcessStack::syncSceneEffects() the first
	 * frame the scene's sea declares its water, never added by an application. A frame whose near plane stands clear
	 * of the water passes the chain colour through: above the sea the effect costs nothing.
	 * @note The sun's SHAFTS (2026-11-25): where the sea draws them and the sun casts cascaded shadows, two more variants
	 * take the sun's share out of the closed form and march it along the path — at each step the sun's shadow and the
	 * caustics' gain the waves give the light on its way down (the light pass's own term,
	 * Graphics/Effects/Shared/SeaCausticsGLSL.hpp), as a deviation from the closed form: a water nothing modulates
	 * gives it back to the bit. Graphics doc 45 § 5.
	 * @note NOT here: the light's own path through the water ONTO what it lights (the sun dimming and turning blue with
	 * depth, the caustics on the seabed — the sun's light pass, graphics doc 45). The fog response
	 * of a surface (the material properties' alpha) is not read: under water nothing is exempt from the water.
	 * @note Reference: S. Premože, M. Ashikhmin, "Rendering Natural Waters", Computer Graphics Forum 20(4), 2001 — the
	 * analytic single-scattering form along a view path under a downwelling light that decays with depth.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class EMEN_API WaterVolume final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"WaterVolumeEffect"};

			/** @brief The share of the sky's light that enters the water (a diffuse sky loses about 6.6 % at the surface). */
			static constexpr auto SkyTransmission{0.934F};

			/** @brief The lowest cosine the downwelling light is given (a grazing sun still lights the water from above). */
			static constexpr auto MinDownwellingCosine{0.5F};

			/** @brief The half-width of the meniscus, in pixels: the dark line where the lens meets the water. */
			static constexpr auto MeniscusHalfWidth{2.0F};

			/** @brief What the meniscus keeps of the light at its centre. */
			static constexpr auto MeniscusTransmission{0.45F};

			/**
			 * @brief The pass's push constants (48 bytes), the same for both variants.
			 */
			struct EMEN_API PushConstants
			{
				/* xyz: the water's extinction σt per channel (1/m), w: the sea level (m, world Y). */
				std::array< float, 4 > extinction;
				/* xyz: the diffuse attenuation with depth Kd per channel (1/m), w: the highest the surface can stand
				 * above the sea level (m) — where an upward ray that meets no drawn surface leaves the water. */
				std::array< float, 4 > depthAttenuation;
				/* xyz: the scattering source S per channel just under the surface, PRE-EXPOSED (nits × E_pre per metre
				 * of path), w: the meniscus' half-width (pixels). */
				std::array< float, 4 > source;
			};

			/** @brief How many steps the sun's shafts are marched with, along the part of a pixel's path in the water. */
			static constexpr uint32_t ShaftSampleCount{24};

			/**
			 * @brief The uniform block of the variants that march the sun's shafts (std140, seven vec4).
			 */
			struct EMEN_API ShaftBlock
			{
				/* xyz: the SUN's scattering source just under the surface, PRE-EXPOSED, per metre of path (the sky's stays
				 * in PushConstants::source), w: the number of march steps. */
				std::array< float, 4 > sunSource;
				/* xyz: the direction the sun's light TRAVELS in the water (refracted, unit, pointing down), w: the
				 * anisotropy of its scattering toward the eye (Henyey-Greenstein g). */
				std::array< float, 4 > sunDirection;
				/* xyz: the camera's forward axis (the shadow cascades are selected on a view depth), w: the cascade count. */
				std::array< float, 4 > cameraForward;
				/* xyz: the tile side of each wave cascade (m), w: how much of the caustics is drawn (0-1). */
				std::array< float, 4 > cascadeSizes;
				/* The shelter map's rectangle: its first corner's world X and Z, 1 / its side on X, 1 / its side on Z. */
				std::array< float, 4 > shelterRectangle;
				/* xyz: what a sheltered water keeps of each cascade, w: the caustics' low-pass (m per metre of depth). */
				std::array< float, 4 > shelteredShares;
				/* x: the depth the caustics fade over (m), y: the shadow bias, z: how much of the shafts is drawn (0-1). */
				std::array< float, 4 > tuning;
			};

			static_assert(sizeof(ShaftBlock) == 112, "The shafts' block is seven vec4.");

			static_assert(sizeof(PushConstants) == 48, "The water volume push constants are three vec4.");
			static_assert(sizeof(PushConstants) <= 128, "Over the 128-byte Vulkan minimum guarantee of maxPushConstantsSize.");

			/**
			 * @brief Constructs a water volume effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			WaterVolume (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note The last of the media: the water is the nearest to the eye. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::WaterVolume;
			}

			/** @copydoc EmEn::Graphics::PostProcessEffect::label() */
			[[nodiscard]]
			const char *
			label () const noexcept override
			{
				return ClassId;
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

		private:

			/**
			 * @brief One variant of the pass (pipeline variants rather than branches in the shader): the surface AT
			 * REST (the level) or the MOVING surface (the level and the waves' displacement), each with the sun's light
			 * scattered evenly along the path (the closed form) or MARCHED — the sun's shafts.
			 */
			struct Variant
			{
				std::shared_ptr< Vulkan::GraphicsPipeline > pipeline;
				std::shared_ptr< Vulkan::PipelineLayout > pipelineLayout;
				std::vector< std::unique_ptr< Vulkan::DescriptorSet > > perFrame;
				/* The marched variants' own blocks, one a frame in flight: the shadow cascades, the shafts. */
				std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > cascadeBlocks;
				std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > shaftBlocks;
			};

			/**
			 * @brief Creates one variant of the pass.
			 * @param variant A reference to the variant to fill.
			 * @param movingSurface Whether the variant stands a pixel against the ocean's mesh.
			 * @param shafts Whether the variant marches the sun's shafts.
			 * @return bool
			 */
			[[nodiscard]]
			bool createVariant (Variant & variant, bool movingSurface, bool shafts) noexcept;

			/**
			 * @brief Fills and binds what a marched variant reads this frame: the sun's shadow cascades, the caustics'
			 * curvature and shelter map, the shafts' block.
			 * @param variant The marched variant.
			 * @param movingSurface Whether it is the moving one (its bindings differ).
			 * @param context The per-frame chain context.
			 * @param sunSource The sun's pre-exposed scattering source per metre of path.
			 * @return bool False when something is missing: the frame falls back on the closed form.
			 */
			[[nodiscard]]
			bool bindShafts (const Variant & variant, bool movingSurface, const FrameContext & context, const std::array< float, 3 > & sunSource) const noexcept;

			/**
			 * @brief Tells whether this frame can march the sun's shafts: a sea that draws them, a sun that casts
			 * cascaded shadows and is up.
			 * @param context The per-frame chain context.
			 * @return bool
			 */
			[[nodiscard]]
			static bool shaftsWanted (const FrameContext & context) noexcept;

			/**
			 * @brief Records the pass when a pixel of the frame can be under water.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param inputColor The chain colour.
			 * @param context The per-frame chain context.
			 * @return bool False when no pixel can be, or when what the pass reads is missing: nothing was recorded.
			 */
			[[nodiscard]]
			bool record (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept;

			IntermediateRenderTarget m_outputTarget;
			Variant m_atRest;
			Variant m_moving;
			Variant m_atRestShafts;
			Variant m_movingShafts;
	};
}
