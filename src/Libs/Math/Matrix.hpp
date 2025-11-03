/*
 * src/Libs/Math/Matrix.hpp
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
#include <cassert>
#include <array>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

/* Local inclusions for usages. */
#include "Base.hpp"
#include "Vector.hpp"

namespace EmEn::Libs::Math
{
	/* Matrix 2x2 column major indexes */
	constexpr auto M2x2Col0Row0 = 0;
	constexpr auto M2x2Col0Row1 = 1;
	constexpr auto M2x2Col1Row0 = 2;
	constexpr auto M2x2Col1Row1 = 3;

	/* Matrix 3x3 column major indexes */
	constexpr auto M3x3Col0Row0 = 0;
	constexpr auto M3x3Col0Row1 = 1;
	constexpr auto M3x3Col0Row2 = 2;
	constexpr auto M3x3Col1Row0 = 3;
	constexpr auto M3x3Col1Row1 = 4;
	constexpr auto M3x3Col1Row2 = 5;
	constexpr auto M3x3Col2Row0 = 6;
	constexpr auto M3x3Col2Row1 = 7;
	constexpr auto M3x3Col2Row2 = 8;

	/* Matrix 4x4 column major indexes */
	constexpr auto M4x4Col0Row0 = 0;
	constexpr auto M4x4Col0Row1 = 1;
	constexpr auto M4x4Col0Row2 = 2;
	constexpr auto M4x4Col0Row3 = 3;
	constexpr auto M4x4Col1Row0 = 4;
	constexpr auto M4x4Col1Row1 = 5;
	constexpr auto M4x4Col1Row2 = 6;
	constexpr auto M4x4Col1Row3 = 7;
	constexpr auto M4x4Col2Row0 = 8;
	constexpr auto M4x4Col2Row1 = 9;
	constexpr auto M4x4Col2Row2 = 10;
	constexpr auto M4x4Col2Row3 = 11;
	constexpr auto M4x4Col3Row0 = 12;
	constexpr auto M4x4Col3Row1 = 13;
	constexpr auto M4x4Col3Row2 = 14;
	constexpr auto M4x4Col3Row3 = 15;

	/* Matrix 2x2 row major indexes */
	constexpr auto M2x2Row0Col0 = 0;
	constexpr auto M2x2Row0Col1 = 2;
	constexpr auto M2x2Row1Col0 = 1;
	constexpr auto M2x2Row1Col1 = 3;

	/* Matrix 3x3 row major indexes */
	constexpr auto M3x3Row0Col0 = 0;
	constexpr auto M3x3Row0Col1 = 3;
	constexpr auto M3x3Row0Col2 = 6;
	constexpr auto M3x3Row1Col0 = 1;
	constexpr auto M3x3Row1Col1 = 4;
	constexpr auto M3x3Row1Col2 = 7;
	constexpr auto M3x3Row2Col0 = 2;
	constexpr auto M3x3Row2Col1 = 5;
	constexpr auto M3x3Row2Col2 = 8;

	/* Matrix 4x4 row major indexes */
	constexpr auto M4x4Row0Col0 = 0;
	constexpr auto M4x4Row0Col1 = 4;
	constexpr auto M4x4Row0Col2 = 8;
	constexpr auto M4x4Row0Col3 = 12;
	constexpr auto M4x4Row1Col0 = 1;
	constexpr auto M4x4Row1Col1 = 5;
	constexpr auto M4x4Row1Col2 = 9;
	constexpr auto M4x4Row1Col3 = 13;
	constexpr auto M4x4Row2Col0 = 2;
	constexpr auto M4x4Row2Col1 = 6;
	constexpr auto M4x4Row2Col2 = 10;
	constexpr auto M4x4Row2Col3 = 14;
	constexpr auto M4x4Row3Col0 = 3;
	constexpr auto M4x4Row3Col1 = 7;
	constexpr auto M4x4Row3Col2 = 11;
	constexpr auto M4x4Row3Col3 = 15;

	/**
	 * @brief Column Major matrix.
	 * @tparam dim_t The dimension of the vector. This can be 2, 3 or 4.
	 * @tparam precision_t The data precision should be a floating point number. Default float.
	 * @note
	 *	Matrix 2x2 memory layout (column major)
	 *		[0][2]
	 *		[1][3]
	 *
	 *	Matrix 3x3 memory layout (column major)
	 *		[0][3][6]
	 *		[1][4][7]
	 *		[2][5][8]
	 *
	 *	Matrix 4x4 memory layout (column major)
	 *		[ 0][ 4][ 8][12]
	 *		[ 1][ 5][ 9][13]
	 *		[ 2][ 6][10][14]
	 *		[ 3][ 7][11][15]
	 *
	 *		Column 0 is for X axis vector
	 *		Column 1 is for Y axis vector
	 *		Column 2 is for Z axis vector
	 *		Column 3 is for position point
	 *
	 *		Object Coordinates * Model Matrix -> World CartesianFrame
	 *		World Coordinates * View Matrix -> Eye CartesianFrame
	 *		Eye Coordinates * Projection Matrix -> Clip CartesianFrame
	 *
	 *	Transformation 4x4 Matrix layout
	 *		[Sx+R][   R][   R][  Tx]
	 *		[   R][Sy+R][   R][  Ty]
	 *		[   R][   R][Sz+R][  Tz]
	 *		[   0][   0][   0][   1]
	 */
	template< size_t dim_t, typename precision_t = float >
	requires (dim_t == 2 || dim_t == 3 || dim_t == 4) && std::is_arithmetic_v< precision_t >
	class Matrix final
	{
		public:

			/**
			 * @brief Constructs a default matrix to identity.
			 */
			constexpr
			Matrix () noexcept
			{
				this->reset();
			}

			/**
			 * @brief Constructs a matrix 2x2 with row major entries.
			 * @note This helps the matrix notation.
			 */
			constexpr
			Matrix (
				precision_t r0c0, precision_t r0c1,
				precision_t r1c0, precision_t r1c1
			) noexcept requires (dim_t == 2)
				: m_data({
					r0c0, r1c0,
					r0c1, r1c1
				})
			{

			}

			/**
			 * @brief Constructs a matrix 3x3 with row major entries.
			 * @note This helps the matrix notation.
			 */
			constexpr
			Matrix (
				precision_t r0c0, precision_t r0c1, precision_t r0c2,
				precision_t r1c0, precision_t r1c1, precision_t r1c2,
				precision_t r2c0, precision_t r2c1, precision_t r2c2
			) noexcept requires (dim_t == 3)
				: m_data({
					r0c0, r1c0, r2c0,
					r0c1, r1c1, r2c1,
					r0c2, r1c2, r2c2
				})
			{

			}

			/**
			 * @brief Constructs a matrix 4x4 with row major entries.
			 * @note This helps the matrix notation.
			 */
			constexpr
			Matrix (
				precision_t r0c0, precision_t r0c1, precision_t r0c2, precision_t r0c3,
				precision_t r1c0, precision_t r1c1, precision_t r1c2, precision_t r1c3,
				precision_t r2c0, precision_t r2c1, precision_t r2c2, precision_t r2c3,
				precision_t r3c0, precision_t r3c1, precision_t r3c2, precision_t r3c3
			) noexcept requires (dim_t == 4)
				: m_data({
					r0c0, r1c0, r2c0, r3c0,
					r0c1, r1c1, r2c1, r3c1,
					r0c2, r1c2, r2c2, r3c2,
					r0c3, r1c3, r2c3, r3c3
				})
			{

			}

			/**
			 * @brief Constructs a matrix 2x2 from 2 vectors (2) as columns.
			 * @param column0 A reference to a vector< 2, precision_t >.
			 * @param column1 A reference to a vector< 2, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 2, precision_t > & column0, const Vector< 2, precision_t > & column1) noexcept requires (dim_t == 2)
				: m_data{
					column0[X], column0[Y],
					column1[X], column1[Y]
				}
			{

			}

			/**
			 * @brief Constructs a matrix 2x2 from 2 vectors (3) as columns.
			 * @param column0 A reference to a vector< 3, precision_t >.
			 * @param column1 A reference to a vector< 3, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 3, precision_t > & column0, const Vector< 3, precision_t > & column1) noexcept requires (dim_t == 2)
				: m_data{
					column0[X], column0[Y],
					column1[X], column1[Y]
				}
			{

			}

			/**
			 * @brief Constructs a matrix 2x2 from 2 vectors (4) as columns.
			 * @param column0 A reference to a vector< 4, precision_t >.
			 * @param column1 A reference to a vector< 4, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 4, precision_t > & column0, const Vector< 4, precision_t > & column1) noexcept requires (dim_t == 2)
				: m_data{
					column0[X], column0[Y],
					column1[X], column1[Y]
				}
			{

			}

			/**
			 * @brief Constructs a matrix 3x3 from 3 vectors (3) as columns.
			 * @param column0 A reference to a vector< 3, precision_t >.
			 * @param column1 A reference to a vector< 3, precision_t >.
			 * @param column2 A reference to a vector< 3, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 3, precision_t > & column0, const Vector< 3, precision_t > & column1, const Vector< 3, precision_t > & column2) noexcept requires (dim_t == 3)
				: m_data{
					column0[X], column0[Y], column0[Z],
					column1[X], column1[Y], column1[Z],
					column2[X], column2[Y], column2[Z]
				}
			{

			}

			/**
			 * @brief Constructs a matrix 3x3 from 3 vectors (4) as columns.
			 * @param column0 A reference to a vector< 4, precision_t >.
			 * @param column1 A reference to a vector< 4, precision_t >.
			 * @param column2 A reference to a vector< 4, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 4, precision_t > & column0, const Vector< 4, precision_t > & column1, const Vector< 4, precision_t > & column2) noexcept requires (dim_t == 3)
				: m_data{
					column0[X], column0[Y], column0[Z],
					column1[X], column1[Y], column1[Z],
					column2[X], column2[Y], column2[Z]
				}
			{

			}

			/**
			 * @brief Constructs a matrix 4x4 from 3 vectors (3) as columns.
			 * @param column0 A reference to a vector< 3, precision_t >.
			 * @param column1 A reference to a vector< 3, precision_t >.
			 * @param column2 A reference to a vector< 3, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 3, precision_t > & column0, const Vector< 3, precision_t > & column1, const Vector< 3, precision_t > & column2) noexcept requires (dim_t == 4)
				: m_data{
					column0[X], column0[Y], column0[Z], 0,
					column1[X], column1[Y], column1[Z], 0,
					column2[X], column2[Y], column2[Z], 0,
					0, 0, 0, 1
				}
			{

			}

			/**
			 * @brief Constructs a matrix 4x4 from 3 vectors as columns.
			 * @param column0 A reference to a vector< 4, precision_t >.
			 * @param column1 A reference to a vector< 4, precision_t >.
			 * @param column2 A reference to a vector< 4, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 4, precision_t > & column0, const Vector< 4, precision_t > & column1, const Vector< 4, precision_t > & column2) noexcept requires (dim_t == 4)
				: m_data{
					column0[X], column0[Y], column0[Z], column0[W],
					column1[X], column1[Y], column1[Z], column1[W],
					column2[X], column2[Y], column2[Z], column2[W],
					0, 0, 0, 1
				}
			{

			}

			/**
			 * @brief Constructs a matrix 4x4 from 4 vectors (3) as columns.
			 * @param column0 A reference to a vector< 3, precision_t >.
			 * @param column1 A reference to a vector< 3, precision_t >.
			 * @param column2 A reference to a vector< 3, precision_t >.
			 * @param column3 A reference to a vector< 3, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 3, precision_t > & column0, const Vector< 3, precision_t > & column1, const Vector< 3, precision_t > & column2, const Vector< 3, precision_t > & column3) noexcept requires (dim_t == 4)
				: m_data{
					column0[X], column0[Y], column0[Z], 0,
					column1[X], column1[Y], column1[Z], 0,
					column2[X], column2[Y], column2[Z], 0,
					column3[X], column3[Y], column3[Z], 1
				}
			{

			}

			/**
			 * @brief Constructs a matrix 4x4 from 4 vectors (4) as columns.
			 * @param column0 A reference to a vector< 4, precision_t >.
			 * @param column1 A reference to a vector< 4, precision_t >.
			 * @param column2 A reference to a vector< 4, precision_t >.
			 * @param column3 A reference to a vector< 4, precision_t >.
			 */
			constexpr
			Matrix (const Vector< 4, precision_t > & column0, const Vector< 4, precision_t > & column1, const Vector< 4, precision_t > & column2, const Vector< 4, precision_t > & column3) noexcept requires (dim_t == 4)
				: m_data{
					column0[X], column0[Y], column0[Z], column0[W],
					column1[X], column1[Y], column1[Z], column1[W],
					column2[X], column2[Y], column2[Z], column2[W],
					column3[X], column3[Y], column3[Z], column3[W]
				}
			{

			}

			/**
			 * @brief Constructs a matrix from STL array (Column major).
			 * @param data Array of dim_t * dim_t value.
			 */
			explicit
			constexpr
			Matrix (const std::array< precision_t, dim_t * dim_t > & data) noexcept
				: m_data{data}
			{

			}

			/**
			 * @brief Constructs a matrix from a C-Style array.
			 * @warning Unsafe !
			 * @param data A pointer to a C-Style array containing at least the dimension of the matrix.
			 */
			explicit
			Matrix (std::span< const precision_t, dim_t * dim_t > & data) noexcept
			{
				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					m_data[index] = data[index];
				}
			}

			/**
			 * @brief Returns a copy of the matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			operator+ () const noexcept
			{
				return *this;
			}

			/**
			 * @brief Returns the inverse copy of the matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			operator- () const noexcept
			{
				Matrix matrix;

				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					matrix.m_data[index] = -m_data[index];
				}

				return matrix;
			}

			/**
			 * @brief Data accessor.
			 * @param index The index of a matrix element.
			 * @return precision_t &
			 */
			precision_t &
			operator[] (size_t index) noexcept
			{
				return m_data[index];
			}

			/**
			 * @brief Data accessor (const version).
			 * @param index The index of a matrix element.
			 * @return const precision_t &
			 */
			[[nodiscard]]
			const precision_t &
			operator[] (size_t index) const noexcept
			{
				return m_data[index];
			}

			/**
			 * @brief Data accessor.
			 * @param row The row index.
			 * @param col The column index.
			 * @return precision_t &
			 */
			[[nodiscard]]
			constexpr
			precision_t &
			operator() (size_t row, size_t col) noexcept
			{
				assert(row < dim_t && col < dim_t);

				return m_data[col * dim_t + row];
			}

			/**
			 * @brief Data accessor (const version).
			 * @param row The row index.
			 * @param col The column index.
			 * @return const precision_t &
			 */
			[[nodiscard]]
			constexpr
			const precision_t &
			operator() (size_t row, size_t col) const noexcept
			{
				assert(row < dim_t && col < dim_t);

				return m_data[col * dim_t + row];
			}

			/**
			 * @brief Overloads the equality operator.
			 * @note Floating point number version.
			 * @param operand A reference to another matrix.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const Matrix & operand) const noexcept requires (std::is_floating_point_v< precision_t > )
			{
				if ( this != &operand )
				{
					for ( size_t index = 0; index < dim_t * dim_t; index++ )
					{
						if ( Utility::different(m_data[index], operand.m_data[index]) )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Overloads the equality operator.
			 * @note Integral number version.
			 * @param operand A reference to another matrix.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const Matrix & operand) const noexcept requires (std::is_integral_v< precision_t > )
			{
				if ( this != &operand )
				{
					for ( size_t index = 0; index < dim_t * dim_t; index++ )
					{
						if ( m_data[index] != operand.m_data[index] )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Overloads the inequality operator.
			 * @param operand A reference to another matrix.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator!= (const Matrix & operand) const noexcept
			{
				return !this->operator==(operand);
			}

			/**
			 * @brief Overloads the addition operator.
			 * @param operand A reference to another matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			operator+ (const Matrix & operand) const noexcept
			{
				Matrix matrix;

				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					matrix[index] = m_data[index] + operand.m_data[index];
				}

				return matrix;
			}

			/**
			 * @brief Overloads the addition and assign operator.
			 * @param operand A reference to another matrix.
			 * @return Matrix
			 */
			Matrix &
			operator+= (const Matrix & operand) noexcept
			{
				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					m_data[index] += operand.m_data[index];
				}

				return *this;
			}

			/**
			 * @brief Overloads the subtraction operator.
			 * @param operand A reference to another matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			operator- (const Matrix & operand) const noexcept
			{
				Matrix matrix;

				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					matrix[index] = m_data[index] - operand.m_data[index];
				}

				return matrix;
			}

			/**
			 * @brief Overloads the subtraction and assign operator.
			 * @param operand A reference to another matrix.
			 * @return Matrix
			 */
			Matrix &
			operator-= (const Matrix & operand) noexcept
			{
				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					m_data[index] -= operand.m_data[index];
				}

				return *this;
			}

			/**
			 * @brief Overloads the multiplication operator.
			 * @param operand A reference to another matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			operator* (const Matrix & operand) const noexcept
			{
				Matrix matrix;

				for ( size_t columnIndex = 0; columnIndex < dim_t; columnIndex++ )
				{
					for ( size_t rowIndex = 0; rowIndex < dim_t; rowIndex++ )
					{
						const size_t index = (columnIndex * dim_t) + rowIndex;

						matrix[index] = 0;

						for ( size_t i = 0; i < dim_t; i++ )
						{
							matrix[index] += (m_data[(i * dim_t) + rowIndex] * operand.m_data[(columnIndex * dim_t) + i]);
						}
					}
				}

				return matrix;
			}

			/**
			 * @brief Overloads the multiply and assign operator.
			 * @param operand A reference to another matrix.
			 * @return Matrix
			 */
			Matrix &
			operator*= (const Matrix & operand) noexcept
			{
				std::array< precision_t, dim_t * dim_t > data;

				for ( size_t col = 0; col < dim_t; ++col )
				{
					for ( size_t row = 0; row < dim_t; ++row )
					{
						precision_t sum = 0;

						for ( size_t i = 0; i < dim_t; ++i )
						{
							sum += m_data[i * dim_t + row] * operand.m_data[col * dim_t + i];
						}

						data[col * dim_t + row] = sum;
					}
				}

				m_data = data;

				return *this;
			}

			/**
			 * @brief Overloads the multiplication operator.
			 * @param operand A value.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			operator* (precision_t operand) const noexcept
			{
				Matrix matrix;

				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					matrix.m_data[index] = m_data[index] * operand;
				}

				return matrix;
			}

			/**
			 * @brief Overloads the multiply and assign operator.
			 * @param operand A value.
			 * @return Matrix
			 */
			Matrix &
			operator*= (precision_t operand) noexcept
			{
				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					m_data[index] *= operand;
				}

				return *this;
			}

			/**
			 * @brief Overloads the multiplication operator.
			 * @param operand A reference to a vector.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			operator* (const Vector< dim_t, precision_t > & operand) const noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {
						(operand[X] * m_data[M2x2Col0Row0]) + (operand[Y] * m_data[M2x2Col1Row0]),
						(operand[X] * m_data[M2x2Col0Row1]) + (operand[Y] * m_data[M2x2Col1Row1])
					};
				}

				if constexpr ( dim_t == 3 )
				{
					return {
						(operand[X] * m_data[M3x3Col0Row0]) + (operand[Y] * m_data[M3x3Col1Row0]) + (operand[Z] * m_data[M3x3Col2Row0]),
						(operand[X] * m_data[M3x3Col0Row1]) + (operand[Y] * m_data[M3x3Col1Row1]) + (operand[Z] * m_data[M3x3Col2Row1]),
						(operand[X] * m_data[M3x3Col0Row2]) + (operand[Y] * m_data[M3x3Col1Row2]) + (operand[Z] * m_data[M3x3Col2Row2])
					};
				}

				if constexpr ( dim_t == 4 )
				{
					return {
						(operand[X] * m_data[M4x4Col0Row0]) + (operand[Y] * m_data[M4x4Col1Row0]) + (operand[Z] * m_data[M4x4Col2Row0]) + (operand[W] * m_data[M4x4Col3Row0]),
						(operand[X] * m_data[M4x4Col0Row1]) + (operand[Y] * m_data[M4x4Col1Row1]) + (operand[Z] * m_data[M4x4Col2Row1]) + (operand[W] * m_data[M4x4Col3Row1]),
						(operand[X] * m_data[M4x4Col0Row2]) + (operand[Y] * m_data[M4x4Col1Row2]) + (operand[Z] * m_data[M4x4Col2Row2]) + (operand[W] * m_data[M4x4Col3Row2]),
						(operand[X] * m_data[M4x4Col0Row3]) + (operand[Y] * m_data[M4x4Col1Row3]) + (operand[Z] * m_data[M4x4Col2Row3]) + (operand[W] * m_data[M4x4Col3Row3])
					};
				}

				return {};
			}

			/**
			 * @brief Returns whether a matrix is equal.
			 * @param operand A reference to another matrix.
			 * @param epsilon The floating point tolerance value. Default C++ epsilon.
			 * @return equal
			 */
			[[nodiscard]]
			bool
			equal (const Matrix & operand, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) const noexcept requires (std::is_floating_point_v< precision_t >)
			{
				if ( this != &operand )
				{
					for ( size_t index = 0; index < dim_t * dim_t; index++ )
					{
						if ( Utility::different(m_data[index], operand.m_data[index], epsilon) )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Returns whether a matrix is different.
			 * @param operand A reference to another matrix.
			 * @param epsilon The floating point tolerance value. Default C++ epsilon.
			 * @return equal
			 */
			[[nodiscard]]
			bool
			different (const Matrix & operand, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) const noexcept requires (std::is_floating_point_v< precision_t >)
			{
				return !this->equal(operand, epsilon);
			}

			/**
			 * @brief Returns whether two matrices are equal.
			 * @param a A reference to a matrix.
			 * @param b A reference to a matrix.
			 * @param epsilon The floating point tolerance value. Default C++ epsilon.
			 * @return equal
			 */
			[[nodiscard]]
			static
			bool
			equal (const Matrix & a, const Matrix & b, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) noexcept requires (std::is_floating_point_v< precision_t >)
			{
				if ( &a != &b )
				{
					for ( size_t index = 0; index < dim_t * dim_t; index++ )
					{
						if ( Utility::different(a.m_data[index], b.m_data[index], epsilon) )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Returns whether two matrices are different.
			 * @param a A reference to a matrix.
			 * @param b A reference to a matrix.
			 * @param epsilon The floating point tolerance value. Default C++ epsilon.
			 * @return equal
			 */
			[[nodiscard]]
			static
			bool
			different (const Matrix & a, const Matrix & b, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) noexcept requires (std::is_floating_point_v< precision_t >)
			{
				return !Matrix::equal(a, b, epsilon);
			}

			/**
			 * @brief Resets matrix to identity.
			 * @return void
			 */
			constexpr
			void
			reset () noexcept
			{
				for ( size_t columnIndex = 0; columnIndex < dim_t; columnIndex++ )
				{
					for ( size_t rowIndex = 0; rowIndex < dim_t; rowIndex++ )
					{
						m_data[(columnIndex * dim_t) + rowIndex] = columnIndex == rowIndex ? 1 : 0;
					}
				}
			}

			/**
			 * @brief Returns whether the matrix is identity.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isIdentity () const noexcept
			{
				for ( size_t col = 0; col < dim_t; ++col )
				{
					for ( size_t row = 0; row < dim_t; ++row )
					{
						const precision_t expected = row == col ? 1 : 0;

						if constexpr ( std::is_floating_point_v< precision_t > )
						{
							if ( Utility::different((*this)(row, col), expected) )
							{
								return false;
							}
						}
						else
						{
							if ( (*this)(row, col) != expected )
							{
								return false;
							}
						}
					}
				}

				return true;
			}

			/**
			 * @brief Returns the matrix data as a C array.
			 * @return precision_t *
			 */
			[[nodiscard]]
			precision_t *
			data () noexcept
			{
				return m_data.data();
			}

			/**
			 * @brief Return the matrix data as a C array.
			 * @return const precision_t *
			 */
			[[nodiscard]]
			const precision_t *
			data () const noexcept
			{
				return m_data.data();
			}

			/**
			 * @brief Sets values to a matrix row.
			 * @param row The index of the row.
			 * @param x The first value.
			 * @param y The second value.
			 * @return void
			 */
			void
			setRow (size_t row, precision_t x, precision_t y) noexcept requires (dim_t == 2)
			{
				assert(row < dim_t);

				m_data[row] = x;
				m_data[row + dim_t] = y;
			}

			/**
			 * @brief Sets values to a matrix row.
			 * @param row The index of the row.
			 * @param x The first value.
			 * @param y The second value.
			 * @param z The third value.
			 * @return void
			 */
			void
			setRow (size_t row, precision_t x, precision_t y, precision_t z) noexcept requires (dim_t == 3)
			{
				assert(row < dim_t);

				m_data[row] = x;
				m_data[row + dim_t] = y;
				m_data[row + (2 * dim_t)] = z;
			}

			/**
			 * @brief Sets values to a matrix row.
			 * @param row The index of the row.
			 * @param x The first value.
			 * @param y The second value.
			 * @param z The third value.
			 * @param w The fourth value.
			 * @return void
			 */
			void
			setRow (size_t row, precision_t x, precision_t y, precision_t z, precision_t w) noexcept requires (dim_t == 4)
			{
				assert(row < dim_t);

				m_data[row] = x;
				m_data[row + dim_t] = y;
				m_data[row + (2 * dim_t)] = z;
				m_data[row + (3 * dim_t)] = w;
			}

			/**
			 * @brief Sets values to a matrix row with a vector.
			 * @param row The index of the row.
			 * @param vector A reference to a vector.
			 * @return void
			 */
			void
			setRow (size_t row, const Vector< dim_t, precision_t > & vector) noexcept
			{
				assert(row < dim_t);

				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[(index * dim_t) + row] = vector[index];
				}
			}

			/**
			 * @brief Returns a matrix row as a vector.
			 * @param row The index of the row.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			row (size_t row) const noexcept
			{
				assert(row < dim_t);

				Vector< dim_t, precision_t > vector;

				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector[index] = m_data[(index * dim_t) + row];
				}

				return vector;
			}

			/**
			 * @brief Sets values to a matrix column.
			 * @param col The index of the col.
			 * @param x The first value.
			 * @param y The second value.
			 * @return void
			 */
			void
			setColumn (size_t col, precision_t x, precision_t y) noexcept requires (dim_t == 2)
			{
				assert(col < dim_t);

				const auto shift = dim_t * col;

				m_data[shift] = x;
				m_data[shift + 1] = y;
			}

			/**
			 * @brief Sets values to a matrix column.
			 * @param col The index of the col.
			 * @param x The first value.
			 * @param y The second value.
			 * @param z The third value.
			 * @return void
			 */
			void
			setColumn (size_t col, precision_t x, precision_t y, precision_t z) noexcept requires (dim_t == 3)
			{
				assert(col < dim_t);

				const auto shift = dim_t * col;

				m_data[shift] = x;
				m_data[shift + 1] = y;
				m_data[shift + 2] = z;
			}

			/**
			 * @brief Sets values to a matrix column.
			 * @param col The index of the col.
			 * @param x The first value.
			 * @param y The second value.
			 * @param z The third value.
			 * @param w The fourth value.
			 * @return void
			 */
			void
			setColumn (size_t col, precision_t x, precision_t y, precision_t z, precision_t w) noexcept requires (dim_t == 4)
			{
				assert(col < dim_t);

				const auto shift = dim_t * col;

				m_data[shift] = x;
				m_data[shift + 1] = y;
				m_data[shift + 2] = z;
				m_data[shift + 3] = w;
			}

			/**
			 * @brief Sets a matrix column from a vector.
			 * @param col The index of the column.
			 * @param vector A reference to a vector.
			 * @return void
			 */
			void
			setColumn (size_t col, const Vector< dim_t, precision_t > & vector) noexcept
			{
				assert(col < dim_t);

				const auto shift = dim_t * col;

				for ( size_t index = 0; index < dim_t; index++ )
				{
					m_data[shift + index] = vector[index];
				}
			}

			/**
			 * @brief Returns a column as a vector.
			 * @param col The index of the column.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			column (size_t col) const noexcept
			{
				assert(col < dim_t);

				Vector< dim_t, precision_t > vector;

				const auto shift = dim_t * col;

				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector[index] = m_data[shift + index];
				}

				return vector;
			}

			/**
			 * @brief Returns the X vector (X+) from a matrix.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			rightVector () const noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {m_data[M2x2Col0Row0], m_data[M2x2Col0Row1]};
				}

				if constexpr ( dim_t == 3 )
				{
					return {m_data[M3x3Col0Row0], m_data[M3x3Col0Row1], m_data[M3x3Col0Row2]};
				}

				if constexpr ( dim_t == 4 )
				{
					return {m_data[M4x4Col0Row0], m_data[M4x4Col0Row1], m_data[M4x4Col0Row2]};
				}

				return {};
			}

			/**
			 * @brief Returns the upward vector (Y+) from a matrix.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			upwardVector () const noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return {m_data[M2x2Col1Row0], m_data[M2x2Col1Row1]};
				}

				if constexpr ( dim_t == 3 )
				{
					return {m_data[M3x3Col1Row0], m_data[M3x3Col1Row1], m_data[M3x3Col1Row2]};
				}

				if constexpr ( dim_t == 4 )
				{
					return {m_data[M4x4Col1Row0], m_data[M4x4Col1Row1], m_data[M4x4Col1Row2]};
				}

				return {};
			}

			/**
			 * @brief Returns the forward vector (Z+) from a matrix.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			forwardVector () const noexcept requires (dim_t == 3 || dim_t == 4)
			{
				if constexpr ( dim_t == 3 )
				{
					return {m_data[M3x3Col2Row0], m_data[M3x3Col2Row1], m_data[M3x3Col2Row2]};
				}

				if constexpr ( dim_t == 4 )
				{
					return {m_data[M4x4Col2Row0], m_data[M4x4Col2Row1], m_data[M4x4Col2Row2]};
				}

				return {};
			}

			/**
			 * @brief Returns the position vector.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			position () const noexcept requires (dim_t == 4)
			{
				return {m_data[M4x4Col3Row0], m_data[M4x4Col3Row1], m_data[M4x4Col3Row2]};
			}

			/**
			 * @brief Returns the scale vector.
			 * @return Vector< dim_t, precision_t >
			 */
			[[nodiscard]]
			Vector< dim_t, precision_t >
			scaleVector () const noexcept
			{
				Vector< dim_t, precision_t > vector;

				for ( size_t index = 0; index < dim_t; index++ )
				{
					vector[index] = m_data[index + (index * dim_t)];
				}

				return vector;
			}

			/**
			 * @brief Returns the determinant of the matrix.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			determinant () const noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return (m_data[M2x2Col0Row0] * m_data[M2x2Col1Row1]) - (m_data[M2x2Col1Row0] * m_data[M2x2Col0Row1]);
				}

				if constexpr ( dim_t == 3 || dim_t == 4 )
				{
					precision_t determinantValue = 0;

					for ( size_t index = 0; index < dim_t; index++ )
					{
						const auto value = m_data[index * dim_t];

						if ( Utility::isZero(value) )
						{
							continue;
						}

						/* Creates a sub matrix. */
						Matrix< dim_t - 1, precision_t > subMatrix;

						for ( size_t dstCol = 0; dstCol < dim_t; dstCol++ )
						{
							/* Skip the current column.  */
							if ( index != dstCol )
							{
								const auto columnIndex = dstCol > index ? dstCol - 1 : dstCol;
								const auto columnVector = this->column(dstCol);

								if constexpr ( dim_t == 3 )
								{
									subMatrix.setColumn(columnIndex, {columnVector[Y], columnVector[Z]});
								}

								if constexpr ( dim_t == 4 )
								{
									subMatrix.setColumn(columnIndex, {columnVector[Y], columnVector[Z], columnVector[W]});
								}
							}
						}

						/* Gets determinant from sub matrix. */
						if ( index % 2 == 0 )
						{
							determinantValue += value * subMatrix.determinant();
						}
						else
						{
							determinantValue -= value * subMatrix.determinant();
						}
					}

					return determinantValue;
				}
				else
				{
					return 0;
				}
			}

			/**
			 * @brief Returns the determinant of the matrix. Fast version.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			fastDeterminant () const noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return (m_data[M2x2Col0Row0] * m_data[M2x2Col1Row1]) - (m_data[M2x2Col1Row0] * m_data[M2x2Col0Row1]);
				}

				if constexpr ( dim_t == 3 )
				{
					return
						(m_data[M3x3Col0Row0] * m_data[M3x3Col1Row1] * m_data[M3x3Col2Row2]) +
						(m_data[M3x3Col1Row0] * m_data[M3x3Col2Row1] * m_data[M3x3Col0Row2]) +
						(m_data[M3x3Col2Row0] * m_data[M3x3Col0Row1] * m_data[M3x3Col1Row2]) -
						(m_data[M3x3Col0Row0] * m_data[M3x3Col2Row1] * m_data[M3x3Col1Row2]) -
						(m_data[M3x3Col1Row0] * m_data[M3x3Col0Row1] * m_data[M3x3Col2Row2]) -
						(m_data[M3x3Col2Row0] * m_data[M3x3Col1Row1] * m_data[M3x3Col0Row2]);
				}

				if constexpr ( dim_t == 4 )
				{
					return
						(m_data[M4x4Col3Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col0Row3]) - (m_data[M4x4Col2Row0] * m_data[M4x4Col3Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col0Row3]) -
						(m_data[M4x4Col3Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col0Row3]) + (m_data[M4x4Col1Row0] * m_data[M4x4Col3Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col0Row3]) +
						(m_data[M4x4Col2Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row2] * m_data[M4x4Col0Row3]) - (m_data[M4x4Col1Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row2] * m_data[M4x4Col0Row3]) -
						(m_data[M4x4Col3Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col0Row2] * m_data[M4x4Col1Row3]) + (m_data[M4x4Col2Row0] * m_data[M4x4Col3Row1] * m_data[M4x4Col0Row2] * m_data[M4x4Col1Row3]) +
						(m_data[M4x4Col3Row0] * m_data[M4x4Col0Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col1Row3]) - (m_data[M4x4Col0Row0] * m_data[M4x4Col3Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col1Row3]) -
						(m_data[M4x4Col2Row0] * m_data[M4x4Col0Row1] * m_data[M4x4Col3Row2] * m_data[M4x4Col1Row3]) + (m_data[M4x4Col0Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row2] * m_data[M4x4Col1Row3]) +
						(m_data[M4x4Col3Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col0Row2] * m_data[M4x4Col2Row3]) - (m_data[M4x4Col1Row0] * m_data[M4x4Col3Row1] * m_data[M4x4Col0Row2] * m_data[M4x4Col2Row3]) -
						(m_data[M4x4Col3Row0] * m_data[M4x4Col0Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row3]) + (m_data[M4x4Col0Row0] * m_data[M4x4Col3Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row3]) +
						(m_data[M4x4Col1Row0] * m_data[M4x4Col0Row1] * m_data[M4x4Col3Row2] * m_data[M4x4Col2Row3]) - (m_data[M4x4Col0Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row2] * m_data[M4x4Col2Row3]) -
						(m_data[M4x4Col2Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col0Row2] * m_data[M4x4Col3Row3]) + (m_data[M4x4Col1Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col0Row2] * m_data[M4x4Col3Row3]) +
						(m_data[M4x4Col2Row0] * m_data[M4x4Col0Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row3]) - (m_data[M4x4Col0Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row3]) -
						(m_data[M4x4Col1Row0] * m_data[M4x4Col0Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row3]) + (m_data[M4x4Col0Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row3]);
				}
				else
				{
					return 0;
				}
			}

			/**
			 * @brief Returns the trace of the matrix.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			trace () const noexcept
			{
				auto value = 0;

				for ( size_t index = 0; index < dim_t; index++ )
				{
					value += m_data[index + (index * dim_t)];
				}

				return value;
			}

			/**
			 * @brief Transposes the matrix.
			 * @return Matrix &
			 */
			constexpr
			Matrix &
			transpose () noexcept
			{
				if constexpr ( dim_t == 2  )
				{
					std::swap(m_data[M2x2Col0Row1], m_data[M2x2Col1Row0]);
				}

				if constexpr ( dim_t == 3  )
				{
					std::swap(m_data[M3x3Col0Row1], m_data[M3x3Col1Row0]);
					std::swap(m_data[M3x3Col0Row2], m_data[M3x3Col2Row0]);
					std::swap(m_data[M3x3Col1Row2], m_data[M3x3Col2Row1]);
				}

				if constexpr ( dim_t == 4  )
				{
					std::swap(m_data[M4x4Col0Row1], m_data[M4x4Col1Row0]);
					std::swap(m_data[M4x4Col0Row2], m_data[M4x4Col2Row0]);
					std::swap(m_data[M4x4Col1Row2], m_data[M4x4Col2Row1]);
					std::swap(m_data[M4x4Col0Row3], m_data[M4x4Col3Row0]);
					std::swap(m_data[M4x4Col1Row3], m_data[M4x4Col3Row1]);
					std::swap(m_data[M4x4Col2Row3], m_data[M4x4Col3Row2]);
				}

				return *this;
			}

			/**
			 * @brief Returns whether the matrix is invertible.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInvertible () const noexcept
			{
				return !Utility::isZero(this->determinant());
			}

			/**
			 * @brief Returns the inverse copy of the matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			Matrix
			inverseTEST () const noexcept
			{
				const auto D = this->determinant();

				if ( Utility::isZero(D) )
				{
					return *this;
				}

				//const auto invD = static_cast< precision_t >(1) / D;

				Matrix inverseMatrix{};

				for ( size_t rowIndex = 0; rowIndex < dim_t; rowIndex++ )
				{
					for ( size_t columnIndex = 0; columnIndex < dim_t; columnIndex++ )
					{
						const auto index = (columnIndex * dim_t) + rowIndex;

						inverseMatrix[index] = index;

						//BUG
					}
				}

				return inverseMatrix;
			}

			/**
			 * @brief Returns the inverse copy of the matrix.
			 * @return Matrix
			 */
			[[nodiscard]]
			constexpr
			Matrix
			inverse () const noexcept
			{
				const auto D = this->determinant();

				if ( Utility::isZero(D) )
				{
					return *this;
				}

				const auto invD = static_cast< precision_t >(1) / D;

				Matrix inverse;

				if constexpr ( dim_t == 2 )
				{
					inverse.m_data[M2x2Col0Row0] = m_data[M2x2Col1Row1] * invD;
					inverse.m_data[M2x2Col0Row1] = -m_data[M2x2Col1Row0] * invD;

					inverse.m_data[M2x2Col1Row0] = -m_data[M2x2Col0Row1] * invD;
					inverse.m_data[M2x2Col1Row1] = m_data[M2x2Col0Row0] * invD;
				}

				if constexpr ( dim_t == 3 )
				{
					inverse.m_data[M3x3Col0Row0] = (m_data[M3x3Col1Row1] * m_data[M3x3Col2Row2] - m_data[M3x3Col1Row2] * m_data[M3x3Col2Row1]) * invD;
					inverse.m_data[M3x3Col0Row1] = (m_data[M3x3Col0Row2] * m_data[M3x3Col2Row1] - m_data[M3x3Col0Row1] * m_data[M3x3Col2Row2]) * invD;
					inverse.m_data[M3x3Col0Row2] = (m_data[M3x3Col0Row1] * m_data[M3x3Col1Row2] - m_data[M3x3Col0Row2] * m_data[M3x3Col1Row1]) * invD;

					inverse.m_data[M3x3Col1Row0] = (m_data[M3x3Col1Row2] * m_data[M3x3Col2Row0] - m_data[M3x3Col1Row0] * m_data[M3x3Col2Row2]) * invD;
					inverse.m_data[M3x3Col1Row1] = (m_data[M3x3Col0Row0] * m_data[M3x3Col2Row2] - m_data[M3x3Col0Row2] * m_data[M3x3Col2Row0]) * invD;
					inverse.m_data[M3x3Col1Row2] = (m_data[M3x3Col0Row2] * m_data[M3x3Col1Row0] - m_data[M3x3Col0Row0] * m_data[M3x3Col1Row2]) * invD;

					inverse.m_data[M3x3Col2Row0] = (m_data[M3x3Col1Row0] * m_data[M3x3Col2Row1] - m_data[M3x3Col1Row1] * m_data[M3x3Col2Row0]) * invD;
					inverse.m_data[M3x3Col2Row1] = (m_data[M3x3Col0Row1] * m_data[M3x3Col2Row0] - m_data[M3x3Col0Row0] * m_data[M3x3Col2Row1]) * invD;
					inverse.m_data[M3x3Col2Row2] = (m_data[M3x3Col0Row0] * m_data[M3x3Col1Row1] - m_data[M3x3Col0Row1] * m_data[M3x3Col1Row0]) * invD;
				}

				if constexpr ( dim_t == 4 )
				{
					inverse.m_data[M4x4Col0Row0] = (m_data[M4x4Col1Row2] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row1] - m_data[M4x4Col1Row3] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row1] + m_data[M4x4Col1Row3] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row2] - m_data[M4x4Col1Row1] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row2] - m_data[M4x4Col1Row2] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row3] + m_data[M4x4Col1Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col0Row1] = (m_data[M4x4Col0Row3] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row2] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row3] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row2] + m_data[M4x4Col0Row1] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row2] + m_data[M4x4Col0Row2] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row3] - m_data[M4x4Col0Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col0Row2] = (m_data[M4x4Col0Row2] * m_data[M4x4Col1Row3] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row3] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row1] + m_data[M4x4Col0Row3] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row2] - m_data[M4x4Col0Row1] * m_data[M4x4Col1Row3] * m_data[M4x4Col3Row2] - m_data[M4x4Col0Row2] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row3] + m_data[M4x4Col0Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col0Row3] = (m_data[M4x4Col0Row3] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row1] - m_data[M4x4Col0Row2] * m_data[M4x4Col1Row3] * m_data[M4x4Col2Row1] - m_data[M4x4Col0Row3] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row2] + m_data[M4x4Col0Row1] * m_data[M4x4Col1Row3] * m_data[M4x4Col2Row2] + m_data[M4x4Col0Row2] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row3] - m_data[M4x4Col0Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row3]) * invD;

					inverse.m_data[M4x4Col1Row0] = (m_data[M4x4Col1Row3] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row0] - m_data[M4x4Col1Row2] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row0] - m_data[M4x4Col1Row3] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row2] + m_data[M4x4Col1Row0] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row2] + m_data[M4x4Col1Row2] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row3] - m_data[M4x4Col1Row0] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col1Row1] = (m_data[M4x4Col0Row2] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row3] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row0] + m_data[M4x4Col0Row3] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row2] - m_data[M4x4Col0Row0] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row2] - m_data[M4x4Col0Row2] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row3] + m_data[M4x4Col0Row0] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col1Row2] = (m_data[M4x4Col0Row3] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row2] * m_data[M4x4Col1Row3] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row3] * m_data[M4x4Col1Row0] * m_data[M4x4Col3Row2] + m_data[M4x4Col0Row0] * m_data[M4x4Col1Row3] * m_data[M4x4Col3Row2] + m_data[M4x4Col0Row2] * m_data[M4x4Col1Row0] * m_data[M4x4Col3Row3] - m_data[M4x4Col0Row0] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col1Row3] = (m_data[M4x4Col0Row2] * m_data[M4x4Col1Row3] * m_data[M4x4Col2Row0] - m_data[M4x4Col0Row3] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row0] + m_data[M4x4Col0Row3] * m_data[M4x4Col1Row0] * m_data[M4x4Col2Row2] - m_data[M4x4Col0Row0] * m_data[M4x4Col1Row3] * m_data[M4x4Col2Row2] - m_data[M4x4Col0Row2] * m_data[M4x4Col1Row0] * m_data[M4x4Col2Row3] + m_data[M4x4Col0Row0] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row3]) * invD;

					inverse.m_data[M4x4Col2Row0] = (m_data[M4x4Col1Row1] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row0] - m_data[M4x4Col1Row3] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row0] + m_data[M4x4Col1Row3] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row1] - m_data[M4x4Col1Row0] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row1] - m_data[M4x4Col1Row1] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row3] + m_data[M4x4Col1Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col2Row1] = (m_data[M4x4Col0Row3] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row1] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row3] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row1] + m_data[M4x4Col0Row0] * m_data[M4x4Col2Row3] * m_data[M4x4Col3Row1] + m_data[M4x4Col0Row1] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row3] - m_data[M4x4Col0Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col2Row2] = (m_data[M4x4Col0Row1] * m_data[M4x4Col1Row3] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row3] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row0] + m_data[M4x4Col0Row3] * m_data[M4x4Col1Row0] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row0] * m_data[M4x4Col1Row3] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row1] * m_data[M4x4Col1Row0] * m_data[M4x4Col3Row3] + m_data[M4x4Col0Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row3]) * invD;
					inverse.m_data[M4x4Col2Row3] = (m_data[M4x4Col0Row3] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row0] - m_data[M4x4Col0Row1] * m_data[M4x4Col1Row3] * m_data[M4x4Col2Row0] - m_data[M4x4Col0Row3] * m_data[M4x4Col1Row0] * m_data[M4x4Col2Row1] + m_data[M4x4Col0Row0] * m_data[M4x4Col1Row3] * m_data[M4x4Col2Row1] + m_data[M4x4Col0Row1] * m_data[M4x4Col1Row0] * m_data[M4x4Col2Row3] - m_data[M4x4Col0Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row3]) * invD;

					inverse.m_data[M4x4Col3Row0] = (m_data[M4x4Col1Row2] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row0] - m_data[M4x4Col1Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row0] - m_data[M4x4Col1Row2] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row1] + m_data[M4x4Col1Row0] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row1] + m_data[M4x4Col1Row1] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row2] - m_data[M4x4Col1Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row2]) * invD;
					inverse.m_data[M4x4Col3Row1] = (m_data[M4x4Col0Row1] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row2] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row0] + m_data[M4x4Col0Row2] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row0] * m_data[M4x4Col2Row2] * m_data[M4x4Col3Row1] - m_data[M4x4Col0Row1] * m_data[M4x4Col2Row0] * m_data[M4x4Col3Row2] + m_data[M4x4Col0Row0] * m_data[M4x4Col2Row1] * m_data[M4x4Col3Row2]) * invD;
					inverse.m_data[M4x4Col3Row2] = (m_data[M4x4Col0Row2] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row0] - m_data[M4x4Col0Row2] * m_data[M4x4Col1Row0] * m_data[M4x4Col3Row1] + m_data[M4x4Col0Row0] * m_data[M4x4Col1Row2] * m_data[M4x4Col3Row1] + m_data[M4x4Col0Row1] * m_data[M4x4Col1Row0] * m_data[M4x4Col3Row2] - m_data[M4x4Col0Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col3Row2]) * invD;
					inverse.m_data[M4x4Col3Row3] = (m_data[M4x4Col0Row1] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row0] - m_data[M4x4Col0Row2] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row0] + m_data[M4x4Col0Row2] * m_data[M4x4Col1Row0] * m_data[M4x4Col2Row1] - m_data[M4x4Col0Row0] * m_data[M4x4Col1Row2] * m_data[M4x4Col2Row1] - m_data[M4x4Col0Row1] * m_data[M4x4Col1Row0] * m_data[M4x4Col2Row2] + m_data[M4x4Col0Row0] * m_data[M4x4Col1Row1] * m_data[M4x4Col2Row2]) * invD;
				}

				return inverse;
			}

			/**
			 * @brief Clears the rotation component of the matrix.
			 * @return void
			 */
			void
			clearRotation () noexcept
			{
				if constexpr ( dim_t == 4 )
				{
					m_data[M4x4Col0Row0] = 1;
					m_data[M4x4Col1Row0] = 0;
					m_data[M4x4Col2Row0] = 0;

					m_data[M4x4Col0Row1] = 0;
					m_data[M4x4Col1Row1] = 1;
					m_data[M4x4Col2Row1] = 0;

					m_data[M4x4Col0Row2] = 0;
					m_data[M4x4Col1Row2] = 0;
					m_data[M4x4Col2Row2] = 1;
				}
				else
				{
					this->reset();
				}
			}

			/**
			 * @brief Convenient way to get an identity matrix.
			 * @note This is the same as calling the default constructor.
			 * @return Matrix
			 */
			[[nodiscard]]
			static
			constexpr
			Matrix
			identity () noexcept
			{
				return {};
			}

			/**
			 * @brief Clears the translation component of the matrix.
			 * @return void
			 */
			void
			clearTranslation () noexcept requires (dim_t == 4)
			{
				m_data[M4x4Col3Row0] = 0;
				m_data[M4x4Col3Row1] = 0;
				m_data[M4x4Col3Row2] = 0;
			}

			/**
			 * @brief Returns the matrix without the translation transformation.
			 * @return Matrix
			 */
			Matrix
			noTranslation () const noexcept requires (dim_t == 4)
			{
				const std::array< precision_t, dim_t * dim_t > data{
					m_data[M4x4Col0Row0], m_data[M4x4Col1Row0], m_data[M4x4Col2Row0], 0,
					m_data[M4x4Col0Row1], m_data[M4x4Col1Row1], m_data[M4x4Col2Row1], 0,
					m_data[M4x4Col0Row2], m_data[M4x4Col1Row2], m_data[M4x4Col2Row2], 0,
					m_data[M4x4Col0Row3], m_data[M4x4Col1Row3], m_data[M4x4Col2Row3], 1
				};

				return Matrix{data};
			}

			/*enum class Quadrant
			{
				TopLeft = 0,
				BottomLeft = 1,
				TopRight = 2,
				BottomRight = 3
			};

			Matrix< 2, precision_t >
			reduceMatrix (Quadrant quadrant) const noexcept requires dim_t == 3
			{
				Matrix< 2, precision_t > matrix;

				switch ( quadrant )
				{
					case Quadrant::TopLeft :
						matrix[M2x2Col0Row0] = m_data[M3x3Col0Row0];
						matrix[M2x2Col0Row1] = m_data[M3x3Col0Row1];

						matrix[M2x2Col1Row0] = m_data[M3x3Col1Row0];
						matrix[M2x2Col1Row1] = m_data[M3x3Col1Row1];
						break;

					case Quadrant::BottomLeft :
						matrix[M2x2Col0Row0] = m_data[M3x3Col1Row0];
						matrix[M2x2Col0Row1] = m_data[M3x3Col1Row1];

						matrix[M2x2Col1Row0] = m_data[M3x3Col2Row0];
						matrix[M2x2Col1Row1] = m_data[M3x3Col2Row1];
						break;

					case Quadrant::TopRight :
						matrix[M2x2Col0Row0] = m_data[M3x3Col0Row1];
						matrix[M2x2Col0Row1] = m_data[M3x3Col0Row2];

						matrix[M2x2Col1Row0] = m_data[M3x3Col1Row1];
						matrix[M2x2Col1Row1] = m_data[M3x3Col1Row2];
						break;

					case Quadrant::BottomRight :
						matrix[M2x2Col0Row0] = m_data[M3x3Col1Row1];
						matrix[M2x2Col0Row1] = m_data[M3x3Col1Row2];

						matrix[M2x2Col1Row0] = m_data[M3x3Col2Row1];
						matrix[M2x2Col1Row1] = m_data[M3x3Col2Row2];
						break;
				}

				return matrix;
			}

			Matrix< 3, precision_t >
			reduceMatrix (Quadrant quadrant) const noexcept requires dim_t == 4
			{
				Matrix< 3, precision_t > matrix;

				switch ( quadrant )
				{
					case Quadrant::TopLeft :
						matrix[M3x3Col0Row0] = m_data[M4x4Col0Row0];
						matrix[M3x3Col0Row1] = m_data[M4x4Col0Row1];
						matrix[M3x3Col0Row2] = m_data[M4x4Col0Row2];

						matrix[M3x3Col1Row0] = m_data[M4x4Col1Row0];
						matrix[M3x3Col1Row1] = m_data[M4x4Col1Row1];
						matrix[M3x3Col1Row2] = m_data[M4x4Col1Row2];

						matrix[M3x3Col2Row0] = m_data[M4x4Col2Row0];
						matrix[M3x3Col2Row1] = m_data[M4x4Col2Row1];
						matrix[M3x3Col2Row2] = m_data[M4x4Col2Row2];
						break;

					case Quadrant::BottomLeft :
						matrix[M3x3Col0Row0] = m_data[M4x4Col1Row0];
						matrix[M3x3Col0Row1] = m_data[M4x4Col1Row1];
						matrix[M3x3Col0Row2] = m_data[M4x4Col1Row2];

						matrix[M3x3Col1Row0] = m_data[M4x4Col2Row0];
						matrix[M3x3Col1Row1] = m_data[M4x4Col2Row1];
						matrix[M3x3Col1Row2] = m_data[M4x4Col2Row2];

						matrix[M3x3Col2Row0] = m_data[M4x4Col3Row0];
						matrix[M3x3Col2Row1] = m_data[M4x4Col3Row1];
						matrix[M3x3Col2Row2] = m_data[M4x4Col3Row2];
						break;

					case Quadrant::TopRight :
						matrix[M3x3Col0Row0] = m_data[M4x4Col0Row1];
						matrix[M3x3Col0Row1] = m_data[M4x4Col0Row2];
						matrix[M3x3Col0Row2] = m_data[M4x4Col0Row3];

						matrix[M3x3Col1Row0] = m_data[M4x4Col1Row1];
						matrix[M3x3Col1Row1] = m_data[M4x4Col1Row2];
						matrix[M3x3Col1Row2] = m_data[M4x4Col1Row3];

						matrix[M3x3Col2Row0] = m_data[M4x4Col2Row1];
						matrix[M3x3Col2Row1] = m_data[M4x4Col2Row2];
						matrix[M3x3Col2Row2] = m_data[M4x4Col2Row3];
						break;

					case Quadrant::BottomRight :
						matrix[M3x3Col0Row0] = m_data[M4x4Col1Row1];
						matrix[M3x3Col0Row1] = m_data[M4x4Col1Row2];
						matrix[M3x3Col0Row2] = m_data[M4x4Col1Row3];

						matrix[M3x3Col1Row0] = m_data[M4x4Col2Row1];
						matrix[M3x3Col1Row1] = m_data[M4x4Col2Row2];
						matrix[M3x3Col1Row2] = m_data[M4x4Col2Row3];

						matrix[M3x3Col2Row0] = m_data[M4x4Col3Row1];
						matrix[M3x3Col2Row1] = m_data[M4x4Col3Row2];
						matrix[M3x3Col2Row2] = m_data[M4x4Col3Row3];
						break;
				}

				return matrix;
			}*/

			/**
			 * @brief Converts a matrix to a matrix of 2x2 dimensions.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			Matrix< 2, precision_t >
			toMatrix2 () const noexcept requires (dim_t == 3 || dim_t == 4)
			{
				Matrix< 2, precision_t > matrix;

				if constexpr ( dim_t == 3 )
				{
					matrix[M2x2Col0Row0] = m_data[M3x3Col0Row0];
					matrix[M2x2Col0Row1] = m_data[M3x3Col0Row1];

					matrix[M2x2Col1Row0] = m_data[M3x3Col1Row0];
					matrix[M2x2Col1Row1] = m_data[M3x3Col1Row1];
				}

				if constexpr ( dim_t == 4 )
				{
					matrix[M2x2Col0Row0] = m_data[M4x4Col0Row0];
					matrix[M2x2Col0Row1] = m_data[M4x4Col0Row1];

					matrix[M2x2Col1Row0] = m_data[M4x4Col1Row0];
					matrix[M2x2Col1Row1] = m_data[M4x4Col1Row1];
				}

				return matrix;
			}

			/**
			 * @brief Converts a matrix to a matrix of 3x3 dimensions.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			Matrix< 3, precision_t >
			toMatrix3 () const noexcept requires (dim_t == 2 || dim_t == 4)
			{
				Matrix< 3, precision_t > matrix;

				if constexpr ( dim_t == 2 )
				{
					matrix[M3x3Col0Row0] = m_data[M2x2Col0Row0];
					matrix[M3x3Col0Row1] = m_data[M2x2Col0Row1];

					matrix[M3x3Col1Row0] = m_data[M2x2Col1Row0];
					matrix[M3x3Col1Row1] = m_data[M2x2Col1Row1];
				}

				if constexpr ( dim_t == 4 )
				{
					matrix[M3x3Col0Row0] = m_data[M4x4Col0Row0];
					matrix[M3x3Col0Row1] = m_data[M4x4Col0Row1];
					matrix[M3x3Col0Row2] = m_data[M4x4Col0Row2];

					matrix[M3x3Col1Row0] = m_data[M4x4Col1Row0];
					matrix[M3x3Col1Row1] = m_data[M4x4Col1Row1];
					matrix[M3x3Col1Row2] = m_data[M4x4Col1Row2];

					matrix[M3x3Col2Row0] = m_data[M4x4Col2Row0];
					matrix[M3x3Col2Row1] = m_data[M4x4Col2Row1];
					matrix[M3x3Col2Row2] = m_data[M4x4Col2Row2];
				}

				return matrix;
			}

			/**
			 * @brief Converts a matrix to a matrix of 4x4 dimensions.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			Matrix< 4, precision_t >
			toMatrix4 () const noexcept requires (dim_t == 2 || dim_t == 3)
			{
				Matrix< 4, precision_t > matrix;

				if constexpr ( dim_t == 2 )
				{
					matrix[M4x4Col0Row0] = m_data[M2x2Col0Row0];
					matrix[M4x4Col0Row1] = m_data[M2x2Col0Row1];

					matrix[M4x4Col1Row0] = m_data[M2x2Col1Row0];
					matrix[M4x4Col1Row1] = m_data[M2x2Col1Row1];
				}

				if constexpr ( dim_t == 3 )
				{
					matrix[M4x4Col0Row0] = m_data[M3x3Col0Row0];
					matrix[M4x4Col0Row1] = m_data[M3x3Col0Row1];
					matrix[M4x4Col0Row2] = m_data[M3x3Col0Row2];

					matrix[M4x4Col1Row0] = m_data[M3x3Col1Row0];
					matrix[M4x4Col1Row1] = m_data[M3x3Col1Row1];
					matrix[M4x4Col1Row2] = m_data[M3x3Col1Row2];

					matrix[M4x4Col2Row0] = m_data[M3x3Col2Row0];
					matrix[M4x4Col2Row1] = m_data[M3x3Col2Row1];
					matrix[M4x4Col2Row2] = m_data[M3x3Col2Row2];
				}

				return matrix;
			}

			/**
			 * @brief Returns a rotation matrix around X.
			 * @note Only usable in 3D.
			 * Layout 4x4 :
			 *		c0	c1	c2	c3
			 *   r0 [  1 ][  0 ][  0 ][  0 ]
			 *   r1 [  0 ][ cos][-sin][  0 ]
			 *   r2 [  0 ][ sin][ cos][  0 ]
			 *   r3 [  0 ][  0 ][  0 ][  1 ]
			 * @param radian Angle of rotation in radian.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			rotationX (precision_t radian) noexcept requires ((dim_t == 3 || dim_t == 4) && std::floating_point< precision_t >)
			{
				const auto sine = std::sin(radian);
				const auto cosine = std::cos(radian);

				Matrix matrix;

				if constexpr ( dim_t == 3 )
				{
					matrix[M3x3Col1Row1] = cosine;
					matrix[M3x3Col1Row2] = sine;

					matrix[M3x3Col2Row1] = -sine;
					matrix[M3x3Col2Row2] = cosine;
				}

				if constexpr ( dim_t == 4 )
				{
					matrix[M4x4Col1Row1] = cosine;
					matrix[M4x4Col1Row2] = sine;

					matrix[M4x4Col2Row1] = -sine;
					matrix[M4x4Col2Row2] = cosine;
				}

				return matrix;
			}

			/**
			 * @brief Returns a rotation matrix around Y.
			 * @note Only usable in 3D.
			 * Layout 4x4:
			 *		c0	c1	c2	c3
			 *   r0 [ cos][  0 ][ sin][  0 ]
			 *   r1 [  0 ][  1 ][  0 ][  0 ]
			 *   r2 [-sin][  0 ][ cos][  0 ]
			 *   r3 [  0 ][  0 ][  0 ][  1 ]
			 * @param radian Angle of rotation in radian.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			rotationY (precision_t radian) noexcept requires ((dim_t == 3 || dim_t == 4) && std::floating_point< precision_t >)
			{
				const auto sine = std::sin(radian);
				const auto cosine = std::cos(radian);

				Matrix matrix;

				if constexpr ( dim_t == 3 )
				{
					matrix[M3x3Col0Row0] = cosine;
					matrix[M3x3Col0Row2] = -sine;

					matrix[M3x3Col2Row0] = sine;
					matrix[M3x3Col2Row2] = cosine;
				}

				if constexpr ( dim_t == 4 )
				{
					matrix[M4x4Col0Row0] = cosine;
					matrix[M4x4Col0Row2] = -sine;

					matrix[M4x4Col2Row0] = sine;
					matrix[M4x4Col2Row2] = cosine;
				}

				return matrix;
			}

			/**
			 * @brief Returns a rotation matrix around Z.
			 * @note Use this version to perform a simple 2D rotation.
			 * Layout 4x4:
			 *		c0	c1	c2	c3
			 *   r0 [ cos][-sin][  0 ][  0 ]
			 *   r1 [ sin][ cos][  0 ][  0 ]
			 *   r2 [  0 ][  0 ][  1 ][  0 ]
			 *   r3 [  0 ][  0 ][  0 ][  1 ]
			 * @param radian Angle of rotation in radian.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			rotationZ (precision_t radian) noexcept requires ((dim_t == 2 || dim_t == 3 || dim_t == 4) && std::floating_point< precision_t >)
			{
				const auto sine = std::sin(radian);
				const auto cosine = std::cos(radian);

				Matrix matrix;

				if constexpr ( dim_t == 2 )
				{
					/* First column */
					matrix[M2x2Col0Row0] = cosine;
					matrix[M2x2Col0Row1] = sine;

					/* Second column */
					matrix[M2x2Col1Row0] = -sine;
					matrix[M2x2Col1Row1] = cosine;
				}

				if constexpr ( dim_t == 3 )
				{
					/* First column */
					matrix[M3x3Col0Row0] = cosine;
					matrix[M3x3Col0Row1] = sine;

					/* Second column */
					matrix[M3x3Col1Row0] = -sine;
					matrix[M3x3Col1Row1] = cosine;
				}

				if constexpr ( dim_t == 4 )
				{
					/* First column */
					matrix[M4x4Col0Row0] = cosine;
					matrix[M4x4Col0Row1] = sine;

					/* Second column */
					matrix[M4x4Col1Row0] = -sine;
					matrix[M4x4Col1Row1] = cosine;
				}

				return matrix;
			}

			/**
			 * @brief Returns a rotation matrix 3x3|4x4 around a custom axis.
			 * @param radian Angle of rotation in radian.
			 * @param x Around X axis.
			 * @param y Around Y axis.
			 * @param z Around Z axis.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			rotation (precision_t radian, precision_t x, precision_t y, precision_t z) noexcept requires ((dim_t == 3 || dim_t == 4) && std::floating_point< precision_t >)
			{
				Matrix matrix;

				const auto sine = std::sin(radian);
				const auto cosine = std::cos(radian);
				const auto arcCosine = static_cast< precision_t >(1.0) - cosine;

				if constexpr ( dim_t == 3 )
				{
					matrix[M3x3Col0Row0] = x * x * arcCosine + cosine;
					matrix[M3x3Col1Row0] = y * x * arcCosine - z * sine;
					matrix[M3x3Col2Row0] = z * x * arcCosine + y * sine;

					matrix[M3x3Col0Row1] = x * y * arcCosine + z * sine;
					matrix[M3x3Col1Row1] = y * y * arcCosine + cosine;
					matrix[M3x3Col2Row1] = z * y * arcCosine - x * sine;

					matrix[M3x3Col0Row2] = x * z * arcCosine - y * sine;
					matrix[M3x3Col1Row2] = y * z * arcCosine + x * sine;
					matrix[M3x3Col2Row2] = z * z * arcCosine + cosine;
				}

				if constexpr ( dim_t == 4 )
				{
					matrix[M4x4Col0Row0] = x * x * arcCosine + cosine;
					matrix[M4x4Col1Row0] = y * x * arcCosine - z * sine;
					matrix[M4x4Col2Row0] = z * x * arcCosine + y * sine;

					matrix[M4x4Col0Row1] = x * y * arcCosine + z * sine;
					matrix[M4x4Col1Row1] = y * y * arcCosine + cosine;
					matrix[M4x4Col2Row1] = z * y * arcCosine - x * sine;

					matrix[M4x4Col0Row2] = x * z * arcCosine - y * sine;
					matrix[M4x4Col1Row2] = y * z * arcCosine + x * sine;
					matrix[M4x4Col2Row2] = z * z * arcCosine + cosine;
				}

				return matrix;
			}

			/**
			 * @brief Returns a rotation matrix 3x3|4x4 from vector 3|4 around a custom axis.
			 * @param radian Angle of rotation in radian.
			 * @param axis A reference to a vector.
			 * @return Matrix< dim_t, precision_t >
			 */
			template< size_t vec_dim_t >
			requires (vec_dim_t == 3 || vec_dim_t == 4)
			[[nodiscard]]
			static
			Matrix
			rotation (precision_t radian, const Vector< vec_dim_t, precision_t > & axis) noexcept requires ((dim_t == 3 || dim_t == 4) && std::floating_point< precision_t >)
			{
				return Matrix::rotation(radian, axis[X], axis[Y], axis[Z]);
			}

			/**
			 * @brief Returns a rotation matrix 3x3|4x4 built from a coordinate system.
			 * @warning The tree axes must form a valid coordinate system.
			 * @tparam vec_dim_t The vector dimension.
			 * @param xVector A reference to a vector X axis.
			 * @param yVector A reference to a vector Y axis.
			 * @param zVector A reference to a vector Z axis.
			 * @return Matrix
			 */
			template< size_t vec_dim_t >
			requires (vec_dim_t == 3 || vec_dim_t == 4)
			[[nodiscard]]
			static
			Matrix
			rotation (const Vector< vec_dim_t, precision_t > & xVector, const Vector< vec_dim_t, precision_t > & yVector, const Vector< vec_dim_t, precision_t > & zVector) noexcept requires ((dim_t == 3 || dim_t == 4) && std::floating_point< precision_t >)
			{
				return {xVector, yVector, zVector};
			}

			/**
			 * @brief Returns a scaling matrix.
			 * @param scale A scalar value.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			scaling (precision_t scale) noexcept
			{
				if constexpr ( dim_t == 2 )
				{
					return Matrix::scaling(scale, scale);
				}
				else
				{
					return Matrix::scaling(scale, scale, scale);
				}
			}

			/**
			 * @brief Returns a scaling 2x2 matrix.
			 * @param xScale A scalar value for X axis.
			 * @param yScale A scalar value for Y axis.
			 * @return Matrix< 2, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			scaling (precision_t xScale, precision_t yScale) noexcept requires (dim_t == 2)
			{
				Matrix matrix;

				matrix[M2x2Col0Row0] = xScale;
				matrix[M2x2Col1Row1] = yScale;

				return matrix;
			}

			/**
			 * @brief Returns a scaling matrix 3x3|4x4.
			 * @param xScale A scalar value for X axis.
			 * @param yScale A scalar value for Y axis.
			 * @param zScale A scalar value for Z axis.
			 * @return Matrix< dim_t, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			scaling (precision_t xScale, precision_t yScale, precision_t zScale) noexcept requires (dim_t == 3 || dim_t == 4)
			{
				Matrix matrix;

				if constexpr ( dim_t == 3 )
				{
					matrix[M3x3Col0Row0] = xScale;
					matrix[M3x3Col1Row1] = yScale;
					matrix[M3x3Col2Row2] = zScale;
				}

				if constexpr ( dim_t == 4 )
				{
					matrix[M4x4Col0Row0] = xScale;
					matrix[M4x4Col1Row1] = yScale;
					matrix[M4x4Col2Row2] = zScale;
				}

				return matrix;
			}

			/**
			 * @brief Returns a scaling 2x2 matrix from any vector.
			 * @tparam vec_dim_t The vector dimension.
			 * @param scaleVector A reference to a vector.
			 * @return Matrix< 2, precision_t >
			 */
			template< size_t vec_dim_t >
			requires (vec_dim_t == 2 || vec_dim_t == 3 || vec_dim_t == 4)
			[[nodiscard]]
			static
			Matrix
			scaling (const Vector< vec_dim_t, precision_t > & scaleVector) noexcept requires (dim_t == 2)
			{
				return Matrix::scaling(scaleVector[X], scaleVector[Y]);
			}

			/**
			 * @brief Returns a scaling matrix 3x3|4x4 from vector 3|4.
			 * @tparam vec_dim_t The vector dimension.
			 * @param scaleVector A reference to a vector.
			 * @return Matrix< dim_t, precision_t >
			 */
			template< size_t vec_dim_t >
			requires (vec_dim_t == 3 || vec_dim_t == 4)
			[[nodiscard]]
			static
			Matrix
			scaling (const Vector< vec_dim_t, precision_t > & scaleVector) noexcept requires (dim_t == 3 || dim_t == 4)
			{
				return Matrix::scaling(scaleVector[X], scaleVector[Y], scaleVector[Z]);
			}

			/**
			 * @brief Returns a uniform scaling 4x4 matrix.
			 * @param scale A scalar value.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			uniformScaling (precision_t scale) noexcept requires (dim_t == 4)
			{
				Matrix matrix;

				matrix[M4x4Col3Row3] = 1 / scale;

				return matrix;
			}

			/**
			 * @brief Returns a translation 4x4 matrix.
			 * @param xShift Shift distance on X axis.
			 * @param yShift Shift distance on Y axis.
			 * @param zShift Shift distance on Z axis.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			translation (precision_t xShift, precision_t yShift, precision_t zShift) noexcept requires (dim_t == 4)
			{
				Matrix matrix;

				matrix[M4x4Col3Row0] = xShift;
				matrix[M4x4Col3Row1] = yShift;
				matrix[M4x4Col3Row2] = zShift;

				return matrix;
			}

			/**
			 * @brief Returns a translation 4x4 matrix from vector 3|4.
			 * @tparam vec_dim_t The vector dimension.
			 * @param shiftVector A reference to a vector.
			 * @return Matrix< 4, precision_t >
			 */
			template< size_t vec_dim_t >
			requires (vec_dim_t == 3 || vec_dim_t == 4)
			[[nodiscard]]
			static
			Matrix
			translation (const Vector< vec_dim_t, precision_t > & shiftVector) noexcept requires (dim_t == 4)
			{
				return Matrix::translation(shiftVector[X], shiftVector[Y], shiftVector[Z]);
			}

			/**
			 * @brief Returns a frustum 4x4 matrix.
			 * @param xLeft Negative X distance.
			 * @param xRight Positive X distance.
			 * @param yBottom Negative Y distance.
			 * @param yTop Positive Y distance.
			 * @param zNear Negative Z distance.
			 * @param zFar Positive Z distance.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			frustum (precision_t xLeft, precision_t xRight, precision_t yBottom, precision_t yTop, precision_t zNear, precision_t zFar) noexcept requires (dim_t == 4)
			{
				Matrix matrix;

				if ( Utility::different(xRight, xLeft) && Utility::different(yTop, yBottom) && Utility::different(zNear, zFar) && zNear >= 0.0 && zFar >= 0.0 )
				{
					matrix[M4x4Col0Row0] = (2.0 * zNear) / (xRight - xLeft);
					matrix[M4x4Col2Row0] = (xRight + xLeft) / (xRight - xLeft);

					matrix[M4x4Col1Row1] = (2.0 * zNear) / (yTop - yBottom);
					matrix[M4x4Col2Row1] = (yTop + yBottom) / (yTop - yBottom);

					matrix[M4x4Col2Row2] = -(zFar + zNear) / (zFar - zNear);
					matrix[M4x4Col3Row2] = -(2.0 * zFar * zNear) / (zFar - zNear);

					matrix[M4x4Col2Row3] = -1;
					matrix[M4x4Col3Row3] = 0;
				}

				return matrix;
			}

			/**
			 * @brief Returns an orthographic projection 4x4 matrix.
			 * @note Vulkan compliant (right-handed).
			 * @param xLeft Negative X distance.
			 * @param xRight Positive X distance.
			 * @param yBottom Negative Y distance.
			 * @param yTop Positive Y distance.
			 * @param zNear Negative Z distance. Default -1.
			 * @param zFar Positive Z distance. Default 1.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			orthographicProjection (precision_t xLeft, precision_t xRight, precision_t yBottom, precision_t yTop, precision_t zNear = -1, precision_t zFar = 1) noexcept requires (dim_t == 4)
			{
				Matrix matrix;

				if ( Utility::different(xRight, xLeft) && Utility::different(yTop, yBottom) && Utility::different(zFar, zNear) )
				{
					auto deltaValue = xRight - xLeft;

					matrix[M4x4Col0Row0] = 2 / deltaValue;
					matrix[M4x4Col3Row0] = -((xRight + xLeft) / deltaValue);

					deltaValue = yTop - yBottom;

					matrix[M4x4Col1Row1] = 2 / deltaValue;
					matrix[M4x4Col3Row1] = -((yTop + yBottom) / deltaValue);

					deltaValue = zFar - zNear;

					matrix[M4x4Col2Row2] = -2 / deltaValue;
					matrix[M4x4Col3Row2] = -((zFar + zNear) / deltaValue);
				}

				return matrix;
			}

			/**
			 * @brief Returns an orthographic projection 4x4 matrix.
			 * @note Vulkan compliant (right-handed).
			 * @param fov Field of view in degree.
			 * @param aspectRatio The aspect ratio of the viewport.
			 * @param zNear Limit for nearest vertices. Must be positive.
			 * @param zFar Limit for farthest vertices. Must be positive.
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			perspectiveProjection (precision_t fov, precision_t aspectRatio, precision_t zNear = 0, precision_t zFar = 1) noexcept requires (dim_t == 4)
			{
				if ( zNear < 0 )
				{
					zNear = 0;
				}

				if ( zFar < 0 )
				{
					zFar = 1;
				}

				const auto a = fastCotan(fov * static_cast< precision_t >(0.5));

				Matrix matrix;
				matrix[M4x4Col0Row0] = a / aspectRatio;

				matrix[M4x4Col1Row1] = a;

				matrix[M4x4Col2Row2] = -((zFar + zNear) / (zFar - zNear));
				matrix[M4x4Col3Row2] = -((2 * zFar * zNear) / (zFar - zNear));

				matrix[M4x4Col2Row3] = -1;
				matrix[M4x4Col3Row3] = 0;

				return matrix;
			}

			/**
			 * @brief Returns a lookAt view 4x4 matrix.
			 * @param eye A reference to a vector for the "eye" direction.
			 * @param center A reference to a vector for the "eye" position.
			 * @param up A reference to a vector for the upward direction from the "eye".
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			lookAt (const Vector< 3, precision_t > & eye, const Vector< 3, precision_t > & center, const Vector< 3, precision_t > & up) noexcept requires (dim_t == 4)
			{
				const auto f = (center - eye).normalize();
				const auto s = Vector< 3, precision_t >::crossProduct(f, up).normalize();
				const auto u = Vector< 3, precision_t >::crossProduct(s, f);

				Matrix matrix;

				matrix(0, 0) = s[X];
				matrix(0, 1) = s[Y];
				matrix(0, 2) = s[Z];

				matrix(1, 0) = u[X];
				matrix(1, 1) = u[Y];
				matrix(1, 2) = u[Z];

				matrix(2, 0) = -f[X];
				matrix(2, 1) = -f[Y];
				matrix(2, 2) = -f[Z];

				matrix(0, 3) = -Vector<3, precision_t>::dotProduct(s, eye);
				matrix(1, 3) = -Vector<3, precision_t>::dotProduct(u, eye);
				matrix(2, 3) =  Vector<3, precision_t>::dotProduct(f, eye);

				return matrix;
			}

			/**
			 * @brief Returns a lookAt view 4x4 matrix.
			 * @param eye A reference to a vector for the "eye" direction.
			 * @param center A reference to a vector for the "eye" position.
			 * @param up A reference to a vector for the upward direction from the "eye".
			 * @return Matrix< 4, precision_t >
			 */
			[[nodiscard]]
			static
			Matrix
			lookAt (const Vector< 4, precision_t > & eye, const Vector< 4, precision_t > & center, const Vector< 4, precision_t > & up) noexcept requires (dim_t == 4)
			{
				const auto f = (center - eye).normalize();
				const auto s = Vector< 4, precision_t >::crossProduct(f, up).normalize();
				const auto u = Vector< 4, precision_t >::crossProduct(s, f);

				Matrix matrix;

				matrix(0, 0) = s[X];
				matrix(0, 1) = s[Y];
				matrix(0, 2) = s[Z];

				matrix(1, 0) = u[X];
				matrix(1, 1) = u[Y];
				matrix(1, 2) = u[Z];

				matrix(2, 0) = -f[X];
				matrix(2, 1) = -f[Y];
				matrix(2, 2) = -f[Z];

				matrix(0, 3) = -Vector<4, precision_t>::dotProduct(s, eye);
				matrix(1, 3) = -Vector<4, precision_t>::dotProduct(u, eye);
				matrix(2, 3) =  Vector<4, precision_t>::dotProduct(f, eye);

				return matrix;
			}

			/**
			 * @brief Copies the matrix data inside a C-Array of precision_t.
			 * @warning Unsafe !
			 * @param target A pointer to the data.
			 * @warning The target must provide enough space.
			 */
			void
			copy (precision_t * target) const noexcept
			{
				for ( size_t index = 0; index < dim_t * dim_t; index++ )
				{
					target[index] = m_data[index];
				}
			}

			/**
			 * @brief STL streams printable object.
			 * @note This will print the matrix in row-major notation.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Matrix & obj) noexcept
			{
				using namespace std;

				out << "Matrix" << dim_t << 'X' << dim_t << '\n' << fixed << setprecision(8);

				for ( size_t rowIndex = 0; rowIndex < dim_t; rowIndex++ )
				{
					for ( size_t columnIndex = 0; columnIndex < dim_t; columnIndex++ )
					{
						out << '[' << setw(16) << obj.m_data[(columnIndex * dim_t) + rowIndex] << ']';
					}

					out << '\n';
				}

				return out;
			}

			/**
			 * @brief Stringifies the object.
			 * @note This will print the matrix in row-major notation.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Matrix & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			std::array< precision_t, dim_t * dim_t > m_data{0};
	};

	/* NOTE: Trait and concept to check a matrix in the template. */
	template< typename T >
	inline constexpr bool is_matrix_v = false;

	template< size_t dim_t, typename precision_t >
	requires (dim_t == 2 || dim_t == 3 || dim_t == 4) && std::is_arithmetic_v< precision_t >
	inline constexpr bool is_matrix_v< Matrix< dim_t, precision_t > > = true;

	template< typename T >
	concept MatrixConcept = is_matrix_v< T >;

	template< typename T >
	struct MatrixTraits;

	template< size_t dim_t, typename precision_t >
	struct MatrixTraits< Matrix< dim_t, precision_t > >
	{
		static constexpr size_t dim = dim_t;
		using precision = precision_t;
	};

	using Matrix2F = Matrix< 2, float >;
	using Matrix3F = Matrix< 3, float >;
	using Matrix4F = Matrix< 4, float >;

	using Matrix2D = Matrix< 2, double >;
	using Matrix3D = Matrix< 3, double >;
	using Matrix4D = Matrix< 4, double >;

	using Matrix2LD = Matrix< 2, long double >;
	using Matrix3LD = Matrix< 3, long double >;
	using Matrix4LD = Matrix< 4, long double >;
}
