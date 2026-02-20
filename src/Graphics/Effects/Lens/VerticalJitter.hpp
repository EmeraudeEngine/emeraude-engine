/*
 * src/Graphics/Effects/Lens/VerticalJitter.hpp
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
	 * @brief The vertical jitter lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Simulates the mechanical instability of the pull-down claw mechanism in film
	 * projectors, causing random micro-displacement of the entire image vertically
	 * on each frame. Especially visible on worn prints.
	 */
	class VerticalJitter final : public Graphics::DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VerticalJitter"};

			/**
			 * @brief Constructs a vertical jitter lens effect.
			 * @param intensity The maximum vertical UV displacement (0.0 = none). Default 0.002.
			 */
			explicit
			VerticalJitter (float intensity = 0.002F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 0.05F)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the jitter intensity (max UV displacement vertical).
			 * @param intensity Value in range [0, 0.05].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 0.05F);
			}

			/**
			 * @brief Returns the jitter intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the film frame rate for discrete frame snapping.
			 * @param fps Film cadence (default 24.0).
			 * @return void
			 */
			void setSpeed (float fps = 24.0F) noexcept;

			/**
			 * @brief Returns the film frame rate.
			 * @return float
			 */
			[[nodiscard]]
			float
			speed () const noexcept
			{
				return m_speed;
			}

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::overrideFragmentFetching() */
			[[nodiscard]]
			bool
			overrideFragmentFetching () const noexcept override
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

			float m_intensity{0.002F};
			float m_speed{24.0F};
	};
}
