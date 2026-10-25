/*
 * src/Graphics/Effects/Atmosphere/StarField.hpp
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
#include "Vulkan/ShaderStorageBufferObject.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
	class StarCatalogueResource;
}

namespace EmEn::Vulkan
{
	class CommandBuffer;
	class DescriptorSetLayout;
	class GraphicsPipeline;
	class PipelineLayout;
	class TextureInterface;
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	/**
	 * @brief The STARS of a physical sky (engine item dynamic-sky-stars-and-milky-way, lot S2; graphics doc 42 § "The
	 * stars"): every star of the sky's catalogue drawn as a point of light carrying its illuminance, through the
	 * atmosphere, on the sky pixels.
	 * @note An OVERLAY effect in EffectSlot::Stars, after the temporal anti-aliasing (owner decision 2026-10-25). ONE
	 * instanced pass at full resolution: a quad per star, additively blended, holding a normalised Gaussian footprint
	 * whose radiance integrates over the pixels' solid angles to the star's illuminance E = E₀ 10^(−0.4 V) × its colour
	 * (B−V → temperature, Ballesteros 2012) × the atmosphere's transmittance toward it. The combine adds that target to
	 * the chain colour where the depth is the sky's (1.0), times the clouds' transmittance when the scene holds clouds.
	 * @note The star's position is the catalogue's J2000 direction turned into the world by the clock's equatorial frame
	 * (Scenes::SkyState::equatorialFrame) and projected with the UNJITTERED projection. Below the horizon (the ray meets
	 * the planet) and behind the star's or a moon's disc (the sky's frame texture) a star is not drawn.
	 * @note SCENE-DRIVEN: filed by PostProcessStack::syncSceneEffects() the first frame the physical sky draws a catalogue
	 * under a celestial clock; on a frame without a star field (FrameContext::stars) it records nothing and adds nothing.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API StarField final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"StarFieldEffect"};

			/** @brief The illuminance of a star of visual magnitude 0 above the atmosphere, in lux: the engine's own Sun
			 * (Scenes::Component::CelestialClock, 133 800 lx at 1 AU) at its V = −26.74 (Allen's Astrophysical
			 * Quantities), so that a star and the Sun obey ONE photometric scale. 2.69e-6 lx, within 6 % of the
			 * 2.54e-6 lx often quoted. */
			static constexpr auto MagnitudeZeroIlluminance{2.69e-6F};

			/** @brief The standard deviation of a star's footprint, in pixels: wide enough that the sampled Gaussian sums
			 * to 1 within 1e-4 wherever the star falls inside its pixel (no sub-pixel twinkling as the sky turns), and
			 * a point still. */
			static constexpr auto FootprintSigma{0.7F};

			/** @brief The half-size of a star's quad, in footprint sigmas: 3.5σ leaves under 0.1 % of the light outside. */
			static constexpr auto FootprintExtent{3.5F};

			/** @brief The ceiling of a star's peak radiance in chain units (a half float stops at 65 504). */
			static constexpr auto MaxChainRadiance{60000.0F};

			/**
			 * @brief A star as the GPU reads it (std430).
			 */
			struct GPUStar
			{
				/** @brief xyz: the unit J2000 direction, w: unused. */
				std::array< float, 4 > direction;
				/** @brief rgb: the illuminance above the atmosphere × the unit-luminance colour, in lux; w: unused. */
				std::array< float, 4 > illuminance;
			};

			static_assert(sizeof(GPUStar) == 32, "A star is two vec4 in the shader's std430 buffer.");

			/**
			 * @brief The push constants of the star pass (vertex and fragment stages).
			 */
			struct EMEN_API PushConstants
			{
				/** @brief The unjittered projection × the view ROTATION, column-major: a world direction to clip space. */
				std::array< float, 16 > viewProjection;
				/** @brief The equatorial axes in world vectors (xyz), then w: the footprint sigma (px), its half-size
				 * (px), the frame's pre-exposure. */
				std::array< float, 4 > axisX;
				std::array< float, 4 > axisY;
				std::array< float, 4 > axisZ;
				/** @brief x, y: the target's size in pixels; z, w: the tangents of the half fields of view (1 / P00,
				 * 1 / P11). */
				std::array< float, 4 > viewport;
			};

			static_assert(sizeof(PushConstants) == 128, "The star pass push constants are a mat4 and four vec4.");
			static_assert(sizeof(PushConstants) <= 128, "Over the 128-byte Vulkan minimum guarantee of maxPushConstantsSize.");

			/**
			 * @brief Constructs a star field.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			StarField (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot() */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::Stars;
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::producesOverlay() */
			[[nodiscard]]
			bool
			producesOverlay () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::recordOverlayPasses() */
			void recordOverlayPasses (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::TextureInterface & inputColor, const FrameContext & context) noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::combineContribution() */
			[[nodiscard]]
			CombineContribution combineContribution (const FrameContext & context) const noexcept override;

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresDepth()
			 * @note The combine adds the stars where the depth is the sky's only. */
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
			 * @copydoc EmEn::Graphics::IndirectPostProcessEffect::consumesCloudTransmittance()
			 * @note The stars are added after the clouds composited the sky: behind a cloud a star is worth the cloud's
			 * transmittance.
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

			/**
			 * @brief Returns a star's illuminance above the atmosphere, in lux: E₀ 10^(−0.4 V).
			 * @param visualMagnitude The Johnson V magnitude.
			 * @return float
			 */
			[[nodiscard]]
			static float illuminance (float visualMagnitude) noexcept;

			/**
			 * @brief Returns a star's effective temperature from its B−V colour index (Ballesteros, "New insights into
			 * black bodies", EPL 97, 34008, 2012): T = 4600 K (1 / (0.92 (B−V) + 1.7) + 1 / (0.92 (B−V) + 0.62)).
			 * @note The colour index is clamped to the catalogue's accepted range (StarCatalogueResource), where both
			 * denominators stay positive.
			 * @param colourIndex The B−V colour index.
			 * @return float The temperature, in kelvins.
			 */
			[[nodiscard]]
			static float temperature (float colourIndex) noexcept;

		private:

			/**
			 * @brief Builds the GPU stars of a catalogue into a new buffer.
			 * @note ⚠️ RENDER THREAD. The previous buffer is retired through the deferred destructor: frames in flight
			 * still read it.
			 * @param catalogue A reference to the loaded catalogue's pointer (kept as a weak reference: the identity of
			 * what the buffer holds).
			 * @return bool
			 */
			[[nodiscard]]
			bool uploadCatalogue (const std::shared_ptr< const StarCatalogueResource > & catalogue) noexcept;

			/**
			 * @brief Records the star pass into the target: the clear, then the stars when there is a set.
			 * @param commandBuffer A reference to the command buffer.
			 * @param descriptorSet A pointer to this frame's written descriptor set, or nullptr: the target is only cleared.
			 * @param pushConstants A reference to the push constants.
			 */
			void recordStarPass (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::DescriptorSet * descriptorSet, const PushConstants & pushConstants) const noexcept;

			/* The clouds' view transmittance for THIS frame, or nullptr (set every frame by the stack). */
			const Vulkan::TextureInterface * m_cloudTransmittance{nullptr};
			/* The catalogue the buffer was built from: expired or another one, the buffer is rebuilt. */
			std::weak_ptr< const StarCatalogueResource > m_uploadedCatalogue;
			IntermediateRenderTarget m_target;
			std::unique_ptr< Vulkan::ShaderStorageBufferObject > m_starBuffer;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_inputLayout;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_perFrame;
			uint32_t m_starCount{0};
	};
}
