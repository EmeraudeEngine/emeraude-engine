/*
 * src/Graphics/Effects/Lens/Retro.hpp
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

/* Local inclusions for inheritances. */
#include "Graphics/DirectPostProcessEffect.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	/**
	 * @brief The retro lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Quantizes each RGB channel to a configurable number of discrete levels,
	 * simulating the limited color palettes of retro hardware.
	 */
	class Retro final : public Graphics::DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Retro"};

			/**
			 * @brief Constructs a retro lens effect.
			 * @param levels Number of discrete levels per RGB channel. Range [2, 256]. Default 8.
			 */
			explicit
			Retro (int levels = 8) noexcept
				: m_levels{std::clamp(levels, 2, 256)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the number of quantization levels per RGB channel.
			 * @param levels Value in range [2, 256].
			 * @return void
			 */
			void setLevels (int levels) noexcept;

			/**
			 * @brief Returns the number of quantization levels per RGB channel.
			 * @return int
			 */
			[[nodiscard]]
			int
			levels () const noexcept
			{
				return m_levels;
			}

		private:

			int m_levels{8};
	};
}
