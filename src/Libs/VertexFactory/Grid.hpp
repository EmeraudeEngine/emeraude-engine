/*
 * src/Libs/VertexFactory/Grid.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

/* Local inclusions. */
#include "Libs/Algorithms/DiamondSquare.hpp"
#include "Libs/Algorithms/PerlinNoise.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "GridQuad.hpp"
#include "Types.hpp"

namespace EmEn::Libs::VertexFactory
{
	template< typename vertex_data_t = float >
	requires (std::is_floating_point_v< vertex_data_t > )
	struct PerlinNoiseParams
	{
		vertex_data_t size{1};
		vertex_data_t factor{0.5};
	};

	template< typename vertex_data_t = float >
	requires (std::is_floating_point_v< vertex_data_t > )
	struct DiamondSquareParams
	{
		vertex_data_t factor{1};
		vertex_data_t roughness{0.5};
		int32_t seed{0};
	};

	/**
	 * @class Grid
	 * @brief Template class for generating 2D grid geometry with height displacement.
	 *
	 * Grid provides a flexible system for creating heightmap-based terrain and ground surfaces.
	 * It generates a square grid of points with configurable dimensions and supports various
	 * height displacement techniques including pixmap-based displacement mapping, procedural
	 * noise generation (Perlin and Diamond-Square algorithms), and direct height manipulation.
	 *
	 * The grid is always square (same number of divisions on both axes) and centered at the origin.
	 * It uses a **Right-Handed Y-DOWN** coordinate system where Y represents height/elevation.
	 *
	 * Key features:
	 * - Configurable grid size and subdivision count
	 * - Multiple displacement mapping modes (Replace, Add, Subtract, Multiply, Divide)
	 * - Procedural noise generation (Perlin, Diamond-Square)
	 * - Height scaling and shifting operations
	 * - Automatic bounding box and bounding sphere computation
	 * - Position, normal, tangent, and texture coordinate generation
	 * - Vertex color sampling from pixmaps
	 * - Template-based for flexible precision control
	 *
	 * @tparam vertex_data_t Floating-point type for vertex data and geometric calculations (default: float).
	 * @tparam index_data_t Unsigned integer type for indexing grid points and quads (default: uint32_t).
	 *
	 * @note The grid is always square in XZ plane with Y as the height dimension.
	 * @note All methods assume the grid is initialized via initializeData() before use.
	 * @note Recent optimizations include template-based transformations and cached neighbor positions.
	 *
	 * @see GridQuad
	 * @see PointTransformationMode
	 * @version 0.8.38
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class Grid final
	{
		public:

			/**
			 * @brief Constructs a default empty grid.
			 * @note Creates an uninitialized grid with zero dimensions. You must call initializeData() before using the grid.
			 * @see initializeData()
			 */
			Grid () noexcept = default;

			/**
			 * @brief Constructs a grid.
			 * @param cellCount The number of quad cells along one dimension. Total quads = division × division.
			 * @param cellSize The dimension of one cell. Default 1.
			 * @see initializeData()
			 */
			explicit
			Grid (index_data_t cellCount, vertex_data_t cellSize = 1) noexcept
			{
				this->initializeByCellSize(cellCount, cellSize);
			}

			/**
			 * @brief Constructs a grid.
			 * @param gridSize The total size of the grid.
			 * @param gridDivision The number of quad cells along one dimension. Total quads = division × division.
			 * @see initializeData()
			 */
			Grid (vertex_data_t gridSize, index_data_t gridDivision) noexcept
			{
				this->initializeByGridSize(gridSize, gridDivision);
			}

			/**
			 * @brief Initializes the grid by specifying cell count and cell size.
			 *
			 * Creates a grid where you define how many cells you want and how large each cell should be.
			 * The total grid size is computed as cellCount × cellSize. This approach is useful when
			 * you need precise control over individual cell dimensions, such as for tile-based systems.
			 *
			 * @param cellCount The number of quad cells along one dimension. Total quads = cellCount × cellCount.
			 * @param cellSize The size of a single cell edge in world units. Default is 1.
			 *
			 * @return True if initialization succeeded, false if parameters are invalid.
			 *
			 * @pre cellCount must be at least 1
			 * @pre cellSize must be positive
			 * @post Grid is cleared before initialization
			 * @post Bounding box is initialized for a flat grid at Y=0
			 * @post Point count = (cellCount + 1)²
			 * @post Total grid size = cellCount × cellSize
			 *
			 * @note The actual number of vertices is (cellCount + 1)² because a grid with N cells needs N+1 points per dimension.
			 *
			 * @see initializeByGridSize()
			 * @see clear()
			 */
			bool
			initializeByCellSize (index_data_t cellCount, vertex_data_t cellSize = 1) noexcept
			{
				if ( cellCount == 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the cell count parameter must be at least 1 !" "\n";

					return false;
				}

				if ( cellSize < 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the cell size parameter must be positive !" "\n";

					return false;
				}

				/* To be sure. */
				this->clear();

				/* NOTE: Declare the total number of points to create the grid. */
				m_squaredQuadCount = cellCount;
				m_squaredPointCount = cellCount + 1;
				m_pointHeights.resize(this->pointCount(), 0);

				if constexpr ( VertexFactoryDebugEnabled )
				{
					/* Shows memory usage */
					const auto memoryAllocated = m_pointHeights.size() * sizeof(vertex_data_t);

					std::cout << "[DEBUG:VERTEX_FACTORY] " << ( static_cast< vertex_data_t >(memoryAllocated) / 1048576 ) << " Mib allocated." "\n";
				}

				constexpr auto Half = static_cast< vertex_data_t >(0.5);

				/* If we specify the size about the division. */
				m_quadSquaredSize = cellSize;
				m_halfSquaredSize = (m_quadSquaredSize * m_squaredQuadCount) * Half;

				/* Initialize the bounding box for a flat ground. */
				m_boundingBox.set({m_halfSquaredSize, 0, m_halfSquaredSize}, {-m_halfSquaredSize, 0, -m_halfSquaredSize});
				m_boundingSphere.setRadius(m_boundingBox.highestLength() * Half);

				return true;
			}

			/**
			 * @brief Initializes the grid by specifying total grid size and subdivision count.
			 *
			 * Creates a grid where you define the total size and how many cells to subdivide it into.
			 * The cell size is computed as gridSize / gridDivision. This approach is useful when
			 * you need a specific overall terrain size regardless of cell granularity.
			 *
			 * @param gridSize The total size of the grid in world units. The grid extends from -gridSize/2 to +gridSize/2.
			 * @param gridDivision The number of quad cells along one dimension. Total quads = gridDivision × gridDivision.
			 *
			 * @return True if initialization succeeded, false if parameters are invalid.
			 *
			 * @pre gridSize must be positive
			 * @pre gridDivision must be at least 1
			 * @post Grid is cleared before initialization
			 * @post Bounding box is initialized for a flat grid at Y=0
			 * @post Point count = (gridDivision + 1)²
			 * @post Cell size = gridSize / gridDivision
			 *
			 * @note The actual number of vertices is (gridDivision + 1)² because a grid with N cells needs N+1 points per dimension.
			 *
			 * @see initializeByCellSize()
			 * @see clear()
			 */
			bool
			initializeByGridSize (vertex_data_t gridSize, index_data_t gridDivision) noexcept
			{
				if ( gridSize < 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the grid size parameter must be positive !" "\n";

					return false;
				}

				if ( gridDivision == 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the grid division parameter must be at least 1 !" "\n";

					return false;
				}

				/* To be sure. */
				this->clear();

				/* NOTE: Declare the total number of points to create the grid. */
				m_squaredQuadCount = gridDivision;
				m_squaredPointCount = gridDivision + 1;
				m_pointHeights.resize(this->pointCount(), 0);

				if constexpr ( VertexFactoryDebugEnabled )
				{
					/* Shows memory usage */
					const auto memoryAllocated = m_pointHeights.size() * sizeof(vertex_data_t);

					std::cout << "[DEBUG:VERTEX_FACTORY] " << ( static_cast< vertex_data_t >(memoryAllocated) / 1048576 ) << " Mib allocated." "\n";
				}

				constexpr auto Half = static_cast< vertex_data_t >(0.5);

				/* If we specify the size about the division. */
				m_quadSquaredSize = gridSize / m_squaredQuadCount;
				m_halfSquaredSize = gridSize * Half;

				/* Initialize the bounding box for a flat ground. */
				m_boundingBox.set({m_halfSquaredSize, 0, m_halfSquaredSize}, {-m_halfSquaredSize, 0, -m_halfSquaredSize});
				m_boundingSphere.setRadius(m_boundingBox.highestLength() * Half);

				return true;
			}

			/**
			 * @brief Applies pixmap-based displacement mapping to modify grid heights.
			 *
			 * Samples a pixmap using UV coordinates and uses its grayscale values to displace
			 * grid point heights. The pixmap is sampled using cosine interpolation for smooth results.
			 * This is commonly used to apply heightmaps from image files to terrain geometry.
			 *
			 * @param map Reference to a valid pixmap containing height data. Grayscale values are used for displacement.
			 * @param factor Multiplier applied to sampled grayscale values (range 0-255) to determine displacement magnitude.
			 * @param mode Transformation mode controlling how new heights combine with existing values (default: Replace).
			 *
			 * @pre map must be valid (map.isValid() returns true)
			 * @post Grid heights are modified according to the specified mode
			 * @post Bounding box is NOT automatically updated; use height transformation methods if needed
			 *
			 * @note Uses cosine interpolation for smooth sampling across the pixmap
			 * @note UV coordinates are computed automatically from grid indices
			 *
			 * @see PointTransformationMode
			 * @see applyPerlinNoise()
			 * @see applyDiamondSquare()
			 */
			void
			applyDisplacementMapping (const PixelFactory::Pixmap< uint8_t > & map, vertex_data_t factor, PointTransformationMode mode = PointTransformationMode::Replace) noexcept
			{
				if ( !map.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", Pixmap is not usable !" "\n";

					return;
				}

				this->applyTransformationWithUV([this, &map, factor, mode] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
					const auto newValue = static_cast< vertex_data_t >(map.cosineSample(coordU, coordV).gray()) * factor;
					applyMode(m_pointHeights[this->index(indexOnX, indexOnY)], newValue, mode);
				});
			}

			/**
			 * @brief Applies procedural Perlin noise to generate organic terrain features.
			 *
			 * Generates height displacement using Perlin noise algorithm, which produces smooth,
			 * natural-looking terrain with organic features. Higher size values create larger-scale
			 * terrain features, while smaller values produce more detailed, fine-grained noise.
			 *
			 * @param size Frequency/scale of the Perlin noise pattern. Larger values create more frequent, detailed features.
			 * @param factor Multiplier for noise output values, controlling displacement magnitude.
			 * @param mode Transformation mode controlling how noise values combine with existing heights (default: Replace).
			 *
			 * @post Grid heights are modified with Perlin noise according to the specified mode
			 * @post Bounding box is automatically updated to encompass new height values
			 *
			 * @note Perlin noise produces values in range [-1, 1], scaled by the factor parameter
			 * @note This method automatically updates the bounding box, unlike applyDisplacementMapping()
			 *
			 * @see PointTransformationMode
			 * @see applyDiamondSquare()
			 * @see applyDisplacementMapping()
			 */
			void
			applyPerlinNoise (vertex_data_t size, vertex_data_t factor, PointTransformationMode mode = PointTransformationMode::Replace) noexcept
			{
				Algorithms::PerlinNoise< vertex_data_t > generator{};

				this->applyTransformationWithUV([this, size, factor, mode, &generator] (index_data_t indexOnX, index_data_t indexOnY, vertex_data_t coordU, vertex_data_t coordV) {
					const auto idx = this->index(indexOnX, indexOnY);
					const auto newValue = generator.generate(coordU * size, coordV * size, 0) * factor;

					applyMode(m_pointHeights[idx], newValue, mode);
					m_boundingBox.mergeY(m_pointHeights[idx]);
				});
			}

			/**
			 * @brief Applies procedural Diamond-Square algorithm for fractal terrain generation.
			 *
			 * Generates height displacement using the Diamond-Square algorithm, a fractal subdivision
			 * technique that produces realistic terrain with controllable roughness. This method is
			 * particularly effective for creating mountainous or hilly landscapes with natural variation.
			 *
			 * @param factor Multiplier for algorithm output values, controlling overall displacement magnitude.
			 * @param roughness Controls terrain roughness/smoothness. Higher values create more jagged, rough terrain; lower values produce smoother landscapes.
			 * @param seed Random seed for reproducible generation (default: 1). Different seeds produce different terrain patterns.
			 * @param mode Transformation mode controlling how generated heights combine with existing values (default: Replace).
			 *
			 * @post Grid heights are modified with Diamond-Square values according to the specified mode
			 * @post Bounding box is automatically updated to encompass new height values
			 *
			 * @note The algorithm requires grid dimensions to be suitable for fractal subdivision
			 * @note This method automatically updates the bounding box
			 * @note Diamond-Square typically produces more angular features than Perlin noise
			 *
			 * @see PointTransformationMode
			 * @see applyPerlinNoise()
			 * @see applyDisplacementMapping()
			 */
			void
			applyDiamondSquare (vertex_data_t factor, vertex_data_t roughness, int32_t seed = 1, PointTransformationMode mode = PointTransformationMode::Replace) noexcept
			{
				Algorithms::DiamondSquare< vertex_data_t > generator{seed, false};

				if ( generator.generate(m_squaredPointCount, roughness) )
				{
					this->applyTransformation([this, factor, mode, &generator] (index_data_t indexOnX, index_data_t indexOnY) {
						const auto idx = this->index(indexOnX, indexOnY);
						const auto newValue = generator.value(indexOnX, indexOnY) * factor;

						applyMode(m_pointHeights[idx], newValue, mode);
						m_boundingBox.mergeY(m_pointHeights[idx]);
					});
				}
			}

			/**
			 * @brief Multiplies all grid heights by a uniform scale factor.
			 *
			 * Applies uniform scaling to all height values in the grid. This is useful for
			 * adjusting the overall vertical scale of terrain after generation or to match
			 * a specific height range requirement.
			 *
			 * @param multiplier Scale factor applied to all heights. Values < 1 compress, values > 1 stretch.
			 *
			 * @post All height values are multiplied by the multiplier
			 * @post Bounding box is updated to reflect new height range
			 * @post Bounding sphere radius is recalculated
			 *
			 * @note Negative multipliers will flip terrain upside-down but are valid
			 *
			 * @see shiftHeight()
			 */
			void
			scaleHeight (vertex_data_t multiplier) noexcept
			{
				std::transform(m_pointHeights.begin(), m_pointHeights.end(), m_pointHeights.begin(), [multiplier] (auto height) -> vertex_data_t {
					return height * multiplier;
				});

				auto maximum = m_boundingBox.maximum();
				auto minimum = m_boundingBox.minimum();

				maximum[Math::Y] *= multiplier;
				minimum[Math::Y] *= multiplier;

				m_boundingBox.set(maximum, minimum);
				m_boundingSphere.setRadius(m_boundingBox.highestLength() * static_cast< vertex_data_t >(0.5));
			}

			/**
			 * @brief Adds a uniform offset to all grid heights.
			 *
			 * Applies a constant vertical translation to all height values. This is useful for
			 * raising or lowering the entire terrain to a specific elevation, such as placing
			 * terrain above sea level or adjusting baseline elevation.
			 *
			 * @param shift Offset value added to all heights. Positive values raise the terrain, negative values lower it.
			 *
			 * @post All height values are increased by the shift amount
			 * @post Bounding box is translated vertically to reflect new height range
			 * @post Bounding sphere radius is recalculated
			 *
			 * @see scaleHeight()
			 */
			void
			shiftHeight (vertex_data_t shift) noexcept
			{
				std::transform(m_pointHeights.begin(), m_pointHeights.end(), m_pointHeights.begin(), [shift] (auto height) -> vertex_data_t {
					return height + shift;
				});

				auto maximum = m_boundingBox.maximum();
				auto minimum = m_boundingBox.minimum();

				maximum[Math::Y] += shift;
				minimum[Math::Y] += shift;

				m_boundingBox.set(maximum, minimum);
				m_boundingSphere.setRadius(m_boundingBox.highestLength() * static_cast< vertex_data_t >(0.5));
			}

			/**
			 * @brief Clears all grid data and resets to uninitialized state.
			 *
			 * Deallocates all memory and resets grid to an empty state equivalent to
			 * a newly constructed Grid. After calling clear(), you must call initializeData()
			 * before using the grid again.
			 *
			 * @post All height data is deallocated
			 * @post Quad and point counts are reset to zero
			 * @post Bounding box and bounding sphere are reset
			 * @post isValid() returns false
			 *
			 * @see initializeData()
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
			 * @brief Sets uniform texture coordinate multiplier for both U and V directions.
			 *
			 * Controls texture tiling across the grid by scaling UV coordinates. Higher values
			 * cause textures to repeat more frequently; lower values stretch textures across
			 * more grid area. Both U and V receive the same multiplier.
			 *
			 * @param UVMultiplier Multiplier applied to both U and V texture coordinates. Must be positive.
			 *
			 * @pre UVMultiplier must be greater than zero
			 * @post If valid, both m_UMultiplier and m_VMultiplier are set to UVMultiplier
			 * @post If invalid (≤ 0), multipliers remain unchanged
			 *
			 * @see setUVMultiplier(vertex_data_t, vertex_data_t)
			 * @see textureCoordinates2D()
			 * @see textureCoordinates3D()
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
			 * @brief Sets independent texture coordinate multipliers for U and V directions.
			 *
			 * Controls texture tiling with different rates for horizontal (U) and vertical (V)
			 * directions. This allows non-uniform texture scaling, useful for anisotropic texturing
			 * or matching textures to non-square aspect ratios.
			 *
			 * @param UMultiplier Multiplier for U (horizontal) texture coordinates. Must be positive.
			 * @param VMultiplier Multiplier for V (vertical) texture coordinates. Must be positive.
			 *
			 * @pre UMultiplier must be greater than zero
			 * @pre VMultiplier must be greater than zero
			 * @post If valid, m_UMultiplier is set to UMultiplier
			 * @post If valid, m_VMultiplier is set to VMultiplier
			 * @post Invalid values (≤ 0) are silently ignored, leaving corresponding multiplier unchanged
			 *
			 * @see setUVMultiplier(vertex_data_t)
			 * @see textureCoordinates2D()
			 * @see textureCoordinates3D()
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
			 * @brief Returns the current U (horizontal) texture coordinate multiplier.
			 *
			 * @return Current U multiplier value.
			 *
			 * @see setUVMultiplier()
			 * @see VMultiplier()
			 */
			[[nodiscard]]
			vertex_data_t
			UMultiplier () const noexcept
			{
				return m_UMultiplier;
			}

			/**
			 * @brief Returns the current V (vertical) texture coordinate multiplier.
			 *
			 * @return Current V multiplier value.
			 *
			 * @see setUVMultiplier()
			 * @see UMultiplier()
			 */
			[[nodiscard]]
			vertex_data_t
			VMultiplier () const noexcept
			{
				return m_VMultiplier;
			}

			/**
			 * @brief Checks if the grid has been initialized and contains valid data.
			 *
			 * @return True if grid is initialized with data, false if uninitialized or cleared.
			 *
			 * @see empty()
			 * @see initializeData()
			 * @see clear()
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return !m_pointHeights.empty();
			}

			/**
			 * @brief Checks if the grid is empty (uninitialized).
			 *
			 * Inverse of isValid(). Provided for consistency with STL container conventions.
			 *
			 * @return True if grid is empty/uninitialized, false if initialized with data.
			 *
			 * @see isValid()
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_pointHeights.empty();
			}

			/**
			 * @brief Returns the axis-aligned bounding box enclosing the entire grid geometry.
			 *
			 * The bounding box is automatically updated by displacement methods like applyPerlinNoise()
			 * and height manipulation methods like scaleHeight() and shiftHeight().
			 *
			 * @return Const reference to the grid's bounding box.
			 *
			 * @note Updated automatically by most height modification methods
			 *
			 * @see boundingSphere()
			 */
			[[nodiscard]]
			const Math::Space3D::AACuboid< vertex_data_t > &
			boundingBox () const noexcept
			{
				return m_boundingBox;
			}

			/**
			 * @brief Returns the bounding sphere enclosing the entire grid geometry.
			 *
			 * The bounding sphere is centered at the origin and has a radius sufficient to
			 * enclose the entire grid including height displacement. Automatically updated
			 * alongside the bounding box.
			 *
			 * @return Const reference to the grid's bounding sphere.
			 *
			 * @see boundingBox()
			 */
			[[nodiscard]]
			const Math::Space3D::Sphere< vertex_data_t > &
			boundingSphere () const noexcept
			{
				return m_boundingSphere;
			}

			/**
			 * @brief Returns the total size of the grid in world units.
			 *
			 * Since the grid is square, this represents both X and Z dimensions.
			 *
			 * @return Full grid size (edge length in world units).
			 *
			 * @see halfSquaredSize()
			 * @see quadSize()
			 */
			[[nodiscard]]
			vertex_data_t
			squaredSize () const noexcept
			{
				return m_halfSquaredSize + m_halfSquaredSize;
			}

			/**
			 * @brief Returns half the total grid size in world units.
			 *
			 * Useful for coordinate calculations since the grid is centered at origin.
			 * Grid extends from -halfSquaredSize to +halfSquaredSize on both X and Z axes.
			 *
			 * @return Half of the full grid size.
			 *
			 * @note Grid coordinates range from [-halfSquaredSize, +halfSquaredSize]
			 *
			 * @see squaredSize()
			 */
			[[nodiscard]]
			vertex_data_t
			halfSquaredSize () const noexcept
			{
				return m_halfSquaredSize;
			}

			/**
			 * @brief Returns the size of a single grid quad (cell) in world units.
			 *
			 * Since the grid is square, this represents the quad size on both X and Z axes.
			 *
			 * @return Size of one grid cell edge in world units.
			 *
			 * @see squaredSize()
			 */
			[[nodiscard]]
			vertex_data_t
			quadSize () const noexcept
			{
				return m_quadSquaredSize;
			}

			/**
			 * @brief Returns the height value at specified grid point indices.
			 *
			 * Retrieves the exact height stored at a grid vertex identified by its X/Z indices.
			 * No interpolation is performed.
			 *
			 * @param indexOnX Grid point index along X axis (0 to squaredPointCount-1).
			 * @param indexOnY Grid point index along Z axis (0 to squaredPointCount-1).
			 *
			 * @return Height value at the specified grid point.
			 *
			 * @note No bounds checking is performed; invalid indices cause undefined behavior
			 *
			 * @see getHeightAt(vertex_data_t, vertex_data_t)
			 */
			[[nodiscard]]
			vertex_data_t
			getHeightAt (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				return m_pointHeights[this->index(indexOnX, indexOnY)];
			}

			/**
			 * @brief Returns the interpolated height at arbitrary world coordinates.
			 *
			 * Performs bilinear interpolation of height values from the four surrounding grid
			 * points. This provides smooth height queries at any position on the grid, not just
			 * at vertex locations.
			 *
			 * @param positionX World coordinate on X axis.
			 * @param positionY World coordinate on Z axis.
			 *
			 * @return Interpolated height value, or 0 if coordinates are outside grid bounds.
			 *
			 * @note Coordinates outside range [-halfSquaredSize, +halfSquaredSize] return 0
			 * @note Uses bilinear interpolation for smooth results
			 *
			 * @see getHeightAt(index_data_t, index_data_t)
			 * @see getNormalAt()
			 */
			[[nodiscard]]
			vertex_data_t
			getHeightAt (vertex_data_t positionX, vertex_data_t positionY) const noexcept
			{
				/* Clamp coordinates to valid range. This ensures smooth behavior at terrain edges
				 * by using edge heights for out-of-bounds positions. */
				constexpr auto epsilon = static_cast< vertex_data_t >(0.0001);
				const auto clampedX = std::clamp(positionX, -m_halfSquaredSize + epsilon, m_halfSquaredSize - epsilon);
				const auto clampedY = std::clamp(positionY, -m_halfSquaredSize + epsilon, m_halfSquaredSize - epsilon);

				const auto realX = (clampedX + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorX = realX - std::floor(realX);

				const auto realY = (clampedY + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorY = realY - std::floor(realY);

				/* Clamp indices to valid range to handle floating-point edge cases. */
				const auto indexX = std::min(static_cast< index_data_t >(std::floor(realX)), m_squaredQuadCount - 1);
				const auto indexY = std::min(static_cast< index_data_t >(std::floor(realY)), m_squaredQuadCount - 1);

				const auto currentQuad = this->quad(indexX, indexY);

				/* Interpolate height from each corner of the quad. First X axis... */
				const auto top = Math::linearInterpolation(m_pointHeights[currentQuad.topLeftIndex()], m_pointHeights[currentQuad.topRightIndex()], factorX);
				const auto bottom = Math::linearInterpolation(m_pointHeights[currentQuad.bottomLeftIndex()], m_pointHeights[currentQuad.bottomRightIndex()], factorX);

				/* ... then Y axis. */
				return Math::linearInterpolation(top, bottom, factorY);
			}

			/**
			 * @brief Returns the interpolated surface normal at arbitrary world coordinates.
			 *
			 * Performs bilinear interpolation of normal vectors from the four surrounding grid
			 * points. Useful for smooth lighting and physics interactions at any position on
			 * the terrain surface.
			 *
			 * @param positionX World coordinate on X axis.
			 * @param positionY World coordinate on Z axis.
			 *
			 * @return Interpolated normalized surface normal vector, or positive Y if outside bounds.
			 *
			 * @note Coordinates outside range [-halfSquaredSize, +halfSquaredSize] return (0, 1, 0)
			 * @note Uses bilinear interpolation for smooth normal transitions
			 *
			 * @see getHeightAt(vertex_data_t, vertex_data_t)
			 * @see normal()
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			getNormalAt (vertex_data_t positionX, vertex_data_t positionY) const noexcept
			{
				/* Clamp coordinates to valid range. This ensures smooth behavior at terrain edges
				 * by using edge normals for out-of-bounds positions. */
				constexpr auto epsilon = static_cast< vertex_data_t >(0.0001);
				const auto clampedX = std::clamp(positionX, -m_halfSquaredSize + epsilon, m_halfSquaredSize - epsilon);
				const auto clampedY = std::clamp(positionY, -m_halfSquaredSize + epsilon, m_halfSquaredSize - epsilon);

				const auto realX = (clampedX + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorX = realX - std::floor(realX);

				const auto realY = (clampedY + m_halfSquaredSize) / m_quadSquaredSize;
				const auto factorY = realY - std::floor(realY);

				/* Clamp indices to valid range to handle floating-point edge cases. */
				const auto indexX = std::min(static_cast< index_data_t >(std::floor(realX)), m_squaredQuadCount - 1);
				const auto indexY = std::min(static_cast< index_data_t >(std::floor(realY)), m_squaredQuadCount - 1);

				const auto coordQuad = this->quad(indexX, indexY);

				/* Interpolate height from each corner of the quad. First X axis... */
				const auto top = Math::linearInterpolation(this->normal(coordQuad.topLeftIndex()), this->normal(coordQuad.topRightIndex()), factorX);
				const auto bottom = Math::linearInterpolation(this->normal(coordQuad.bottomLeftIndex()), this->normal(coordQuad.bottomRightIndex()), factorX);

				/* ... then Y axis. */
				return Math::linearInterpolation(top, bottom, factorY);
			}

			/**
			 * @brief Returns the total number of grid vertices (points).
			 *
			 * For a grid with N divisions per dimension, the point count is (N+1)².
			 *
			 * @return Total number of vertices in the grid.
			 *
			 * @see squaredPointCount()
			 * @see quadCount()
			 */
			[[nodiscard]]
			index_data_t
			pointCount () const noexcept
			{
				return m_squaredPointCount * m_squaredPointCount;
			}

			/**
			 * @brief Returns the number of grid vertices along one dimension.
			 *
			 * This is the number of divisions plus one (N+1).
			 *
			 * @return Number of points along X or Z axis.
			 *
			 * @see pointCount()
			 */
			[[nodiscard]]
			index_data_t
			squaredPointCount () const noexcept
			{
				return m_squaredPointCount;
			}

			/**
			 * @brief Returns the total number of grid quads (cells).
			 *
			 * For a grid with N divisions per dimension, the quad count is N².
			 *
			 * @return Total number of quads in the grid.
			 *
			 * @see squaredQuadCount()
			 * @see pointCount()
			 */
			[[nodiscard]]
			index_data_t
			quadCount () const noexcept
			{
				return m_squaredQuadCount * m_squaredQuadCount;
			}

			/**
			 * @brief Returns the number of grid quads along one dimension.
			 *
			 * This is the number of divisions specified in initializeData().
			 *
			 * @return Number of quads along X or Z axis.
			 *
			 * @see quadCount()
			 */
			[[nodiscard]]
			index_data_t
			squaredQuadCount () const noexcept
			{
				return m_squaredQuadCount;
			}

			/**
			 * @brief Converts 2D grid coordinates to linear buffer index.
			 *
			 * Maps (X, Z) grid coordinates to a linear index for accessing the height buffer.
			 * Uses row-major ordering.
			 *
			 * @param indexOnX Grid point index along X axis.
			 * @param indexOnY Grid point index along Z axis.
			 *
			 * @return Linear index into the height buffer.
			 *
			 * @note Row-major formula: index = x + (z * width)
			 *
			 * @see indexOnX()
			 * @see indexOnY()
			 */
			[[nodiscard]]
			index_data_t
			index (index_data_t indexOnX, index_data_t indexOnY) const noexcept
			{
				return indexOnX + (indexOnY * m_squaredPointCount);
			}

			/**
			 * @brief Extracts X grid coordinate from linear buffer index.
			 *
			 * Inverse operation of index(). Converts a linear buffer index back to its
			 * X grid coordinate.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return Grid point index along X axis.
			 *
			 * @see index()
			 * @see indexOnY()
			 */
			[[nodiscard]]
			index_data_t
			indexOnX (index_data_t index) const noexcept
			{
				return index % m_squaredPointCount;
			}

			/**
			 * @brief Extracts Z grid coordinate from linear buffer index.
			 *
			 * Inverse operation of index(). Converts a linear buffer index back to its
			 * Z grid coordinate.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return Grid point index along Z axis.
			 *
			 * @see index()
			 * @see indexOnX()
			 */
			[[nodiscard]]
			index_data_t
			indexOnY (index_data_t index) const noexcept
			{
				return index / m_squaredPointCount;
			}

			/**
			 * @brief Returns a GridQuad structure for the quad at specified grid coordinates.
			 *
			 * A GridQuad contains the four vertex indices (top-left, top-right, bottom-left,
			 * bottom-right) that form a quad cell in the grid.
			 *
			 * @param indexOnX Quad index along X axis (0 to squaredQuadCount-1).
			 * @param indexOnY Quad index along Z axis (0 to squaredQuadCount-1).
			 *
			 * @return GridQuad containing the four corner vertex indices, or empty GridQuad if indices overflow.
			 *
			 * @note Prints error to stderr if indices are out of bounds
			 *
			 * @see GridQuad
			 * @see nearestQuad()
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
			 * @brief Finds the nearest quad at arbitrary world coordinates.
			 *
			 * Converts world coordinates to grid indices and returns the quad containing
			 * or nearest to those coordinates. Useful for spatial queries and collision detection.
			 *
			 * @param coordX World coordinate on X axis.
			 * @param coordY World coordinate on Z axis.
			 *
			 * @return GridQuad containing the quad at or nearest to the specified coordinates.
			 *
			 * @see quad()
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
			 * @brief Builds a flat vector containing all vertex positions in XYZ format.
			 *
			 * Generates a contiguous array of position data suitable for GPU upload or further
			 * processing. Each vertex contributes 3 values (X, Y, Z) in sequence.
			 *
			 * @return Vector containing interleaved position data: [x0, y0, z0, x1, y1, z1, ...].
			 *		 Size is pointCount() × 3.
			 *
			 * @note Data layout is optimized for GPU vertex buffer creation
			 * @note Y coordinate contains the height value at each point
			 *
			 * @see position()
			 */
			[[nodiscard]]
			std::vector< vertex_data_t >
			buildPositionVector () const noexcept
			{
				const auto totalPoints = static_cast< size_t >(m_squaredPointCount) * m_squaredPointCount;
				std::vector< vertex_data_t > vector(totalPoints * 3);

				size_t writeIndex = 0;

				for ( index_data_t yIndex = 0; yIndex < m_squaredPointCount; ++yIndex )
				{
					const auto zCoord = (yIndex * m_quadSquaredSize) - m_halfSquaredSize;

					for ( index_data_t xIndex = 0; xIndex < m_squaredPointCount; ++xIndex )
					{
						vector[writeIndex++] = (xIndex * m_quadSquaredSize) - m_halfSquaredSize;
						vector[writeIndex++] = m_pointHeights[this->index(xIndex, yIndex)];
						vector[writeIndex++] = zCoord;
					}
				}

				return vector;
			}

			/**
			 * @brief Returns the 3D position vector at specified grid point indices.
			 *
			 * Computes world-space position (X, Y, Z) for a grid vertex. The position includes
			 * the stored height value as the Y component.
			 *
			 * @param positionX Grid point index along X axis.
			 * @param positionY Grid point index along Z axis.
			 *
			 * @return 3D position vector in world coordinates.
			 *
			 * @see position(index_data_t)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			position (index_data_t positionX, index_data_t positionY) const noexcept
			{
				return {
					(positionX * m_quadSquaredSize) - m_halfSquaredSize + m_worldOffset[0],
					m_pointHeights[this->index(positionX, positionY)],
					(positionY * m_quadSquaredSize) - m_halfSquaredSize + m_worldOffset[1]
				};
			}

			/**
			 * @brief Returns the 3D position vector at specified linear index.
			 *
			 * Convenience overload that converts linear index to 2D coordinates and computes position.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return 3D position vector in world coordinates.
			 *
			 * @see position(index_data_t, index_data_t)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			position (index_data_t index) const noexcept
			{
				return this->position(this->indexOnX(index), this->indexOnY(index));
			}

			/**
			 * @brief Computes the surface normal at specified grid point with pre-computed position.
			 *
			 * Calculates the normal vector by averaging normals from all adjacent quads (up to 4).
			 * This method uses a provided position vector to avoid recomputation, improving performance
			 * when position is already known. Recent optimization caches neighbor positions to reduce
			 * redundant calculations.
			 *
			 * @param indexOnX Grid point index along X axis.
			 * @param indexOnY Grid point index along Z axis.
			 * @param thisPosition Pre-computed 3D position at this grid point.
			 *
			 * @return Normalized surface normal vector, or negative Y (0, -1, 0) if on a flat region or edge.
			 *
			 * @note Averages normals from surrounding quads for smooth results
			 * @note Edge and corner vertices have fewer adjacent quads to consider
			 * @note Recently optimized with cached neighbor positions
			 *
			 * @see normal(index_data_t, index_data_t)
			 * @see normal(index_data_t)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t indexOnX, index_data_t indexOnY, const Math::Vector< 3, vertex_data_t > & thisPosition) const noexcept
			{
				Math::Vector< 3, vertex_data_t > normal{};

				const bool hasTop = indexOnY > 0;
				const bool hasBottom = indexOnY < (m_squaredPointCount - 1);
				const bool hasLeft = indexOnX > 0;
				const bool hasRight = indexOnX < (m_squaredPointCount - 1);

				/* Cache neighbor positions that may be used multiple times. */
				Math::Vector< 3, vertex_data_t > left{};
				Math::Vector< 3, vertex_data_t > right{};

				if ( hasLeft && (hasTop || hasBottom) )
				{
					left = this->position(indexOnX - 1, indexOnY);
				}

				if ( hasRight && (hasTop || hasBottom) )
				{
					right = this->position(indexOnX + 1, indexOnY);
				}

				/* Checks the two quads top to this position.
				 * NOTE: indexOnY:0 = top. */
				if ( hasTop )
				{
					const auto top = this->position(indexOnX, indexOnY - 1);

					/* Top-Left quad. */
					if ( hasLeft )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(top, thisPosition, left);
					}

					/* Top-Right quad. */
					if ( hasRight )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(right, thisPosition, top);
					}
				}

				/* Checks the two quads bottom to this position. */
				if ( hasBottom )
				{
					const auto bottom = this->position(indexOnX, indexOnY + 1);

					/* Bottom-Left quad. */
					if ( hasLeft )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(left, thisPosition, bottom);
					}

					/* Bottom-Right quad. */
					if ( hasRight )
					{
						normal += Math::Vector< 3, vertex_data_t >::normal(bottom, thisPosition, right);
					}
				}

				if ( normal.isZero() )
				{
					return Math::Vector< 3, vertex_data_t >::negativeY();
				}

				return normal.normalize();
			}

			/**
			 * @brief Computes the surface normal at specified grid point indices.
			 *
			 * Convenience overload that computes position automatically.
			 *
			 * @param positionX Grid point index along X axis.
			 * @param positionY Grid point index along Z axis.
			 *
			 * @return Normalized surface normal vector.
			 *
			 * @see normal(index_data_t, index_data_t, const Math::Vector&)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t positionX, index_data_t positionY) const noexcept
			{
				return this->normal(positionX, positionY, this->position(positionX, positionY));
			}

			/**
			 * @brief Computes the surface normal at specified linear index with pre-computed position.
			 *
			 * @param index Linear buffer index.
			 * @param thisPosition Pre-computed 3D position at this grid point.
			 *
			 * @return Normalized surface normal vector.
			 *
			 * @see normal(index_data_t, index_data_t, const Math::Vector&)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			normal (index_data_t index, const Math::Vector< 3, vertex_data_t > & thisPosition) const noexcept
			{
				return this->normal(this->indexOnX(index), this->indexOnY(index), thisPosition);
			}

			/**
			 * @brief Computes the surface normal at specified linear index.
			 *
			 * Convenience overload that computes position automatically.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return Normalized surface normal vector.
			 *
			 * @see normal(index_data_t, index_data_t)
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
			 * @brief Computes the surface tangent vector at specified grid point with pre-computed data.
			 *
			 * Calculates the tangent vector by averaging tangents from all adjacent quads (up to 4).
			 * Tangent vectors are essential for normal mapping and anisotropic shading. This method
			 * uses provided position and UV data to avoid recomputation. Recent optimization caches
			 * neighbor positions and UVs to reduce redundant calculations.
			 *
			 * @param indexOnX Grid point index along X axis.
			 * @param indexOnY Grid point index along Z axis.
			 * @param thisPosition Pre-computed 3D position at this grid point.
			 * @param thisUV Pre-computed 3D texture coordinates at this grid point.
			 *
			 * @return Normalized tangent vector, or positive X (1, 0, 0) if on a flat region or edge.
			 *
			 * @note Averages tangents from surrounding quads for smooth results
			 * @note Edge and corner vertices have fewer adjacent quads to consider
			 * @note Recently optimized with cached neighbor positions and UVs
			 * @note Requires 3D texture coordinates (U, V, height-based W)
			 *
			 * @see tangent(index_data_t, index_data_t)
			 * @see tangent(index_data_t)
			 * @see textureCoordinates3D()
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t indexOnX, index_data_t indexOnY, const Math::Vector< 3, vertex_data_t > & thisPosition, const Math::Vector< 3, vertex_data_t > & thisUV) const
			{
				Math::Vector< 3, vertex_data_t > tangent{};

				const bool hasTop = indexOnY > 0;
				const bool hasBottom = indexOnY < (m_squaredPointCount - 1);
				const bool hasLeft = indexOnX > 0;
				const bool hasRight = indexOnX < (m_squaredPointCount - 1);

				/* Cache neighbor positions and UVs that may be used multiple times. */
				Math::Vector< 3, vertex_data_t > leftPosition{};
				Math::Vector< 3, vertex_data_t > leftUV{};
				Math::Vector< 3, vertex_data_t > rightPosition{};
				Math::Vector< 3, vertex_data_t > rightUV{};

				if ( hasLeft && (hasTop || hasBottom) )
				{
					leftPosition = this->position(indexOnX - 1, indexOnY);
					leftUV = this->textureCoordinates3D(indexOnX - 1, indexOnY);
				}

				if ( hasRight && (hasTop || hasBottom) )
				{
					rightPosition = this->position(indexOnX + 1, indexOnY);
					rightUV = this->textureCoordinates3D(indexOnX + 1, indexOnY);
				}

				/* Checks the two quads top to this position.
				 * NOTE: indexOnY:0 = top. */
				if ( hasTop )
				{
					const auto topPosition = this->position(indexOnX, indexOnY - 1);
					const auto topUV = this->textureCoordinates3D(indexOnX, indexOnY - 1);

					/* Top-Left quad. */
					if ( hasLeft )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							leftPosition, leftUV,
							thisPosition, thisUV,
							topPosition, topUV
						);
					}

					/* Top-Right quad. */
					if ( hasRight )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							topPosition, topUV,
							thisPosition, thisUV,
							rightPosition, rightUV
						);
					}
				}

				/* Checks the two quads bottom to this position. */
				if ( hasBottom )
				{
					const auto bottomPosition = this->position(indexOnX, indexOnY + 1);
					const auto bottomUV = this->textureCoordinates3D(indexOnX, indexOnY + 1);

					/* Bottom-Left quad. */
					if ( hasLeft )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							bottomPosition, bottomUV,
							thisPosition, thisUV,
							leftPosition, leftUV
						);
					}

					/* Bottom-Right quad. */
					if ( hasRight )
					{
						tangent += Math::Vector< 3, vertex_data_t >::tangent(
							rightPosition, rightUV,
							thisPosition, thisUV,
							bottomPosition, bottomUV
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
			 * @brief Computes the surface tangent at specified grid point indices.
			 *
			 * Convenience overload that computes position and texture coordinates automatically.
			 *
			 * @param positionX Grid point index along X axis.
			 * @param positionY Grid point index along Z axis.
			 *
			 * @return Normalized tangent vector.
			 *
			 * @see tangent(index_data_t, index_data_t, const Math::Vector&, const Math::Vector&)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t positionX, index_data_t positionY) const noexcept
			{
				return this->tangent(positionX, positionY, this->position(positionX, positionY), this->textureCoordinates3D(positionX, positionY));
			}

			/**
			 * @brief Computes the surface tangent at specified linear index.
			 *
			 * Convenience overload that computes position and texture coordinates automatically.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return Normalized tangent vector.
			 *
			 * @see tangent(index_data_t, index_data_t)
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
			 * @brief Computes the surface tangent at specified linear index with pre-computed data.
			 *
			 * @param index Linear buffer index.
			 * @param thisPosition Pre-computed 3D position at this grid point.
			 * @param thisUV Pre-computed 3D texture coordinates at this grid point.
			 *
			 * @return Normalized tangent vector.
			 *
			 * @see tangent(index_data_t, index_data_t, const Math::Vector&, const Math::Vector&)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			tangent (index_data_t index, const Math::Vector< 3, vertex_data_t > & thisPosition, const Math::Vector< 3, vertex_data_t > & thisUV) const noexcept
			{
				return this->tangent(this->indexOnX(index), this->indexOnY(index), thisPosition, thisUV);
			}

			/**
			 * @brief Computes 2D texture coordinates (UV) at specified grid point indices.
			 *
			 * Generates normalized texture coordinates in range [0, 1] scaled by the UV multipliers.
			 * The coordinates are based on grid position, not height.
			 *
			 * @param indexOnX Grid point index along X axis.
			 * @param indexOnY Grid point index along Z axis.
			 *
			 * @return 2D texture coordinate vector (U, V).
			 *
			 * @note UV values are scaled by m_UMultiplier and m_VMultiplier
			 *
			 * @see textureCoordinates2D(index_data_t)
			 * @see textureCoordinates3D()
			 * @see setUVMultiplier()
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
			 * @brief Computes 2D texture coordinates at specified linear index.
			 *
			 * Convenience overload that converts linear index to 2D coordinates.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return 2D texture coordinate vector (U, V).
			 *
			 * @see textureCoordinates2D(index_data_t, index_data_t)
			 */
			[[nodiscard]]
			Math::Vector< 2, vertex_data_t >
			textureCoordinates2D (index_data_t index) const noexcept
			{
				return this->textureCoordinates2D(this->indexOnX(index), this->indexOnY(index));
			}

			/**
			 * @brief Computes 3D texture coordinates (UVW) at specified grid point indices.
			 *
			 * Generates texture coordinates with a third component (W) based on normalized height.
			 * The W component represents the point's height relative to the bounding box, useful
			 * for height-based texture blending or effects.
			 *
			 * @param indexOnX Grid point index along X axis.
			 * @param indexOnY Grid point index along Z axis.
			 *
			 * @return 3D texture coordinate vector (U, V, W) where W is normalized height.
			 *
			 * @note UV values are scaled by multipliers; W is in range [0, 1] based on bounding box
			 *
			 * @see textureCoordinates3D(index_data_t)
			 * @see textureCoordinates2D()
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
			 * @brief Computes 3D texture coordinates at specified linear index.
			 *
			 * Convenience overload that converts linear index to 2D coordinates.
			 *
			 * @param index Linear buffer index.
			 *
			 * @return 3D texture coordinate vector (U, V, W).
			 *
			 * @see textureCoordinates3D(index_data_t, index_data_t)
			 */
			[[nodiscard]]
			Math::Vector< 3, vertex_data_t >
			textureCoordinates3D (index_data_t index) const noexcept
			{
				return this->textureCoordinates3D(this->indexOnX(index), this->indexOnY(index));
			}

			/**
			 * @brief Samples a vertex color from a pixmap at specified grid point.
			 *
			 * Uses the grid point's UV coordinates to sample a color from the provided pixmap.
			 * This is useful for applying color maps or terrain splat maps to grid geometry.
			 *
			 * @param index Linear buffer index.
			 * @param pixmap Reference to pixmap containing color data.
			 *
			 * @return Sampled color value using linear interpolation.
			 *
			 * @note Uses linear sampling for smooth color transitions
			 *
			 * @see textureCoordinates2D()
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
			 * @brief Returns direct read-only access to the internal height buffer.
			 *
			 * Provides const reference to the underlying height storage for efficient access
			 * or bulk processing. Heights are stored in row-major order.
			 *
			 * @return Const reference to the height data vector.
			 *
			 * @note Heights are stored in row-major order: index = x + (z * squaredPointCount)
			 *
			 * @see getHeightAt()
			 */
			const std::vector< vertex_data_t > &
			heights () const noexcept
			{
				return m_pointHeights;
			}

			/**
			 * @brief Extracts a sub-grid from this grid at a specified center position.
			 *
			 * Creates a new grid by extracting a square region of cellCount × cellCount cells
			 * centered around the given position. The center position is automatically clamped
			 * to ensure the extracted region stays within the parent grid bounds.
			 *
			 * This means positions near edges or corners will produce the same sub-grid,
			 * as the center is adjusted to keep the extraction region valid.
			 *
			 * @param centerPosition The desired center point in world coordinates (X, Z).
			 * @param cellCount The number of cells per dimension for the sub-grid. Result has cellCount × cellCount cells.
			 *
			 * @return A new Grid containing the extracted region with copied height data.
			 *
			 * @pre cellCount must be less than or equal to squaredQuadCount()
			 * @post The returned grid has the same cell size as the parent
			 * @post The returned grid's heights are copied from the parent
			 *
			 * @note Useful for LOD systems, local collision detection, or terrain streaming.
			 *
			 * @see initializeByCellSize()
			 */
			[[nodiscard]]
			Grid
			subGrid (const Math::Vector< 2, vertex_data_t > & centerPosition, index_data_t cellCount) const noexcept
			{
				/* Ensure cellCount doesn't exceed the parent grid. */
				const auto clampedCellCount = std::min(cellCount, m_squaredQuadCount);
				const auto halfCellCount = clampedCellCount / 2;

				/* Convert world position to grid indices. */
				auto centerIndexX = static_cast< index_data_t >(std::floor((centerPosition[0] + m_halfSquaredSize) / m_quadSquaredSize));
				auto centerIndexY = static_cast< index_data_t >(std::floor((centerPosition[1] + m_halfSquaredSize) / m_quadSquaredSize));

				/* Clamp center so the sub-grid stays within bounds.
				 * The center must be at least halfCellCount from edges. */
				const auto minCenter = halfCellCount;
				const auto maxCenter = m_squaredQuadCount - (clampedCellCount - halfCellCount);

				centerIndexX = std::clamp(centerIndexX, minCenter, maxCenter);
				centerIndexY = std::clamp(centerIndexY, minCenter, maxCenter);

				/* Calculate starting indices. */
				const auto startX = centerIndexX - halfCellCount;
				const auto startY = centerIndexY - halfCellCount;

				/* Create the sub-grid with the same cell size. */
				Grid result;
				result.initializeByCellSize(clampedCellCount, m_quadSquaredSize);

				{
					const auto UVRatio = static_cast< float >(cellCount) / static_cast< float >(this->squaredQuadCount());

					result.setUVMultiplier(m_UMultiplier * UVRatio, m_VMultiplier * UVRatio);
				}

				/* Calculate world offset for the sub-grid center. */
				result.m_worldOffset[0] = (static_cast< vertex_data_t >(centerIndexX) * m_quadSquaredSize) - m_halfSquaredSize;
				result.m_worldOffset[1] = (static_cast< vertex_data_t >(centerIndexY) * m_quadSquaredSize) - m_halfSquaredSize;

				const auto pointCount = clampedCellCount + 1;
				auto minHeight = std::numeric_limits< vertex_data_t >::max();
				auto maxHeight = std::numeric_limits< vertex_data_t >::lowest();

				/* Copy row by row (contiguous in memory due to row-major layout). */
				for ( index_data_t y = 0; y < pointCount; ++y )
				{
					const auto * srcRow = &m_pointHeights[this->index(startX, startY + y)];
					auto * dstRow = &result.m_pointHeights[y * pointCount];

					std::copy_n(srcRow, pointCount, dstRow);

					/* Find min/max for this row. */
					const auto [rowMin, rowMax] = std::minmax_element(dstRow, dstRow + pointCount);
					minHeight = std::min(minHeight, *rowMin);
					maxHeight = std::max(maxHeight, *rowMax);
				}

				/* Single bounding box update. */
				result.m_boundingBox.set(
					{result.m_halfSquaredSize, maxHeight, result.m_halfSquaredSize},
					{-result.m_halfSquaredSize, minHeight, -result.m_halfSquaredSize}
				);
				result.m_boundingSphere.setRadius(result.m_boundingBox.highestLength() * static_cast< vertex_data_t >(0.5));

				return result;
			}

			/**
			 * @brief Outputs grid information to an output stream.
			 *
			 * Provides formatted debug output showing grid dimensions, counts, sizes,
			 * and bounding volumes. Useful for debugging and logging.
			 *
			 * @param out Output stream to write to.
			 * @param obj Grid instance to output.
			 *
			 * @return Reference to the output stream for chaining.
			 *
			 * @see to_string()
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
			 * @brief Converts grid information to a string representation.
			 *
			 * Creates a formatted string containing grid dimensions, counts, sizes,
			 * and bounding volumes. Identical output to operator<<.
			 *
			 * @param obj Grid instance to stringify.
			 *
			 * @return String containing formatted grid information.
			 *
			 * @see operator<<()
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
			 * @brief Applies a transformation mode to update a value.
			 *
			 * Helper function that performs the arithmetic operation specified by the
			 * PointTransformationMode enum. This is used internally by displacement methods
			 * to combine new height values with existing ones.
			 *
			 * @tparam value_t Type of value to operate on (typically vertex_data_t).
			 *
			 * @param current Reference to the current value to modify.
			 * @param newValue The new value to apply.
			 * @param mode Transformation mode specifying the operation (Replace, Add, Subtract, Multiply, Divide).
			 *
			 * @note Recent optimization: converted from std::function to template for better performance
			 *
			 * @see PointTransformationMode
			 * @see applyDisplacementMapping()
			 * @see applyPerlinNoise()
			 * @see applyDiamondSquare()
			 */
			template< typename value_t >
			static void
			applyMode (value_t & current, value_t newValue, PointTransformationMode mode) noexcept
			{
				switch ( mode )
				{
					case PointTransformationMode::Replace:
						current = newValue;
						break;

					case PointTransformationMode::Add:
						current += newValue;
						break;

					case PointTransformationMode::Subtract:
						current -= newValue;
						break;

					case PointTransformationMode::Multiply:
						current *= newValue;
						break;

					case PointTransformationMode::Divide:
						current /= newValue;
						break;
				}
			}

			/**
			 * @brief Applies a callable transformation to all grid points with UV coordinates.
			 *
			 * Iterates over all grid points and invokes the provided callable with grid indices
			 * and pre-computed UV coordinates. This template-based approach provides efficient
			 * iteration for displacement mapping operations.
			 *
			 * @tparam Func Callable type with signature (index_data_t, index_data_t, vertex_data_t, vertex_data_t).
			 *
			 * @param transform Callable invoked for each grid point with (indexOnX, indexOnY, coordU, coordV).
			 *
			 * @note Recent optimization: template-based instead of std::function for better performance
			 *
			 * @see applyTransformation()
			 * @see applyDisplacementMapping()
			 */
			template< typename Func >
			void
			applyTransformationWithUV (Func && transform) noexcept
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
						transform(xIndex, yIndex, coordU, coordV);
					}
				}
			}

			/**
			 * @brief Applies a callable transformation to all grid points.
			 *
			 * Iterates over all grid points and invokes the provided callable with grid indices.
			 * This template-based approach provides efficient iteration for procedural generation
			 * and height manipulation operations.
			 *
			 * @tparam Func Callable type with signature (index_data_t, index_data_t).
			 *
			 * @param transform Callable invoked for each grid point with (indexOnX, indexOnY).
			 *
			 * @note Recent optimization: template-based instead of std::function for better performance
			 *
			 * @see applyTransformationWithUV()
			 * @see applyDiamondSquare()
			 */
			template< typename Func >
			void
			applyTransformation (Func && transform) noexcept
			{
				/* Loop over Y axis. */
				for ( index_data_t yIndex = 0; yIndex < m_squaredPointCount; ++yIndex )
				{
					/* Loop over X axis. */
					for ( index_data_t xIndex = 0; xIndex < m_squaredPointCount; ++xIndex )
					{
						/* Send data to transformation function. */
						transform(xIndex, yIndex);
					}
				}
			}

			/**
			 * @brief Recalculates the bounding box from current height data.
			 *
			 * Iterates through all height values to determine the minimum and maximum Y extents,
			 * then updates the bounding box accordingly. This is used when heights are modified
			 * in ways that don't automatically update the bounding box.
			 *
			 * @note Currently unused in the public API; height modification methods handle bounding box updates directly
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

			index_data_t m_squaredQuadCount{0}; ///< Number of quad cells per dimension (N divisions).
			index_data_t m_squaredPointCount{0}; ///< Number of vertices per dimension (N+1 points for N quads).
			std::vector< vertex_data_t > m_pointHeights{}; ///< Height values for all grid points, stored in row-major order.
			vertex_data_t m_quadSquaredSize{2}; ///< World-space size of a single quad cell edge.
			vertex_data_t m_halfSquaredSize{1}; ///< Half of the total grid size; grid ranges from -half to +half.
			vertex_data_t m_UMultiplier{1}; ///< Texture coordinate multiplier for U (horizontal) direction.
			vertex_data_t m_VMultiplier{1}; ///< Texture coordinate multiplier for V (vertical) direction.
			Math::Vector< 2, vertex_data_t > m_worldOffset{}; ///< World-space offset applied to positions (X, Z).
			Math::Space3D::AACuboid< vertex_data_t > m_boundingBox; ///< Axis-aligned bounding box encompassing all grid geometry.
			Math::Space3D::Sphere< vertex_data_t > m_boundingSphere; ///< Bounding sphere encompassing all grid geometry.
	};
}
