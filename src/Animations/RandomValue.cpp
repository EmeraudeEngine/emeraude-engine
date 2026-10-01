/*
 * src/Animations/RandomValue.cpp
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

#include "RandomValue.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Animations
{
	using namespace Base;

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
			/* NOTE: The Windows build returned Variant{0} (an int, not random) here, and the base quickRandom()
			 * truncated rand() into a negative 8 / 16-bit value (out of range) everywhere: fixed in the base. */
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

			/* NOTE: The composite types are drawn per component, between the components of the minimum and the
			 * maximum (asFloat() on them returned 0 with a type-mismatch diagnostic). */
			case Variant::Type::Vector2Float :
			{
				const auto minimum = m_minimumValue.asVector2Float();
				const auto maximum = m_maximumValue.asVector2Float();

				return Variant{Math::Vector< 2, float >(
					Utility::quickRandom(minimum[Math::X], maximum[Math::X]),
					Utility::quickRandom(minimum[Math::Y], maximum[Math::Y])
				)};
			}

			case Variant::Type::Vector3Float :
			{
				const auto minimum = m_minimumValue.asVector3Float();
				const auto maximum = m_maximumValue.asVector3Float();

				return Variant{Math::Vector< 3, float >(
					Utility::quickRandom(minimum[Math::X], maximum[Math::X]),
					Utility::quickRandom(minimum[Math::Y], maximum[Math::Y]),
					Utility::quickRandom(minimum[Math::Z], maximum[Math::Z])
				)};
			}

			case Variant::Type::Vector4Float :
			{
				const auto minimum = m_minimumValue.asVector4Float();
				const auto maximum = m_maximumValue.asVector4Float();

				return Variant{Math::Vector< 4, float >(
					Utility::quickRandom(minimum[Math::X], maximum[Math::X]),
					Utility::quickRandom(minimum[Math::Y], maximum[Math::Y]),
					Utility::quickRandom(minimum[Math::Z], maximum[Math::Z]),
					Utility::quickRandom(minimum[Math::W], maximum[Math::W])
				)};
			}

			case Variant::Type::Matrix2Float :
			case Variant::Type::Matrix3Float :
			case Variant::Type::Matrix4Float :
				return {};

			case Variant::Type::CartesianFrameFloat :
			{
				/* A frame at a random position between the positions of the two frames. */
				const auto minimum = m_minimumValue.asCartesianFrameFloat().position();
				const auto maximum = m_maximumValue.asCartesianFrameFloat().position();

				return Variant{Math::CartesianFrame< float >(Math::Vector< 3, float >(
					Utility::quickRandom(minimum[Math::X], maximum[Math::X]),
					Utility::quickRandom(minimum[Math::Y], maximum[Math::Y]),
					Utility::quickRandom(minimum[Math::Z], maximum[Math::Z])
				))};
			}

			case Variant::Type::Color :
			{
				const auto minimum = m_minimumValue.asColor();
				const auto maximum = m_maximumValue.asColor();

				return Variant{PixelFactory::Color< float >{
					Utility::quickRandom(minimum.red(), maximum.red()),
					Utility::quickRandom(minimum.green(), maximum.green()),
					Utility::quickRandom(minimum.blue(), maximum.blue()),
					Utility::quickRandom(minimum.alpha(), maximum.alpha())
				}};
			}

			case Variant::Type::Null :
				return {};
		}

		return {};
	}
}
