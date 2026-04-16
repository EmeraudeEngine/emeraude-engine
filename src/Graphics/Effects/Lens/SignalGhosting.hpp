/*
 * src/Graphics/Effects/Lens/SignalGhosting.hpp
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
	 * @brief The signal ghosting lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Simulates the ghost image (multipath reception) seen on analog TV,
	 * where a faint duplicate of the image appears offset to the right
	 * due to reflected antenna signals arriving with a delay.
	 */
	class SignalGhosting final : public DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SignalGhosting"};

			/**
			 * @brief Constructs a signal ghosting lens effect.
			 * @param intensity The ghost opacity (0 = off, 1 = full duplicate). Default 0.12.
			 */
			explicit
			SignalGhosting (float intensity = 0.12F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the ghost opacity.
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the ghost opacity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the horizontal UV offset of the ghost image.
			 * @param offset Value > 0. Default 0.015.
			 * @return void
			 */
			void setOffset (float offset) noexcept;

			/**
			 * @brief Returns the horizontal offset.
			 * @return float
			 */
			[[nodiscard]]
			float
			offset () const noexcept
			{
				return m_offset;
			}

			/**
			 * @brief Enables or disables a second, fainter ghost.
			 * @param enabled True to enable the second ghost (further offset, lower opacity).
			 * @return void
			 */
			void
			enableSecondGhost (bool enabled) noexcept
			{
				m_secondGhostEnabled = enabled;
			}

			/**
			 * @brief Returns whether the second ghost is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			secondGhostEnabled () const noexcept
			{
				return m_secondGhostEnabled;
			}

		private:

			float m_intensity{0.12F};
			float m_offset{0.015F};
			bool m_secondGhostEnabled{false};
	};
}
