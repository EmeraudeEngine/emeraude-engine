/*
 * src/Physics/EnvironmentPhysicalProperties.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Third-party inclusions. */
#include "magic_enum/magic_enum.hpp"

/* Local inclusions for usages. */
#include "Audio/Types.hpp"
#include "Math/Vector.hpp"
#include "Physics.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief The environment physical properties structure.
	 * It holds the global physical parameters of a scene.
	 */
	class EMEN_API EnvironmentPhysicalProperties final
	{
		public:

			/** @brief The direction gravity pulls toward.
			 * ⚠️ This is the ONE place the physics loop learns which way is down. The integrators
			 * add the vector and never test a Y sign, so a world with sideways or radial gravity
			 * only needs this to become per-environment data. */
			static constexpr Base::Math::Vector< 3, float > DownDirection{0.0F, -1.0F, 0.0F};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"EnvironmentPhysicalProperties"};

			/**
			 * @brief Constructs environment physical properties.
			 * @param surfaceGravity The gravity at surface expressed in m/s².
			 * @param atmosphericDensity The atmospheric density expressed in kg/m³.
			 * @param planetRadius The radius of the planet environment in m.
			 */
			EnvironmentPhysicalProperties (float surfaceGravity, float atmosphericDensity, float planetRadius) noexcept
				: m_surfaceGravity{DownDirection * surfaceGravity},
				m_steppedSurfaceGravity{DownDirection * surfaceGravity * WorldPhysicsUpdateCycleDurationS< float >},
				m_atmosphericDensity{atmosphericDensity},
				m_planetRadius{planetRadius}
			{

			}

			/**
			 * @brief Returns the surface gravity as an ACCELERATION VECTOR in m/s².
			 * @note Direction is data, not a convention baked into the physics code: an integrator
			 * adds this vector and never needs to know which way is down. It points along
			 * DownDirection, which is `{0, -1, 0}` in the Y-up world.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			surfaceGravity () const noexcept
			{
				return m_surfaceGravity;
			}

			/**
			 * @brief Returns the surface gravity magnitude in m/s².
			 * @note For callers that want "how strong is gravity here", not "which way does it pull".
			 * @return float
			 */
			[[nodiscard]]
			float
			surfaceGravityMagnitude () const noexcept
			{
				return m_surfaceGravity.length();
			}

			/**
			 * @brief Returns the surface gravity vector in m/s² per engine update cycle.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
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
				return m_surfaceGravity.length();
			}

			/**
			 * @brief Returns the gravity in m/s² according to an altitude per engine update cycle.
			 * @return float
			 */
			[[nodiscard]]
			float
			steppedGravity (float altitude) const noexcept
			{
				return this->gravity(altitude) * WorldPhysicsUpdateCycleDurationS< float >;
			}

			/**
			 * @brief Returns the atmospheric density expressed in kg/m³.
			 * @param altitude The altitude in meters. Default at water level.
			 * @param temperature The ambient temperature in degree C°. Default 20.
			 * @return float
			 */
			[[nodiscard]]
			float
			atmosphericDensity (float altitude = 0.0F, float temperature = 20.0F) const noexcept
			{
				(void)altitude;
				(void)temperature;

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
			 * @brief Sets the speed of sound.
			 * @param speed The value in meter per second.
			 */
			void
			setSpeedOfSound (float speed) noexcept
			{
				if ( speed > 0.0F )
				{
					m_speedOfSound = speed;
				}
			}

			/**
			 * @brief Returns the current speed of sound.
			 * @return float
			 */
			[[nodiscard]]
			float
			speedOfSound () const noexcept
			{
				return m_speedOfSound;
			}

			/**
			 * @brief Sets the doppler effect factor.
			 * @param dopplerFactor
			 */
			void
			setDopplerFactor (float dopplerFactor) noexcept
			{
				if ( dopplerFactor >= 0.0F )
				{
					m_dopplerFactor = dopplerFactor;
				}
			}

			/**
			 * @brief Returns the current doppler effect factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			dopplerFactor () const noexcept
			{
				return m_dopplerFactor;
			}

			/**
			 * @brief Sets the distance model for the sound attenuation.
			 * @param model One of the DistanceModel enum values.
			 */
			void
			setDistanceModel (Audio::DistanceModel model) noexcept
			{
				m_distanceModel = model;
			}

			/**
			 * @brief Returns the current distance model in use for the sound attenuation.
			 * @return Audio::DistanceModel
			 */
			[[nodiscard]]
			Audio::DistanceModel
			distanceModel () const noexcept
			{
				return m_distanceModel;
			}

			/**
			 * @brief Returns earth environment properties.
			 * @return EnvironmentPhysicalProperties
			 */
			static
			EnvironmentPhysicalProperties
			Earth () noexcept
			{
				return {Gravity::Earth< float >, Density::EarthStandardAir< float >, Radius::Earth< float >};
			}

			/**
			 * @brief Returns moon environment properties.
			 * @return EnvironmentPhysicalProperties
			 */
			static
			EnvironmentPhysicalProperties
			Moon () noexcept
			{
				return {Gravity::Moon< float >, 0.0F, Radius::Moon< float >};
			}

			/**
			 * @brief Returns mars environment properties.
			 * @return EnvironmentPhysicalProperties
			 */
			static
			EnvironmentPhysicalProperties
			Mars () noexcept
			{
				return {Gravity::Mars< float >, 0.020F, Radius::Mars< float >};
			}

			/**
			 * @brief Returns jupiter environment properties.
			 * @return EnvironmentPhysicalProperties
			 */
			static
			EnvironmentPhysicalProperties
			Jupiter () noexcept
			{
				return {Gravity::Jupiter< float >, 1.326F, Radius::Jupiter< float >};
			}

			/**
			 * @brief Returns space environment properties.
			 * @return EnvironmentPhysicalProperties
			 */
			static
			EnvironmentPhysicalProperties
			Void () noexcept
			{
				return {0.0F, 0.0F, 0.0F};
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const EnvironmentPhysicalProperties & obj);

			Base::Math::Vector< 3, float > m_surfaceGravity;
			Base::Math::Vector< 3, float > m_steppedSurfaceGravity;
			float m_atmosphericDensity;
			float m_planetRadius;
			float m_speedOfSound{Physics::SpeedOfSound::Air< float >};
			float m_dopplerFactor{1.0F};
			Audio::DistanceModel m_distanceModel{Audio::DistanceModel::Exponent};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	EMEN_API std::string to_string (const EnvironmentPhysicalProperties & obj) noexcept;
}
