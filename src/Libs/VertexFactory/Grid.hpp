/*
 * src/Libs/VertexFactory/Grid.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <functional>
#include <type_traits>

/* Local inclusions. */
#include "Libs/Algorithms/DiamondSquare.hpp"
#include "Libs/Algorithms/PerlinNoise.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "GridQuad.hpp"

namespace EmEn::Libs::VertexFactory
{
	/** @brief Mode of applying a transformation on the grid data. */
	enum class Mode : uint8_t
	{
		Replace,
		Add,
		Subtract,
		Multiply,
		Divide
	};

	/**
	 * @brief The grid geometry class.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class Grid final
	{
		public:

			/**
			 * @brief Constructs a default grid.
			 */
			Grid () noexcept = default;

			/**
			 * @brief Reserves data for a new grid.
			 * @note All parameters are for one dimension, because a Grid is always a square.
			 * @param size The size of the grid or a cell grid depending on the third parameter.
			 * @param division The number of cells per dimension (square).
			 * @param isSizeForCell Tells if the size parameter is the size of a cell or the whole grid.
			 * @return bool
			 */
			bool
			initializeData (vertex_data_t size, index_data_t division, bool isSizeForCell = false) noexcept
			{
				if ( division == 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", division parameter must be at least 1 !" "\n";

					return false;
				}

				if ( size < 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", size parameter must be positive !" "\n";

					return false;
				}

				/* To be sure. */
				this->clear();

				m_squaredQuadCount = division;
				m_squaredPointCount = division + 1;
				m_pointHeights.resize(this->pointCount(), 0);

				if constexpr ( VertexFactoryDebugEnabled )
				{
					/* Shows memory usage */
					const auto memoryAllocated = m_pointHeights.size() * sizeof(vertex_data_t);

					std::cout << "[DEBUG:VERTEX_FACTORY] " << ( static_cast< vertex_data_t >(memoryAllocated) / 1048576 ) << " Mib allocated." "\n";
				}

				constexpr auto Half = static_cast< vertex_data_t >(0.5);

				/* If we specify the size about the division. */
				if ( isSizeForCell )
				{
					m_quadSquaredSize = size;
					m_halfSquaredSize = (m_quadSquaredSize * m_squaredQuadCount) * Half;
				}
				else
				{
					m_quadSquaredSize = size / m_squaredQuadCount;
					m_halfSquaredSize = size * Half;
				}

				/* Initialize the bounding box for a flat ground. */
				m_boundingBox.set({m_halfSquaredSize, 0, m_halfSquaredSize}, {-m_halfSquaredSize, 0, -m_halfSquaredSize});
				m_boundingSphere.setRadius(m_boundingBox.highestLength() * Half);

				return true;
			}

			/**
			 * @brief Applies a pixmap as a height map to the geometry.
			 * @param map A reference to a pixmap.
			 * @param factor The factor of displacement.
			 * @param mode The mode for moving vertices high. Default Replace.
			 * @return void
			 */
			void
			applyDisplacementMapping (const PixelFactory::Pixmap< uint8_t > & map, vertex_data_t factor, Mode mode = Mode::Replace) noexcept
			{
				if ( !map.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", Pixmap is not usable !" "\n";

					return;
				}

				switch ( mode )
				{
					case Mode::Replace :
						this->applyTransformation([this, &map, factor] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							m_pointHeights[this->index(indexOnX, indexOnY)] = map.cosineSample(coordU, coordV).gray() * factor;

							return true;
						});
						break;

					case Mode::Add :
						this->applyTransformation([this, &map, factor] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							m_pointHeights[this->index(indexOnX, indexOnY)] += map.cosineSample(coordU, coordV).gray() * factor;

							return true;
						});
						break;

					case Mode::Subtract :
						this->applyTransformation([this, &map, factor] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							m_pointHeights[this->index(indexOnX, indexOnY)] -= map.cosineSample(coordU, coordV).gray() * factor;

							return true;
						});
						break;

					case Mode::Multiply :
						this->applyTransformation([this, &map, factor] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							m_pointHeights[this->index(indexOnX, indexOnY)] *= map.cosineSample(coordU, coordV).gray() * factor;

							return true;
						});
						break;

					case Mode::Divide :
						this->applyTransformation([this, &map, factor] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							m_pointHeights[this->index(indexOnX, indexOnY)] /= map.cosineSample(coordU, coordV).gray() * factor;

							return true;
						});
						break;
				}
			}

			/**
			 * @brief Applies the perlin noise algorithm on the grid.
			 * @param size The size of the perlin algorithm.
			 * @param factor The factor of displacement.
			 * @param mode The mode for moving vertices high. Default Replace.
			 * @return void
			 */
			void
			applyPerlinNoise (vertex_data_t size, vertex_data_t factor, Mode mode = Mode::Replace) noexcept
			{
				Algorithms::PerlinNoise< vertex_data_t > generator{};

				switch ( mode )
				{
					case Mode::Replace :
						this->applyTransformation([this, size, factor, &generator] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							const auto index = this->index(indexOnX, indexOnY);

							m_pointHeights[index] = generator.generate(coordU * size, coordV * size, 0) * factor;
							m_boundingBox.mergeY(m_pointHeights[index]);

							return true;
						});
						break;

					case Mode::Add :
						this->applyTransformation([this, size, factor, &generator] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							const auto index = this->index(indexOnX, indexOnY);

							m_pointHeights[index] += generator.generate(coordU * size, coordV * size, 0) * factor;
							m_boundingBox.mergeY(m_pointHeights[index]);

							return true;
						});
						break;

					case Mode::Subtract :
						this->applyTransformation([this, size, factor, &generator] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							const auto index = this->index(indexOnX, indexOnY);

							m_pointHeights[index] -= generator.generate(coordU * size, coordV * size, 0) * factor;
							m_boundingBox.mergeY(m_pointHeights[index]);

							return true;
						});
						break;

					case Mode::Multiply :
						this->applyTransformation([this, size, factor, &generator] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							const auto index = this->index(indexOnX, indexOnY);

							m_pointHeights[index] *= generator.generate(coordU * size, coordV * size, 0) * factor;
							m_boundingBox.mergeY(m_pointHeights[index]);

							return true;
						});
						break;

					case Mode::Divide :
						this->applyTransformation([this, size, factor, &generator] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
							const auto index = this->index(indexOnX, indexOnY);

							m_pointHeights[index] /= generator.generate(coordU * size, coordV * size, 0) * factor;
							m_boundingBox.mergeY(m_pointHeights[index]);

							return true;
						});
						break;
				}
			}

			/**
			 * @brief Applies the diamond square algorithm on the grid.
			 * @param factor Noise strength.
			 * @param roughness Noise roughness.
			 * @param seed Randomize the generation. Default 1.
			 * @param mode The mode for moving vertices high. Default Replace.
			 * @return void
			 */
			void
			applyDiamondSquare (vertex_data_t factor, vertex_data_t roughness, int32_t seed = 1, Mode mode = Mode::Replace) noexcept
			{
				Algorithms::DiamondSquare< vertex_data_t > map{seed, false};

				if ( map.generate(m_squaredPointCount, roughness) )
				{
					switch ( mode )
					{
						case Mode::Replace :
							this->applyTransformation([this, factor, &map] (index_data_t indexOnX, index_data_t indexOnY) {
								const auto index = this->index(indexOnX, indexOnY);

								m_pointHeights[index] = map.value(indexOnX, indexOnY) * factor;
								m_boundingBox.mergeY(m_pointHeights[index]);

								return true;
							});
							break;

						case Mode::Add :
							this->applyTransformation([this, factor, &map] (index_data_t indexOnX, index_data_t indexOnY) {
								const auto index = this->index(indexOnX, indexOnY);

								m_pointHeights[index] += map.value(indexOnX, indexOnY) * factor;
								m_boundingBox.mergeY(m_pointHeights[index]);

								return true;
							});
							break;

						case Mode::Subtract :
							this->applyTransformation([this, factor, &map] (index_data_t indexOnX, index_data_t indexOnY) {
								const auto index = this->index(indexOnX, indexOnY);

								m_pointHeights[index] -= map.value(indexOnX, indexOnY) * factor;
								m_boundingBox.mergeY(m_pointHeights[index]);

								return true;
							});
							break;

						case Mode::Multiply :
							this->applyTransformation([this, factor, &map] (index_data_t indexOnX, index_data_t indexOnY) {
								const auto index = this->index(indexOnX, indexOnY);

								m_pointHeights[index] *= map.value(indexOnX, indexOnY) * factor;
								m_boundingBox.mergeY(m_pointHeights[index]);

								return true;
							});
							break;

						case Mode::Divide :
							this->applyTransformation([this, factor, &map] (index_data_t indexOnX, index_data_t indexOnY) {
								const auto index = this->index(indexOnX, indexOnY);

								m_pointHeights[index] /= map.value(indexOnX, indexOnY) * factor;
								m_boundingBox.mergeY(m_pointHeights[index]);

								return true;
							});
							break;
					}
				}
			}

			/**
			 * @brief Multiplies uniformly the heights of the grid.
			 * @param value The scale value.
			 * @return void
			 */
			void
			scale (vertex_data_t value) noexcept
			{
				constexpr auto Half = static_cast< vertex_data_t >(0.5);

				std::transform(m_pointHeights.begin(), m_pointHeights.end(), m_pointHeights.begin(), [value] (auto height) -> vertex_data_t {
					return height * value;
				});

				auto maximum = m_boundingBox.maximum();
				auto minimum = m_boundingBox.minimum();

				maximum[Math::Y] *= value;
				minimum[Math::Y] *= value;

				m_boundingBox.set(maximum, minimum);
				m_boundingSphere.setRadius(m_boundingBox.highestLength() * Half);
			}

			/**
			 * @brief Clears the grid data.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_pointHeights.clear();

				m_squaredQuadCount = 0;
				m_squaredPointCount = 0;

				m_boundingBox.reset();
				m_boundingSphere.reset();
			}

			/**
			 * @brief Sets a multiplier to texture coordinates.
			 * @param UVMultiplier The multiplier in both direction for the texture coordinates.
			 * @return void
			 */
			void
			setUVMultiplier (vertex_data_t UVMultiplier) noexcept
			{
				if ( UVMultiplier > 0 )
				{
					m_UMultiplier = UVMultiplier;
					m_VMultiplier = UVMultiplier;
				}
			}

			/**
			 * @brief Sets a multiplier to texture coordinates.
			 * @param UMultiplier The multiplier in X direction for the texture coordinates.
			 * @param VMultiplier The multiplier in Y direction for the texture coordinates.
			 * @return void
			 */
			void
			setUVMultiplier (vertex_data_t UMultiplier, vertex_data_t VMultiplier) noexcept
			{
				if ( UMultiplier > 0 )
				{
					m_UMultiplier = UMultiplier;
				}

				if ( VMultiplier > 0 )
				{
					m_VMultiplier = VMultiplier;
				}
			}

			/**
			 * @brief Returns the multiplier to the U component of texture coordinates.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			UMultiplier () const noexcept
			{
				return m_UMultiplier;
			}

			/**
			 * @brief Returns the multiplier to the V component of texture coordinates.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			VMultiplier () const noexcept
			{
				return m_VMultiplier;
			}

			/**
			 * @brief Checks the validity of the data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return !m_pointHeights.empty();
			}

			/**
			 * @brief Checks if the grid data is empty.
			 * @note Inverse of Grid::isValid(). Provided to satisfy C++ conventions.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_pointHeights.empty();
			}

			/**
			 * @brief Returns a bounding box enclosing the whole grid.
			 * @return const Math::Space3D::AACuboid< vertex_data_t > &
			 */
			[[nodiscard]]
			const Math::Space3D::AACuboid< vertex_data_t > &
			boundingBox () const noexcept
			{
				return m_boundingBox;
			}

			/**
			 * @brief Returns the bounding radius.
			 * @return const Math::Space3D::Sphere< vertex_data_t > &
			 */
			[[nodiscard]]
			const Math::Space3D::Sphere< vertex_data_t > &
			boundingSphere () const noexcept
			{
				return m_boundingSphere;
			}

			/**
			 * @brief Returns the squared dimension of the grid.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			squaredSize () const noexcept
			{
				return m_halfSquaredSize + m_halfSquaredSize;
			}

			/**
			 * @brief Returns the half squared dimension of the grid.
			 * @note Useful when using coordinates thinking the origin of the grid is at his center.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			halfSquaredSize () const noexcept
			{
				return m_halfSquaredSize;
			}

			/**
			 * @brief Returns the size of a grid cell.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			quadSize () const noexcept
			{
				return m_quadSquaredSize;
			}

			/**
			 * @brief Returns the height at cell coordinates.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			getHeightAt (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				return m_pointHeights[this->index(indexOnX, indexOnY)];
			}

			/**
			 * @brief Returns the height at an arbitrary X/Y coordinates.
			 * @param positionX The coordinate on X axis.
			 * @param positionY The coordinate on Y axis.
			 * @return vertex_data_t
			 */
			[[nodiscard]]
			vertex_data_t
			getHeightAt (vertex_data_t positionX, vertex_data_t positionY) const noexcept
			{
				/* If coordinates are outside the grid, we return zero. */
				if ( positionX <= -m_halfSquaredSize || positionX >= m_halfSquaredSize || positionY <= -m_halfSquaredSize || positionY >= m_halfSquaredSize )
				{
					return 0;
				}

				const auto realX = (positionX + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorX = realX - std::floor(realX);

				const auto realY = (positionY + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorY = realY - std::floor(realY);

				const auto currentQuad = this->quad(static_cast< index_data_t >(std::floor(realX)), static_cast< index_data_t >(std::floor(realY)));

				/* Interpolate height from each corner of the quad. First X axis... */
				const auto top = Math::linearInterpolation(m_pointHeights[currentQuad.topLeftIndex()], m_pointHeights[currentQuad.topRightIndex()], factorX);
				const auto bottom = Math::linearInterpolation(m_pointHeights[currentQuad.bottomLeftIndex()], m_pointHeights[currentQuad.bottomRightIndex()], factorX);

				/* ... then Y axis. */
				return Math::linearInterpolation(top, bottom, factorY);
			}

			/**
			 * @brief Returns the normal vector at an arbitrary X/Y coordinates.
			 * @param positionX The coordinate on X axis.
			 * @param positionY The coordinate on Y axis.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			getNormalAt (vertex_data_t positionX, vertex_data_t positionY) const noexcept
			{
				/* If coordinates are outside the grid, we return zero. */
				if ( positionX <= -m_halfSquaredSize || positionX >= m_halfSquaredSize || positionY <= -m_halfSquaredSize || positionY >= m_halfSquaredSize )
				{
					return Math::Vector< 3, vertex_data_t>::positiveY();
				}

				const auto realX = (positionX + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorX = realX - std::floor(realX);

				const auto realY = (positionY + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorY = realY - std::floor(realY);

				const auto coordQuad = this->quad(static_cast< index_data_t >(std::floor(realX)), static_cast< index_data_t >(std::floor(realY)));

				/* Interpolate height from each corner of the quad. First X axis... */
				const auto top = Math::linearInterpolation(this->normal(coordQuad.topLeftIndex()), this->normal(coordQuad.topRightIndex()), factorX);
				const auto bottom = Math::linearInterpolation(this->normal(coordQuad.bottomLeftIndex()), this->normal(coordQuad.bottomRightIndex()), factorX);

				/* ... then Y axis. */
				return Math::linearInterpolation(top, bottom, factorY);
			}

			/**
			 * @brief Returns the point count.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			pointCount () const noexcept
			{
				return m_squaredPointCount * m_squaredPointCount;
			}

			/**
			 * @brief Returns the point count in one dimension.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			squaredPointCount () const noexcept
			{
				return m_squaredPointCount;
			}

			/**
			 * @brief Returns the number of grid cell.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			quadCount () const noexcept
			{
				return m_squaredQuadCount * m_squaredQuadCount;
			}

			/**
			 * @brief Returns the number of grid cell in one dimension.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			squaredQuadCount () const noexcept
			{
				return m_squaredQuadCount;
			}

			/**
			 * @brief Convert XY indexes to buffer index.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			index (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				return indexOnX + (indexOnY * m_squaredPointCount);
			}

			/**
			 * @brief Returns the X grid coordinates from grid index.
			 * @param index The grid index.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			indexOnX (index_data_t index) const noexcept
			{
				return index % m_squaredPointCount;
			}
			/**
			 * @brief Returns the Y grid coordinates from grid index.
			 * @param index The grid index.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			indexOnY (index_data_t index) const noexcept
			{
				return static_cast< index_data_t >(std::floor(index / m_squaredPointCount));
			}

			/**
			 * @brief Returns the quad at grid coordinates.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @return GridQuad
			 */
			[[nodiscard]]
			GridQuad< index_data_t >
			quad (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				if ( indexOnX >= m_squaredQuadCount || indexOnY >= m_squaredQuadCount )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", quad indexes X:" << indexOnX << ", Y:" << indexOnY << " overflows " << m_squaredQuadCount << " !" "\n";

					return {};
				}

				/* NOTE: We add y at the end to correspond to vertex indices,
				 * because quads are composed of 4 vertices. */
				const auto base = indexOnX + (indexOnY * m_squaredQuadCount) + indexOnY;

				return {
					base,
					base + m_squaredPointCount,
					/* NOTE: this is only left plus one. */
					base + 1,
					base + m_squaredPointCount + 1
				};
			}

			/**
			 * @brief Returns the nearest quad at coordinates.
			 * @param coordX
			 * @param coordY
			 * @return GridQuad< index_data_t >
			 */
			[[nodiscard]]
			GridQuad< index_data_t >
			nearestQuad (vertex_data_t coordX, vertex_data_t coordY) const noexcept
			{
				const auto realX = std::floor((coordX + m_halfSquaredSize) / m_quadSquaredSize);
				const auto realY = std::floor((coordY + m_halfSquaredSize) / m_quadSquaredSize);

				return this->quad(static_cast< index_data_t >(realX), static_cast< index_data_t >(realY));
			}

			/**
			 * @brief Builds position vectors.
			 * @return std::vector< vertex_data_t >
			 */
			std::vector< vertex_data_t >
			buildPositionVector () const noexcept
			{
				std::vector< vertex_data_t > vector{m_squaredPointCount * m_squaredPointCount * 3};

				#pragma omp simd
				for ( index_data_t xIndex = 0; xIndex < m_squaredPointCount; ++xIndex )
				{
					for ( index_data_t yIndex = 0; yIndex < m_squaredPointCount; ++yIndex )
					{
						vector.push_back((xIndex * m_quadSquaredSize) - m_halfSquaredSize);
						vector.push_back(m_pointHeights[this->index(xIndex, yIndex)]);
						vector.push_back((yIndex * m_quadSquaredSize) - m_halfSquaredSize);
					}
				}

				return vector;
			}

			/**
			 * @brief Returns the position vector at grid coordinates.
			 * @param positionX The grid quad index on X axis.
			 * @param positionY The grid quad index on Y axis.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			position (index_data_t positionX, index_data_t positionY) const noexcept
			{
				return {
					(positionX * m_quadSquaredSize) - m_halfSquaredSize,
					m_pointHeights[this->index(positionX, positionY)],
					(positionY * m_quadSquaredSize) - m_halfSquaredSize
				};
			}

			/**
			 * @brief Returns the position vector at grid cell index.
			 * @param index The grid cell index.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			position (index_data_t index) const noexcept
			{
				return this->position(this->indexOnX(index), this->indexOnY(index));
			}

			/**
			 * @brief Returns the normal vector at grid coordinates.
			 * @note Will return negative Y vector by default.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @param thisPosition The reference to the position at this same spot.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t indexOnX, index_data_t indexOnY, const Math::Vector< 3, vertex_data_t > & thisPosition) const noexcept
			{
				Math::Vector< 3, vertex_data_t > normal{};

				/* Checks the two quads top to this position.
				 * NOTE: indexOnY:0 = top. */
				if ( indexOnY > 0 )
				{
					const auto top = this->position(indexOnX, indexOnY - 1);

					/* Top-Left quad. */
					if ( indexOnX > 0 )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(
							top,
							thisPosition,
							this->position(indexOnX - 1, indexOnY)
						);
					}

					/* Top-Right quad. */
					if ( indexOnX < (m_squaredPointCount - 1) )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(
							this->position(indexOnX + 1, indexOnY),
							thisPosition,
							top
						);
					}
				}

				/* Checks the two quads bottom to this position. */
				if ( indexOnY < (m_squaredPointCount - 1) )
				{
					const auto bottom = this->position(indexOnX, indexOnY + 1);

					/* Bottom-Left quad. */
					if ( indexOnX > 0 )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(
							this->position(indexOnX - 1, indexOnY),
							thisPosition,
							bottom
						);
					}

					/* Bottom-Right quad. */
					if ( indexOnX < (m_squaredPointCount - 1) )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(
							bottom,
							thisPosition,
							this->position(indexOnX + 1, indexOnY)
						);
					}
				}

				if ( normal.isZero() )
				{
					return Math::Vector< 3, vertex_data_t >::negativeY();
				}

				return normal.normalize();
			}

			/**
			 * @brief Returns the normal vector at grid coordinates.
			 * @param positionX The grid quad index on X axis.
			 * @param positionY The grid quad index on Y axis.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t positionX, index_data_t positionY) const noexcept
			{
				return this->normal(positionX, positionY, this->position(positionX, positionY));
			}

			/**
			 * @brief Returns the normal vector at grid index.
			 * @param index The grid quad index.
			 * @param thisPosition The reference to the position at this same spot.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t index, const Math::Vector< 3, vertex_data_t > & thisPosition) const noexcept
			{
				return this->normal(this->indexOnX(index), this->indexOnY(index), thisPosition);
			}

			/**
			 * @brief Returns the normal vector at grid index.
			 * @param index The grid quad index.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t index) const noexcept
			{
				const auto xIndex = this->indexOnX(index);
				const auto yIndex = this->indexOnY(index);

				return this->normal(xIndex, yIndex, this->position(xIndex, yIndex));
			}

			/**
			 * @brief Returns the tangent vector at grid coordinates.
			 * @note Will return positive X vector by default.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @param thisPosition The reference to the position at this same spot.
			 * @param thisUV The reference to the texture coordinates at this same spot.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t indexOnX, index_data_t indexOnY, const Math::Vector< 3, vertex_data_t > & thisPosition, const Math::Vector< 3, vertex_data_t > & thisUV) const
			{
				Math::Vector< 3, vertex_data_t > tangent{};

				/* Checks the two quads top to this position.
				 * NOTE: indexOnY:0 = top. */
				if ( indexOnY > 0 )
				{
					const auto topPosition = this->position(indexOnX, indexOnY - 1);
					const auto topUV = this->textureCoordinates3D(indexOnX, indexOnY - 1);

					/* Top-Left quad. */
					if ( indexOnX > 0 )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							this->position(indexOnX - 1, indexOnY),
							this->textureCoordinates3D(indexOnX - 1, indexOnY),
							thisPosition,
							thisUV,
							topPosition,
							topUV
						);
					}

					/* Top-Right quad. */
					if ( indexOnX < (m_squaredPointCount - 1) )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							topPosition,
							topUV,
							thisPosition,
							thisUV,
							this->position(indexOnX + 1, indexOnY),
							this->textureCoordinates3D(indexOnX + 1, indexOnY)
						);
					}
				}

				/* Checks the two quads bottom to this position. */
				if ( indexOnY < (m_squaredPointCount - 1) )
				{
					const auto bottomPosition = this->position(indexOnX, indexOnY + 1);
					const auto bottomUV = this->textureCoordinates3D(indexOnX, indexOnY + 1);

					/* Bottom-Left quad. */
					if ( indexOnX > 0 )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							bottomPosition,
							bottomUV,
							thisPosition,
							thisUV,
							this->position(indexOnX - 1, indexOnY),
							this->textureCoordinates3D(indexOnX - 1, indexOnY)
						);
					}

					/* Bottom-Right quad. */
					if ( indexOnX < (m_squaredPointCount - 1) )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							this->position(indexOnX + 1, indexOnY),
							this->textureCoordinates3D(indexOnX + 1, indexOnY),
							thisPosition,
							thisUV,
							bottomPosition,
							bottomUV
						);
					}
				}

				if ( tangent.isZero() )
				{
					return Math::Vector< 3, vertex_data_t >::positiveX();
				}

				return tangent.normalize();
			}

			/**
			 * @brief Returns the tangent vector at grid coordinates.
			 * @param positionX The grid quad index on X axis.
			 * @param positionY The grid quad index on Y axis.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t positionX, index_data_t positionY) const noexcept
			{
				return this->tangent(positionX, positionY, this->position(positionX, positionY), this->textureCoordinates3D(positionX, positionY));
			}

			/**
			 * @brief Returns the tangent vector at grid index.
			 * @param index The grid quad index.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t index) const noexcept
			{
				const auto xIndex = this->indexOnX(index);
				const auto yIndex = this->indexOnY(index);

				return this->tangent(xIndex, yIndex, this->position(xIndex, yIndex), this->textureCoordinates3D(xIndex, yIndex));
			}

			/**
			 * @brief Returns the tangent vector at grid index.
			 * @param index The grid quad index.
			 * @param thisPosition The reference to the position at this same spot.
			 * @param thisUV The reference to the texture coordinates at this same spot.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t index, const Math::Vector< 3, vertex_data_t > & thisPosition, const Math::Vector< 3, vertex_data_t > & thisUV) const noexcept
			{
				return this->tangent(this->indexOnX(index), this->indexOnY(index), thisPosition, thisUV);
			}

			/**
			 * @brief Returns a 2D texture coordinates at grid coordinates.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @return Math::Vector< 2, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 2, vertex_data_t >
			textureCoordinates2D (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				const auto div = static_cast< vertex_data_t >(m_squaredQuadCount);

				return {
					(static_cast< vertex_data_t >(indexOnX) / div) * m_UMultiplier,
					(static_cast< vertex_data_t >(indexOnY) / div) * m_VMultiplier
				};
			}

			/**
			 * @brief Returns a 2D texture coordinates at grid index.
			 * @param index The grid quad index.
			 * @return Math::Vector< 2, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 2, vertex_data_t >
			textureCoordinates2D (index_data_t index) const noexcept
			{
				return this->textureCoordinates2D(this->indexOnX(index), this->indexOnY(index));
			}

			/**
			 * @brief Returns a 3D texture coordinates at grid coordinates.
			 * @param indexOnX The grid quad index on X axis.
			 * @param indexOnY The grid quad index on Y axis.
			 * @return Math::Vector< 2, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			textureCoordinates3D (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				const auto div = static_cast< vertex_data_t >(m_squaredQuadCount);

				return {
					(static_cast< vertex_data_t >(indexOnX) / div) * m_UMultiplier,
					(static_cast< vertex_data_t >(indexOnY) / div) * m_VMultiplier,
					(m_pointHeights[this->index(indexOnX, indexOnY)] - m_boundingBox.minimum()[Math::Y]) / m_boundingBox.height()
				};
			}

			/**
			 * @brief Returns a 3D texture coordinates at grid index.
			 * @param index The grid quad index.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			textureCoordinates3D (index_data_t index) const noexcept
			{
				return this->textureCoordinates3D(this->indexOnX(index), this->indexOnY(index));
			}

			/**
			 * @brief Returns a vertex color at grid index.
			 * @param index The grid quad index.
			 * @param pixmap A reference to a pixmap.
			 * @return PixelFactory::Color< float >
			 */
			[[nodiscard]]
			PixelFactory::Color< float >
			vertexColor (index_data_t index, const PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept
			{
				const auto coordU = static_cast< vertex_data_t >(this->indexOnX(index)) / static_cast< vertex_data_t >(m_squaredPointCount);
				const auto coordV = static_cast< vertex_data_t >(this->indexOnY(index)) / static_cast< vertex_data_t >(m_squaredPointCount);

				return pixmap.linearSample(coordU, coordV);
			}

			/**
			 * @brief Returns the list of heights;
			 * @return const std::vector< vertex_data_t > &
			 */
			const std::vector< vertex_data_t > &
			heights () const noexcept
			{
				return m_pointHeights;
			}

			/**
			 * @brief Returns the mode in enum.
			 * @param string A reference to a string.
			 * @return Mode
			 */
			static
			Mode
			getMode (const std::string & string) noexcept
			{
				if ( string == "Add" )
				{
					return Mode::Add;
				}

				if ( string == "Subtract" )
				{
					return Mode::Subtract;
				}

				if ( string == "Multiply" )
				{
					return Mode::Multiply;
				}

				if ( string == "Divide" )
				{
					return Mode::Divide;
				}
				return Mode::Replace;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Grid & obj) noexcept
			{
				return out << "Grid data :\n"
					"Quad count : " << obj.quadCount() << "(Squared: " << obj.m_squaredQuadCount << ")\n"
					"Point count : " << obj.pointCount() << "(Squared: " << obj.m_squaredPointCount << ")\n"
					"Vector< vertex_data_t >::size() : " << obj.m_pointHeights.size() << "\n"
					"UV multiplier : " << obj.m_UMultiplier << ", " << obj.m_VMultiplier << "\n"
					"Quad size (squared) : " << obj.m_quadSquaredSize << "\n"
					"Grid size (squared) : " << ( obj.m_halfSquaredSize + obj.m_halfSquaredSize ) << "\n"
					"BoundingBox : " << obj.m_boundingBox << "\n"
					"BoundingRadius : " << obj.m_boundingSphere << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Grid & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			/**
			 * @brief Applies a transformation on grid data.
			 * @param transform A reference to a lambda.
			 * @return void
			 */
			void
			applyTransformation (const std::function< bool (index_data_t, index_data_t, vertex_data_t, vertex_data_t) > & transform) noexcept
			{
				const auto square = static_cast< vertex_data_t >(m_squaredQuadCount);

				/* Loop over Y axis and get the V coordinates. */
				for ( index_data_t yIndex = 0; yIndex < m_squaredPointCount; ++yIndex )
				{
					const auto coordV = static_cast< vertex_data_t >(yIndex) / square;

					/* Loop over X axis and get the U coordinates. */
					for ( index_data_t xIndex = 0; xIndex < m_squaredPointCount; ++xIndex )
					{
						const auto coordU = static_cast< vertex_data_t >(xIndex) / square;

						/* Send data to transformation function. */
						if ( !transform(xIndex, yIndex, coordU, coordV) )
						{
							std::cerr << __PRETTY_FUNCTION__ << ", transform method failed !" "\n";

							return;
						}
					}
				}
			}

			/**
			 * @brief Applies a transformation on grid data.
			 * @param transform A reference to a lambda.
			 * @return void
			 */
			void
			applyTransformation (const std::function< bool (index_data_t, index_data_t) > & transform) const noexcept
			{
				/* Loop over Y axis and get the V coordinates. */
				for ( index_data_t yIndex = 0; yIndex < m_squaredPointCount; ++yIndex )
				{
					/* Loop over X axis and get the U coordinates. */
					for ( index_data_t xIndex = 0; xIndex < m_squaredPointCount; ++xIndex )
					{
						/* Send data to transformation function. */
						if ( !transform(xIndex, yIndex) )
						{
							std::cerr << __PRETTY_FUNCTION__ << ", transform method failed !" "\n";

							return;
						}
					}
				}
			}

			/**
			 * @brief Updates the bounding box with new data.
			 * @return void
			 */
			void
			updateBoundingBox () noexcept
			{
				m_boundingBox.reset();

				for ( auto height : m_pointHeights )
				{
					m_boundingBox.merge({0.0F, height, 0.0F});
				}
			}

			index_data_t m_squaredQuadCount{0};
			/* NOTE: m_squareQuads + 1 */
			index_data_t m_squaredPointCount{0};
			std::vector< vertex_data_t > m_pointHeights{};
			vertex_data_t m_quadSquaredSize{2};
			vertex_data_t m_halfSquaredSize{1};
			vertex_data_t m_UMultiplier{1};
			vertex_data_t m_VMultiplier{1};
			Math::Space3D::AACuboid< vertex_data_t > m_boundingBox;
			Math::Space3D::Sphere< vertex_data_t > m_boundingSphere;
	};
}
