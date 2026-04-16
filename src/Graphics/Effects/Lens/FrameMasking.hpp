/*
 * src/Graphics/Effects/Lens/FrameMasking.hpp
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

/* Local inclusions for inheritances. */
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	/**
	 * @brief The CRT frame masking lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Simulates the rounded corners of a CRT tube bezel by applying
	 * a soft black mask using a signed distance field rounded rectangle.
	 */
	class FrameMasking final : public DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FrameMasking"};

			/**
			 * @brief Constructs a frame masking lens effect.
			 * @param cornerRadius The corner radius as a fraction of screen size. Default 0.04.
			 */
			explicit
			FrameMasking (float cornerRadius = 0.04F) noexcept
				: m_cornerRadius{cornerRadius}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the corner radius.
			 * @param radius Value in range (0, 0.5]. Default 0.04.
			 * @return void
			 */
			void setCornerRadius (float radius) noexcept;

			/**
			 * @brief Returns the corner radius.
			 * @return float
			 */
			[[nodiscard]]
			float
			cornerRadius () const noexcept
			{
				return m_cornerRadius;
			}

			/**
			 * @brief Sets the edge softness of the mask transition.
			 * @param softness Value > 0. Default 0.01.
			 * @return void
			 */
			void setEdgeSoftness (float softness) noexcept;

			/**
			 * @brief Returns the edge softness.
			 * @return float
			 */
			[[nodiscard]]
			float
			edgeSoftness () const noexcept
			{
				return m_edgeSoftness;
			}

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_cornerRadius{0.04F};
			float m_edgeSoftness{0.01F};
	};
}
