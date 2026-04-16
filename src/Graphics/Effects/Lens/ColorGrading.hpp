/*
 * src/Graphics/Effects/Lens/ColorGrading.hpp
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

/* STL inclusions. */
#include <algorithm>

/* Local inclusions for inheritance. */
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	/**
	 * @brief The color grading lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 */
	class ColorGrading final : public DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ColorGrading"};

			/**
			 * @brief Constructs a color grading lens effect.
			 */
			explicit ColorGrading () noexcept = default;

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the saturation level.
			 * @param saturation 0 = grayscale, 1 = normal, >1 = over-saturated.
			 * @return void
			 */
			void setSaturation (float saturation) noexcept;

			/**
			 * @brief Returns the saturation level.
			 * @return float
			 */
			[[nodiscard]]
			float
			saturation () const noexcept
			{
				return m_saturation;
			}

			/**
			 * @brief Sets the hue rotation angle.
			 * @param hue Rotation in radians (YIQ color space).
			 * @return void
			 */
			void
			setHue (float hue) noexcept
			{
				m_hue = hue;
			}

			/**
			 * @brief Returns the hue rotation angle.
			 * @return float
			 */
			[[nodiscard]]
			float
			hue () const noexcept
			{
				return m_hue;
			}

			/**
			 * @brief Sets the brightness offset.
			 * @param brightness Value in range [-1, 1].
			 * @return void
			 */
			void
			setBrightness (float brightness) noexcept
			{
				m_brightness = std::clamp(brightness, -1.0F, 1.0F);
			}

			/**
			 * @brief Returns the brightness offset.
			 * @return float
			 */
			[[nodiscard]]
			float
			brightness () const noexcept
			{
				return m_brightness;
			}

			/**
			 * @brief Sets the contrast multiplier.
			 * @param contrast Value >= 0.
			 * @return void
			 */
			void setContrast (float contrast) noexcept;

			/**
			 * @brief Returns the contrast multiplier.
			 * @return float
			 */
			[[nodiscard]]
			float
			contrast () const noexcept
			{
				return m_contrast;
			}

			/**
			 * @brief Sets the gamma correction value.
			 * @param gamma Value > 0.
			 * @return void
			 */
			void setGamma (float gamma) noexcept;

			/**
			 * @brief Returns the gamma correction value.
			 * @return float
			 */
			[[nodiscard]]
			float
			gamma () const noexcept
			{
				return m_gamma;
			}

		private:

			float m_saturation{1.0F};
			float m_hue{0.0F};
			float m_brightness{0.0F};
			float m_contrast{1.0F};
			float m_gamma{1.0F};
	};
}
