/*
 * src/Graphics/Effects/Lens/PhosphorBloom.hpp
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
	 * @brief The CRT phosphor bloom lens effect class.
	 * @extends EmEn::Graphics::DirectPostProcessEffect This is a framebuffer effect.
	 *
	 * Simulates the luminous halo produced by CRT phosphors around bright areas,
	 * using a weighted 9-tap cross filter on the color buffer.
	 */
	class PhosphorBloom final : public Graphics::DirectPostProcessEffect
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PhosphorBloom"};

			/**
			 * @brief Constructs a phosphor bloom lens effect.
			 * @param intensity The bloom intensity (0 = off, 1 = full). Default 0.3.
			 */
			explicit
			PhosphorBloom (float intensity = 0.3F) noexcept
				: m_intensity{std::clamp(intensity, 0.0F, 1.0F)}
			{

			}

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the bloom intensity.
			 * @param intensity Value in range [0, 1].
			 * @return void
			 */
			void
			setIntensity (float intensity) noexcept
			{
				m_intensity = std::clamp(intensity, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the bloom intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Sets the luminosity threshold above which bloom activates.
			 * @param threshold Value in range [0, 1]. Default 0.6.
			 * @return void
			 */
			void
			setThreshold (float threshold) noexcept
			{
				m_threshold = std::clamp(threshold, 0.0F, 1.0F);
			}

			/**
			 * @brief Returns the bloom threshold.
			 * @return float
			 */
			[[nodiscard]]
			float
			threshold () const noexcept
			{
				return m_threshold;
			}

			/**
			 * @brief Sets the bloom diffusion radius in pixels.
			 * @param spread Value > 0. Default 2.0.
			 * @return void
			 */
			void setSpread (float spread) noexcept;

			/**
			 * @brief Returns the bloom diffusion radius.
			 * @return float
			 */
			[[nodiscard]]
			float
			spread () const noexcept
			{
				return m_spread;
			}

		private:

			/** @copydoc EmEn::Graphics::DirectPostProcessEffect::requestScreenSize() */
			[[nodiscard]]
			bool
			requestScreenSize () const noexcept override
			{
				return true;
			}

			float m_intensity{0.3F};
			float m_threshold{0.6F};
			float m_spread{2.0F};
	};
}
