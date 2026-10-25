/*
 * src/Graphics/Effects/Atmosphere/AerialPerspective.hpp
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

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Vulkan
{
	class GraphicsPipeline;
	class PipelineLayout;
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	/**
	 * @brief The AERIAL PERSPECTIVE of a physical sky: the distance haze of the atmosphere the sky is drawn from.
	 * @details Each geometry pixel becomes `scene · T + L`, T the RGB transmittance and L the in-scattered radiance
	 * between the camera and the surface, read from the froxel volume the sky's atmosphere builds each frame
	 * (SkyAtmosphere::recordAerialPerspective(): 32 × 32 froxels, 32 slices in a squared distribution of 4 km slices).
	 * The sky pixels are left alone: the drawn sky already is the whole atmosphere.
	 * @note A SCENE-DRIVEN effect (EffectSlot::AerialPerspective): filed by PostProcessStack::syncSceneEffects() the
	 * first frame the scene's background is a Renderable::DynamicSkyResource, never added by an application. A frame
	 * without a submitted atmosphere (FrameContext::atmosphere null) passes the chain colour through.
	 * @note Owner decisions (2026-10-25, engine graphics doc 42 § "The aerial perspective"): it owns the distance haze
	 * under a dynamic sky (the Fog slot keeps the LOCAL medium), RGB transmittance, its own slot after the geometry
	 * and before the clouds and the fog.
	 * @note Reference: S. Hillaire, "A Scalable and Production Ready Sky and Atmosphere Rendering Technique", EGSR 2020,
	 * and his reference implementation (github.com/sebh/UnrealEngineSkyAtmosphere, MIT licence:
	 * RenderSkyRayMarching.hlsl, the AerialPerspective application of RenderRaymarchingPS) — the squared slice
	 * distribution, the square-root lookup and the linear fade over the first half slice.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a single-pass post-process effect.
	 */
	class EMEN_API AerialPerspective final : public IndirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AerialPerspectiveEffect"};

			/**
			 * @brief The pass's push constants (80 bytes).
			 */
			struct EMEN_API PushConstants
			{
				/* The inverse of projection × view ROTATION (camera-relative), column-major: the pixel's distance from
				 * the camera, reconstructed exactly as the volume's froxels were placed. */
				std::array< float, 16 > invRelativeViewProj;
				/* x: the frame's pre-exposure, y: kilometres per slice, z: the slice count, w: unused (the world unit is the metre). */
				std::array< float, 4 > parameters;
			};

			static_assert(sizeof(PushConstants) == 80, "The aerial perspective push constants are a mat4 and a vec4.");
			static_assert(sizeof(PushConstants) <= 128, "Over the 128-byte Vulkan minimum guarantee of maxPushConstantsSize.");

			/**
			 * @brief Constructs an aerial perspective effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			AerialPerspective (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::slot()
			 * @note After the geometry, before the clouds and the fog. */
			[[nodiscard]]
			EffectSlot
			slot () const noexcept override
			{
				return EffectSlot::AerialPerspective;
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

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresMaterialProperties()
			 * @note The fog response (the high nibble of the alpha channel): a surface immune to the fog (an in-world
			 * icon, a HUD overlay) is immune to the haze too. */
			[[nodiscard]]
			bool
			requiresMaterialProperties () const noexcept override
			{
				return true;
			}

		private:

			IntermediateRenderTarget m_outputTarget;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_perFrame;
	};
}
