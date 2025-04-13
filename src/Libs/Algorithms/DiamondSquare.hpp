/*
 * src/Libs/Algorithms/DiamondSquare.hpp
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
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <vector>

/* Local inclusions for usages. */
#include "Libs/Randomizer.hpp"
#include "Libs/Math/Base.hpp"

namespace EmEn::Libs::Algorithms
{
	/**
	 * @brief Class performing the diamond square algorithm for terrain generation.
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float > requires (std::is_floating_point_v< number_t >)
	class DiamondSquare final
	{
		public:

			/**
			 * @brief Constructs a diamond square processor.
			 * @param useSameValueForCorner Use the same value for corner.
			 */
			explicit
			DiamondSquare (bool useSameValueForCorner) noexcept
				: m_useSameValueForCorner(useSameValueForCorner)
			{

			}

			/**
			 * @brief Constructs a diamond square processor with a seed.
			 * @param seed A seed value.
			 * @param useSameValueForCorner Use the same value for corner.
			 */
			DiamondSquare (int32_t seed, bool useSameValueForCorner) noexcept
				: m_randomizer(seed),
				m_useSameValueForCorner(useSameValueForCorner)
			{

			}

			/**
			 * @brief Returns the generated data.
			 * @return const std::vector< type_t > &
			 */
			[[nodiscard]]
			const std::vector< number_t > &
			data () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns a specific point from the generated data.
			 * @param coordX The coordinate in X.
			 * @param coordY The coordinate in Y.
			 * @return type_t
			 */
			[[nodiscard]]
			number_t
			value (size_t coordX, size_t coordY) const noexcept
			{
				return m_data[this->index(coordX, coordY)];
			}

			/**
			 * @brief Generates the noise data.
			 * @param size The size of pattern.
			 * @param roughness A value from 0 to 1 to controls the roughness.
			 * @return bool
			 */
			bool
			generate (size_t size, number_t roughness) noexcept
			{
				if ( !this->initializeData(size) )
				{
					return false;
				}

				roughness = Math::clampToUnit(roughness);

				this->cornerStep();

				auto currentSize = m_size - 1;

				while ( currentSize > 1 )
				{
					auto halfSize = currentSize / 2;

					/* Diamond step. The centers of each tile. */
					this->diamondStep(currentSize, halfSize, roughness);

					/* Square step. The midpoints of the sides. */
					this->squareStep(currentSize, halfSize, roughness);

					currentSize = halfSize;
				}

				return true;
			}

		private:

			/**
			 * @brief The index of a point from coordinates.
			 * @param coordX The coordinate in X.
			 * @param coordY The coordinate in Y.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			index (size_t coordX, size_t coordY) const noexcept
			{
				return (coordY * m_size) + coordX;
			}

			/**
			 * @brief Initializes the data before noise generation.
			 * @param size The size of pattern.
			 * @return bool
			 */
			bool
			initializeData (size_t size) noexcept
			{
				if ( size < 3 )
				{
					return false;
				}

				if ( size % 2 == 0 )
				{
					size++;
				}

				m_size = size;
				m_data.resize(m_size * m_size);

				return true;
			}

			/**
			 * @brief Performs the corner step.
			 * @return void
			 */
			void
			cornerStep () noexcept
			{
				const auto size = static_cast< number_t >(m_size);

				if ( m_useSameValueForCorner )
				{
					const auto randValue = m_randomizer.value(-size, size);

					m_data[0] = randValue;
					m_data[m_size - 1] = randValue;
					m_data[m_size * (m_size - 1)] = randValue;
					m_data[(m_size * m_size) - 1] = randValue;
				}
				else
				{
					m_data[0] = m_randomizer.value(-size, size);
					m_data[m_size - 1] = m_randomizer.value(-size, size);
					m_data[m_size * (m_size - 1)] = m_randomizer.value(-size, size);
					m_data[(m_size * m_size) - 1] = m_randomizer.value(-size, size);
				}
			}

			/**
			 * @brief Performs the diamond step.
			 * @param size
			 * @param halfSize
			 * @param roughness
			 * @return void
			 */
			void
			diamondStep (size_t size, size_t halfSize, number_t roughness) noexcept
			{
				for ( size_t coordX = halfSize; coordX < m_size; coordX += size )
				{
					for ( size_t coordY = halfSize; coordY < m_size; coordY += size )
					{
						const auto posX = coordX - halfSize;
						const auto negX = coordX + halfSize;
						const auto negY = coordY - halfSize;
						const auto posY = coordY + halfSize;

						auto average = m_data[this->index(negX, negY)];
						average += m_data[this->index(negX, posY)];
						average += m_data[this->index(posX, posY)];
						average += m_data[this->index(posX, negY)];
						average *= 0.25;

						m_data[this->index(coordX, coordY)] = average + m_randomizer.value(
							-roughness * halfSize,
							roughness * halfSize
						);
					}
				}
			}

			/**
			 * @brief Performs the square step.
			 * @param size
			 * @param halfSize
			 * @param roughness
			 * @return void
			 */
			void
			squareStep (size_t size, size_t halfSize, number_t roughness) noexcept
			{
				size_t offset = 0;

				for ( size_t coordX = 0; coordX < m_size; coordX += halfSize )
				{
					if ( offset == 0 )
					{
						offset = halfSize;
					}
					else
					{
						offset = 0;
					}

					for ( size_t coordY = offset; coordY < m_size; coordY += size )
					{
						number_t sum = 0;
						size_t count = 0;

						if ( coordX >= halfSize )
						{
							sum += m_data[index(coordX - halfSize, coordY)];
							count++;
						}

						if ( coordX + halfSize < m_size )
						{
							sum += m_data[index(coordX + halfSize, coordY)];
							count++;
						}

						if ( coordY >= halfSize )
						{
							sum += m_data[index(coordX, coordY - halfSize)];
							count++;
						}

						if ( coordY + halfSize < m_size )
						{
							sum += m_data[index(coordX, coordY + halfSize)];
							count++;
						}

						m_data[this->index(coordX, coordY)] = (sum / count) + m_randomizer.value(
							-roughness * halfSize,
							roughness * halfSize
						);
					}
				}
			}

			size_t m_size{0};
			std::vector< number_t > m_data;
			Randomizer< number_t > m_randomizer;
			bool m_useSameValueForCorner{false};
	};
}
