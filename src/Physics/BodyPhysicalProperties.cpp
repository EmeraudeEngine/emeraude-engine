/*
 * src/Physics/BodyPhysicalProperties.cpp
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

#include "BodyPhysicalProperties.hpp"

/* STL inclusions. */
#include <array>
#include <limits>
#include <sstream>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Constants.hpp"
#include "Tracer.hpp"

namespace EmEn::Physics
{
	using namespace Libs;
	using namespace Libs::Math;

	bool
	BodyPhysicalProperties::setMass (float value, bool fireEvents) noexcept
	{
		if ( value < 0.0F )
		{
			Tracer::warning(ClassId, "Mass can't be negative !");

			return false;
		}

		if ( value == m_mass )
		{
			return false;
		}

		m_mass = value;
		m_inverseMass = 1.0F / m_mass;

		if ( fireEvents )
		{
			this->notify(MassChanged, m_mass);
			this->notify(PropertiesChanged);
		}

		return true;
	}

	bool
	BodyPhysicalProperties::setSurface (float value, bool fireEvents) noexcept
	{
		if ( value < 0.0F )
		{
			Tracer::warning(ClassId, "Surface can't be negative !");

			return false;
		}

		if ( value == m_surface )
		{
			return false;
		}

		m_surface = value;

		if ( fireEvents )
		{
			this->notify(SurfaceChanged, m_surface);
			this->notify(PropertiesChanged);
		}

		return true;
	}

	bool
	BodyPhysicalProperties::setDragCoefficient (float value, bool fireEvents) noexcept
	{
		if ( value < 0.0F )
		{
			Tracer::warning(ClassId, "Drag coefficient can't be negative.");

			return false;
		}

		if ( value == m_dragCoefficient )
		{
			return false;
		}

		m_dragCoefficient = value;

		if ( fireEvents )
		{
			this->notify(DragCoefficientChanged, m_dragCoefficient);
			this->notify(PropertiesChanged);
		}

		return true;
	}

	bool
	BodyPhysicalProperties::setAngularDragCoefficient (float value, bool fireEvents) noexcept
	{
		if ( value < 0.0F || value > 1.0F )
		{
			Tracer::warning(ClassId, "Angular drag must be a scalar value [0.0 -> 1.0].");

			return false;
		}

		if ( value == m_angularDragCoefficient )
		{
			return false;
		}

		m_angularDragCoefficient = clampToUnit(value);

		if ( fireEvents )
		{
			this->notify(AngularDragCoefficientChanged, m_angularDragCoefficient);
			this->notify(PropertiesChanged);
		}

		return true;
	}

	bool
	BodyPhysicalProperties::setBounciness (float value, bool fireEvents) noexcept
	{
		if ( value < 0.0F || value > 1.0F )
		{
			Tracer::warning(ClassId, "Bounciness must be a scalar value [0.0 -> 1.0].");

			return false;
		}

		if ( value == m_bounciness )
		{
			return false;
		}

		m_bounciness = clampToUnit(value);

		if ( fireEvents )
		{
			this->notify(BouncinessChanged, m_bounciness);
			this->notify(PropertiesChanged);
		}

		return true;
	}

	bool
	BodyPhysicalProperties::setStickiness (float value, bool fireEvents) noexcept
	{
		if ( value < 0.0F || value > 1.0F )
		{
			Tracer::warning(ClassId, "Stickiness must be a scalar value [0.0 -> 1.0].");

			return false;
		}

		if ( value == m_stickiness )
		{
			return false;
		}

		m_stickiness = value;

		if ( fireEvents )
		{
			this->notify(StickinessChanged, m_stickiness);
			this->notify(PropertiesChanged);
		}

		return true;
	}

	bool
	BodyPhysicalProperties::setProperties (float mass, float surface, float dragCoefficient, float angularDragCoefficient, float bounciness, float stickiness, const Matrix< 3, float > & inertiaTensor) noexcept
	{
		auto changes = false;

		if ( this->setMass(mass, false) )
		{
			this->notify(MassChanged, m_mass);

			changes = true;
		}

		if ( this->setSurface(surface, false) )
		{
			this->notify(SurfaceChanged, m_surface);

			changes = true;
		}

		if ( this->setDragCoefficient(dragCoefficient, false) )
		{
			this->notify(DragCoefficientChanged, m_dragCoefficient);

			changes = true;
		}

		if ( this->setAngularDragCoefficient(angularDragCoefficient, false) )
		{
			this->notify(AngularDragCoefficientChanged, m_angularDragCoefficient);

			changes = true;
		}

		if ( this->setBounciness(bounciness, false) )
		{
			this->notify(BouncinessChanged, m_bounciness);

			changes = true;
		}

		if ( this->setStickiness(stickiness, false) )
		{
			this->notify(StickinessChanged, m_stickiness);

			changes = true;
		}

		if ( this->setInertiaTensor(inertiaTensor, false) )
		{
			this->notify(InertiaTensorChanged, m_inertiaTensor);

			changes = true;
		}

		if ( changes )
		{
			this->notify(PropertiesChanged);
		}

		return changes;
	}

	bool
	BodyPhysicalProperties::setProperties (const Json::Value & data) noexcept
	{
		struct property_t {
			const char * jsonKey = nullptr;
			bool (BodyPhysicalProperties::* method)(float, bool) = nullptr;
			int notificationCode = std::numeric_limits< int >::max();
		};

		constexpr std::array< property_t, 5 > properties{{
			{MassKey, &BodyPhysicalProperties::setMass, MassChanged},
			{SurfaceKey, &BodyPhysicalProperties::setSurface, SurfaceChanged},
			{DragCoefficientKey, &BodyPhysicalProperties::setDragCoefficient, DragCoefficientChanged},
			{BouncinessKey, &BodyPhysicalProperties::setBounciness, BouncinessChanged},
			{StickinessKey, &BodyPhysicalProperties::setStickiness, StickinessChanged}
		}};

		auto changes = false;

		for ( const auto & property : properties )
		{
			/* Not present in the JSON. */
			if ( !data.isMember(property.jsonKey) )
			{
				continue;
			}

			/* Checking the value type and pop an error on bad one. */
			if ( !data[property.jsonKey].isNumeric() )
			{
				TraceError{ClassId} << '\'' << property.jsonKey << "' key must be a floating number !";

				continue;
			}

			/* Set the value, if changes prepares to declaring it as event. */
			const auto value = data[property.jsonKey].asFloat();

			if ( !(this->*property.method)(value, false) )
			{
				continue;
			}

			this->notify(property.notificationCode, value);

			changes = true;
		}

		if ( changes )
		{
			this->notify(PropertiesChanged);
		}

		return changes;
	}

	void
	BodyPhysicalProperties::merge (const BodyPhysicalProperties & other) noexcept
	{
		m_mass += other.m_mass;
		m_inverseMass = 1.0F / m_mass;
		if ( other.m_surface > m_surface )
		{
			m_surface = other.m_surface;
		}
		m_dragCoefficient = (m_dragCoefficient + other.m_dragCoefficient) * Half< float >;
		m_angularDragCoefficient = (m_angularDragCoefficient + other.m_angularDragCoefficient) * Half< float >;
		m_bounciness = (m_bounciness + other.m_bounciness) * Half< float >;
		m_stickiness = (m_stickiness + other.m_stickiness) * Half< float >;
		/* FIXME: Not sure about this !! */
		m_inertiaTensor = (m_inertiaTensor + other.m_inertiaTensor) * Half< float >;
	}

	bool
	BodyPhysicalProperties::setInertiaTensor (const Libs::Math::Matrix< 3, float > & inertiaTensor, bool fireEvents) noexcept
	{
		/* Check if all diagonal values are non-negative (physical constraint). */
		if ( inertiaTensor[M3x3Col0Row0] < 0.0F || inertiaTensor[M3x3Col1Row1] < 0.0F || inertiaTensor[M3x3Col2Row2] < 0.0F )
		{
			Tracer::warning(ClassId, "Inertia tensor diagonal values can't be negative !");

			return false;
		}

		/* Check if the tensor changed. */
		if ( m_inertiaTensor == inertiaTensor )
		{
			return false;
		}

		m_inertiaTensor = inertiaTensor;

		if ( fireEvents )
		{
			this->notify(InertiaTensorChanged);
			this->notify(PropertiesChanged);
		}

		return true;
	}
}
