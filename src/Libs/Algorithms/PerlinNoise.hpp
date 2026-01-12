/*
 * src/Libs/Algorithms/PerlinNoise.hpp
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
#include <array>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"

namespace EmEn::Libs::Algorithms
{
	/**
	 * @brief Class performing the perlin noise algorithm for a set of points.
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float >
	requires (std::is_floating_point_v< number_t >)
	class PerlinNoise final
	{
		public:

			/**
			 * @brief Constructs a perlin noise processor.
			 * @note Initialize with the reference values for the permutation vector.
			 */
			PerlinNoise () noexcept
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
			 * @brief Constructs a perlin noise processor.
			 * @note Generate a new permutation vector based on the value of seed.
			 * @param seed The seed number.
			 */
			explicit
			PerlinNoise (uint32_t seed) noexcept
			{
				m_permutations.resize(256);

				/* Fill p with values from 0 to 255. */
				std::iota(m_permutations.begin(), m_permutations.end(), 0);

				/* Initialize a random engine with seed. */
				std::default_random_engine engine(seed);

				/* Shuffle  using the above random engine. */
				std::ranges::shuffle(m_permutations, engine);

				/* Duplicate the permutation vector. */
				m_permutations.insert(m_permutations.end(), m_permutations.begin(), m_permutations.end());
			}

			/**
			 * @brief Generates a noise value.
			 * @param x A value for X axis.
			 * @param y A value for Y axis.
			 * @param z A value for Z axis. Optional for 2D.
			 * @return type_t
			 */
			[[nodiscard]]
			number_t
			generate (number_t x, number_t y, number_t z) noexcept
			{
				constexpr uint32_t TableMask{255};
				constexpr auto One{static_cast< number_t >(1.0)};
				constexpr auto Half{static_cast< number_t >(0.5)};

				/* Find the unit cube that contains the point. */
				const auto X = static_cast< uint32_t >(std::floor(x)) & TableMask;
				const auto Y = static_cast< uint32_t >(std::floor(y)) & TableMask;
				const auto Z = static_cast< uint32_t >(std::floor(z)) & TableMask;

				/* Find relative x, y,z of point in cube. */
				x -= std::floor(x);
				y -= std::floor(y);
				z -= std::floor(z);

				/* Compute fade curves for each of x, y, z. */
				auto u = PerlinNoise::fade(x);
				auto v = PerlinNoise::fade(y);
				auto w = PerlinNoise::fade(z);

				/* Hash coordinates of the 8 cube corners. */
				const auto A = m_permutations[X] + Y;
				const auto AA = m_permutations[A] + Z;
				const auto AB = m_permutations[A + 1] + Z;
				const auto B = m_permutations[X + 1] + Y;
				const auto BA = m_permutations[B] + Z;
				const auto BB = m_permutations[B + 1] + Z;

				/* Adds blended results from 8 corners of cube. */
				const auto result = Math::linearInterpolation(
					Math::linearInterpolation(
						Math::linearInterpolation(
							PerlinNoise::grad(m_permutations[AA], x, y, z),
							PerlinNoise::grad(m_permutations[BA], x - 1, y, z),
							u
						),
						Math::linearInterpolation(
							PerlinNoise::grad(m_permutations[AB], x, y - 1, z),
							PerlinNoise::grad(m_permutations[BB], x - 1, y - 1, z),
							u
						),
						v
					),
					Math::linearInterpolation(
						Math::linearInterpolation(
							PerlinNoise::grad(m_permutations[AA + 1], x, y, z - 1),
							PerlinNoise::grad(m_permutations[BA + 1], x - 1, y, z - 1),
							u
						),
						Math::linearInterpolation(
							PerlinNoise::grad(m_permutations[AB + 1], x, y - 1, z - 1),
							PerlinNoise::grad(m_permutations[BB + 1], x - 1, y - 1, z - 1),
							u
						),
						v
					),
					w
				);

				return (result + One) * Half;
			}

			/**
			 * @brief Generates a noise value for 8-bit integer.
			 * @param x A value for X axis.
			 * @param y A value for Y axis.
			 * @return uint8_t
			 */
			[[nodiscard]]
			uint8_t
			generate8bits (number_t x, number_t y) noexcept
			{
				constexpr auto Scale8bit{static_cast< number_t >(256.0)};

				return static_cast< uint8_t >(Scale8bit * this->generate(x, y, 0));
			}

		private:

			/**
			 * @brief Computes the gradient value based on the hashed index and 3D coordinates.
			 * @param hash The hashed value, typically derived from a permutation table, which determines the gradient direction.
			 * @param x The x-coordinate of the input position.
			 * @param y The y-coordinate of the input position.
			 * @param z The z-coordinate of the input position.
			 * @return The calculated gradient value (dot product between the gradient vector and position vector).
			 */
			[[nodiscard]]
			static
			number_t
			grad (uint32_t hash, number_t x, number_t y, number_t z) noexcept
			{
				const auto & gradVector = GradientTable[hash & 15];

				return static_cast< number_t >(gradVector.x) * x + static_cast< number_t >(gradVector.y) * y + static_cast< number_t >(gradVector.z) * z;
			}

			/**
			 * @brief Performs a fade.
			 * @param t
			 * @return number_t
			 */
			[[nodiscard]]
			static
			number_t
			fade (number_t t) noexcept
			{
				constexpr auto FadeC1{static_cast< number_t >(6.0)};
				constexpr auto FadeC2{static_cast< number_t >(15.0)};
				constexpr auto FadeC3{static_cast< number_t >(10.0)};

				return t * t * t * (t * (t * FadeC1 - FadeC2) + FadeC3);
			}

			struct GradientVector
			{
				int8_t x, y, z;
			};

			static constexpr std::array< GradientVector, 16 > GradientTable {{
				{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, // Indices 0-3   (Gradients XY)
				{1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, // Indices 4-7   (Gradients XZ)
				{0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}, // Indices 8-11  (Gradients YZ)
				{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}  // Indices 12-15 (Répétition first 4)
			}};

			std::vector< uint32_t > m_permutations{};
	};
}
