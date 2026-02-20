/*
 * src/Graphics/Effects/Lens/DustAndHair.hpp
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
	 * @brief The dust and hair lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Simulates dust particles and hair fibers deposited in the projector gate,
	 * cast as semi-transparent shadows onto the projected image. Dust particles
	 * appear and disappear quickly, while hair fibers persist longer.
	 */
	class DustAndHair final : public Graphics::DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DustAndHair"};

			/**
			 * @brief Constructs a dust and hair lens effect.
			 * @param intensity The overall density/opacity (0.0 = none, 1.0 = heavy). Default 0.3.
			 */
			explicit
			DustAndHair (float intensity = 0.3F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the overall intensity (density/opacity).
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the overall intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the number of dust particles.
			 * @param count Number of particles (default 8.0).
			 * @return void
			 */
			void setDustCount (float count) noexcept;

			/**
			 * @brief Returns the number of dust particles.
			 * @return float
			 */
			[[nodiscard]]
			float
			dustCount () const noexcept
			{
				return m_dustCount;
			}

			/**
			 * @brief Sets the number of hair fibers.
			 * @param count Number of fibers (default 2.0).
			 * @return void
			 */
			void setHairCount (float count) noexcept;

			/**
			 * @brief Returns the number of hair fibers.
			 * @return float
			 */
			[[nodiscard]]
			float
			hairCount () const noexcept
			{
				return m_hairCount;
			}

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::requestScreenCoordinates() */
			[[nodiscard]]
			bool
			requestScreenCoordinates () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_intensity{0.3F};
			float m_dustCount{8.0F};
			float m_hairCount{2.0F};
	};
}
