/*
 * src/Libs/Math/LineFormula.hpp
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
#include <cmath>
#include <vector>
#include <type_traits>

/* Local inclusions. */
#include "Base.hpp"
#include "Vector.hpp"

namespace EmEn::Libs::Math
{
	/**
	 * @brief The class for the line formula.
	 * @tparam data_t The precision type.
	 */
	template< typename data_t = float >
	requires (std::is_arithmetic_v< data_t >)
	class LineFormula
	{
		public:

			/**
			 * @brief Constructs the line formula.
			 */
			LineFormula () noexcept = default;

			/**
			 * @brief Constructs the line formula.
			 * @param points A reference to a vector.
			 */
			explicit
			LineFormula (const std::vector< Vector< 2, data_t > > & points) noexcept
			{
				for ( const auto & point : points )
				{
					this->addPoint(point);
				}
			}

			/**
			 * @brief Adds a point to complete the line.
			 * @param x X coordinate of the point.
			 * @param y Y coordinate of the point.
			 * @return void
			 */
			void
			addPoint (data_t x, data_t y) noexcept
			{
				m_xValues.emplace_back(x);
				m_yValues.emplace_back(y);
			}

			/**
			 * @brief Adds a point to complete the line.
			 * @param point A reference to a vector.
			 * @return void
			 */
			void
			addPoint (const Vector< 2, data_t > & point) noexcept
			{
				this->addPoint(point[X], point[Y]);
			}

			/**
			 * @brief Updates every values.
			 * @return bool
			 */
			bool
			compute () noexcept
			{
				if ( m_xValues.empty() )
				{
					return false;
				}

				const auto limit = m_xValues.size();
				const auto xAverage = Math::average(m_xValues);
				const auto yAverage = Math::average(m_yValues);

				data_t sumProdAverageDeviation = 0;
				data_t sumXAverageDeviationSquared = 0;
				data_t sumYAverageDeviationSquared = 0;

				for ( size_t index = 0; index < limit; index++ )
				{
					const auto x = m_xValues[index] - xAverage;
					const auto y = m_yValues[index] - yAverage;

					sumProdAverageDeviation += x * y;
					sumXAverageDeviationSquared += std::pow(x, 2);
					sumYAverageDeviationSquared += std::pow(y, 2);
				}

				m_slope = sumProdAverageDeviation / sumXAverageDeviationSquared;
				m_r2 = (sumProdAverageDeviation * sumProdAverageDeviation) / (sumXAverageDeviationSquared * sumYAverageDeviationSquared);
				m_yIntercept = yAverage - m_slope * xAverage;
				m_robustness = std::abs(m_slope / m_yIntercept);

				return true;
			}

			/**
			 * @brief Clears points.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_xValues.clear();
				m_yValues.clear();
				m_slope = 0;
				m_r2 = 0;
				m_yIntercept = 0;
				m_robustness = 0;
			}

			/**
			 * @brief Returns the slope.
			 * @return data_t
			 */
			data_t
			getSlope () const noexcept
			{
				return m_slope;
			}

			/**
			 * @brief Returns the y intersect. (or origin ordinate)
			 * @return data_t
			 */
			data_t
			getYIntersect () const noexcept
			{
				return m_yIntercept;
			}

			/**
			 * @brief Returns the coefficient of determination (R²).
			 * @return data_t
			 */
			data_t
			getCoefficientDetermination () const noexcept
			{
				return m_r2;
			}

			/**
			 * @brief Returns the robustness.
			 * @return float
			 */
			data_t
			getRobustness () const noexcept
			{
				return m_robustness;
			}

		private:

			std::vector< data_t > m_xValues{};
			std::vector< data_t > m_yValues{};
			data_t m_slope{0};
			data_t m_r2{0};
			data_t m_yIntercept{0};
			data_t m_robustness{0};
	};
}
