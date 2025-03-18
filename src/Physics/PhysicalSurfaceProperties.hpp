/*
 * src/Physics/PhysicalSurfaceProperties.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <cmath>

namespace EmEn::Physics
{
	/**
	 * @brief Class defining physical properties of a surface.
	 */
	class PhysicalSurfaceProperties final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PhysicalSurfaceProperties"};

			static constexpr auto DefaultDensity{1000.0F};

			/**
			 * @brief Constructs a physical surface properties.
			 */
			PhysicalSurfaceProperties () noexcept = default;

			/**
			 * @brief Sets the density of the material.
			 * @param density The density.
			 */
			void
			setDensity (float density) noexcept
			{
				m_density = std::abs(density);
			}

			/**
			 * @brief Returns the density of the material.
			 * @return float
			 */
			[[nodiscard]]
			float
			density () const noexcept
			{
				return m_density;
			}

			/**
			 * @brief Resets properties to defaults.
			 * @return void
			 */
			void reset () noexcept;

		private:

			float m_density{DefaultDensity};
	};
}
