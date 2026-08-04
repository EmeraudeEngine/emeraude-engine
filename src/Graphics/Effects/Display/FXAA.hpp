/*
 * src/Graphics/Effects/Display/FXAA.hpp
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

/* Local inclusions for inheritances. */
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Saphir
{
	class FragmentShader;
}

namespace EmEn::Graphics::Effects::Display
{
	/**
	 * @brief FXAA 3.11 Quality 12 anti-aliasing, compiled into the final post-process shader.
	 * @note DISPLAY effect: operates on display-referred (post-tonemap) values inside the
	 * final fullscreen pass — it costs zero extra render pass, zero render target.
	 * Successor of the retired Effects::Framebuffer::FXAA single-pass effect.
	 * Add it to the scene through PostProcessStack::addDisplayEffect().
	 * @warning This effect OVERRIDES the fragment fetching (it reconstructs the anti-aliased
	 * color from the neighborhood): at most ONE fetch-overriding display effect per stack.
	 * Based on Timothy Lottes' FXAA 3.11 (NVIDIA).
	 * @extends EmEn::Graphics::DirectPostProcessEffect This effect generates fragment shader code.
	 */
	class EMEN_API FXAA final : public DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DisplayFXAA"};

			/** @brief Effect parameters, baked into the generated shader. */
			struct EMEN_API Parameters
			{
				float subpixelQuality{0.75F};	/**< 0=off, 0.75=default, 1=max subpixel AA */
				float edgeThreshold{0.166F};	 /**< Lower = more edges detected (0.125-0.333) */
				float edgeThresholdMin{0.0833F}; /**< Dark area minimum threshold (0.0312-0.0833) */
			};

			/**
			 * @brief Constructs a display FXAA effect.
			 * @param parameters The effect parameters.
			 */
			explicit
			FXAA (const Parameters & parameters) noexcept
				: m_parameters{parameters}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Returns the effect parameters.
			 * @return const Parameters &
			 */
			[[nodiscard]]
			const Parameters &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Declares the em_fxaaLuma()/em_fxaa() GLSL functions into a fragment shader.
			 * @note Shared with the FXAASharpen display effect, which calls em_fxaa() before
			 * its own sharpening code. Declaring twice in the same shader is invalid GLSL —
			 * hence the "at most one fetch-overriding display effect" rule.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			static bool declareFunctions (Saphir::FragmentShader & fragmentShader) noexcept;

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::overrideFragmentFetching() */
			[[nodiscard]]
			bool
			overrideFragmentFetching () const noexcept override
			{
				return true;
			}

			Parameters m_parameters{};
	};
}
