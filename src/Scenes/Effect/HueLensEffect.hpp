/*
 * src/Scenes/Effect/HueLensEffect.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for inheritances. */
#include "Saphir/FramebufferEffectInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureResource/Texture2D.hpp"

namespace Emeraude::Scenes
{
	/**
	 * @brief The hue lens effect class.
	 * @extends Emeraude::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class HueLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"HueLensEffect"};

			/**
			* @brief Constructs a vignetting lens effect.
			* @param hue The initial hue value in radians. 0.0 does nothing.
			*/
			explicit HueLensEffect (float hue = 0.0F) noexcept;

			/** @copydoc Emeraude::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the hue degree.
			 * @param value A value between 0.0 and 360.0.
			 * @return void
			 */
			void setHue (float value) noexcept;

			/**
			 * @brief Returns the hue degree.
			 * @return float
			 */
			[[nodiscard]]
			float hue () const noexcept;

		private:

			static constexpr auto Hue{"em_Hue"};

			float m_hue{0.0F};
	};
}
