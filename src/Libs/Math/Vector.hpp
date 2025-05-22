/*
 * src/Libs/Math/Vector.hpp
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
#include <cstddef>
#include <cmath>
#include <array>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <span>
#include <limits>

/* Local inclusions for usages. */
#include "Libs/Randomizer.hpp"
#include "Libs/String.hpp"

namespace EmEn::Libs::Math
{
	constexpr auto X{0UL};
	constexpr auto Y{1UL};
	constexpr auto Z{2UL};
	constexpr auto W{3UL};

	constexpr auto U{0UL};
	constexpr auto V{1UL};

	constexpr auto R{0UL};
	constexpr auto G{1UL};
	constexpr auto B{2UL};
	constexpr auto A{3UL};

	/**
	 * @brief Defines a vector from two to four values suitable for 3D graphics.
	 * @tparam dim_t The dimension of the vector. This can be 2, 3 or 4.
	 * @tparam precision_t The data precision should be a floating point number. Default float.
	 * @note For position, the fourth component should be 1, and for a direction, it should be 0.
	 * In that way, translation with matrices won't affect direction vectors.
	 * @todo Check pertinence of OpenMP pragmas
	 */
	template< size_t dim_t, typename precision_t = float >
	requires (dim_t == 2 || dim_t == 3 || dim_t == 4) && std::is_arithmetic_v< precision_t >
	class Vector final
	{
		public:

			/**
			 * @brief Constructs a default vector.
			 */
			constexpr
			Vector () noexcept = default;

			/**
			 * @brief Swizzle constructor for Vector< 2, precision_t >.
			 * @param vector A reference to a Vector< 3, precision_t >.
			 */
			template< size_t N = dim_t >
			constexpr
			explicit
			Vector (const Vector< 3, precision_t > & vector) noexcept requires (N == 2)
				: m_data({vector[X], vector[Y]})
			{

			}

			/**
			 * @brief Swizzle constructor for Vector< 2, precision_t >.
			 * @param vector A reference to a Vector< 4, precision_t >.
			 */
			template< size_t N = dim_t >
			constexpr
			explicit
			Vector (const Vector< 4, precision_t > & vector) noexcept requires (N == 2)
				: m_data({vector[X], vector[Y]})
			{

			}

			/**
			 * @brief Swizzle constructor for Vector< 3, precision_t >.
			 * @param vector A reference to a Vector< 2, precision_t >.
			 * @param z Z component value. Default 0.
			 */
			template< size_t N = dim_t >
			constexpr
			explicit
			Vector (const Vector< 2, precision_t > & vector, precision_t z = 0) noexcept requires (N == 3)
				: m_data({vector[X], vector[Y], z})
			{

			}

			/**
			 * @brief Swizzle constructor for Vector< 3, precision_t >.
			 * @param vector A reference to a Vector< 4, precision_t >.
			 */
			template< size_t N = dim_t >
			constexpr
			explicit
			Vector (const Vector< 4, precision_t > & vector) noexcept requires (N == 3)
				: m_data({vector[X], vector[Y], vector[Z]})
			{

			}

			/**
			 * @brief Swizzle constructor for Vector< 4, precision_t >.
			 * @param vector A reference to a Vector< 2, precision_t >.
			 * @param z Z component value. Default 0.
			 * @param w W component value. Default 0.
			 */
			template< size_t N = dim_t >
			constexpr
			explicit
			Vector (const Vector< 2, precision_t > & vector, precision_t z = 0, precision_t w = 0) noexcept requires (N == 4)
				: m_data({vector[X], vector[Y], z, w})
			{

			}

			/**
			 * @brief Swizzle constructor for Vector< 4, precision_t >.
			 * @param vector A reference to a Vector< 3, precision_t >.
			 * @param w W component value. Default 0.
			 */
			template< size_t N = dim_t >
			constexpr
			explicit
			Vector (const Vector< 3, precision_t > & vector, precision_t w = 0) noexcept requires (N == 4)
				: m_data({vector[X], vector[Y], vector[Z], w})
			{

			}

			/**
			 * @brief Simple parametric constructor for Vector< 2, precision_t >.
			 * @param x Value of the vector first component.
			 * @param y Value of the vector second component.
			 */
			template< size_t N = dim_t >
			constexpr
			Vector (precision_t x, precision_t y) noexcept requires (N == 2)
				: m_data({x, y})
			{

			}

			/**
			 * @brief Simple parametric constructor for Vector< 3, precision_t >.
			 * @param x Value of the vector first component.
			 * @param y Value of the vector second component.
			 * @param z Value of the vector third component.
			 */
			template< size_t N = dim_t >
			constexpr
			Vector (precision_t x, precision_t y, precision_t z) noexcept requires (N == 3)
				: m_data({x, y, z})
			{

			}

			/**
			 * @brief Simple parametric constructor for Vector< 4, precision_t >.
			 * @param x Value of the vector first component.
			 * @param y Value of the vector second component.
			 * @param z Value of the vector third component.
			 * @param w Value of the vector fourth component. Default is 0.
			 */
			template< size_t N = dim_t >
			constexpr
			Vector (precision_t x, precision_t y, precision_t z, precision_t w = 0) noexcept requires (N == 4)
				: m_data({x, y, z, w})
			{

			}

			/**
			 * @brief Constructs a vector from STL array.
			 * @param data A std::array of N value.
			 */
			explicit
			constexpr
			Vector (const std::array< precision_t, dim_t > & data) noexcept
				: m_data(data)
			{

			}

			/**
			 * @brief Constructs a vector from C-Style array.
			 * @param data A pointer to a C-Style containing at least the dimension of the vector.
			 */
			explicit
			Vector (std::span< const precision_t, dim_t > data) noexcept
			{
				std::copy(data.cbegin(), data.cend(), m_data.begin());
			}

			/**
			 * @brief Copy raw data to vector.
			 * @param data A pointer to a C-Style containing at least the dimension of the vector.
			 * @return void
			 */
			void
			copy (std::span< precision_t, dim_t > data) const noexcept
			{
				std::copy(m_data.begin(), m_data.end(), data.begin());
			}

			/**
			 * @brief Builds a Vector< dim_t, precision_t > using a raw definition from a std::string.
			 * @warning Bad-formatted string will result to an origin vector.
			 * @param str The string containing the vector definition.
			 * @param separator A char to specify the value separator. Default is white space.
			 * @param offset An index where to execute copying values. Default is 0.
			 */
			explicit
			Vector (const std::string & str, char separator = ' ', size_t offset = 0)
			{
				const auto chunks = String::explode(str, separator, false);

				if ( chunks.size() < offset + dim_t )
				{
					return;
				}

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] = String::toNumber< precision_t >(chunks[offset + index]);
				}
			}

			/**
			 * @brief Copy assignment operator specialization from Vector< 2, precision_t >.
			 * @param copy The copied vector.
			 * @return Vector
			 */
			template< size_t N = dim_t >
			Vector &
			operator= (const Vector< 2, precision_t > & copy) noexcept requires (N != 2)
			{
				m_data[X] = copy[X];
				m_data[Y] = copy[Y];

				if constexpr ( dim_t == 3 || dim_t == 4 )
				{
					m_data[Z] = 0;
				}

				if constexpr ( dim_t == 4 )
				{
					m_data[W] = 0;
				}

				return *this;
			}

			/**
			 * @brief Copy assignment operator specialization from Vector< 3, precision_t >.
			 * @param copy The copied vector.
			 * @return Vector &
			 */
			template< size_t N = dim_t >
			Vector &
			operator= (const Vector< 3, precision_t > & copy) noexcept requires (N != 3)
			{
				m_data[X] = copy[X];
				m_data[Y] = copy[Y];

				if constexpr ( dim_t == 3 || dim_t == 4 )
				{
					m_data[Z] = copy[Z];
				}

				return *this;
			}

			/**
			 * @brief Copy assignment operator specialization from Vector< 4, precision_t >.
			 * @param copy The copied vector.
			 * @return Vector &
			 */
			template< size_t N = dim_t >
			Vector &
			operator= (const Vector< 4, precision_t > & copy) noexcept requires (N != 4)
			{
				m_data[X] = copy[X];
				m_data[Y] = copy[Y];

				if constexpr ( dim_t == 3 || dim_t == 4 )
				{
					m_data[Z] = copy[Z];
				}

				if constexpr ( dim_t == 4 )
				{
					m_data[W] = copy[W];
				}

				return *this;
			}

			/**
			 * @brief Returns a copy of the Vector< dim_t, precision_t >.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator+ () const noexcept
			{
				return *this;
			}

			/**
			 * @brief Returns the reverse copy of the Vector< dim_t, precision_t >.
			 * @note Same as operation as method inverse().
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator- () const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = -m_data[index];
				}

				return vector;
			}

			/**
			 * @brief Data accessor.
			 * @param index The index of vector element.
			 * @return precision_t &
			 */
			precision_t &
			operator[] (size_t index) noexcept
			{
				return m_data[index];
			}

			/**
			 * @brief Data accessor (const version).
			 * @param index The index of vector element.
			 * @return const precision_t &
			 */
			[[nodiscard]]
			const precision_t &
			operator[] (size_t index) const noexcept
			{
				return m_data[index];
			}

			/**
			 * @brief Performs an equality comparison between vectors.
			 * @note Floating point number version.
			 * @param operand A reference to another vector.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const Vector & operand) const noexcept requires (std::is_floating_point_v< precision_t > )
			{
				if ( this != &operand )
				{
					if ( Utility::different(m_data[X], operand.m_data[X]) )
					{
						return false;
					}

					if ( Utility::different(m_data[Y], operand.m_data[Y]) )
					{
						return false;
					}

					if constexpr ( dim_t == 3 || dim_t == 4 )
					{
						if ( Utility::different(m_data[Z], operand.m_data[Z]) )
						{
							return false;
						}
					}

					if constexpr ( dim_t == 4 )
					{
						if ( Utility::different(m_data[W], operand.m_data[W]) )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Performs an equality comparison between vectors.
			 * @note Integral number version.
			 * @param operand A reference to another vector.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const Vector & operand) const noexcept requires (std::is_integral_v< precision_t > )
			{
				if ( this != &operand )
				{
					if ( m_data[X] != operand.m_data[X] )
					{
						return false;
					}

					if ( m_data[Y] != operand.m_data[Y] )
					{
						return false;
					}

					if constexpr ( dim_t == 3 || dim_t == 4 )
					{
						if ( m_data[Z] != operand.m_data[Z] )
						{
							return false;
						}
					}

					if constexpr ( dim_t == 4 )
					{
						if ( m_data[W] != operand.m_data[W] )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Performs an inequality comparison between vectors.
			 * @param operand A reference to another vector.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator!= (const Vector & operand) const noexcept
			{
				return !this->operator==(operand);
			}

			/**
			 * @brief Returns the addition between two vectors.
			 * @param operand A reference to another vector.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator+ (const Vector & operand) const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = m_data[index] + operand.m_data[index];
				}

				return vector;
			}

			/**
			 * @brief Returns the addition between a vector and a scalar.
			 * @param operand A scalar.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator+ (const precision_t & operand) const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = m_data[index] + operand;
				}

				return vector;
			}

			/**
			 * @brief Returns the addition with another vector.
			 * @param operand A reference to another vector.
			 * @return Vector &
			 */
			Vector &
			operator+= (const Vector & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] += operand.m_data[index];
				}

				return *this;
			}

			/**
			 * @brief Returns the addition with a scalar.
			 * @param operand A scalar.
			 * @return Vector &
			 */
			Vector &
			operator+= (const precision_t & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] += operand;
				}

				return *this;
			}

			/**
			 * @brief Returns the subtraction between two vectors.
			 * @param operand A reference to another vector.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator- (const Vector & operand) const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = m_data[index] - operand.m_data[index];
				}

				return vector;
			}

			/**
			 * @brief Returns the subtraction between a vector and a scalar.
			 * @param operand A scalar.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator- (const precision_t & operand) const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = m_data[index] - operand;
				}

				return vector;
			}

			/**
			 * @brief Returns the subtraction with another vector.
			 * @param operand A reference to another vector.
			 * @return Vector &
			 */
			Vector &
			operator-= (const Vector & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] -= operand.m_data[index];
				}

				return *this;
			}

			/**
			 * @brief Returns the subtraction with a scalar.
			 * @param operand A scalar.
			 * @return Vector &
			 */
			Vector &
			operator-= (const precision_t & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] -= operand;
				}

				return *this;
			}

			/**
			 * @brief Returns the multiplication between two vectors.
			 * @param operand A reference to another vector.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator* (const Vector & operand) const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = m_data[index] * operand.m_data[index];
				}

				return vector;
			}

			/**
			 * @brief Returns the multiplication between a vector and a scalar.
			 * @param operand A scalar.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator* (const precision_t & operand) const noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = m_data[index] * operand;
				}

				return vector;
			}

			/**
			 * @brief Returns the multiplication with another vector.
			 * @param operand A reference to another vector.
			 * @return Vector &
			 */
			Vector &
			operator*= (const Vector & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] *= operand.m_data[index];
				}

				return *this;
			}

			/**
			 * @brief Returns the multiplication with a scalar.
			 * @param operand A scalar.
			 * @return Vector &
			 */
			Vector &
			operator*= (const precision_t & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] *= operand;
				}

				return *this;
			}

			/**
			 * @brief Divides vector components by a scalar.
			 * @warning Division by zero does not throw exception!
			 * @param operand A scalar.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			operator/ (const precision_t & operand) const noexcept
			{
				Vector vector;

				if ( Utility::isZero(operand) )
				{
					#pragma omp simd
					for ( size_t index = 0; index < dim_t; index++ )
					{
						vector.m_data[index] = std::numeric_limits< precision_t >::quiet_NaN();
					}
				}
				else
				{
					#pragma omp simd
					for ( size_t index = 0; index < dim_t; index++ )
					{
						vector.m_data[index] = m_data[index] / operand;
					}
				}

				return vector;
			}

			/**
			 * @brief Divides vector components by a scalar.
			 * @warning A division by zero is quietly ignored.
			 * @param operand A scalar.
			 * @return Vector &
			 */
			Vector &
			operator/= (const precision_t & operand) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] /= operand;
				}
				
				return *this;
			}

			/**
			 * @brief Returns the X component.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			x () const noexcept
			{
				return m_data[X];
			}

			/**
			 * @brief Returns the Y component.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			y () const noexcept
			{
				return m_data[Y];
			}

			/**
			 * @brief Returns the Z component.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			z () const noexcept requires (dim_t == 3 || dim_t == 4)
			{
				return m_data[Z];
			}

			/**
			 * @brief Returns the W component.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			w () const noexcept requires (dim_t == 4)
			{
				return m_data[W];
			}

			/**
			 * @brief Performs an equality comparison between vectors.
			 * @param operand A reference to another vector.
			 * @param epsilon The floating point tolerance value. Default C++ epsilon.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			equal (const Vector & operand, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) const noexcept
			{
				for ( size_t index = 0; index < dim_t ; index++ )
				{
					if ( Utility::different(m_data[index], operand.m_data[index], epsilon) )
					{
						return false;
					}
				}

				return true;
			}

			/**
			 * @brief Performs an inequality comparison between vectors.
			 * @param operand A reference to another vector.
			 * @param epsilon The floating point tolerance value. Default C++ epsilon.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			different (const Vector & operand, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) const noexcept
			{
				return !this->equal(operand, epsilon);
			}

			/** @brief Resets vector to 0. */
			void
			reset () noexcept
			{
				m_data.fill(0);
			}

			/**
			 * @brief Returns the vector data as a C array.
			 * @return precision_t *
			 */
			[[nodiscard]]
			precision_t *
			data () noexcept
			{
				return m_data.data();
			}

			/**
			 * @brief Returns the vector data as a C array.
			 * @return const precision_t *
			 */
			[[nodiscard]]
			const precision_t *
			data () const noexcept
			{
				return m_data.data();
			}

			/**
			 * @brief Swaps two vectors data.
			 * @param target A reference to another Vector.
			 */
			void
			swap (Vector & target) noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					std::swap(m_data[index], target.m_data[index]);
				}
			}

			/**
			 * @brief Reverses the vector direction.
			 * @return Vector &
			 */
			Vector &
			inverse () noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[index] = -m_data[index];
				}

				return *this;
			}

			/**
			 * @brief Returns the inverted vector.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			inversed () const noexcept
			{
				return Vector::operator-();
			}

			/**
			 * @brief Scales the vector length by a scalar value.
			 * @param scalar Value to multiply with.
			 * @return Vector &
			 */
			Vector &
			scale (precision_t scalar) noexcept
			{
				return *this *= scalar;
			}

			/**
			 * @brief Returns a scaled copy of this vector.
			 * @param scalar Value to multiply with.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			scaled (precision_t scalar) const noexcept
			{
				return *this * scalar;
			}

			/**
			 * @brief Scales the normalized vector length by a scalar value.
			 * @param scalar Value to multiply with.
			 * @return Vector &
			 */
			[[nodiscard]]
			Vector &
			rescale (precision_t scalar) noexcept
			{
				return this->normalize().scale(scalar);
			}

			/**
			 * @brief Returns a rescaled copy of this vector.
			 * @param scalar Value to multiply with.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			rescaled (precision_t scalar) const noexcept
			{
				return this->normalized().scale(scalar);
			}

			/**
			 * @brief Returns the squared length of the vector.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			lengthSquared () const noexcept
			{
				precision_t value = 0;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					value += m_data[index] * m_data[index];
				}

				return value;
			}

			/**
			 * @brief Returns the norm (length) of the vector.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			length () const noexcept
			{
				return std::sqrt(this->lengthSquared());
			}

			/**
			 * @brief Rescales the vector length to unit.
			 * @return Vector &
			 */
			Vector &
			normalize () noexcept requires (std::is_floating_point_v< precision_t >)
			{
				const auto length = this->lengthSquared();

				if ( !Utility::isZero(length) )
				{
					this->scale(1 / static_cast< precision_t >(std::sqrt(length)));
				}

				return *this;
			}

			/**
			 * @brief Rescales the vector 2 length to unit (Snap to dominant axis).
			 * @warning This version is mathematically incorrect due to integer usage.
			 * This will, however, make the vector point to the highest direction.
			 * @return Vector &
			 */
			Vector &
			normalize () noexcept requires (dim_t == 2 && std::is_integral_v< precision_t >)
			{
				const auto absX = std::abs(m_data[X]);
				const auto absY = std::abs(m_data[Y]);

				if ( absX != 0 || absY != 0 )
				{
					if ( absX >= absY )
					{
						m_data[X] = m_data[X] > 0 ? 1 : -1;
						m_data[Y] = 0;
					}
					else
					{
						m_data[X] = 0;
						m_data[Y] = m_data[Y] > 0 ? 1 : -1;
					}
				}
				else
				{
					m_data[X] = 0;
					m_data[Y] = 0;
				}

				return *this;
			}

			/**
			 * @brief Rescales the vector 3|4 length to unit (Snap to dominant axis).
			 * @warning This version is mathematically incorrect due to integer usage.
			 * This will, however, make the vector point to the highest direction.
			 * @return Vector &
			 */
			Vector &
			normalize () noexcept requires ((dim_t == 3 || dim_t == 4) && std::is_integral_v< precision_t >)
			{
				const auto absX = std::abs(m_data[X]);
				const auto absY = std::abs(m_data[Y]);
				const auto absZ = std::abs(m_data[Z]);

				if ( absX != 0 || absY != 0 || absZ != 0 )
				{
					if ( absX >= absY && absX >= absZ )
					{
						m_data[X] = m_data[X] > 0 ? 1 : -1;
						m_data[Y] = 0;
						m_data[Z] = 0;
					}
					else if ( absY >= absZ )
					{
						m_data[X] = 0;
						m_data[Y] = m_data[Y] > 0 ? 1 : -1;
						m_data[Z] = 0;
					}
					else
					{
						m_data[X] = 0;
						m_data[Y] = 0;
						m_data[Z] = m_data[Z] > 0 ? 1 : -1;
					}
				}
				else
				{
					m_data[X] = 0;
					m_data[Y] = 0;
					m_data[Z] = 0;
				}

				/* NOTE: If the vector is dimension 4, the last component must be set to zero. */
				if constexpr ( dim_t == 4 )
				{
					m_data[W] = 0;
				}

				return *this;
			}

			/**
			 * @brief Rescales the vector length to unit.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			normalized () const noexcept requires (std::is_floating_point_v< precision_t >)
			{
				const auto length = this->lengthSquared();

				if ( !Utility::isZero(length) )
				{
					return this->scaled(static_cast< precision_t >(1) / std::sqrt(length));
				}

				return {};
			}

			/**
			 * @brief Rescales the vector 2 length to unit (Snap to dominant axis).
			 * @warning This version is mathematically incorrect due to integer usage.
			 * This will however make the vector point to the highest direction.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			normalized () const noexcept requires (dim_t == 2 && std::is_integral_v< precision_t >)
			{
				const auto absX = std::abs(m_data[X]);
				const auto absY = std::abs(m_data[Y]);

				if ( absX != 0 || absY != 0 )
				{
					if ( absX >= absY )
					{
						return {m_data[X] > 0 ? 1 : -1, 0};
					}

					return {0, m_data[Y] > 0 ? 1 : -1};
				}

				return {};
			}

			/**
			 * @brief Rescales the vector 3|4 length to unit (Snap to dominant axis).
			 * @warning This version is mathematically incorrect due to integer usage.
			 * This will however make the vector point to the highest direction.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			normalized () const noexcept requires ((dim_t == 3 || dim_t == 4) && std::is_integral_v< precision_t >)
			{
				const auto absX = std::abs(m_data[X]);
				const auto absY = std::abs(m_data[Y]);
				const auto absZ = std::abs(m_data[Z]);

				if ( absX != 0 || absY != 0 || absZ != 0 )
				{
					if ( absX >= absY && absX >= absZ )
					{
						return {m_data[X] > 0 ? 1 : -1, 0, 0};
					}

					if ( absY >= absZ )
					{
						return {0, m_data[Y] > 0 ? 1 : -1, 0};
					}

					return {0, 0, m_data[Z] > 0 ? 1 : -1};
				}

				return {};
			}

			/**
			 * @brief Performs a dot-product between two vectors.
			 * @note A value close to 1.0 or -1.0 means that the vectors tend to be parallel.
			 * A value close to 0.0 means that the vectors tend to be perpendicular.
			 * @param lhs A Vector as first operand.
			 * @param rhs A Vector as second operand.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			static
			precision_t
			dotProduct (const Vector & lhs, const Vector & rhs) noexcept
			{
				precision_t value = 0;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					value += lhs.m_data[index] * rhs.m_data[index];
				}

				return value;
			}

			/**
			 * @brief Performs a cross-product between two vectors.
			 * @note Specialization for vector 2.
			 * @param lhs A Vector as first operand.
			 * @param rhs A Vector as second operand.
			 * @return precision_t
			 */
			[[nodiscard]]
			static
			precision_t
			crossProduct (const Vector & lhs, const Vector & rhs) noexcept requires (dim_t == 2)
			{
				return (lhs[X] * rhs[Y]) - (lhs[Y] * rhs[X]);
			}

			/**
			 * @brief Performs a cross-product between two vectors.
			 * @note Specialization for vector 3.
			 * @param lhs A Vector as first operand.
			 * @param rhs A Vector as second operand.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			crossProduct (const Vector & lhs, const Vector & rhs) noexcept requires (dim_t == 3 || dim_t == 4)
			{
				if constexpr ( dim_t == 3 )
				{
					return {
						(lhs.m_data[Y] * rhs.m_data[Z]) - (lhs.m_data[Z] * rhs.m_data[Y]),
						(lhs.m_data[Z] * rhs.m_data[X]) - (lhs.m_data[X] * rhs.m_data[Z]),
						(lhs.m_data[X] * rhs.m_data[Y]) - (lhs.m_data[Y] * rhs.m_data[X])
					};
				}

				if constexpr ( dim_t == 4 )
				{
					return {
						(lhs.m_data[Y] * rhs.m_data[Z]) - (lhs.m_data[Z] * rhs.m_data[Y]),
						(lhs.m_data[Z] * rhs.m_data[X]) - (lhs.m_data[X] * rhs.m_data[Z]),
						(lhs.m_data[X] * rhs.m_data[Y]) - (lhs.m_data[Y] * rhs.m_data[X]),
						0.0F
					};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Returns distance between two points.
			 * @param lhs A Vector representing the first point.
			 * @param rhs A Vector representing the second point.
			 * @return precision_t
			 */
			[[nodiscard]]
			static
			precision_t
			distance (const Vector & lhs, const Vector & rhs) noexcept
			{
				/* Formula : sqrt( (x2-x1)² + (y2-y1)² + (z2-z1) ) */
				return (rhs - lhs).length();
			}

			/**
			 * @brief Returns squared distance between two points.
			 * @note Faster than distance() to compare two distances.
			 * @param lhs A Vector representing the first point.
			 * @param rhs A Vector representing the second point.
			 * @return precision_t
			 */
			[[nodiscard]]
			static
			precision_t
			distanceSquared (const Vector & lhs, const Vector & rhs) noexcept
			{
				return (rhs - lhs).lengthSquared();
			}

			/**
			 * @brief Returns distance between this vector and a point.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			distanceToPoint (const Vector & point) const noexcept
			{
				return (*this - point).length();
			}

			/**
			 * @brief Returns squared distance between this vector and a point.
			 * @note Faster than distanceToPoint() to compare two distances.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			distanceSquaredToPoint (const Vector & point) const noexcept
			{
				return (*this - point).lengthSquared();
			}

			/**
			 * @brief Returns distance between this vector and a line.
			 * @param point A point where the line is passing.
			 * @param direction The direction vector of the line.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			distanceToLine (const Vector & point, const Vector & direction) const noexcept
			{
				if ( direction.isZero() )
				{
					return (*this - point).length();
				}

				const auto p = point + (Vector::dotProduct(*this - point, direction) * direction);

				return (*this - p).length();
			}

			/**
			 * @brief Returns distance between this vector and a line.
			 * @note Faster than distanceToLine() to compare two distances.
			 * @param point A point where the line is passing.
			 * @param direction The direction vector of the line.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			distanceSquaredToLine (const Vector & point, const Vector & direction) const noexcept
			{
				if ( direction.isZero() )
				{
					return (*this - point).lengthSquared();
				}

				const auto p = point + (Vector::dotProduct(*this - point, direction) * direction);

				return (*this - p).lengthSquared();
			}

			/**
			 * @brief Returns distance between this vector and a plane.
			 * @param plane A vector that defines the plane.
			 * @param normal A vector perpendicular to the plane.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			distanceToPlane (const Vector & plane, const Vector & normal) const noexcept
			{
				return Vector::dotProduct(*this - plane, normal);
			}

			/**
			 * @brief Returns distance between this vector and a plane.
			 * @param pointA A point lying in the plane.
			 * @param pointB A point lying in the plane.
			 * @param pointC A point lying in the plane.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			distanceToPlane (const Vector & pointA, const Vector & pointB, const Vector & pointC) const noexcept
			{
				return Vector::dotProduct(*this - pointA, this->normal(pointB - pointA, pointC - pointA));
			}

			/**
			 * @brief Returns the angle between two vectors in radian.
			 * @note You can use Math::Degree() to convert the result.
			 * @param vectorA A Vector as first operand.
			 * @param vectorB A Vector as second operand.
			 * @return precision_t
			 */
			[[nodiscard]]
			static
			precision_t
			angle (const Vector & vectorA, const Vector & vectorB) noexcept requires (std::is_floating_point_v< precision_t >)
			{
				precision_t factor = vectorA.length() * vectorB.length();

				if ( factor < std::numeric_limits< precision_t >::epsilon() )
				{
					return static_cast< precision_t >(0);
				}

				const precision_t dotResult = Vector::dotProduct(vectorA, vectorB) / factor;

				return std::acos(std::clamp(dotResult, static_cast< precision_t >(-1), static_cast< precision_t >(1)));
			}

			/**
			 * @brief Returns the angle of a 2D vector.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			angle2D ()
			const noexcept requires (dim_t == 2 && std::is_floating_point_v< precision_t >)
			{
				return std::atan2(this->y(), this->x());
			}

			/**
			 * @brief Returns the reflected vector on a surface.
			 * @param incident The Vector to reflect.
			 * @param normal The Vector representing the surface normal where the reflection occurs.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			reflect (const Vector & incident, const Vector & normal) noexcept
			{
				/* I' = I - 2 * dot(N, I) * N; */
				//return incident - 2.0F * Vector3::dot(normal, incident) * normal;
				return incident - (normal * (Vector::dotProduct(normal, incident) * 2.0F));
			}

			/**
			 * @brief Returns the refracted vector in a surface.
			 * @param incident The Vector to refract.
			 * @param normal A reference to a Vector representing the surface normal where the refraction occurs.
			 * @param eta Specifies the ratio of indices of refraction.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			refract (const Vector & incident, const Vector & normal, float eta) noexcept
			{
				const auto d = Vector::dotProduct(normal, incident);

				const auto k = 1.0 - (eta * eta * (1.0 - d * d));

				if ( k < 0.0 )
				{
					return {};
				}

				return (eta * incident) - ((eta * d + std::sqrt(k)) * normal);
			}

			/**
			 * @brief Returns the normal vector of a plane defined by two vectors.
			 * @param lhs A reference to a Vector.
			 * @param rhs A reference to a Vector.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			normal (const Vector & lhs, const Vector & rhs) noexcept
			{
				return Vector::crossProduct(lhs, rhs).normalize();
			}

			/**
			 * @brief Gets the normal vector from a triangle defined by three points.
			 * @note Gives points in counter-clock wise order!
			 * @param pointA Vertex A position of the triangle.
			 * @param pointB Vertex B position of the triangle.
			 * @param pointC Vertex C position of the triangle.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			normal (const Vector & pointA, const Vector & pointB, const Vector & pointC) noexcept
			{
				return Vector::crossProduct(pointA - pointB, pointB - pointC).normalize();
			}

			/**
			 * @brief Gets the tangent vector from a triangle defined by three points and three texture coordinates.
			 * @note Gives points in counter-clock wise order!
			 * @param pointA Vertex A position of the triangle.
			 * @param uvA Vertex A texture coordinates of the triangle.
			 * @param pointB Vertex B position of the triangle.
			 * @param uvB Vertex B texture coordinates of the triangle.
			 * @param pointC Vertex C position of the triangle.
			 * @param uvC Vertex C texture coordinates of the triangle.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			tangent (const Vector & pointA, const Vector & uvA, const Vector & pointB, const Vector & uvB, const Vector & pointC, const Vector & uvC) noexcept
			{
				const auto tmpA = (pointA - pointB) * (uvC - uvB)[Y];
				const auto tmpB = (pointB - pointC) * (uvB - uvA)[Y];

				return (tmpA - tmpB).normalize();
			}

			/**
			 * @brief Gets the perpendicular vector.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			perpendicular () const noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return Vector{-m_data[Y], m_data[X]}.normalize();
				}
				else
				{
					if ( std::abs(m_data[X]) > std::abs(m_data[Z]) )
					{
						return Vector{-m_data[Y], m_data[X], 0}.normalize();
					}

					return Vector{0, -m_data[Z], m_data[Y]}.normalize();
				}
			}

			/**
			 * @brief Gets the mid-point.
			 * @param lhs Left-handed side vector.
			 * @param rhs Right-handed side vector.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			midPoint (const Vector & lhs, const Vector & rhs) noexcept
			{
				Vector result;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; ++index )
				{
					result[index] = (lhs[index] + rhs[index]) * static_cast< precision_t >(0.5);
				}

				return result;
			}

			/**
			 * @brief Checks whether the vector length is zero.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isZero () const noexcept
			{
				return std::all_of(m_data.cbegin(), m_data.cend(), [] (const auto & value) {
					return Utility::isZero(value);
				});
			}

			/**
			 * @brief Checks if the vector is a unit vector (has length of 1).
			 * @note Floating point number version using tolerance.
			 * @param tolerance The acceptable deviation from 1.0 for the squared length.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOne (precision_t tolerance = std::numeric_limits< precision_t >::epsilon() * 100) const noexcept requires std::is_floating_point_v< precision_t >
			{
				const precision_t lenSq = this->lengthSquared();

				return std::abs(lenSq - static_cast< precision_t >(1)) < tolerance;
			}

			/**
			 * @brief Checks if the vector is a unit vector (has length of 1).
			 * @note Integral number version. Length must be exactly 1.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOne () const noexcept requires std::is_integral_v< precision_t >
			{
				return this->lengthSquared() == static_cast< precision_t >(1);
			}

			/**
			 * @brief Checks whether every component of the vector is one.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAllComponentOne () const noexcept
			{
				return std::all_of(m_data.cbegin(), m_data.cend(), [] (const auto & value) {
					return value == static_cast< precision_t >(1);
				});
			}

			/**
			 * @brief Returns a vector that is made from the largest components of two vectors.
			 * @param a first vector.
			 * @param b second vector.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			min (const Vector & a, const Vector & b) noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = std::min(a[index], b[index]);
				}

				return vector;
			}

			/**
			 * @brief Returns a vector that is made from the smallest components of two vectors.
			 * @param a first vector.
			 * @param b second vector.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			max (const Vector & a, const Vector & b) noexcept
			{
				Vector vector;

				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector.m_data[index] = std::max(a[index], b[index]);
				}

				return vector;
			}

			/**
			 * @brief Swaps axis.
			 * @return void
			 */
			void
			swapAxis () noexcept requires (dim_t == 2)
			{
				std::swap(m_data[X], m_data[Y]);
			}

			/**
			 * @brief Converts a vector to a vector of 2 values.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector< 2, precision_t >
			toVector2 () const noexcept requires (dim_t == 3 || dim_t == 4)
			{
				return {m_data[X], m_data[Y]};
			}

			/**
			 * @brief Converts a vector to a vector of 3 values.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector< 3, precision_t >
			toVector3 () const noexcept requires (dim_t == 2 || dim_t == 4)
			{
				if constexpr ( dim_t == 2 )
				{
					return {m_data[X], m_data[Y], 0};
				}

				if constexpr ( dim_t == 4 )
				{
					return {m_data[X], m_data[Y], m_data[Z]};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Converts a vector to a vector of 4 values.
			 * @param lastComponent The value of the last component. Default 0.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector< 4, precision_t >
			toVector4 (precision_t lastComponent = 0) const noexcept requires (dim_t == 2 || dim_t == 3)
			{
				if constexpr ( dim_t == 2 )
				{
					return {m_data[X], m_data[Y], 0, lastComponent};
				}

				if constexpr ( dim_t == 3 )
				{
					return {m_data[X], m_data[Y], m_data[Z], lastComponent};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Copies the vector data inside a C-Array of precision_t.
			 * @warning The target must provide enough space.
			 * @param target Where to copy vector data.
			 */
			void
			copy (precision_t * target) const noexcept
			{
				#pragma omp simd
				for ( size_t index = 0; index < dim_t; index++ )
				{
					target[index] = m_data[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic): We use raw data here.
				}
			}

			/**
			 * @brief Vector constant at origin.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			origin () noexcept
			{
				if constexpr ( dim_t == 4 )
				{
					return {0, 0, 0, 1};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Vector constant pointing toward positive X axis.
			 * @note In world space, this point to the right.
			 * @param distance The distance in the axis. Default 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			positiveX (precision_t distance = 1) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {distance, 0};
				}

				if constexpr ( dim_t == 3 )
				{
					return {distance, 0, 0};
				}

				if constexpr ( dim_t == 4 )
				{
					return {distance, 0, 0, 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Vector constant pointing toward negative X axis.
			 * @note In world space, this point to the left.
			 * @param distance The distance in the axis. Default 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			negativeX (precision_t distance = 1) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {-distance, 0};
				}

				if constexpr ( dim_t == 3 )
				{
					return {-distance, 0, 0};
				}

				if constexpr ( dim_t == 4 )
				{
					return {-distance, 0, 0, 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Vector constant pointing toward positive Y axis.
			 * @note In world space, this point to bottom.
			 * @param distance The distance in the axis. Default 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			positiveY (precision_t distance = 1) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {0, distance};
				}

				if constexpr ( dim_t == 3 )
				{
					return {0, distance, 0};
				}

				if constexpr ( dim_t == 4 )
				{
					return {0, distance, 0, 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Vector constant pointing toward negative Y axis.
			 * @note In world space, this point to top.
			 * @param distance The distance in the axis. Default 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			negativeY (precision_t distance = 1) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {0, -distance};
				}

				if constexpr ( dim_t == 3 )
				{
					return {0, -distance, 0};
				}

				if constexpr ( dim_t == 4 )
				{
					return {0, -distance, 0, 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Vector constant pointing toward positive Z axis.
			 * @note In world space, this point to the user which looks the screen.
			 * @param distance The distance in the axis. Default 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			positiveZ (precision_t distance = 1) noexcept requires (dim_t == 3 || dim_t == 4)
			{
				if constexpr ( dim_t == 3 )
				{
					return {0, 0, distance};
				}

				if constexpr ( dim_t == 4 )
				{
					return {0, 0, distance, 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Vector constant pointing toward negative Z axis.
			 * @note In world space, this point into the screen.
			 * @param distance The distance in the axis. Default 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			constexpr
			Vector
			negativeZ (precision_t distance = 1) noexcept requires (dim_t == 3 || dim_t == 4)
			{
				if constexpr ( dim_t == 3 )
				{
					return {0, 0, -distance};
				}

				if constexpr ( dim_t == 4 )
				{
					return {0, 0, -distance, 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Returns a random vector.
			 * @warning This version use the unreliable old C rand() function. Use EmEn::Libs::Vector::random class instead.
			 * @param min The minimum value.
			 * @param max The maximum value.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			quickRandom (precision_t min, precision_t max) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {Utility::quickRandom(min, max), Utility::quickRandom(min, max)};
				}

				if constexpr ( dim_t == 3 )
				{
					return {Utility::quickRandom(min, max), Utility::quickRandom(min, max), Utility::quickRandom(min, max)};
				}

				if constexpr ( dim_t == 4 )
				{
					return {Utility::quickRandom(min, max), Utility::quickRandom(min, max), Utility::quickRandom(min, max), 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Returns a random vector.
			 * @param min The minimum value.
			 * @param max The maximum value.
			 * @param randomizer A reference to a randomizer.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			random (precision_t min, precision_t max, Randomizer< precision_t > & randomizer) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {randomizer.value(min, max), randomizer.value(min, max)};
				}

				if constexpr ( dim_t == 3 )
				{
					return {randomizer.value(min, max), randomizer.value(min, max), randomizer.value(min, max)};
				}

				if constexpr ( dim_t == 4 )
				{
					return {randomizer.value(min, max), randomizer.value(min, max), randomizer.value(min, max), 0};
				}
				else
				{
					return {};
				}
			}

			/**
			 * @brief Performs a linear interpolation between two vectors (LERP).
			 * @param operandA A reference to a vector.
			 * @param operandB A reference to a vector.
			 * @param factor The factor between 0 and 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			linearInterpolation (const Vector & operandA, const Vector & operandB, precision_t factor) noexcept
			{
				return (operandA * factor) + (operandB * (1 - factor));
			}

			/**
			 * @brief Performs a quadratic Bezier interpolation between 3 points.
			 * @param operandA The first point.
			 * @param operandB The second point.
			 * @param operandC The third point.
			 * @param factor The factor between 0 and 1.
			 * @return Vector
			 */
			[[nodiscard]]
			static
			Vector
			quadraticBezierInterpolation (const Vector & operandA, const Vector & operandB, const Vector & operandC, precision_t factor) noexcept
			{
				return linearInterpolation(
					linearInterpolation(operandA, operandB, factor),
					linearInterpolation(operandB, operandC, factor),
					factor
				);
			}

			/**
			 * @brief Performs a cubic Bezier interpolation between 4 points.
			 * @param operandA The first point.
			 * @param operandB The second point.
			 * @param operandC The third point.
			 * @param operandD The fourth point.
			 * @param factor The factor between 0 and 1.
			 * @return precision_t
			 */
			[[nodiscard]]
			static
			Vector
			cubicBezierInterpolation (const Vector & operandA, const Vector & operandB, const Vector & operandC, const Vector & operandD, precision_t factor) noexcept
			{
				const auto p01 = linearInterpolation(operandA, operandB, factor);
				const auto p12 = linearInterpolation(operandB, operandC, factor);
				const auto p23 = linearInterpolation(operandC, operandD, factor);
				const auto p012 = linearInterpolation(p01, p12, factor);
				const auto p123 = linearInterpolation(p12, p23, factor);

				return linearInterpolation(p012, p123, factor);
			}

			/**
			 * @brief Projects a vector to another one.
			 * @param onto A reference to a vector.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			project (const Vector & onto) const noexcept requires std::is_floating_point_v< precision_t >
			{
				const precision_t D = onto.lengthSquared();

				if ( Utility::isZero(D) )
				{
					return {};
				}

				return onto * (Vector::dotProduct(*this, onto) / D);
			}

			/**
			 * @brief Rejects a projection.
			 * @param from A reference to a vector.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			reject (const Vector& from) const noexcept requires std::is_floating_point_v< precision_t >
			{
				return *this - this->project(from);
			}

			/**
			 * @brief Check if a vector is parallel.
			 * @param other A reference to another vector.
			 * @param tolerance Tolerance value. default epsilon.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isParallel (const Vector & other, precision_t tolerance = std::numeric_limits< precision_t >::epsilon() * 100) const noexcept requires std::is_floating_point_v< precision_t >
			{
				const precision_t lenSqA = this->lengthSquared();
				const precision_t lenSqB = other.lengthSquared();

				if ( lenSqA <= tolerance || lenSqB <= tolerance )
				{
					return false;
				}

				return std::abs(Vector::dotProduct(*this, other) / std::sqrt(lenSqA * lenSqB)) >= static_cast< precision_t >(1) - tolerance;
			}

			/**
			 * @brief Check if a vector is perpendicular.
			 * @param other A reference to another vector.
			 * @param tolerance Tolerance value. default epsilon.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPerpendicular (const Vector & other, precision_t tolerance = std::numeric_limits< precision_t >::epsilon() * 100) const noexcept requires std::is_floating_point_v< precision_t >
			{
				const precision_t lenSqA = this->lengthSquared();
				const precision_t lenSqB = other.lengthSquared();

				if ( lenSqA <= tolerance || lenSqB <= tolerance )
				{
					return true;
				}

				return std::abs(Vector::dotProduct(*this, other) / std::sqrt(lenSqA * lenSqB)) <= tolerance;
			}

			/**
			 * @brief Returns the absolute vector.
			 * @return Vector
			 */
			[[nodiscard]]
			Vector
			absolute () const noexcept
			{
				Vector result;

				#pragma omp simd
				for ( size_t i = 0; i < dim_t; ++i )
				{
					result[i] = std::abs(m_data[i]);
				}

				return result;
			}

			/**
			 * @brief Restricts each component of the vector between a minimum and maximum scalar value.
			 * @note If minVal > maxVal, the behavior is defined by std::clamp (the value will be clamped to maxVal).
			 * @param minVal The minimum value allowed for each component.
			 * @param maxVal The maximum value allowed for each component.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			clamp (precision_t minVal, precision_t maxVal) const noexcept
			{
				Vector result;

				#pragma omp simd
				for ( size_t i = 0; i < dim_t; ++i )
				{
					result.m_data[i] = std::clamp(m_data[i], minVal, maxVal);
				}

				return result;
			}

			/**
			 * @brief Restricts each component of the vector between the corresponding min/max vector components.
			 * @note The behavior if minVec[i] > maxVec[i] is defined by std::clamp.
			 * @param minVec The vector containing the minimum bounds for each component.
			 * @param maxVec The vector containing the maximum bounds for each component.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			clamp (const Vector & minVec, const Vector & maxVec) const noexcept
			{
				Vector result;

				#pragma omp simd
				for ( size_t i = 0; i < dim_t; ++i )
				{
					result.m_data[i] = std::clamp(m_data[i], minVec.m_data[i], maxVec.m_data[i]);
				}

				return result;
			}

			/**
			 * @brief Restricts each component of the vector between 0 and 1 (saturation).
			 * @note Utility function equivalent to clamp(0, 1). Very useful for colors, normalized factors, etc. Mainly for floating-point types.
			 * @return Vector
			 */
			[[nodiscard]]
			constexpr
			Vector
			saturate () const noexcept requires std::is_floating_point_v< precision_t >
			{
				return this->clamp(static_cast< precision_t >(0), static_cast< precision_t >(1));
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Vector & obj)
			{
				using namespace std;

				if constexpr ( dim_t == 2 )
				{
					return out << "Vector2(" << fixed << setprecision(8) << '[' << setw(16) << obj.m_data[X] << ", " << setw(16) << obj.m_data[Y] << ')';
				}

				if constexpr ( dim_t == 3 )
				{
					return out << "Vector3(" << fixed << setprecision(8) << '[' << setw(16) << obj.m_data[X] << ", " << setw(16) << obj.m_data[Y] << ", " << setw(16) << obj.m_data[Z] << ')';
				}

				if constexpr ( dim_t == 4 )
				{
					return out << "Vector4(" << fixed << setprecision(8) << '[' << setw(16) << obj.m_data[X] << ", " << setw(16) << obj.m_data[Y] << ", " << setw(16) << obj.m_data[Z] << ", " << setw(16) << obj.m_data[W] << ')';
				}
				else
				{
					return out << "InvalidVectorX()";
				}
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Vector & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			std::array< precision_t, dim_t > m_data{};
	};

	using Vector2I = Vector< 2, signed int >;
	using Vector3I = Vector< 3, signed int >;
	using Vector4I = Vector< 4, signed int >;

	using Vector2IL = Vector< 2, signed long int >;
	using Vector3IL = Vector< 3, signed long int >;
	using Vector4IL = Vector< 4, signed long int >;

	using Vector2LLI = Vector< 2, signed long long int >;
	using Vector3LLI = Vector< 3, signed long long int >;
	using Vector4LLI = Vector< 4, signed long long int >;

	using Vector2UI = Vector< 2, unsigned int >;
	using Vector3UI = Vector< 3, unsigned int >;
	using Vector4UI = Vector< 4, unsigned int >;

	using Vector2ULI = Vector< 2, unsigned long int >;
	using Vector3ULI = Vector< 3, unsigned long int >;
	using Vector4ULI = Vector< 4, unsigned long int >;

	using Vector2ULLI = Vector< 2, unsigned long long int >;
	using Vector3ULLI = Vector< 3, unsigned long long int >;
	using Vector4ULLI = Vector< 4, unsigned long long int >;

	using Vector2F = Vector< 2, float >;
	using Vector3F = Vector< 3, float >;
	using Vector4F = Vector< 4, float >;

	using Vector2D = Vector< 2, double >;
	using Vector3D = Vector< 3, double >;
	using Vector4D = Vector< 4, double >;

	using Vector2LD = Vector< 2, long double >;
	using Vector3LD = Vector< 3, long double >;
	using Vector4LD = Vector< 4, long double >;
}
