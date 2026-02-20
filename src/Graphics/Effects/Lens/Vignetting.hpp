/*
 * src/Graphics/Effects/Lens/Vignetting.hpp
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
	 * @brief The vignetting lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 */
	class Vignetting final : public Graphics::DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Vignetting"};

			/**
			 * @brief Constructs a vignetting lens effect.
			 * @param intensity The darkening intensity at the edges (0 = off, 1 = full black). Default 0.5.
			 */
			explicit
			Vignetting (float intensity = 0.5F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the darkening intensity at the edges.
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the darkening intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the vignette inner radius (where darkening begins).
			 * @param radius Value in range [0, 1], as a fraction of the half-diagonal. Default 0.4.
			 * @return void
			 */
			void
			setRadius (float radius) noexcept
			{
				m_radius = std::clamp(radius, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the vignette inner radius.
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Sets the softness of the transition from clear to dark.
			 * @param softness Value > 0. Higher = smoother transition. Default 0.5.
			 * @return void
			 */
			void setSoftness (float softness) noexcept;

			/**
			 * @brief Returns the transition softness.
			 * @return float
			 */
			[[nodiscard]]
			float
			softness () const noexcept
			{
				return m_softness;
			}

			/**
			 * @brief Sets the roundness of the vignette shape.
			 * @param roundness 1.0 = circular, 2.0 = more rectangular. Default 1.0.
			 * @return void
			 */
			void setRoundness (float roundness) noexcept;

			/**
			 * @brief Returns the vignette shape roundness.
			 * @return float
			 */
			[[nodiscard]]
			float
			roundness () const noexcept
			{
				return m_roundness;
			}

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_intensity{0.5F};
			float m_radius{0.4F};
			float m_softness{0.5F};
			float m_roundness{1.0F};
	};
}
