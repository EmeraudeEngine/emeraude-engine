/*
 * src/Graphics/Effects/Display/Sharpen.hpp
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

namespace EmEn::Graphics::Effects::Display
{
	/**
	 * @brief Adaptive contrast sharpening (CAS-style), compiled into the final
	 * post-process shader.
	 * @note DISPLAY effect: operates on display-referred (post-tonemap) values inside
	 * the final fullscreen pass — it costs zero extra render pass, zero render target.
	 * Successor of the retired Effects::Framebuffer::Sharpen single-pass effect.
	 * Add it to the scene through PostProcessStack::addDisplayEffect().
	 * @extends EmEn::Graphics::DirectPostProcessEffect This effect generates fragment shader code.
	 */
	class EMEN_API Sharpen final : public DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DisplaySharpen"};

			/** @brief Effect parameters, baked into the generated shader. */
			struct EMEN_API Parameters
			{
				float sharpness{0.5F}; /**< 0.0 = off, 0.5 = moderate, 1.0+ = strong */
			};

			/**
			 * @brief Constructs a display sharpen effect.
			 */
			Sharpen () noexcept = default;

			/**
			 * @brief Constructs a display sharpen effect.
			 * @param parameters The effect parameters.
			 */
			explicit
			Sharpen (const Parameters & parameters) noexcept
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

		private:

			Parameters m_parameters{};
	};
}
