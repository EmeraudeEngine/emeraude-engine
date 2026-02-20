/*
 * src/Libs/Algorithms/VoronoiNoise.hpp
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
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"

namespace EmEn::Libs::Algorithms
{
	/**
	 * @brief Class performing 3D Voronoi noise evaluation (F1/F2 distances).
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_floating_point_v< number_t >)
	class VoronoiNoise final
	{
		public:

			/**
			 * @brief Result of a Voronoi distance evaluation.
			 */
			struct DistanceResult
			{
				number_t f1;
				number_t f2;
			};

			/**
			 * @brief Constructs a Voronoi noise processor.
			 * @note Initialize with the reference values for the permutation vector.
			 */
			VoronoiNoise () noexcept
				: m_permutations{
					151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
					8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
					35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
					134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
					55,46,245,40,244,102,143,54, 65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
					18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
					250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
					189,28,42,223,183,170,213,119,248,152,2,44,154,163, 70,221,153,101,155,167,
					43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185,112,104,218,246,
					97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,
					107,49,192,214, 31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
					138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
				}
			{
				/* Duplicate the permutation vector. */
				m_permutations.insert(m_permutations.end(), m_permutations.begin(), m_permutations.end());
			}

			/**
			 * @brief Constructs a Voronoi noise processor.
			 * @note Generate a new permutation vector based on the value of seed.
			 * @param seed The seed number.
			 */
			explicit
			VoronoiNoise (uint32_t seed) noexcept
			{
				m_permutations.resize(256);

				/* Fill p with values from 0 to 255. */
				std::iota(m_permutations.begin(), m_permutations.end(), 0);

				/* Initialize a random engine with seed. */
				std::default_random_engine engine(seed);

				/* Shuffle using the above random engine. */
				std::ranges::shuffle(m_permutations, engine);

				/* Duplicate the permutation vector. */
				m_permutations.insert(m_permutations.end(), m_permutations.begin(), m_permutations.end());
			}

			/**
			 * @brief Evaluates Voronoi distances (F1, F2) at a 3D point.
			 * @param x A value for X axis.
			 * @param y A value for Y axis.
			 * @param z A value for Z axis.
			 * @return DistanceResult containing F1 (nearest) and F2 (second nearest) distances.
			 */
			[[nodiscard]]
			DistanceResult
			evaluate (number_t x, number_t y, number_t z) const noexcept
			{
				constexpr uint32_t TableMask{255};
				constexpr auto InvScale{static_cast< number_t >(1.0 / 256.0)};

				/* Find the unit cube that contains the point. */
				const auto floorX = static_cast< int32_t >(std::floor(x));
				const auto floorY = static_cast< int32_t >(std::floor(y));
				const auto floorZ = static_cast< int32_t >(std::floor(z));

				auto f1Sq = std::numeric_limits< number_t >::max();
				auto f2Sq = std::numeric_limits< number_t >::max();

				/* Search the 3x3x3 neighborhood. */
				for ( int32_t di = -1; di <= 1; di++ )
				{
					for ( int32_t dj = -1; dj <= 1; dj++ )
					{
						for ( int32_t dk = -1; dk <= 1; dk++ )
						{
							const auto ci = floorX + di;
							const auto cj = floorY + dj;
							const auto ck = floorZ + dk;

							/* Hash the cell coordinates to get a pseudo-random feature point. */
							const auto hi = static_cast< uint32_t >(ci) & TableMask;
							const auto hj = static_cast< uint32_t >(cj) & TableMask;
							const auto hk = static_cast< uint32_t >(ck) & TableMask;

							const auto hashX = m_permutations[m_permutations[m_permutations[hi] + hj] + hk];
							const auto hashY = m_permutations[m_permutations[m_permutations[hi + 1] + hj + 1] + hk + 1];
							const auto hashZ = m_permutations[m_permutations[m_permutations[hi + 2] + hj + 2] + hk + 2];

							/* Feature point position within the cell. */
							const auto px = static_cast< number_t >(ci) + static_cast< number_t >(hashX) * InvScale;
							const auto py = static_cast< number_t >(cj) + static_cast< number_t >(hashY) * InvScale;
							const auto pz = static_cast< number_t >(ck) + static_cast< number_t >(hashZ) * InvScale;

							/* Squared euclidean distance. */
							const auto dx = x - px;
							const auto dy = y - py;
							const auto dz = z - pz;
							const auto distSq = dx * dx + dy * dy + dz * dz;

							/* Track the two closest distances. */
							if ( distSq < f1Sq )
							{
								f2Sq = f1Sq;
								f1Sq = distSq;
							}
							else if ( distSq < f2Sq )
							{
								f2Sq = distSq;
							}
						}
					}
				}

				return {std::sqrt(f1Sq), std::sqrt(f2Sq)};
			}

			/**
			 * @brief Evaluates the caustic pattern (F2-F1) at a 3D point, clamped to [0,1].
			 * @param x A value for X axis.
			 * @param y A value for Y axis.
			 * @param z A value for Z axis.
			 * @return number_t The caustic value in [0,1].
			 */
			[[nodiscard]]
			number_t
			caustic (number_t x, number_t y, number_t z) const noexcept
			{
				const auto result = this->evaluate(x, y, z);

				return Math::clampToUnit(result.f2 - result.f1);
			}

			/**
			 * @brief Evaluates the caustic pattern (F2-F1) at a 3D point, mapped to [0,255].
			 * @param x A value for X axis.
			 * @param y A value for Y axis.
			 * @param z A value for Z axis.
			 * @return uint8_t The caustic value in [0,255].
			 */
			[[nodiscard]]
			uint8_t
			caustic8bits (number_t x, number_t y, number_t z) const noexcept
			{
				constexpr auto Scale8bit{static_cast< number_t >(255.0)};

				return static_cast< uint8_t >(Scale8bit * this->caustic(x, y, z));
			}

		private:

			std::vector< uint32_t > m_permutations{};
	};
}
