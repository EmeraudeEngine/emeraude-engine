/*
 * src/Libs/Variant.cpp
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

#include "Variant.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <iostream>

/* Local inclusions. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Libs
{
	using namespace Math;
	using namespace PixelFactory;

	Variant::Variant (const Variant & other) noexcept
		: m_type(other.m_type)
	{
		if ( other.m_data == nullptr )
		{
			return;
		}

		switch ( other.m_type )
		{
			case Type::Integer8 :
				m_data = static_cast< void * >(new int8_t(*static_cast< int8_t * >(other.m_data)));
				break;

			case Type::UnsignedInteger8 :
				m_data = static_cast< void * >(new uint8_t(*static_cast< uint8_t * >(other.m_data)));
				break;

			case Type::Integer16 :
				m_data = static_cast< void * >(new int16_t(*static_cast< int16_t * >(other.m_data)));
				break;

			case Type::UnsignedInteger16 :
				m_data = static_cast< void * >(new uint16_t(*static_cast< uint16_t * >(other.m_data)));
				break;

			case Type::Integer32 :
				m_data = static_cast< void * >(new int32_t(*static_cast< int32_t * >(other.m_data)));
				break;

			case Type::UnsignedInteger32 :
				m_data = static_cast< void * >(new uint32_t(*static_cast< uint32_t * >(other.m_data)));
				break;

			case Type::Integer64 :
				m_data = static_cast< void * >(new int64_t(*static_cast< int64_t * >(other.m_data)));
				break;

			case Type::UnsignedInteger64 :
				m_data = static_cast< void * >(new uint64_t(*static_cast< uint64_t * >(other.m_data)));
				break;

			case Type::Float :
				m_data = static_cast< void * >(new float(*static_cast< float * >(other.m_data)));
				break;

			case Type::Double :
				m_data = static_cast< void * >(new double(*static_cast< double * >(other.m_data)));
				break;

			case Type::LongDouble :
				m_data = static_cast< void * >(new long double(*static_cast< long double * >(other.m_data)));
				break;

			case Type::Boolean :
				m_data = static_cast< void * >(new bool(*static_cast< bool * >(other.m_data)));
				break;

			case Type::Vector2Float :
				m_data = static_cast< void * >(new Vector2F(*static_cast< Vector2F * >(other.m_data)));
				break;

			case Type::Vector3Float :
				m_data = static_cast< void * >(new Vector3F(*static_cast< Vector3F * >(other.m_data)));
				break;

			case Type::Vector4Float :
				m_data = static_cast< void * >(new Vector4F(*static_cast< Vector4F * >(other.m_data)));
				break;

			case Type::Matrix2Float :
				m_data = static_cast< void * >(new Matrix2F(*static_cast< Matrix2F * >(other.m_data)));
				break;

			case Type::Matrix3Float :
				m_data = static_cast< void * >(new Matrix3F(*static_cast< Matrix3F * >(other.m_data)));
				break;

			case Type::Matrix4Float :
				m_data = static_cast< void * >(new Matrix4F(*static_cast< Matrix4F * >(other.m_data)));
				break;

			case Type::CartesianFrameFloat :
				m_data = static_cast< void * >(new CartesianFrameF(*static_cast< CartesianFrameF * >(other.m_data)));
				break;

			case Type::Color :
				m_data = static_cast< void * >(new ColorF(*static_cast< ColorF * >(other.m_data)));
				break;

			case Type::Null :
				break;
		}
	}

	Variant::Variant (Variant && other) noexcept
		:  m_data(other.m_data), m_type(other.m_type)
	{
		other.m_data = nullptr;
		other.m_type = Type::Null;
	}

	Variant::Variant (int8_t variable) noexcept
		: m_data(static_cast< void * >(new int8_t(variable))), m_type(Type::Integer8)
	{

	}

	Variant::Variant (uint8_t variable) noexcept
		: m_data(static_cast< void * >(new uint8_t(variable))), m_type(Type::UnsignedInteger8)
	{

	}

	Variant::Variant (int16_t variable) noexcept
		: m_data(static_cast< void * >(new int16_t(variable))), m_type(Type::Integer16)
	{

	}

	Variant::Variant (uint16_t variable) noexcept
		: m_data(static_cast< void * >(new uint16_t(variable))), m_type(Type::UnsignedInteger16)
	{

	}

	Variant::Variant (int32_t variable) noexcept
		: m_data(static_cast< void * >(new int32_t(variable))), m_type(Type::Integer32)
	{

	}

	Variant::Variant (uint32_t variable) noexcept
		: m_data(static_cast< void * >(new uint32_t(variable))), m_type(Type::UnsignedInteger32)
	{

	}

	Variant::Variant (int64_t variable) noexcept
		: m_data(static_cast< void * >(new int64_t(variable))), m_type(Type::Integer64)
	{

	}

	Variant::Variant (uint64_t variable) noexcept
		: m_data(static_cast< void * >(new uint64_t(variable))), m_type(Type::UnsignedInteger64)
	{

	}

	Variant::Variant (float variable) noexcept
		: m_data(static_cast< void * >(new float(variable))), m_type(Type::Float)
	{

	}

	Variant::Variant (double variable) noexcept
		: m_data(static_cast< void * >(new double(variable))), m_type(Type::Double)
	{

	}

	Variant::Variant (long double variable) noexcept
		: m_data(static_cast< void * >(new long double(variable))), m_type(Type::LongDouble)
	{

	}

	Variant::Variant (bool variable) noexcept
		: m_data(static_cast< void * >(new bool(variable))), m_type(Type::Boolean)
	{

	}

	Variant::Variant (const Vector2F & variable) noexcept
		: m_data(static_cast< void * >(new Vector2F(variable))), m_type(Type::Vector2Float)
	{

	}

	Variant::Variant (const Vector3F & variable) noexcept
		: m_data(static_cast< void * >(new Vector3F(variable))), m_type(Type::Vector3Float)
	{

	}

	Variant::Variant (const Vector4F & variable) noexcept
		: m_data(static_cast< void * >(new Vector4F(variable))), m_type(Type::Vector4Float)
	{

	}

	Variant::Variant (const Matrix2F & variable) noexcept
		: m_data(static_cast< void * >(new Matrix2F(variable))), m_type(Type::Matrix2Float)
	{

	}


	Variant::Variant (const Matrix3F & variable) noexcept
		: m_data(static_cast< void * >(new Matrix3F(variable))), m_type(Type::Matrix3Float)
	{

	}


	Variant::Variant (const Matrix4F & variable) noexcept
		: m_data(static_cast< void * >(new Matrix4F(variable))), m_type(Type::Matrix4Float)
	{

	}

	Variant::Variant (const CartesianFrameF & variable) noexcept
		: m_data(static_cast< void * >(new CartesianFrameF(variable))), m_type(Type::CartesianFrameFloat)
	{

	}

	Variant::Variant (const ColorF & variable) noexcept
		: m_data(static_cast< void * >(new ColorF(variable))), m_type(Type::Color)
	{

	}

	Variant::~Variant ()
	{
		this->reset();
	}

	Variant &
	Variant::operator= (const Variant & other) noexcept
	{
		if ( &other != this )
		{
			m_type = other.m_type;

			switch ( other.m_type )
			{
				case Type::Integer8 :
					m_data = static_cast< void * >(new int8_t(*static_cast< int8_t * >(other.m_data)));
					break;

				case Type::UnsignedInteger8 :
					m_data = static_cast< void * >(new uint8_t(*static_cast< uint8_t * >(other.m_data)));
					break;

				case Type::Integer16 :
					m_data = static_cast< void * >(new int16_t(*static_cast< int16_t * >(other.m_data)));
					break;

				case Type::UnsignedInteger16 :
					m_data = static_cast< void * >(new uint16_t(*static_cast< uint16_t * >(other.m_data)));
					break;

				case Type::Integer32 :
					m_data = static_cast< void * >(new int32_t(*static_cast< int32_t * >(other.m_data)));
					break;

				case Type::UnsignedInteger32 :
					m_data = static_cast< void * >(new uint32_t(*static_cast< uint32_t * >(other.m_data)));
					break;

				case Type::Integer64 :
					m_data = static_cast< void * >(new int64_t(*static_cast< int64_t * >(other.m_data)));
					break;

				case Type::UnsignedInteger64 :
					m_data = static_cast< void * >(new uint64_t(*static_cast< uint64_t * >(other.m_data)));
					break;

				case Type::Float :
					m_data = static_cast< void * >(new float(*static_cast< float * >(other.m_data)));
					break;

				case Type::Double :
					m_data = static_cast< void * >(new double(*static_cast< double * >(other.m_data)));
					break;

				case Type::LongDouble :
					m_data = static_cast< void * >(new long double(*static_cast< long double * >(other.m_data)));
					break;

				case Type::Boolean :
					m_data = static_cast< void * >(new bool(*static_cast< bool * >(other.m_data)));
					break;

				case Type::Vector2Float :
					m_data = static_cast< void * >(new Vector2F(*static_cast< Vector2F * >(other.m_data)));
					break;

				case Type::Vector3Float :
					m_data = static_cast< void * >(new Vector3F(*static_cast< Vector3F * >(other.m_data)));
					break;

				case Type::Vector4Float :
					m_data = static_cast< void * >(new Vector4F(*static_cast< Vector4F * >(other.m_data)));
					break;

				case Type::Matrix2Float :
					m_data = static_cast< void * >(new Matrix2F(*static_cast< Matrix2F * >(other.m_data)));
					break;

				case Type::Matrix3Float :
					m_data = static_cast< void * >(new Matrix3F(*static_cast< Matrix3F * >(other.m_data)));
					break;

				case Type::Matrix4Float :
					m_data = static_cast< void * >(new Matrix4F(*static_cast< Matrix4F * >(other.m_data)));
					break;

				case Type::CartesianFrameFloat :
					m_data = static_cast< void * >(new CartesianFrameF(*static_cast< CartesianFrameF * >(other.m_data)));
					break;

				case Type::Color :
					m_data = static_cast< void * >(new ColorF(*static_cast< ColorF * >(other.m_data)));
					break;

				case Type::Null :
					break;
			}
		}

		return *this;
	}

	Variant &
	Variant::operator= (Variant && other) noexcept
	{
		m_type = other.m_type;
		m_data = other.m_data;

		other.m_data = nullptr;
		other.m_type = Type::Null;

		return *this;
	}

	void
	Variant::set (int8_t variable) noexcept
	{
		this->reset();

		m_data = new int8_t(variable);
		m_type = Type::Integer8;
	}

	void
	Variant::set (uint8_t variable) noexcept
	{
		this->reset();

		m_data = new uint8_t(variable);
		m_type = Type::UnsignedInteger8;
	}

	void
	Variant::set (int16_t variable) noexcept
	{
		this->reset();

		m_data = new int16_t(variable);
		m_type = Type::Integer16;
	}

	void
	Variant::set (uint16_t variable) noexcept
	{
		this->reset();

		m_data = new uint16_t(variable);
		m_type = Type::UnsignedInteger16;
	}

	void
	Variant::set (int32_t variable) noexcept
	{
		this->reset();

		m_data = new int32_t(variable);
		m_type = Type::Integer32;
	}

	void
	Variant::set (uint32_t variable) noexcept
	{
		this->reset();

		m_data = new uint32_t(variable);
		m_type = Type::UnsignedInteger32;
	}

	void
	Variant::set (int64_t variable) noexcept
	{
		this->reset();

		m_data = new int64_t(variable);
		m_type = Type::Integer64;
	}

	void
	Variant::set (uint64_t variable) noexcept
	{
		this->reset();

		m_data = new uint64_t(variable);
		m_type = Type::UnsignedInteger64;
	}

	void
	Variant::set (float variable) noexcept
	{
		this->reset();

		m_data = new float(variable);
		m_type = Type::Float;
	}

	void
	Variant::set (double variable) noexcept
	{
		this->reset();

		m_data = new double(variable);
		m_type = Type::Double;
	}

	void
	Variant::set (long double variable) noexcept
	{
		this->reset();

		m_data = new long double(variable);
		m_type = Type::LongDouble;
	}

	void
	Variant::set (bool variable) noexcept
	{
		this->reset();

		m_data = new bool(variable);
		m_type = Type::Boolean;
	}

	void
	Variant::set (const Vector2F & variable) noexcept
	{
		this->reset();

		m_data = new Vector2F(variable);
		m_type = Type::Vector2Float;
	}

	void
	Variant::set (const Vector3F & variable) noexcept
	{
		this->reset();

		m_data = new Vector3F(variable);
		m_type = Type::Vector3Float;
	}

	void
	Variant::set (const Vector4F & variable) noexcept
	{
		this->reset();

		m_data = new Vector4F(variable);
		m_type = Type::Vector4Float;
	}

	void
	Variant::set (const Matrix2F & variable) noexcept
	{
		this->reset();

		m_data = new Matrix2F(variable);
		m_type = Type::Matrix2Float;
	}

	void
	Variant::set (const Matrix3F & variable) noexcept
	{
		this->reset();

		m_data = new Matrix3F(variable);
		m_type = Type::Matrix3Float;
	}

	void
	Variant::set (const Matrix4F & variable) noexcept
	{
		this->reset();

		m_data = new Matrix4F(variable);
		m_type = Type::Matrix4Float;
	}

	void
	Variant::set (const CartesianFrameF & variable) noexcept
	{
		this->reset();

		m_data = new CartesianFrameF(variable);
		m_type = Type::CartesianFrameFloat;
	}

	void
	Variant::set (const ColorF & variable) noexcept
	{
		this->reset();

		m_data = new Color(variable);
		m_type = Type::Color;
	}

	bool
	Variant::isNull () const noexcept
	{
		return m_type == Type::Null;
	}

	Variant::Type
	Variant::type () const noexcept
	{
		return m_type;
	}

	void *
	Variant::getData () const noexcept
	{
		return m_data;
	}

	int8_t
	Variant::asInteger8 () const noexcept
	{
		if ( m_type != Type::Integer8 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'int8_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< int8_t * >(m_data);
	}

	uint8_t
	Variant::asUnsignedInteger8 () const noexcept
	{
		if ( m_type != Type::UnsignedInteger8 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'uint8_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< uint8_t * >(m_data);
	}

	int16_t
	Variant::asInteger16 () const noexcept
	{
		if ( m_type != Type::Integer16 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'int16_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< int16_t * >(m_data);
	}

	uint16_t
	Variant::asUnsignedInteger16 () const noexcept
	{
		if ( m_type != Type::UnsignedInteger16 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'uint16_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< uint16_t * >(m_data);
	}

	int32_t
	Variant::asInteger32 () const noexcept
	{
		if ( m_type != Type::Integer32 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'int32_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< int32_t * >(m_data);
	}

	uint32_t
	Variant::asUnsignedInteger32 () const noexcept
	{
		if ( m_type != Type::UnsignedInteger32 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'uint32_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< uint32_t * >(m_data);
	}

	int64_t
	Variant::asInteger64 () const noexcept
	{
		if ( m_type != Type::Integer64 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'int64_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< int64_t * >(m_data);
	}

	uint64_t
	Variant::asUnsignedInteger64 () const noexcept
	{
		if ( m_type != Type::UnsignedInteger64 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'uint64_t' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< uint64_t * >(m_data);
	}

	float
	Variant::asFloat () const noexcept
	{
		if ( m_type != Type::Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'float' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< float * >(m_data);
	}

	double
	Variant::asDouble () const noexcept
	{
		if ( m_type != Type::Double )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'double' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< double * >(m_data);
	}

	long double
	Variant::asLongDouble () const noexcept
	{
		if ( m_type != Type::LongDouble )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'long double' !" "\n";

			return 0;
		}

		if ( m_data == nullptr )
		{
			return 0;
		}

		return *static_cast< long double * >(m_data);
	}

	bool
	Variant::asBool () const noexcept
	{
		if ( m_type != Type::Boolean )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'bool' !" "\n";

			return false;
		}

		if ( m_data == nullptr )
		{
			return false;
		}

		return *static_cast< bool * >(m_data);
	}

	Vector2F
	Variant::asVector2Float () const noexcept
	{
		if ( m_type != Type::Vector2Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Vector2F' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< Vector2F * >(m_data);
	}

	Vector3F
	Variant::asVector3Float () const noexcept
	{
		if ( m_type != Type::Vector3Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Vector3F' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< Vector3F * >(m_data);
	}

	Vector4F
	Variant::asVector4Float () const noexcept
	{
		if ( m_type != Type::Vector4Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Vector4F' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< Vector4F * >(m_data);
	}

	Matrix2F
	Variant::asMatrix2Float () const noexcept
	{
		if ( m_type != Type::Matrix2Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Matrix2F' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< Matrix2F * >(m_data);
	}

	Matrix3F
	Variant::asMatrix3Float () const noexcept
	{
		if ( m_type != Type::Matrix3Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Matrix3F' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< Matrix3F * >(m_data);
	}

	Matrix4F
	Variant::asMatrix4Float () const noexcept
	{
		if ( m_type != Type::Matrix4Float )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Matrix4F' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< Matrix4F * >(m_data);
	}

	CartesianFrameF
	Variant::asCartesianFrameFloat () const noexcept
	{
		if ( m_type != Type::CartesianFrameFloat )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'CoordinatesF' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< CartesianFrameF * >(m_data);
	}

	ColorF
	Variant::asColor () const noexcept
	{
		if ( m_type != Type::Color )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", this Variant(" << to_cstring(m_type) << ") is not a 'Color' !" "\n";

			return {};
		}

		if ( m_data == nullptr )
		{
			return {};
		}

		return *static_cast< ColorF * >(m_data);
	}

	void
	Variant::reset () noexcept
	{
		if ( m_data == nullptr )
		{
			m_type = Type::Null;

			return;
		}

		switch ( m_type )
		{
			case Type::Integer8 :
				delete static_cast< int8_t * >(m_data);
				break;

			case Type::UnsignedInteger8 :
				delete static_cast< uint8_t * >(m_data);
				break;

			case Type::Integer16 :
				delete static_cast< int16_t * >(m_data);
				break;

			case Type::UnsignedInteger16 :
				delete static_cast< uint16_t * >(m_data);
				break;

			case Type::Integer32 :
				delete static_cast< int32_t * >(m_data);
				break;

			case Type::UnsignedInteger32 :
				delete static_cast< uint32_t * >(m_data);
				break;

			case Type::Integer64 :
				delete static_cast< int64_t * >(m_data);
				break;

			case Type::UnsignedInteger64 :
				delete static_cast< uint64_t * >(m_data);
				break;

			case Type::Float :
				delete static_cast< float * >(m_data);
				break;

			case Type::Double :
				delete static_cast< double * >(m_data);
				break;

			case Type::LongDouble :
				delete static_cast< long double * >(m_data);
				break;

			case Type::Boolean :
				delete static_cast< bool * >(m_data);
				break;

			case Type::Vector2Float :
				delete static_cast< Vector2F * >(m_data);
				break;

			case Type::Vector3Float :
				delete static_cast< Vector3F * >(m_data);
				break;

			case Type::Vector4Float :
				delete static_cast< Vector4F * >(m_data);
				break;

			case Type::Matrix2Float :
				delete static_cast< Matrix2F * >(m_data);
				break;

			case Type::Matrix3Float :
				delete static_cast< Matrix3F * >(m_data);
				break;

			case Type::Matrix4Float :
				delete static_cast< Matrix4F * >(m_data);
				break;

			case Type::CartesianFrameFloat :
				delete static_cast< CartesianFrameF * >(m_data);
				break;

			case Type::Color :
				delete static_cast< ColorF * >(m_data);
				break;

			case Type::Null :
				break;
		}

		m_type = Type::Null;
		m_data = nullptr;
	}

	std::ostream &
	operator<< (std::ostream & out, const Variant & variant) noexcept
	{
		switch ( variant.m_type )
		{
			case Variant::Type::Integer8 :
				out << *static_cast< int8_t * >(variant.m_data);
				break;

			case Variant::Type::UnsignedInteger8 :
				out << *static_cast< uint8_t * >(variant.m_data);
				break;

			case Variant::Type::Integer16 :
				out << *static_cast< int16_t * >(variant.m_data);
				break;

			case Variant::Type::UnsignedInteger16 :
				out << *static_cast< uint16_t * >(variant.m_data);
				break;

			case Variant::Type::Integer32 :
				out << *static_cast< int32_t * >(variant.m_data);
				break;

			case Variant::Type::UnsignedInteger32 :
				out << *static_cast< uint32_t * >(variant.m_data);
				break;

			case Variant::Type::Integer64 :
				out << *static_cast< int64_t * >(variant.m_data);
				break;

			case Variant::Type::UnsignedInteger64 :
				out << *static_cast< uint64_t * >(variant.m_data);
				break;

			case Variant::Type::Float :
				out << *static_cast< float * >(variant.m_data);
				break;

			case Variant::Type::Double :
				out << *static_cast< double * >(variant.m_data);
				break;

			case Variant::Type::LongDouble :
				out << *static_cast< long double * >(variant.m_data);
				break;

			case Variant::Type::Boolean :
				out << *static_cast< bool * >(variant.m_data);
				break;

			case Variant::Type::Vector2Float :
				out << *static_cast< Vector2F * >(variant.m_data);
				break;

			case Variant::Type::Vector3Float :
				out << *static_cast< Vector3F * >(variant.m_data);
				break;

			case Variant::Type::Vector4Float :
				out << *static_cast< Vector4F * >(variant.m_data);
				break;

			case Variant::Type::Matrix2Float :
				out << *static_cast< Matrix2F * >(variant.m_data);
				break;

			case Variant::Type::Matrix3Float :
				out << *static_cast< Matrix3F * >(variant.m_data);
				break;

			case Variant::Type::Matrix4Float :
				out << *static_cast< Matrix4F * >(variant.m_data);
				break;

			case Variant::Type::CartesianFrameFloat :
				out << *static_cast< CartesianFrameF * >(variant.m_data);
				break;

			case Variant::Type::Color :
				out << *static_cast< ColorF * >(variant.m_data);
				break;

			case Variant::Type::Null :
				out << "Null";
				break;
		}

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
