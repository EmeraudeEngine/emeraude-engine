/*
 * src/Scenes/Effect/SaturationLensEffect.hpp
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
	 * @brief The saturation lens effect class;
	 * @extends EmEn::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class SaturationLensEffect final : public Saphir::FramebufferEffectInterface
	{
		public:

			/**
			 * @brief Constructs a saturation lens effect.
			 * @param saturation The initial saturation value. 0.0 does nothing.
			 */
			explicit SaturationLensEffect (float saturation = 0.0F) noexcept;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the saturation level.
			 * @param value The saturation value.
			 * @return void
			 */
			void setSaturation (float value) noexcept;

			/**
			 * @brief Returns the saturation level.
			 * @return float
			 */
			[[nodiscard]]
			float saturation () const noexcept;

		private:

			static constexpr auto Saturation{"em_Saturation"};

			float m_saturation{0.0F};
	};
}
