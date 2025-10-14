/*
 * src/Animations/RandomValue.cpp
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

#include "RandomValue.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Animations
{
	using namespace Libs;

	void
	RandomValue::setValue (const Variant & minimum, const Variant & maximum) noexcept
	{
		if ( minimum.type() != maximum.type() )
		{
			Tracer::error(ClassId, "Value should be the same type !");

			return;
		}

		m_minimumValue = minimum;
		m_maximumValue = maximum;
	}

	Variant
	RandomValue::getNextValue () noexcept
	{
		switch ( m_minimumValue.type() )
		{
#if IS_WINDOWS
			case Variant::Type::Integer8 :
			case Variant::Type::UnsignedInteger8 :
				return Variant{0};
#else
			case Variant::Type::Integer8 :

				return Variant{static_cast< int8_t >(Utility::quickRandom(
					m_minimumValue.asInteger8(),
					m_maximumValue.asInteger8()
				))};

			case Variant::Type::UnsignedInteger8 :
				return Variant{static_cast< uint8_t >(Utility::quickRandom(
					m_minimumValue.asUnsignedInteger8(),
					m_maximumValue.asUnsignedInteger8()
				))};
#endif

			case Variant::Type::Integer16 :
				return Variant{static_cast< int16_t >(Utility::quickRandom(
					m_minimumValue.asInteger16(),
					m_maximumValue.asInteger16()
				))};

			case Variant::Type::UnsignedInteger16 :
				return Variant{static_cast< uint16_t >(Utility::quickRandom(
					m_minimumValue.asUnsignedInteger16(),
					m_maximumValue.asUnsignedInteger16()
				))};

			case Variant::Type::Integer32 :
				return Variant{static_cast< int32_t >(Utility::quickRandom(
					m_minimumValue.asInteger32(),
					m_maximumValue.asInteger32()
				))};

			case Variant::Type::UnsignedInteger32 :
				return Variant{static_cast< uint32_t >(Utility::quickRandom(
					m_minimumValue.asUnsignedInteger32(),
					m_maximumValue.asUnsignedInteger32()
				))};

			case Variant::Type::Integer64 :
				return Variant{static_cast< int64_t >(Utility::quickRandom(
					m_minimumValue.asInteger64(),
					m_maximumValue.asInteger64()
				))};

			case Variant::Type::UnsignedInteger64 :
				return Variant{static_cast< uint64_t >(Utility::quickRandom(
					m_minimumValue.asUnsignedInteger64(),
					m_maximumValue.asUnsignedInteger64()
				))};

			case Variant::Type::Float :
				return Variant{static_cast< float >(Utility::quickRandom(
					m_minimumValue.asFloat(),
					m_maximumValue.asFloat()
				))};

			case Variant::Type::Double :
				return Variant{static_cast< double >(Utility::quickRandom(
					m_minimumValue.asDouble(),
					m_maximumValue.asDouble()
				))};

			case Variant::Type::LongDouble :
				return Variant{static_cast< long double >(Utility::quickRandom(
					m_minimumValue.asLongDouble(),
					m_maximumValue.asLongDouble()
				))};

			case Variant::Type::Boolean :
				if ( Utility::quickRandom(0, 1) > 0 )
				{
					return Variant{true};
				}

				return Variant{false};

			case Variant::Type::Vector2Float :
				return Variant{Math::Vector< 2, float >(
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat())
				)};

			case Variant::Type::Vector3Float :
				return Variant{Math::Vector< 3, float >(
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat())
				)};

			case Variant::Type::Vector4Float :
				return Variant{Math::Vector< 4, float >(
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					0.0F
				)};

			case Variant::Type::Matrix2Float :
			case Variant::Type::Matrix3Float :
			case Variant::Type::Matrix4Float :
				return {};

			case Variant::Type::CartesianFrameFloat :
				return Variant{Math::CartesianFrame< float >(Math::Vector< 3, float >(
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat()),
					Utility::quickRandom(m_minimumValue.asFloat(), m_maximumValue.asFloat())
				))};

			case Variant::Type::Color :
				return Variant{PixelFactory::Color< float >::quickRandom(
					m_minimumValue.asFloat(),
					m_maximumValue.asFloat()
				)};

			case Variant::Type::Null :
				return {};
		}

		return {};
	}
}
