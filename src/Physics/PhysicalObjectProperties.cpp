/*
 * src/Physics/PhysicalObjectProperties.cpp
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

#include "PhysicalObjectProperties.hpp"

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
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	const size_t PhysicalObjectProperties::ClassUID{getClassUID(ClassId)};

	PhysicalObjectProperties::PhysicalObjectProperties (float mass, float surface, float dragCoefficient, float bounciness, float stickiness) noexcept
		: m_mass(mass), m_inverseMass(1.0F / m_mass), m_surface(surface), m_dragCoefficient(dragCoefficient), m_bounciness(bounciness), m_stickiness(stickiness)
	{

	}

	size_t
	PhysicalObjectProperties::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	PhysicalObjectProperties::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	PhysicalObjectProperties::setMass (float value, bool fireEvents) noexcept
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
	PhysicalObjectProperties::setSurface (float value, bool fireEvents) noexcept
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
	PhysicalObjectProperties::setDragCoefficient (float value, bool fireEvents) noexcept
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
	PhysicalObjectProperties::setBounciness (float value, bool fireEvents) noexcept
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
	PhysicalObjectProperties::setStickiness (float value, bool fireEvents) noexcept
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
	PhysicalObjectProperties::setProperties (float mass, float surface, float dragCoefficient, float bounciness, float stickiness) noexcept
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

		if ( changes )
		{
			this->notify(PropertiesChanged);
		}

		return changes;
	}

	bool
	PhysicalObjectProperties::setProperties (const Json::Value & data) noexcept
	{
		struct property_t {
			const char * jsonKey = nullptr;
			bool (PhysicalObjectProperties::* method)(float, bool) = nullptr;
			int notificationCode = std::numeric_limits< int >::max();
		};

		constexpr std::array< property_t, 5 > properties{{
			{MassKey, &PhysicalObjectProperties::setMass, MassChanged},
			{SurfaceKey, &PhysicalObjectProperties::setSurface, SurfaceChanged},
			{DragCoefficientKey, &PhysicalObjectProperties::setDragCoefficient, DragCoefficientChanged},
			{BouncinessKey, &PhysicalObjectProperties::setBounciness, BouncinessChanged},
			{StickinessKey, &PhysicalObjectProperties::setStickiness, StickinessChanged}
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
	PhysicalObjectProperties::merge (const PhysicalObjectProperties & other) noexcept
	{
		m_mass += other.m_mass;
		m_inverseMass = 1.0F / m_mass;
		if ( other.m_surface > m_surface )
		{
			m_surface = other.m_surface;
		}
		m_dragCoefficient = (m_dragCoefficient + other.m_dragCoefficient) * Half< float >;
		m_bounciness = (m_bounciness + other.m_bounciness) * Half< float >;
		m_stickiness = (m_stickiness + other.m_stickiness) * Half< float >;
	}

	void
	PhysicalObjectProperties::reset () noexcept
	{
		m_mass = DefaultMass;
		m_inverseMass = 0.0F;
		m_surface = DefaultSurface;
		m_dragCoefficient = DefaultDragCoefficient;
		m_bounciness = DefaultBounciness;
		m_stickiness = DefaultStickiness;
	}

	std::ostream &
	operator<< (std::ostream & out, const PhysicalObjectProperties & obj)
	{
		return out << "Physical object properties :" "\n"
			"Mass : " << obj.m_mass << " Kg (Inverse: " << obj.m_inverseMass << ")" << '\n' <<
			"Surface : " << obj.m_surface << " m²" << '\n' <<
			"Drag Coefficient : " << obj.m_dragCoefficient << '\n' <<
			"Bounciness : " << obj.m_bounciness << '\n' <<
			"Stickiness : " << obj.m_stickiness << '\n';
	}

	std::string
	to_string (const PhysicalObjectProperties & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
