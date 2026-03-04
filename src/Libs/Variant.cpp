/*
 * src/Libs/Variant.cpp
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

#include "Variant.hpp"

/* STL inclusions. */
#include <iostream>
#include <type_traits>

namespace EmEn::Libs
{
	using namespace Math;
	using namespace PixelFactory;

	int8_t
	Variant::asInteger8 () const noexcept
	{
		if ( const auto * p = std::get_if< int8_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'int8_t' !" "\n";

		return 0;
	}

	uint8_t
	Variant::asUnsignedInteger8 () const noexcept
	{
		if ( const auto * p = std::get_if< uint8_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'uint8_t' !" "\n";

		return 0;
	}

	int16_t
	Variant::asInteger16 () const noexcept
	{
		if ( const auto * p = std::get_if< int16_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'int16_t' !" "\n";

		return 0;
	}

	uint16_t
	Variant::asUnsignedInteger16 () const noexcept
	{
		if ( const auto * p = std::get_if< uint16_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'uint16_t' !" "\n";

		return 0;
	}

	int32_t
	Variant::asInteger32 () const noexcept
	{
		if ( const auto * p = std::get_if< int32_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'int32_t' !" "\n";

		return 0;
	}

	uint32_t
	Variant::asUnsignedInteger32 () const noexcept
	{
		if ( const auto * p = std::get_if< uint32_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'uint32_t' !" "\n";

		return 0;
	}

	int64_t
	Variant::asInteger64 () const noexcept
	{
		if ( const auto * p = std::get_if< int64_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'int64_t' !" "\n";

		return 0;
	}

	uint64_t
	Variant::asUnsignedInteger64 () const noexcept
	{
		if ( const auto * p = std::get_if< uint64_t >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'uint64_t' !" "\n";

		return 0;
	}

	float
	Variant::asFloat () const noexcept
	{
		if ( const auto * p = std::get_if< float >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'float' !" "\n";

		return 0.0F;
	}

	double
	Variant::asDouble () const noexcept
	{
		if ( const auto * p = std::get_if< double >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'double' !" "\n";

		return 0.0;
	}

	long double
	Variant::asLongDouble () const noexcept
	{
		if ( const auto * p = std::get_if< long double >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'long double' !" "\n";

		return 0.0L;
	}

	bool
	Variant::asBool () const noexcept
	{
		if ( const auto * p = std::get_if< bool >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'bool' !" "\n";

		return false;
	}

	Vector2F
	Variant::asVector2Float () const noexcept
	{
		if ( const auto * p = std::get_if< Vector2F >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Vector2F' !" "\n";

		return {};
	}

	Vector3F
	Variant::asVector3Float () const noexcept
	{
		if ( const auto * p = std::get_if< Vector3F >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Vector3F' !" "\n";

		return {};
	}

	Vector4F
	Variant::asVector4Float () const noexcept
	{
		if ( const auto * p = std::get_if< Vector4F >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Vector4F' !" "\n";

		return {};
	}

	Matrix2F
	Variant::asMatrix2Float () const noexcept
	{
		if ( const auto * p = std::get_if< Matrix2F >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Matrix2F' !" "\n";

		return {};
	}

	Matrix3F
	Variant::asMatrix3Float () const noexcept
	{
		if ( const auto * p = std::get_if< Matrix3F >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Matrix3F' !" "\n";

		return {};
	}

	Matrix4F
	Variant::asMatrix4Float () const noexcept
	{
		if ( const auto * p = std::get_if< Matrix4F >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Matrix4F' !" "\n";

		return {};
	}

	CartesianFrameF
	Variant::asCartesianFrameFloat () const noexcept
	{
		if ( const auto * p = std::get_if< CartesianFrameF >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'CoordinatesF' !" "\n";

		return {};
	}

	ColorF
	Variant::asColor () const noexcept
	{
		if ( const auto * p = std::get_if< ColorF >(&m_data) )
		{
			return *p;
		}

		std::cerr << "This Variant(" << to_cstring(type()) << ") is not a 'Color' !" "\n";

		return {};
	}

	std::ostream &
	operator<< (std::ostream & out, const Variant & variant) noexcept
	{
		std::visit([&out] < typename value_t >(const value_t  & value) {
			using T = std::decay_t< value_t  >;

			if constexpr ( std::is_same_v< T, std::monostate > )
			{
				out << "Null";
			}
			else
			{
				out << value;
			}
		}, variant.m_data);

		return out;
	}

	const char *
	Variant::to_cstring (Type type) noexcept
	{
		switch ( type )
		{
			case Type::Integer8 :
				return Integer8String;

			case Type::UnsignedInteger8 :
				return UnsignedInteger8String;

			case Type::Integer16 :
				return Integer16String;

			case Type::UnsignedInteger16 :
				return UnsignedInteger16String;

			case Type::Integer32 :
				return Integer32String;

			case Type::UnsignedInteger32 :
				return UnsignedInteger32String;

			case Type::Integer64 :
				return Integer64String;

			case Type::UnsignedInteger64 :
				return UnsignedInteger64String;

			case Type::Float :
				return FloatString;

			case Type::Double :
				return DoubleString;

			case Type::LongDouble :
				return LongDoubleString;

			case Type::Boolean :
				return BooleanString;

			case Type::Vector2Float :
				return Vector2FloatString;

			case Type::Vector3Float :
				return Vector3FloatString;

			case Type::Vector4Float :
				return Vector4FloatString;

			case Type::Matrix2Float :
				return Matrix2FloatString;

			case Type::Matrix3Float :
				return Matrix3FloatString;

			case Type::Matrix4Float :
				return Matrix4FloatString;

			case Type::CartesianFrameFloat:
				return CartesianFrameString;

			case Type::Color :
				return ColorString;

			case Type::Null :
				return NullString;
		}

		return NullString;
	}
}
