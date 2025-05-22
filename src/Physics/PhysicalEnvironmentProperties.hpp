/*
 * src/Physics/PhysicalEnvironmentProperties.hpp
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

/* Local inclusions for usages. */
#include "Physics.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief The PhysicalEnvironmentProperties class
	 */
	class PhysicalEnvironmentProperties final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PhysicalEnvironmentProperties"};

			/**
			 * @brief Constructs physical environment properties.
			 * @param surfaceGravity The gravity at surface expressed in m/s².
			 * @param atmosphericDensity The atmospheric density expressed in kg/m³.
			 * @param planetRadius The radius of the planet environment in m.
			 */
			PhysicalEnvironmentProperties (float surfaceGravity, float atmosphericDensity, float planetRadius) noexcept;

			/**
			 * @brief Returns the surface gravity in m/s².
			 * @return float
			 */
			[[nodiscard]]
			float
			surfaceGravity () const noexcept
			{
				return m_surfaceGravity;
			}

			/**
			 * @brief Returns the surface gravity in m/s² per engine update cycle.
			 * @return float
			 */
			[[nodiscard]]
			float
			steppedSurfaceGravity () const noexcept
			{
				return m_steppedSurfaceGravity;
			}

			/**
			 * @brief Returns the gravity in m/s² according to an altitude.
			 * @brief altitude The altitude in meters.
			 * @return float
			 */
			[[nodiscard]]
			float
			gravity (float /*altitude*/) const noexcept
			{
				// FIXME: TODO ...

				// gh = g (1 + h/R)–2
				// R is your distance from the center of the Earth
				return m_surfaceGravity;
			}

			/**
			 * @brief Returns the gravity in m/s² according to an altitude per engine update cycle.
			 * @return float
			 */
			[[nodiscard]]
			float
			steppedGravity (float altitude) const noexcept
			{
				return this->gravity(altitude) * EngineUpdateCycleDurationS< float >;
			}

			/**
			 * @brief Returns the atmospheric density expressed in kg/m³.
			 * @param altitude The altitude in meters. Default at surface.
			 * @param temperature The ambient temperature in degree C°. Default 20.
			 * @return float
			 */
			[[nodiscard]]
			float
			atmosphericDensity (/*float altitude = 0.0F, float temperature = 20.0F*/) const noexcept
			{
				// FIXME: TODO ...

				return m_atmosphericDensity;
			}

			/**
			 * @brief Returns the planet radius in m.
			 * @return float
			 */
			[[nodiscard]]
			float
			planetRadius () const noexcept
			{
				return m_planetRadius;
			}

			/**
			 * @brief Returns earth environment properties.
			 * @return PhysicalEnvironmentProperties
			 */
			static
			PhysicalEnvironmentProperties
			Earth () noexcept
			{
				return {Gravity::Earth< float >, Density::EarthStandardAir< float >, Radius::Earth< float >};
			}

			/**
			 * @brief Returns moon environment properties.
			 * @return PhysicalEnvironmentProperties
			 */
			static
			PhysicalEnvironmentProperties
			Moon () noexcept
			{
				return {Gravity::Moon< float >, 0.0F, Radius::Moon< float >};
			}

			/**
			 * @brief Returns mars environment properties.
			 * @return PhysicalEnvironmentProperties
			 */
			static
			PhysicalEnvironmentProperties
			Mars () noexcept
			{
				return {Gravity::Mars< float >, 0.020F, Radius::Mars< float >};
			}

			/**
			 * @brief Returns jupiter environment properties.
			 * @return PhysicalEnvironmentProperties
			 */
			static
			PhysicalEnvironmentProperties
			Jupiter () noexcept
			{
				return {Gravity::Jupiter< float >, 1.326F, Radius::Jupiter< float >};
			}

			/**
			 * @brief Returns space environment properties.
			 * @return PhysicalEnvironmentProperties
			 */
			static
			PhysicalEnvironmentProperties
			Void () noexcept
			{
				return {0.0F, 0.0F, 0.0F};
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const PhysicalEnvironmentProperties & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const PhysicalEnvironmentProperties & obj) noexcept;

		private:

			float m_surfaceGravity;
			float m_steppedSurfaceGravity;
			float m_atmosphericDensity;
			float m_planetRadius;
	};
}
