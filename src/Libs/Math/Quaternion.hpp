/*
 * src/Libs/Math/Quaternion.hpp
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
#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "Vector.hpp"
#include "Matrix.hpp"

namespace EmEn::Libs::Math
{
	/**
	 * @brief Class defining a Quaternion.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Quaternion final
	{
		public:

			/* Imaginary part */
			static constexpr auto X = 0UL;
			static constexpr auto Y = 1UL;
			static constexpr auto Z = 2UL;
			/* Real part */
			static constexpr auto W = 3UL;

			/**
			 * @brief Constructs a default quaternion.
			 */
			constexpr
			Quaternion () noexcept = default;

			/**
			 * @brief Constructs a quaternion from an STL array.
			 * @param data A reference to an array.
			 */
			explicit
			constexpr
			Quaternion (const std::array< precision_t, 4 > & data) noexcept
				: m_data(data)
			{

			}

			/**
			 * @brief Constructs a quaternion from values.
			 * @param x
			 * @param y
			 * @param z
			 * @param w
			 */
			constexpr
			Quaternion (precision_t x, precision_t y, precision_t z, precision_t w) noexcept
				: m_data{x, y, z, w}
			{

			}

			/**
			 * @brief Constructs a quaternion from euler's angle.
			 * @param x Angle on X-axis expressed in radians.
			 * @param y Angle on Y-axis expressed in radians.
			 * @param z Angle on Z-axis expressed in radians.
			 */
			Quaternion (precision_t x, precision_t y, precision_t z)
			{
				this->setFromEulerAngles(x, y, z);
			}

			/**
			 * @brief Constructs a quaternion from euler's angle.
			 * @param vector A reference to a vector of angles X/Y/Z expressed in radians.
			 */
			explicit
			Quaternion (const Vector< 3, precision_t > & vector)
			{
				this->setFromEulerAngles(vector);
			}

			/**
			 * @brief Constructs a quaternion from vector 3.
			 * @param vector A reference to a vector.
			 * @param real
			 */
			explicit
			constexpr
			Quaternion (const Vector< 3, precision_t > & vector, precision_t real) noexcept
				: m_data{vector[X], vector[Y], vector[Z], real}
			{

			}

			/**
			 * @brief Constructs a quaternion from vector 4.
			 * @param vector A reference to a vector.
			 */
			explicit
			constexpr
			Quaternion (const Vector< 4, precision_t > & vector) noexcept
				: m_data{vector[X], vector[Y], vector[Z], vector[W]}
			{

			}

			/**
			 * @brief Constructs a quaternion from matrix.
			 * @param matrix A reference to a matrix.
			 */
			explicit
			Quaternion (const Matrix< 4, precision_t > & matrix)
			{
				*this = matrix;
			}

			/**
			 * @brief Copy assignment operator from matrix.
			 * @param matrix The copied matrix.
			 * @return Vector
			 */
			Quaternion &
			operator= (const Matrix< 4, precision_t > & matrix)
			{
				const auto diag = matrix[0] + matrix[5] + matrix[10] + 1;

				if ( diag > static_cast< precision_t >(0) )
				{
					/* Det scale from diagonal. */
					const auto scale = std::sqrt(diag) * static_cast< precision_t >(2.0);

					m_data = {
						(matrix[6] - matrix[9]) / scale,
						(matrix[8] - matrix[2]) / scale,
						(matrix[1] - matrix[4]) / scale,
						static_cast< precision_t >(0.25) * scale
					};
				}
				else
				{
					if ( matrix[0] > matrix[5] && matrix[0] > matrix[10] )
					{
						/* 1st element of diag is the greatest value find scale according to 1st element, and precision_t it. */
						const auto scale = std::sqrt(static_cast< precision_t >(1.0) + matrix[0] - matrix[5] - matrix[10]) * static_cast< precision_t >(2.0);

						m_data = {
							static_cast< precision_t >(0.25) * scale,
							(matrix[4] + matrix[1]) / scale,
							(matrix[2] + matrix[8]) / scale,
							(matrix[6] - matrix[9]) / scale
						};
					}
					else if ( matrix[5] > matrix[10] )
					{
						/* 2nd element of diag is the greatest value find scale according to 2nd element, and precision_t it. */
						const auto scale = std::sqrt(static_cast< precision_t >(1.0) + matrix[5] - matrix[0] - matrix[10]) * static_cast< precision_t >(2.0);

						m_data = {
							(matrix[4] + matrix[1]) / scale,
							static_cast< precision_t >(0.25) * scale,
							(matrix[9] + matrix[6]) / scale,
							(matrix[8] - matrix[2]) / scale
						};
					}
					else
					{
						/* 3rd element of diag is the greatest value find scale according to 3rd element, and precision_t it. */
						const auto scale = std::sqrt(static_cast< precision_t >(1.0) + matrix[10] - matrix[0] - matrix[5]) * static_cast< precision_t >(2.0);

						m_data = {
							(matrix[8] + matrix[2]) / scale,
							(matrix[9] + matrix[6]) / scale,
							static_cast< precision_t >(0.25) * scale,
							(matrix[1] - matrix[4]) / scale
						};
					}
				}

				this->normalize();

				return *this;
			}

			/**
			 * @brief Returns a copy of the quaternion.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator+ () const noexcept
			{
				return *this;
			}

			/**
			 * @brief Returns the negated quaternion.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator- () const
			{
				return {-m_data[X], -m_data[Y], -m_data[Z], -m_data[W]};
			}

			/**
			 * @brief Data accessor.
			 * @param index The index of a vector element.
			 * @return precision_t &
			 */
			precision_t &
			operator[] (size_t index) noexcept
			{
				return m_data[index];
			}

			/**
			 * @brief Data accessor (const version).
			 * @param index The index of a vector element.
			 * @return const precision_t &
			 */
			[[nodiscard]]
			const precision_t &
			operator[] (size_t index) const noexcept
			{
				return m_data[index];
			}

			/**
			 * @brief Performs an equality comparison between quaternion.
			 * @param operand A reference to another quaternion.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const Quaternion & operand) const noexcept
			{
				return Utility::equal(m_data[X], operand.m_data[X]) &&
					   Utility::equal(m_data[Y], operand.m_data[Y]) &&
					   Utility::equal(m_data[Z], operand.m_data[Z]) &&
					   Utility::equal(m_data[W], operand.m_data[W]);
			}

			/**
			 * @brief Performs an inequality comparison between quaternion.
			 * @param operand A reference to another quaternion.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator!= (const Quaternion & operand) const noexcept
			{
				return !this->operator==(operand);
			}

			/**
			 * @brief Returns the addition between two quaternions.
			 * @param operand A reference to another quaternion.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator+ (const Quaternion & operand) const noexcept
			{
				return {
					m_data[X] + operand.m_data[X],
					m_data[Y] + operand.m_data[Y],
					m_data[Z] + operand.m_data[Z],
					m_data[W] + operand.m_data[W]
				};
			}

			/**
			 * @brief Returns the addition with another quaternion.
			 * @param operand A reference to another quaternion.
			 * @return Quaternion &
			 */
			constexpr
			Quaternion &
			operator+= (const Quaternion & operand) noexcept
			{
				if ( this != &operand )
				{
					m_data[X] += operand.m_data[X];
					m_data[Y] += operand.m_data[Y];
					m_data[Z] += operand.m_data[Z];
					m_data[W] += operand.m_data[W];
				}

				return *this;
			}

			/**
			 * @brief Returns the subtraction between two quaternions.
			 * @param operand A reference to another quaternion.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator- (const Quaternion & operand) const noexcept
			{
				return {
					m_data[X] - operand.m_data[X],
					m_data[Y] - operand.m_data[Y],
					m_data[Z] - operand.m_data[Z],
					m_data[W] - operand.m_data[W]
				};
			}

			/**
			 * @brief Returns the subtraction with another quaternion.
			 * @param operand A reference to another quaternion.
			 * @return Quaternion &
			 */
			constexpr
			Quaternion &
			operator-= (const Quaternion & operand) noexcept
			{
				if ( this != &operand )
				{
					m_data[X] -= operand.m_data[X];
					m_data[Y] -= operand.m_data[Y];
					m_data[Z] -= operand.m_data[Z];
					m_data[W] -= operand.m_data[W];
				}

				return *this;
			}

			/**
			 * @brief Returns the product between two quaternions.
			 * @param operand A reference to another quaternion.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator* (const Quaternion & operand) const noexcept
			{
				return Quaternion::product(*this, operand);
			}

			/**
			 * @brief Returns the quaternion component multiplied by a scalar.
			 * @param operand A scalar.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator* (precision_t operand) const noexcept
			{
				return {
					m_data[X] * operand,
					m_data[Y] * operand,
					m_data[Z] * operand,
					m_data[W] * operand
				};
			}

			/**
			 * @brief Multiplication operator with a vector.
			 * @param operand A reference to a vector.
			 * @return Vector< 3, precision_t >
			 */
			[[nodiscard]]
			constexpr
			Vector< 3, precision_t >
			operator* (const Vector< 3, precision_t > & operand) const noexcept
			{
				const Vector< 3, precision_t > qVector{m_data[X], m_data[Y], m_data[Z]};

				auto uv = Vector< 3, precision_t >::crossProduct(qVector, operand);
				auto uuv = Vector< 3, precision_t >::crossProduct(qVector, uv);

				uv *= 2.0 * m_data[W];
				uuv *= 2.0;

				return operand + uv + uuv;
			}

			/**
			 * @brief Multiplication operator with a vector.
			 * @param operand A reference to a vector.
			 * @return Vector< 4, precision_t >
			 */
			constexpr
			Vector< 4, precision_t >
			operator* (const Vector< 4, precision_t > & operand) const noexcept
			{
				Vector< 4, precision_t > qVector{m_data[X], m_data[Y], m_data[Z]};

				auto uv = Vector< 4, precision_t >::crossProduct(qVector, operand);
				auto uuv = Vector< 4, precision_t >::crossProduct(qVector, uv);

				uv *= 2.0 * m_data[W];
				uuv *= 2.0;

				return operand + uv + uuv;
			}

			/**
			 * @brief Returns the product with another quaternion.
			 * @param operand A reference to another quaternion.
			 * @return Quaternion &
			 */
			constexpr
			Quaternion &
			operator*= (const Quaternion & operand) noexcept
			{
				*this = Quaternion::product(*this, operand);

				return *this;
			}

			/**
			 * @brief Returns the quaternion component multiplied by a scalar.
			 * @param operand A scalar.
			 * @return Quaternion &
			 */
			constexpr
			Quaternion &
			operator*= (precision_t operand) noexcept
			{
				m_data[X] *= operand;
				m_data[Y] *= operand;
				m_data[Z] *= operand;
				m_data[W] *= operand;

				return *this;
			}

			/**
			 * @brief Quaternion scalar division operator.
			 * @warning Will return an identity quaternion on division by 0.
			 * @param operand A scalar.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			operator/ (precision_t operand) const noexcept
			{
				if ( Utility::isZero(operand) )
				{
					return {};
				}

				return {
					m_data[X] / operand,
					m_data[Y] / operand,
					m_data[Z] / operand,
					m_data[W] / operand
				};
			}

			/**
			 * @brief Quaternion scalar division operator.
			 * @warning The quaternion will remain unchanged on division by zero.
			 * @param operand A scalar.
			 * @return Quaternion
			 */
			constexpr
			Quaternion &
			operator/= (precision_t operand) noexcept
			{
				if ( !Utility::isZero(operand) )
				{
					m_data[X] /= operand;
					m_data[Y] /= operand;
					m_data[Z] /= operand;
					m_data[W] /= operand;
				}

				return *this;
			}

			/**
			 * @brief Sets the complex part of the quaternion.
			 * @param x
			 * @param y
			 * @param z
			 * @return void
			 */
			void
			setComplex (precision_t x, precision_t y, precision_t z) noexcept
			{
				m_data[X] = x;
				m_data[Y] = y;
				m_data[Z] = z;
			}

			/**
			 * @brief Sets the complex part of the quaternion.
			 * @param vector a reference to vector.
			 * @return void
			 */
			void
			setComplex (const Vector< 3, precision_t > & vector) noexcept
			{
				m_data[X] = vector[X];
				m_data[Y] = vector[Y];
				m_data[Z] = vector[Z];
			}

			/**
			 * @brief Sets the real part of the quaternion.
			 * @param value A scalar.
			 * @return void
			 */
			void
			setReal (precision_t value) noexcept
			{
				m_data[W] = value;
			}

			/**
			 * @brief Sets the data of the quaternion.
			 * @param x X component.
			 * @param y Y component.
			 * @param z Z component.
			 * @param w W component.
			 * @return Quaternion &
			 */
			Quaternion &
			set (precision_t x, precision_t y, precision_t z, precision_t w) noexcept
			{
				m_data[X] = x;
				m_data[Y] = y;
				m_data[Z] = z;
				m_data[W] = w;

				return *this;
			}

			/**
			 * @brief Sets the data of the quaternion.
			 * @param complex a reference to vector.
			 * @param real A scalar.
			 * @return void
			 */
			void
			set (const Vector< 3, precision_t > & complex, precision_t real) noexcept
			{
				m_data[X] = complex[X];
				m_data[Y] = complex[Y];
				m_data[Z] = complex[Z];
				m_data[W] = real;
			}

			/**
			 * @brief Sets the data of the quaternion.
			 * @param vector a reference to vector.
			 * @return void
			 */
			void
			set (const Vector< 4, precision_t > & vector) noexcept
			{
				m_data[X] = vector[X];
				m_data[Y] = vector[Y];
				m_data[Z] = vector[Z];
				m_data[W] = vector[W];
			}

			/**
			 * @brief Sets the quaternion data from euler's angles (ZYX Tait-Bryan).
			 * @param x Angle on X-axis expressed in radians.
			 * @param y Angle on Y-axis expressed in radians.
			 * @param z Angle on Z-axis expressed in radians.
			 * @return void
			 */
			void
			setFromEulerAngles (precision_t x, precision_t y, precision_t z)
			{
				constexpr auto Half{static_cast< precision_t >(0.5)};

				auto angle = x * Half;
				const auto sr = std::sin(angle);
				const auto cr = std::cos(angle);

				angle = y * Half;
				const auto sp = std::sin(angle);
				const auto cp = std::cos(angle);

				angle = z * Half;
				const auto sy = std::sin(angle);
				const auto cy = std::cos(angle);

				const auto cpcy = cp * cy;
				const auto spcy = sp * cy;
				const auto cpsy = cp * sy;
				const auto spsy = sp * sy;

				m_data[X] = sr * cpcy - cr * spsy;
				m_data[Y] = cr * spcy + sr * cpsy;
				m_data[Z] = cr * cpsy - sr * spcy;
				m_data[W] = cr * cpcy + sr * spsy;

				this->normalize();
			}

			/**
			 * @brief Sets the quaternion data from euler's angles (ZYX Tait-Bryan).
			 * @param vector A reference to a vector of angles X/Y/Z expressed in radians.
			 * @return void
			 */
			void
			setFromEulerAngles (const Vector< 3, precision_t > & vector)
			{
				this->setFromEulerAngles(vector[X], vector[Y], vector[Z]);
			}

			[[nodiscard]]
			Vector< 3, precision_t >
			complex () const noexcept
			{
				return {m_data[X], m_data[Y], m_data[Z]};
			}

			[[nodiscard]]
			precision_t
			real () const noexcept
			{
				return m_data[W];
			}

			/**
			 * @brief Returns an equivalent euler angles representation (radians) of this quaternion (ZYX Tait-Bryan).
			 * @return Vector< 3, precision_t >.
			 */
			[[nodiscard]]
			Vector< 3, precision_t >
			eulerAngles () const noexcept
			{
				constexpr auto PiOver2 = std::numbers::pi_v< precision_t > / 2;

				Vector< 3, precision_t > vector;

				// Quick conversion to Euler angles to give tilt to user.
				auto sqw = m_data[3] * m_data[3];
				auto sqx = m_data[0] * m_data[0];
				auto sqy = m_data[1] * m_data[1];
				auto sqz = m_data[2] * m_data[2];

				vector[Y] = std::asin(2.0 * (m_data[3] * m_data[1] - m_data[0] * m_data[2]));

				if ( PiOver2 - std::abs(vector[Y]) > std::numeric_limits< precision_t >::epsilon() )
				{
					vector[Z] = std::atan2(2.0 * (m_data[0] * m_data[1] + m_data[3] * m_data[2]), sqx - sqy - sqz + sqw);
					vector[X] = std::atan2(2.0 * (m_data[3] * m_data[0] + m_data[1] * m_data[2]), sqw - sqx - sqy + sqz);
				}
				else
				{
					// Compute heading from local 'down' vector.
					vector[Z] = std::atan2(2 * m_data[1] * m_data[2] - 2 * m_data[0] * m_data[3], 2 * m_data[0] * m_data[2] + 2 * m_data[1] * m_data[3]);
					vector[X] = 0.0;

					// NOTE: Original code had a check `if (vector[X] < 0)` here, which was unreachable
					// because vector[X] is set to 0.0 above. It has been removed.
				}

				return vector;
			}

			/**
			 * @brief Returns the quaternion as a vector without any conversion.
			 * @return Vector< 4, precision_t >
			 */
			[[nodiscard]]
			constexpr
			Vector< 4, precision_t >
			getAsVector4 () const noexcept
			{
				return {m_data[X], m_data[Y], m_data[Z], m_data[W]};
			}

			/**
			 * @brief Conjugates the quaternion.
			 * @return Quaternion &
			 */
			constexpr
			Quaternion &
			conjugate () noexcept
			{
				m_data[X] = -m_data[X];
				m_data[Y] = -m_data[Y];
				m_data[Z] = -m_data[Z];

				return *this;
			}

			/**
			 * @brief Returns a conjugated quaternion.
			 * @return Quaternion
			 */
			[[nodiscard]]
			constexpr
			Quaternion
			conjugated () const noexcept
			{
				return {-m_data[X], -m_data[Y], -m_data[Z], m_data[W]};
			}

			/**
			 * @brief Returns the length (norm or magnitude) of the quaternion.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			squaredLength () const noexcept
			{
				return
					(m_data[X] * m_data[X]) +
					(m_data[Y] * m_data[Y]) +
					(m_data[Z] * m_data[Z]) +
					(m_data[W] * m_data[W]);
			}

			/**
			 * @brief Returns the length (norm or magnitude) of the quaternion.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			length () const
			{
				return std::sqrt(this->squaredLength());
			}

			/**
			 * @brief Returns the inverse of this quaternion.
			 * @brief The quaternion will remain unchanged on division by zero.
			 * @return Quaternion &
			 */
			Quaternion &
			inverse ()
			{
				const auto normal = this->squaredLength();

				if ( !Utility::isZero(normal) )
				{
					m_data[X] = -m_data[X] / normal;
					m_data[Y] = -m_data[Y] / normal;
					m_data[Z] = -m_data[Z] / normal;
					m_data[W] /= normal;
				}

				return *this;
			}

			/**
			 * @brief Computes the inverse of this quaternion.
			 * @brief The returned quaternion will be identity on division by zero.
			 * @return Quaternion
			 */
			[[nodiscard]]
			Quaternion
			inversed () const
			{
				const auto normal = this->squaredLength();

				if ( Utility::isZero(normal) )
				{
					return {};
				}

				return {
					-m_data[X] / normal,
					-m_data[Y] / normal,
					-m_data[Z] / normal,
					 m_data[W] / normal
				};
			}

			/**
			 * @brief Normalizes the quaternion.
			 * @warning The quaternion will remain unchanged if normal is zero.
			 * @return Quaternion &
			 */
			Quaternion &
			normalize ()
			{
				const auto normal = this->squaredLength();

				if ( !Utility::isZero(normal) && !Utility::isOne(normal) )
				{
					const auto r = reciprocalSquareRoot(normal);

					m_data[X] *= r;
					m_data[Y] *= r;
					m_data[Z] *= r;
					m_data[W] *= r;
				}

				return *this;
			}

			/**
			 * @brief Returns a normalized quaternion.
			 * @warning The returned quaternion will be identity if normal is zero.
			 * @return Quaternion
			 */
			[[nodiscard]]
			Quaternion
			normalized () const
			{
				const auto normal = this->squaredLength();

				if ( Utility::isZero(normal) )
				{
					return {};
				}

				if ( Utility::isOne(normal) )
				{
					return *this;
				}

				const auto r = reciprocalSquareRoot(normal);

				return {
					m_data[X] * r,
					m_data[Y] * r,
					m_data[Z] * r,
					m_data[W] * r
				};
			}

			/**
			 * @brief Computes the dot-product between two quaternions.
			 * @param qA A reference to a quaternion.
			 * @param qB A reference to a quaternion.
			 * @return precision_t
			 */
			static
			constexpr
			precision_t
			dotProduct (const Quaternion & qA, const Quaternion & qB) noexcept
			{
				return
					(qA.m_data[0] * qB.m_data[0]) +
					(qA.m_data[1] * qB.m_data[1]) +
					(qA.m_data[2] * qB.m_data[2]) +
					(qA.m_data[3] * qB.m_data[3]);
			}

			/**
			 * @brief Computes the product between two quaternions.
			 * @return Quaternion
			 */
			[[nodiscard]]
			static
			constexpr
			Quaternion
			product (const Quaternion & qA, const Quaternion & qB) noexcept
			{
				return {
					qA.m_data[Y] * qB.m_data[Z] - qA.m_data[Z] * qB.m_data[Y] + qA.m_data[X] * qB.m_data[W] + qA.m_data[W] * qB.m_data[X],
					qA.m_data[Z] * qB.m_data[X] - qA.m_data[X] * qB.m_data[Z] + qA.m_data[Y] * qB.m_data[W] + qA.m_data[W] * qB.m_data[Y],
					qA.m_data[X] * qB.m_data[Y] - qA.m_data[Y] * qB.m_data[X] + qA.m_data[Z] * qB.m_data[W] + qA.m_data[W] * qB.m_data[Z],
					qA.m_data[W] * qB.m_data[W] - qA.m_data[X] * qB.m_data[X] - qA.m_data[Y] * qB.m_data[Y] - qA.m_data[Z] * qB.m_data[Z]
				};
			}

			/**
			 * @brief Computes a linear interpolation between two quaternions.
			 * @param qA A reference to a quaternion.
			 * @param qB A reference to a quaternion.
			 * @param time The factor of interpolation.
			 * @return Quaternion
			 */
			[[nodiscard]]
			static
			constexpr
			Quaternion
			lerp (const Quaternion & qA, const Quaternion & qB, precision_t time) noexcept
			{
				return (qA * (1.0 - time)) + (qB * time);
			}

			/**
			 * @brief Computes a spherical linear interpolation between two quaternions.
			 * @param qA A reference to a quaternion.
			 * @param qB A reference to a quaternion.
			 * @param time The factor of interpolation.
			 * @return Quaternion
			 */
			[[nodiscard]]
			static
			Quaternion
			slerp (const Quaternion & qA, const Quaternion & qB, precision_t time)
			{
				precision_t cosOmega = Quaternion::dotProduct(qA, qB);

				auto qBCopy = qB;

				// Use the short path
				if (cosOmega < 0.0)
				{
					cosOmega = -cosOmega;
					qBCopy = -qB;
				}

				// If angle is small, use linear interpolation
				if (cosOmega > static_cast<precision_t>(1.0) - std::numeric_limits<precision_t>::epsilon())
				{
					return Quaternion::lerp(qA, qBCopy, time).normalized();
				}

				const auto omega = std::acos(cosOmega);
				const auto sinOmega = std::sin(omega);

				const auto scaleA = std::sin((static_cast<precision_t>(1.0) - time) * omega) / sinOmega;
				const auto scaleB = std::sin(time * omega) / sinOmega;

				return (qA * scaleA) + (qBCopy * scaleB);
			}

			/**
			 * @brief Sets this quaternion to the result of the spherical interpolation between two quaternions
			 * @param qA A reference to a quaternion.
			 * @param qB A reference to a quaternion.
			 * @param time The factor of interpolation.
			 * @param threshold To avoid inaccuracies at the end (time=1) the interpolation switches to linear interpolation at some point.
			 * This value defines how much of the remaining interpolation will be calculated with linearInterpolation.
			 * Everything from 1-threshold up will be linear interpolation.
			 * @return Quaternion
			 */
			[[nodiscard]]
			static
			Quaternion
			slerp (const Quaternion & qA, const Quaternion & qB, precision_t time, precision_t threshold = 0.05)
			{
				auto angle = Quaternion::dotProduct(qA, qB);

				auto qACopy = qA;

				/* NOTE: Make sure we use the short rotation. */
				if ( angle < static_cast< precision_t >(0) )
				{
					qACopy *= -static_cast< precision_t >(1);
					angle *= -static_cast< precision_t >(1);
				}

				/* NOTE: Spherical interpolation. */
				if ( angle <= static_cast< precision_t >(1) - threshold )
				{
					const auto theta = std::acos(angle);
					const auto invSinTheta = reciprocal(std::sin(theta));
					const auto scale = std::sin(theta * (static_cast< precision_t >(1) - time)) * invSinTheta;
					const auto invScale = std::sin(theta * time) * invSinTheta;

					return (qACopy * scale) + (qB * invScale);
				}

				/* linear interpolation. */
				return Quaternion::lerp(qACopy, qB, time);
			}

			/**
			 * @brief Creates quaternion from rotation angle and rotation axis.
			 * @note Axis must be unit length. The quaternion representing the rotation is "q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)".
			 * @param angle Rotation Angle in radians.
			 * @param axis Rotation axis.
			 * @return Quaternion &
			 */
			Quaternion &
			fromAngleAxis (precision_t angle, const Vector< 3, precision_t > & axis)
			{
				const auto halfAngle = 0.5 * angle;
				const auto sin = std::sin(halfAngle);

				m_data[W] = std::cos(halfAngle);
				m_data[X] = sin * axis[X];
				m_data[Y] = sin * axis[Y];
				m_data[Z] = sin * axis[Z];

				return *this;
			}

			/**
			 * @brief Fills an angle around an axis (unit vector)
			 * @param angle An angle expressed in radians.
			 * @param axis A reference to a vector.
			 * @return void
			 */
			void
			toAngleAxis (precision_t & angle, Vector< 3, precision_t > & axis) const
			{
				const auto sqLen = this->squaredLength();

				if ( Utility::isZero(sqLen) )
				{
					angle = 0.0;
					axis[X] = 0.0;
					axis[Y] = 1.0;
					axis[Z] = 0.0;

					return;
				}

				/* Clamp w to [-1, 1] to avoid numerical errors with acos */
				const auto w_clamped = std::clamp(m_data[W], static_cast<precision_t>(-1.0), static_cast<precision_t>(1.0));

				angle = 2.0 * std::acos(w_clamped);

				/* sin(angle/2) = sqrt(x² + y² + z²) for unit quaternion */
				const auto sinHalfAngle = std::sqrt(m_data[X] * m_data[X] + m_data[Y] * m_data[Y] + m_data[Z] * m_data[Z]);

				if ( Utility::isZero(sinHalfAngle) )
				{
					/* Angle is 0 or 2π, axis is arbitrary */
					axis[X] = 0.0;
					axis[Y] = 1.0;
					axis[Z] = 0.0;
				}
				else
				{
					const auto invSinHalfAngle = reciprocal(sinHalfAngle);
					axis[X] = m_data[X] * invSinHalfAngle;
					axis[Y] = m_data[Y] * invSinHalfAngle;
					axis[Z] = m_data[Z] * invSinHalfAngle;
				}
			}

			/**
			 * @brief Sets quaternion to represent a rotation from one vector to another.
			 * @param from A reference to vector.
			 * @param to A reference to vector.
			 * @return void
			 */
			void
			rotationFromTo (const Vector< 3, precision_t > & from, const Vector< 3, precision_t > & to)
			{
				auto v0 = from.normalized();
				auto v1 = to.normalized();

				const auto d = Vector< 3, precision_t >::dotProduct(v0, v1);

				/* If dot == 1, vectors are the same */
				if ( d >= 1.0 )
				{
					this->reset();

					return;
				}

				/* exactly opposite */
				if ( d <= -1.0 )
				{
					auto axis = Vector< 3, precision_t >::crossProduct({1.0, 0.0, 0.0}, v0);

					if ( axis.length() == 0.0 )
					{
						axis = Vector< 3, precision_t >::crossProduct({0.0, 1.0, 0.0}, v0);
					}

					/* same as fromAngleAxis(core::PI, axis).normalize(); */
					this->set(axis[X], axis[Y], axis[Z], 0.0).normalize();

					return;
				}

				const auto s = std::sqrt((1.0 + d) * 2.0); // optimize inv_sqrt
				const auto invS = 1.0 / s;
				const auto c = Vector< 3, precision_t >::crossProduct(v0, v1) * invS;

				this->set(c[X], c[Y], c[Z], s * 0.5F).normalize();
			}

			/**
			 * @brief Computes the rotation matrix represented by a unit quaternion.
			 * @note This does not check that this quaternion is normalized.
			 * It returns the matrix, which will not be a rotation if the quaternion is non-unit.
			 * @return Matrix< 3, precision_t >
			 */
			constexpr
			Matrix< 3, precision_t >
			rotationMatrix () const noexcept
			{
				std::array< precision_t, 9 > m {
					static_cast< precision_t >(1.0) - static_cast< precision_t >(2.0) * m_data[Y] * m_data[Y] - static_cast< precision_t >(2.0) * m_data[Z] * m_data[Z],
						  static_cast< precision_t >(2.0) * m_data[X] * m_data[Y] - static_cast< precision_t >(2.0) * m_data[Z] * m_data[W],
						  static_cast< precision_t >(2.0) * m_data[X] * m_data[Z] + static_cast< precision_t >(2.0) * m_data[Y] * m_data[W],

						  static_cast< precision_t >(2.0) * m_data[X] * m_data[Y] + static_cast< precision_t >(2.0) * m_data[Z] * m_data[W],
					static_cast< precision_t >(1.0) - static_cast< precision_t >(2.0) * m_data[X] * m_data[X] - static_cast< precision_t >(2.0) * m_data[Z] * m_data[Z],
						  static_cast< precision_t >(2.0) * m_data[Y] * m_data[Z] - static_cast< precision_t >(2.0) * m_data[X] * m_data[W],

						  static_cast< precision_t >(2.0) * m_data[X] * m_data[Z] - static_cast< precision_t >(2.0) * m_data[Y] * m_data[W],
						  static_cast< precision_t >(2.0) * m_data[Y] * m_data[Z] + static_cast< precision_t >(2.0) * m_data[X] * m_data[W],
					static_cast< precision_t >(1.0) - static_cast< precision_t >(2.0) * m_data[X] * m_data[X] - static_cast< precision_t >(2.0) * m_data[Y] * m_data[Y]
				};

				return Matrix< 3, precision_t >{m};
			}

			/**
			 * @brief Sets quaternion to be same as rotation by scaled axis w.
			 * @param scaledAxis A reference to a vector.
			 * @return void
			 */
			void
			setFromScaledAxis (const Vector< 3, precision_t > & scaledAxis)
			{
				auto theta = scaledAxis.length();

				if ( theta > std::numeric_limits< precision_t >::epsilon() )
				{
					auto s = std::sin(theta / 2.0);

					Vector< 3, precision_t > tmp(scaledAxis / theta * s);

					m_data[0] = tmp[X];
					m_data[1] = tmp[Y];
					m_data[2] = tmp[Z];
					m_data[3] = std::cos(theta / 2.0);
				}
				else
				{
					m_data[0] = m_data[1] = m_data[2] = 0;
					m_data[3] = 1.0;
				}
			}

			/**
			 * @brief Returns the scaled-axis representation of this quaternion rotation.
			 * @return Vector< 3, precision_t >
			 */
			[[nodiscard]]
			Vector< 3, precision_t >
			getScaledAxis () const
			{
				precision_t angle;
				Vector< 3, precision_t > scaledAxis;

				this->toAngleAxis(angle, scaledAxis);

				if ( Utility::isZero(angle) )
				{
					return {0.0, 0.0, 0.0};
				}

				return scaledAxis * angle;
			}

			/**
			 * @brief Returns a vector rotated by this quaternion.
			 * @param vector A reference to a vector.
			 * @return Vector< 3, precision_t >
			 */
			constexpr
			Vector< 3, precision_t >
			rotatedVector (const Vector< 3, precision_t > & vector) const noexcept
			{
				return (*this * Quaternion(vector, 0) * this->conjugate()).complex();
			}

			/**
			 * @brief Returns a matrix representation of this quaternion.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			constexpr
			Matrix< 4, precision_t >
			getMatrix () const noexcept
			{
				std::array< precision_t, 16 > matrixData{
					 m_data[W], -m_data[Z],  m_data[Y],  m_data[X],
					 m_data[Z],  m_data[W], -m_data[X],  m_data[Y],
					-m_data[Y],  m_data[X],  m_data[W],  m_data[Z],
					-m_data[X], -m_data[Y], -m_data[Z],  m_data[W]
				};

				return Matrix< 4, precision_t >{matrixData};
			}

			/**
			 * @brief Returns a matrix representation of this quaternion for right multiplication.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			constexpr
			Matrix< 4, precision_t >
			getRightMatrix () const noexcept
			{
				std::array< precision_t, 16 > matrixData{
					 m_data[W], -m_data[Z],  m_data[Y], -m_data[X],
					 m_data[Z],  m_data[W], -m_data[X], -m_data[Y],
					-m_data[Y],  m_data[X],  m_data[W], -m_data[Z],
					 m_data[X],  m_data[Y],  m_data[Z],  m_data[W]
				};

				return Matrix< 4, precision_t >{matrixData};
			}

			/**
			 * @brief Computes the Swing-Twist decomposition of this quaternion.
			 * @param twistAxis A reference to a vector. Must be normalized.
			 * @return std::pair< Quaternion, Quaternion >
			 */
			[[nodiscard]]
			std::pair< Quaternion, Quaternion >
			swingTwistDecomposition (const Vector< 3, precision_t > & twistAxis) const
			{
				const auto sqLen = twistAxis.squaredLength();

				if ( sqLen < std::numeric_limits< precision_t >::epsilon() * std::numeric_limits< precision_t >::epsilon() )
				{
					//throw std::invalid_argument("twistAxis must be non-zero for swing-twist decomposition.");
					return {};
				}

				const auto rotatedAxis = this->rotatedVector(twistAxis);

				Quaternion swing;
				swing.rotationFromTo(twistAxis, rotatedAxis);

				auto twist = swing.conjugated() * (*this);

				const auto twistVectorPart = twist.complex();
				const auto projectedTwistVector = Vector< 3, precision_t >::dotProduct(twistVectorPart, twistAxis) * twistAxis;

				twist.set(projectedTwistVector, twist.real());
				twist.normalize(); 

				return {swing, twist};
			}

			/**
			 * @brief Computes the Swing-Twist decomposition of this quaternion with Z+ axis.
			 * @return std::pair< Quaternion, Quaternion >
			 */
			[[nodiscard]]
			std::pair< Quaternion, Quaternion >
			swingTwistDecomposition () const
			{
				return this->swingTwistDecomposition(Vector< 3, precision_t >::positiveZ());
			}

			/**
			 * @brief Resets the quaternion to identity.
			 * @return Quaternion &
			 */
			constexpr
			Quaternion &
			reset () noexcept
			{
				m_data[X] = 0;
				m_data[Y] = 0;
				m_data[Z] = 0;
				m_data[W] = 1;

				return *this;
			}

			/**
			 * @brief Copies the quaternion data insided a C-Array of precision_t.
			 * @warning The target must provide enough space.
			 * @return void
			 */
			void
			copy (precision_t * target) const noexcept
			{
				target[0] = m_data[X];
				target[1] = m_data[Y];
				target[2] = m_data[Z];
				target[3] = m_data[W];
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Quaternion & obj)
			{
				using namespace std;

				return out << "Quaternion(" << fixed << setprecision(8) << '[' << setw(16) << obj.m_data[0] << ", " << setw(16) << obj.m_data[1] << ", " << setw(16) << obj.m_data[2] << ", " << setw(16) << obj.m_data[3] << ')';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Quaternion & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			std::array< precision_t, 4 > m_data{0.0, 0.0, 0.0, 1.0};
	};
}
