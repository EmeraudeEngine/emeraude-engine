/*
 * src/Graphics/Effects/Style/ColorGrading.hpp
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
#include <algorithm>

/* Local inclusions for inheritances. */
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Graphics::Effects::Style
{
	/**
	 * @brief The color grading lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 */
	class EMEN_API ColorGrading final : public DirectPostProcessEffect
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
			 * @brief Sets the white balance: the image is graded as if lit by a black body at @a kelvin.
			 * @note THE tool for a warm or cool look (2026-09-26). 6500 K is exactly neutral; lower is warmer
			 * (3500 K reads as a golden hour), higher is cooler (10000 K as a blue hour). Applied FIRST, in
			 * LINEAR light (the tone mapper's 2.2 is undone, then redone): per-channel gains from
			 * Photometry::linearColorFromTemperature(), divided by the 6500 K value, normalized to keep the
			 * Rec.709 luminance — a white balance moves the colour, never the brightness.
			 * ⚠️ Never warm an image with setHue(): a hue ROTATION turns every colour by the same angle, which
			 * adds no orange at all — it is what made Golden Hour green-cyan (sky 70/119/167 -> 36/104/123) and
			 * every "warm" style of the catalogue green or magenta instead.
			 * @param kelvin The colour temperature, clamped to [1667, 25000] K.
			 * @param tint The green-magenta axis, Lightroom convention: positive = magenta, negative = green, 0 = none
			 * (the green gain is scaled by 2^-tint, then the luminance is normalized).
			 * @return void
			 */
			void setWhiteBalance (float kelvin, float tint = 0.0F) noexcept;

			/**
			 * @brief Returns the white-balance temperature, in kelvins (6500 = neutral).
			 * @return float
			 */
			[[nodiscard]]
			float
			whiteBalanceTemperature () const noexcept
			{
				return m_temperature;
			}

			/**
			 * @brief Returns the white-balance tint (0 = none, positive = magenta).
			 * @return float
			 */
			[[nodiscard]]
			float
			whiteBalanceTint () const noexcept
			{
				return m_tint;
			}

			/**
			 * @brief Sets the hue rotation angle.
			 * @note A ROTATION of every hue — a creative colour shift (a VHS drift). ⚠️ Not a warm/cool control:
			 * use setWhiteBalance() for that.
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
			float m_temperature{6500.0F};
			float m_tint{0.0F};
	};
}
