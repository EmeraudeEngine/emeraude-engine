/*
 * src/Libs/Math/DeterminantAverage.hpp
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

#pragma once

/* STL inclusions. */
#include <type_traits>
#include <vector>
#include <algorithm>

/* Local inclusions. */
#include "Base.hpp"

namespace EmEn::Libs::Math
{
	template< typename data_t = float >
	requires (std::is_arithmetic_v< data_t >)
	class DeterminantAverage final
	{
		public:

			/**
			 * @brief Constructs the default tolerance computer.
			 */
			DeterminantAverage () noexcept = default;

			/**
			 * @brief Constructs the tolerance computer.
			 * @param values An array of initial values to check.
			 * @param tolerance The tolerance value.
			 */
			DeterminantAverage (const std::vector< data_t > & values, data_t tolerance) noexcept
				: m_values(values), m_tolerance(tolerance)
			{

			}

			/**
			 * @brief Sets the tolerance to use.
			 * @param tolerance The tolerance value.
			 * @return void
			 */
			void
			setTolerance (data_t tolerance) noexcept
			{
				m_tolerance = std::max(0, tolerance);
			}

			/**
			 * @brief Adds a value to the list.
			 * @param value A number to average.
			 * @return void
			 */
			void
			addValue (data_t value) noexcept
			{
				m_values.emplace_back(value);
			}

			/**
			 * @brief Replaces all values.
			 * @param values An array of values to check.
			 * @return void
			 */
			void
			setValues (const std::vector< data_t > & values) noexcept
			{
				this->clear();

				m_values = values;
			}

			/**
			 * @brief Computes the average.
			 * @return bool
			 */
			bool
			compute () noexcept
			{
				if ( m_values.empty() )
				{
					return false;
				}

				const auto limit = m_values.size();

				struct Couple
				{
					size_t indexA;
					size_t indexB;
					bool isValid;
				};

				std::vector< Couple > couples{};
				couples.reserve(limit * 2);

				for ( size_t indexA = 0; indexA < limit; indexA++ )
				{
					const auto valueA = m_values[indexA];

					for ( auto indexB = indexA + 1; indexB < limit; indexB++ )
					{
						const auto valueB = m_values[indexB];

						const auto isValid = Math::difference(valueA, valueB) <= m_tolerance;

						couples.emplace_back(Couple{indexA, indexB, isValid});
					}
				}

				std::vector< unsigned int > tokens(limit, 0);

				for ( const auto & couple : couples )
				{
					if ( !couple.isValid )
					{
						continue;
					}

					++tokens[couple.indexA];
					++tokens[couple.indexB];
				}

				const auto maxTokens = *std::ranges::max_element(tokens);

				if ( maxTokens == 0 )
				{
					return false;
				}

				m_retainedIndexes.resize(limit);

				for ( size_t index = 0; index < limit; index++ )
				{
					m_retainedIndexes[index] = tokens[index] == maxTokens;
				}

				m_retainedValues.reserve(limit);

				for ( size_t index = 0; index < limit; index++ )
				{
					if ( m_retainedIndexes[index] )
					{
						m_retainedValues.emplace_back(m_values[index]);
					}
				}

				if ( m_retainedValues.empty() )
				{
					return false;
				}

				m_average = Math::average(m_retainedValues);

				return true;
			}

			/**
			 * @brief Clears all values except tolerance.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_values.clear();
				m_retainedIndexes.clear();
				m_retainedValues.clear();
				m_average = 0;
			}

			/**
			 * @brief Returns whether one or more values have been removed.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValueRemoved () const noexcept
			{
				return std::ranges::any_of(m_retainedIndexes, [] (auto isRetained) {
					return !isRetained;
				});
			}

			/**
			 * @brief Returns whether all values have been removed.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAllValuesRemoved () const noexcept
			{
				return std::ranges::all_of(m_retainedIndexes, [] (auto isRetained) {
					return !isRetained;
				});
			}

			/**
			 * @brief Returns the number of removed values.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			getRemovedValueCount () const noexcept
			{
				return m_values.size() - m_retainedValues.size();
			}

			/**
			 * @brief Returns the used tolerance in computation.
			 * @return data_t
			 */
			[[nodiscard]]
			data_t
			getTolerance () const noexcept
			{
				return m_tolerance;
			}

			/**
			 * @brief Returns an array with all values involved in the computation.
			 * @return const std::vector< data_t > &
			 */
			[[nodiscard]]
			const std::vector< data_t > &
			getValues () const noexcept
			{
				return m_values;
			}

			/**
			 * @brief Returns an associative array with the keys as an index into array
			 * from DeterminantAverage::getValues() and the values as a boolean
			 * for the retention state.
			 * @return const std::vector< bool > &
			 */
			[[nodiscard]]
			const std::vector< bool > &
			getRetainedIndexes () const noexcept
			{
				return m_retainedIndexes;
			}

			/**
			 * @brief Returns an array with all retained values.
			 * @return const std::vector< data_t > &
			 */
			[[nodiscard]]
			const std::vector< data_t > &
			getRetainedValues () const noexcept
			{
				return m_retainedValues;
			}

			/**
			 * @brief Returns the average of retained values.
			 * @return data_t
			 */
			[[nodiscard]]
			data_t
			getAverage () const noexcept
			{
				return m_average;
			}

		private:

			std::vector< data_t > m_values;
			std::vector< bool > m_retainedIndexes;
			std::vector< data_t > m_retainedValues;
			data_t m_tolerance{0};
			data_t m_average{0};
	};
}
