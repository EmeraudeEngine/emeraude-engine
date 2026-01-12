/*
 * src/Scenes/Effect/BrightnessContrastLensEffect.hpp
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
#include "Saphir/FramebufferEffectInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureResource/Texture2D.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The brightness contrast lens effect class.
	 * @extends EmEn::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class BrightnessContrastLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"BrightnessContrastLensEffect"};

			/**
			 * @brief Constructs a brightness contrast lens effect.
			 * @param brightness The initial brightness value. 0.0 does nothing.
			 * @param contrast The initial contrast value. 1.0 does nothing.
			 */
			explicit BrightnessContrastLensEffect (float brightness = 0.0F, float contrast = 1.0F) noexcept;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the brightness.
			 * @param value The brightness value.
			 * @return void
			 */
			void setBrightness (float value) noexcept;

			/**
			 * @brief Returns the brightness value.
			 * @return float
			 */
			[[nodiscard]]
			float brightness () const noexcept;

			/**
			 * @brief Sets the contrast.
			 * @param value The contrast value.
			 * @return void
			 */
			void setContrast (float value) noexcept;

			/**
			 * @brief Returns the contrast value.
			 * @return float
			 */
			[[nodiscard]]
			float contrast () const noexcept;

		private:

			static constexpr auto Brightness{"em_Brightness"};
			static constexpr auto Contrast{"em_Contrast"};

			float m_brightness{0.0F};
			float m_contrast{1.0F};
	};
}
