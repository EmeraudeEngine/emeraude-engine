/*
 * src/Libs/VertexFactory/ShapeGenerator.hpp
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

/* Local inclusions for usages. */
#include "CapUVMapping.hpp"
#include "ShapeBuilder.hpp"
#include "ShapeAssembler.hpp"

namespace EmEn::Libs::VertexFactory::ShapeGenerator
{
	/**
	 * @brief Generates a triangle front-facing the camera (Z+).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The size of the triangle. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateTriangle (vertex_data_t size = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{1};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto height = size * (std::sqrt(static_cast< vertex_data_t >(3)) * static_cast< vertex_data_t >(0.5));
		const auto halfSize = size * static_cast< vertex_data_t >(0.5);

		builder.beginConstruction(ConstructionMode::Triangles);

		builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());

		/* The top of the triangle (Negative Y). */
		builder.setPosition(0, -(height * static_cast< vertex_data_t >(0.5)), 0);
		builder.setTextureCoordinates(0.5, 0);
		builder.setVertexColor(1, 0, 0);
		builder.newVertex();

		/* The left of the triangle (Negative X). */
		builder.setPosition(-halfSize, halfSize, 0);
		builder.setTextureCoordinates(0, 1);
		builder.setVertexColor(0, 1, 0);
		builder.newVertex();

		/* The right of the triangle (Positive X). */
		builder.setPosition(halfSize, halfSize, 0);
		builder.setTextureCoordinates(1, 1);
		builder.setVertexColor(0, 0, 1);
		builder.newVertex();

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a quad front-facing the camera (Z+).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param xScale The size in X-axis.
	 * @param yScale The size in Y-axis. Default same as X-axis.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateQuad (vertex_data_t xScale, vertex_data_t yScale = 0, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		constexpr auto Half = static_cast< vertex_data_t >(0.5);

		Shape< vertex_data_t, index_data_t > shape{2};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		xScale *= Half;

		if ( Utility::isZero(yScale) )
		{
			yScale = xScale;
		}
		else
		{
			yScale *= Half;
		}

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());

		/* Top-left */
		builder.setPosition(-xScale, -yScale, 0);
		builder.setTextureCoordinates(0, 0);
		builder.setVertexColor(0, 0, 0);
		builder.newVertex();

		/* Bottom-left */
		builder.setPosition(-xScale, yScale, 0);
		builder.setTextureCoordinates(0, 1);
		builder.setVertexColor(0, 1, 0);
		builder.newVertex();

		/* Top-right */
		builder.setPosition(xScale, -yScale, 0);
		builder.setTextureCoordinates(1, 0);
		builder.setVertexColor(1, 0, 0);
		builder.newVertex();

		/* Bottom-right */
		builder.setPosition(xScale, yScale, 0);
		builder.setTextureCoordinates(1, 1);
		builder.setVertexColor(1, 1, 0, 1);
		builder.newVertex();

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a cuboid shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param width The width of the cuboid shape. Default 1.
	 * @param height The height of the cuboid shape. Default same as width.
	 * @param depth The depth of the cuboid shape. Default same as width.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCuboid (vertex_data_t width = 1, vertex_data_t height = 0, vertex_data_t depth = 0, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		constexpr auto half = static_cast< vertex_data_t >(0.5);

		/* Centering value on axis */
		{
			/* X-axis */
			width *= half;

			/* Y-axis */
			if ( height > 0 )
			{
				height *= half;
			}
			else
			{
				height = width;
			}

			/* Z-axis */
			if ( depth > 0 )
			{
				depth *= half;
			}
			else
			{
				depth = width;
			}
		}

		Shape< vertex_data_t, index_data_t > shape{12};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		/* Right face (X+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveX());

			builder.setPosition(width, -height, -depth);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.setPosition(width, -height, depth);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(width, height, -depth);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(width, height, depth);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Left face (X-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeX());

			builder.setPosition(-width, height, -depth);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition(-width, height, depth);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(-width, -height, -depth);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(-width, -height, depth);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Top face (Y+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveY());

			builder.setPosition(-width, height, -depth);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition(width, height, -depth);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(-width, height, depth);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(width, height, depth);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Bottom face (Y-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeY());

			builder.setPosition(-width, -height, depth);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.setPosition(width, -height, depth);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(-width, -height, -depth);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(width, -height, -depth);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Front face (Z+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());

			builder.setPosition(-width, -height, depth);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.setPosition(-width, height, depth);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(width, -height, depth);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(width, height, depth);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Back face (Z-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeZ());

			builder.setPosition(width, -height, -depth);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.setPosition(width, height, -depth);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(-width, -height, -depth);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(-width, height, -depth);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a cuboid shape from a vector 3.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The dimension of the cuboid. X for width, Y for height and Z for depth.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCuboid (const Math::Vector< 3, vertex_data_t > & size, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateCuboid(size[Math::X], size[Math::Y], size[Math::Z], options);
	}

	/**
	 * @brief Generates a cuboid shape from a vector 4.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The dimension of the cuboid. X for width, Y for height and Z for depth.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCuboid (const Math::Vector< 4, vertex_data_t > & size, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateCuboid(size[Math::X], size[Math::Y], size[Math::Z], options);
	}

	/**
	 * @brief Generates a cuboid shape by using a minimum and a maximum vector.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param max Positive point of the cuboid.
	 * @param min Negative point of the cuboid.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCuboid (const Math::Vector< 3, vertex_data_t > & max, const Math::Vector< 3, vertex_data_t > & min, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{12};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		/* Right face (X+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveX());

			builder.setPosition(max[Math::X], min[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.setPosition(max[Math::X], min[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(max[Math::X], max[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(max[Math::X], max[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Left face (X-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeX());

			builder.setPosition(min[Math::X], max[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition(min[Math::X], max[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(min[Math::X], min[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(min[Math::X], min[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Top face (Y+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveY());

			builder.setPosition(min[Math::X], max[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition(max[Math::X], max[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(min[Math::X], max[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(max[Math::X], max[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Bottom face (Y-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeY());

			builder.setPosition(min[Math::X], min[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.setPosition(max[Math::X], min[Math::Y], max[Math::Z]);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(min[Math::X], min[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition(max[Math::X], min[Math::Y], min[Math::Z]);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Front face (Z+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());

			builder.setPosition(min[Math::X], min[Math::Y], max[Math::Z]); // Top-left
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.setPosition(min[Math::X], max[Math::Y], max[Math::Z]); // Bottom-left
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(max[Math::X], min[Math::Y], max[Math::Z]); // Top-right
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(max[Math::X], max[Math::Y], max[Math::Z]); // Bottom-right
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Back face (Z-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeZ());

			builder.setPosition(max[Math::X], min[Math::Y], min[Math::Z]); // Top-right
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.setPosition(max[Math::X], max[Math::Y], min[Math::Z]); // Bottom-right
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(min[Math::X], min[Math::Y], min[Math::Z]); // Top-left
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(min[Math::X], max[Math::Y], min[Math::Z]); // Bottom-left
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();;

			builder.resetCurrentTriangle();
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a cuboid shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param box A reference to a cuboid definition.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCuboid (const Math::Space3D::AACuboid< vertex_data_t > & box, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateCuboid(box.maximum(), box.minimum(), options);
	}

	/**
	 * @brief Generates a hollowed cube shape (wireframe edges as 3D beams).
	 * @note Ready for vulkan default world axis.
	 * @note Vertex color is uniform white because ShapeAssembler rotations
	 * do not transform vertex colors, making volumetric mapping invalid.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The total side length of the cube. Default 1.
	 * @param borderSize The thickness of each edge beam.
	 * @param uvMapping UV mapping mode: PerSegment (each face independently mapped) or Planar (cube-projected UVs). Default PerSegment.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateHollowedCube (vertex_data_t size, vertex_data_t borderSize, CapUVMapping uvMapping = CapUVMapping::PerSegment, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		/* Centering value on axis */
		size *= static_cast< vertex_data_t >(0.5);

		Shape< vertex_data_t, index_data_t > shape{8 * 12};

		{
			Shape< vertex_data_t, index_data_t > intermediateShape{8 * 4};

			ShapeAssembler< vertex_data_t, index_data_t > finalAssembler{shape};

			{
				Shape< vertex_data_t, index_data_t > wireShape{8};

				ShapeBuilder< vertex_data_t, index_data_t > builder{wireShape, options};

				ShapeAssembler< vertex_data_t, index_data_t > assembler{intermediateShape};

				/* Uniform white vertex color (volumetric mapping is not
				 * possible here because ShapeAssembler transforms positions
				 * but not vertex colors after merge). */
				builder.options().enableGlobalVertexColor(PixelFactory::White);

				/* UV proportional to physical dimensions so that texel density
				 * is uniform across all beam faces.  U spans the beam width
				 * (normalized to 1.0) and V spans the beam length (ratio of
				 * length to width).  The texture coordinates multiplier then
				 * controls the final tiling density uniformly. */
				const auto beamLength = static_cast< vertex_data_t >(2) * size - static_cast< vertex_data_t >(2) * borderSize;
				const auto vInner = beamLength / borderSize;
				const auto vOuter = (static_cast< vertex_data_t >(2) * size) / borderSize;
				constexpr auto vBorder = static_cast< vertex_data_t >(1);

				builder.beginConstruction(ConstructionMode::TriangleStrip);

				/* Inner face (+X direction, facing cube center) */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveX());
				builder.setPosition(-size + borderSize, -size + borderSize, -size);
				builder.setTextureCoordinates(0, 0);
				builder.newVertex();
				builder.setPosition(-size + borderSize, -size + borderSize, -size + borderSize);
				builder.setTextureCoordinates(1, 0);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size);
				builder.setTextureCoordinates(0, vInner);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size + borderSize);
				builder.setTextureCoordinates(1, vInner);
				builder.newVertex();
				builder.resetCurrentTriangle();

				/* Outer face (-X direction, on cube surface).
				 * This face is a trapezoid: the long edge (at Z = -size) spans
				 * the full cube height 2*size while the short edge (at Z = -size+borderSize)
				 * spans 2*size - 2*borderSize.  V offsets on the short edge must
				 * reflect the physical Y position to avoid UV shear. */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeX());
				builder.setPosition(-size,  size, -size);
				builder.setTextureCoordinates(0, 0);
				builder.newVertex();
				builder.setPosition(-size,  size - borderSize, -size + borderSize);
				builder.setTextureCoordinates(1, vBorder);
				builder.newVertex();
				builder.setPosition(-size, -size, -size);
				builder.setTextureCoordinates(0, vOuter);
				builder.newVertex();
				builder.setPosition(-size, -size + borderSize, -size + borderSize);
				builder.setTextureCoordinates(1, vOuter - vBorder);
				builder.newVertex();
				builder.resetCurrentTriangle();

				/* Inner face (+Z direction, facing cube center) */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());
				builder.setPosition(-size, -size + borderSize, -size + borderSize);
				builder.setTextureCoordinates(0, 0);
				builder.newVertex();
				builder.setPosition(-size,  size - borderSize, -size + borderSize);
				builder.setTextureCoordinates(0, vInner);
				builder.newVertex();
				builder.setPosition(-size + borderSize, -size + borderSize, -size + borderSize);
				builder.setTextureCoordinates(1, 0);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size + borderSize);
				builder.setTextureCoordinates(1, vInner);
				builder.newVertex();
				builder.resetCurrentTriangle();

				/* Outer face (-Z direction, on cube surface).
				 * Same trapezoid correction as -X face. */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeZ());
				builder.setPosition(-size + borderSize, -size + borderSize, -size);
				builder.setTextureCoordinates(0, vBorder);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size);
				builder.setTextureCoordinates(0, vOuter - vBorder);
				builder.newVertex();
				builder.setPosition(-size, -size, -size);
				builder.setTextureCoordinates(1, 0);
				builder.newVertex();
				builder.setPosition(-size,  size, -size);
				builder.setTextureCoordinates(1, vOuter);
				builder.newVertex();
				builder.resetCurrentTriangle();

				builder.endConstruction();

				assembler.merge(wireShape);
				assembler.merge(wireShape, Math::Matrix< 4, float >::rotation(Math::Radian(90.0F), 0.0F, 1.0F, 0.0F));
				assembler.merge(wireShape, Math::Matrix< 4, float >::rotation(Math::Radian(180.0F), 0.0F, 1.0F, 0.0F));
				assembler.merge(wireShape, Math::Matrix< 4, float >::rotation(Math::Radian(270.0F), 0.0F, 1.0F, 0.0F));
			}

			finalAssembler.merge(intermediateShape);
			finalAssembler.merge(intermediateShape, Math::Matrix< 4, float >::rotation(Math::Radian(90.0F), 1.0F, 0.0F, 0.0F));
			finalAssembler.merge(intermediateShape, Math::Matrix< 4, float >::rotation(Math::Radian(90.0F), 0.0F, 0.0F, 1.0F));
		}

		/* Planar UV mode: rewrite all UVs by projecting world-space positions
		 * onto the dominant-normal cube face, mapped to [0,1]x[0,1].
		 * This makes the hollowed cube look like a solid cube when textured. */
		if ( uvMapping == CapUVMapping::Planar )
		{
			constexpr auto one = static_cast< vertex_data_t >(1);
			const auto invSize = one / (static_cast< vertex_data_t >(2) * size);
			const auto & mult = options.textureCoordinatesMultiplier();

			for ( auto & vertex : shape.vertices() )
			{
				const auto & pos = vertex.position();
				const auto & normal = vertex.normal();
				const auto absNX = std::abs(normal[Math::X]);
				const auto absNY = std::abs(normal[Math::Y]);
				const auto absNZ = std::abs(normal[Math::Z]);

				vertex_data_t u, v;

				if ( absNX >= absNY && absNX >= absNZ )
				{
					u = (pos[Math::Z] + size) * invSize;
					v = (pos[Math::Y] + size) * invSize;
				}
				else if ( absNY >= absNX && absNY >= absNZ )
				{
					u = (pos[Math::X] + size) * invSize;
					v = (pos[Math::Z] + size) * invSize;
				}
				else
				{
					u = (pos[Math::X] + size) * invSize;
					v = (pos[Math::Y] + size) * invSize;
				}

				vertex.setTextureCoordinates(Math::Vector< 3, vertex_data_t >{
					u * mult[Math::X], v * mult[Math::Y], 0
				});

				/* Tangent = direction of increasing U in world space. */
				if ( absNX >= absNY && absNX >= absNZ )
				{
					vertex.setTangent(Math::Vector< 3, vertex_data_t >{0, 0, one});
				}
				else
				{
					vertex.setTangent(Math::Vector< 3, vertex_data_t >{one, 0, 0});
				}
			}
		}

		return shape;
	}

	/**
	 * @brief Generates a sphere shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the sphere shape. Default 1.
	 * @param slices Slices precision of the sphere shape. Default 16.
	 * @param stacks Stack precision of the sphere shape. Default 8.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateSphere (vertex_data_t radius = 1, index_data_t slices = 16, index_data_t stacks = 8, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{slices * stacks * 6};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		if ( radius < 0 )
		{
			radius *= -1;
		}
		else if ( Utility::isZero(radius) )
		{
			radius = 1;
		}

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto dRHO = std::numbers::pi_v< vertex_data_t > / static_cast< vertex_data_t >(stacks);
		const auto dTheta = (2 * std::numbers::pi_v< vertex_data_t >) / static_cast< vertex_data_t >(slices);

		const auto deltaU = one / static_cast< vertex_data_t >(slices);
		const auto deltaV = one / static_cast< vertex_data_t >(stacks);

		std::array< Math::Vector< 3, vertex_data_t >, 4 > positions{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > normals{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > textureCoordinates{};

		auto texCoordU = static_cast< vertex_data_t >(1);

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		for ( index_data_t stackIndex = 0; stackIndex < stacks; ++stackIndex )
		{
			const auto RHO = dRHO * static_cast< vertex_data_t >(stackIndex);

			const auto sineRHO = std::sin(RHO);
			const auto cosineRHO = std::cos(RHO);

			const auto sineRHOdRHO = std::sin(RHO + dRHO);
			const auto cosineRHOdRHO = std::cos(RHO + dRHO);

			/* Many sources of OpenGL sphere drawing code uses a triangle fan
			 * for the caps of the sphere. This however introduces texturing
			 * artifacts at the poles on some OpenGL implementations. */
			auto texCoordV = static_cast< vertex_data_t >(0);

			for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex)
			{
				auto theta = sliceIndex == slices ? 0 : static_cast< vertex_data_t >(sliceIndex) * dTheta;
				auto sTheta = -std::sin(theta);
				auto cTheta = std::cos(theta);

				auto normalX = sTheta * sineRHO;
				auto normalY = cosineRHO;
				auto normalZ = cTheta * sineRHO;

				positions[0] = {normalX * radius, normalY * radius, normalZ * radius};
				textureCoordinates[0] = {texCoordU, texCoordV, 0};
				normals[0] = {normalX, normalY, normalZ};

				normalX = sTheta * sineRHOdRHO;
				normalY = cosineRHOdRHO;
				normalZ = cTheta * sineRHOdRHO;

				positions[1] = {normalX * radius, normalY * radius, normalZ * radius};
				textureCoordinates[1] = {texCoordU - deltaV, texCoordV, 0};
				normals[1] = {normalX, normalY, normalZ};

				theta = sliceIndex + 1 == slices ? 0 : static_cast< vertex_data_t >(sliceIndex + 1) * dTheta;
				sTheta = -std::sin(theta);
				cTheta = std::cos(theta);

				normalX = sTheta * sineRHO;
				normalY = cosineRHO;
				normalZ = cTheta * sineRHO;

				texCoordV += deltaU;

				positions[2] = {normalX * radius, normalY * radius, normalZ * radius};
				textureCoordinates[2] = {texCoordU, texCoordV, 0};
				normals[2] = {normalX, normalY, normalZ};

				normalX = sTheta * sineRHOdRHO;
				normalY = cosineRHOdRHO;
				normalZ = cTheta * sineRHOdRHO;

				positions[3] = {normalX * radius, normalY * radius, normalZ * radius};
				textureCoordinates[3] = {texCoordU - deltaV, texCoordV, 0};
				normals[3] = {normalX, normalY, normalZ};

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setNormal(normals[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.setVertexColor((normals[0][Math::X] + one) * half, (normals[0][Math::Y] + one) * half, (normals[0][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setNormal(normals[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.setVertexColor((normals[1][Math::X] + one) * half, (normals[1][Math::Y] + one) * half, (normals[1][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setNormal(normals[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.setVertexColor((normals[2][Math::X] + one) * half, (normals[2][Math::Y] + one) * half, (normals[2][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setNormal(normals[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
				builder.setVertexColor((normals[3][Math::X] + one) * half, (normals[3][Math::Y] + one) * half, (normals[3][Math::Z] + one) * half);
				builder.newVertex();

				builder.resetCurrentTriangle();
			}

			texCoordU -= deltaV;
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Subdivision method for geodesic sphere generation.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param builder A reference to the builder.
	 * @param vectorA A reference to a vector.
	 * @param vectorB A reference to a vector.
	 * @param vectorC A reference to a vector.
	 * @param depth The current depth.
	 * @return void
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	void
	subdivide (ShapeBuilder< vertex_data_t, index_data_t > & builder, const Math::Vector< 3, vertex_data_t > & vectorA, const Math::Vector< 3, vertex_data_t > & vectorB, const Math::Vector< 3, vertex_data_t > & vectorC, index_data_t depth) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		if ( depth == 0 )
		{
			/* Compute spherical UV matching generateSphere() convention:
			 * tex.X = latitude  (1.0 at north pole Y=+1, 0.0 at south pole Y=-1)
			 * tex.Y = longitude (0.0 to 1.0 around the equator)
			 *
			 * Reconstructing from the vertex position on the unit sphere:
			 *   rho   = acos(Y)         => U = 1 - acos(Y)/pi
			 *   theta = atan2(-X, Z)    => V = (atan2(-X, Z) + pi) / (2*pi)
			 *
			 * NOTE: atan2(-X, Z) matches the classic sphere's -sin(theta) for X. */
			constexpr auto zero = static_cast< vertex_data_t >(0);
			constexpr auto half = static_cast< vertex_data_t >(0.5);
			constexpr auto one = static_cast< vertex_data_t >(1);
			constexpr auto twoPi = static_cast< vertex_data_t >(2) * std::numbers::pi_v< vertex_data_t >;
			constexpr auto poleThreshold = static_cast< vertex_data_t >(0.999);

			const auto sphericalUV = [twoPi, zero, one](const Math::Vector< 3, vertex_data_t > & v) {
				const auto latitude = one - std::acos(std::clamp(v[Math::Y], -one, one)) / std::numbers::pi_v< vertex_data_t >;
				const auto longitude = (std::atan2(-v[Math::X], v[Math::Z]) + std::numbers::pi_v< vertex_data_t >) / twoPi;

				return Math::Vector< 3, vertex_data_t >{latitude, longitude, zero};
			};

			auto uvA = sphericalUV(vectorA);
			auto uvB = sphericalUV(vectorB);
			auto uvC = sphericalUV(vectorC);

			/* Fix seam: when a triangle crosses the longitude V=0/V=1
			 * boundary, the texture interpolation wraps the wrong way.
			 * Since each triangle has its own vertices (no indexing),
			 * we can shift V values past 1.0 for correct interpolation.
			 * The GPU repeat/wrap mode handles V > 1.0 transparently. */
			const auto maxV = std::max({uvA[Math::Y], uvB[Math::Y], uvC[Math::Y]});
			const auto minV = std::min({uvA[Math::Y], uvB[Math::Y], uvC[Math::Y]});

			if ( maxV - minV > half )
			{
				if ( uvA[Math::Y] < half ) { uvA[Math::Y] += one; }
				if ( uvB[Math::Y] < half ) { uvB[Math::Y] += one; }
				if ( uvC[Math::Y] < half ) { uvC[Math::Y] += one; }
			}

			/* Fix poles: at Y ≈ ±1, atan2(~0, ~0) gives an arbitrary
			 * longitude. Replace with the average V of the other two
			 * vertices (after seam fix) to avoid a visible pinch. */
			if ( std::abs(vectorA[Math::Y]) > poleThreshold )
			{
				uvA[Math::Y] = (uvB[Math::Y] + uvC[Math::Y]) * half;
			}

			if ( std::abs(vectorB[Math::Y]) > poleThreshold )
			{
				uvB[Math::Y] = (uvA[Math::Y] + uvC[Math::Y]) * half;
			}

			if ( std::abs(vectorC[Math::Y]) > poleThreshold )
			{
				uvC[Math::Y] = (uvA[Math::Y] + uvB[Math::Y]) * half;
			}

			/* Volumetric vertex color: map XYZ [-1, 1] to RGB [0, 1]. */
			const auto volumetricColor = [half, one](const Math::Vector< 3, vertex_data_t > & v) {
				return Math::Vector< 4, vertex_data_t >{
					(v[Math::X] + one) * half,
					(v[Math::Y] + one) * half,
					(v[Math::Z] + one) * half,
					one
				};
			};

			builder.setPosition(vectorA);
			builder.setNormal(vectorA);
			builder.setTextureCoordinates(uvA);
			builder.setVertexColor(volumetricColor(vectorA));
			builder.newVertex();

			builder.setPosition(vectorB);
			builder.setNormal(vectorB);
			builder.setTextureCoordinates(uvB);
			builder.setVertexColor(volumetricColor(vectorB));
			builder.newVertex();

			builder.setPosition(vectorC);
			builder.setNormal(vectorC);
			builder.setTextureCoordinates(uvC);
			builder.setVertexColor(volumetricColor(vectorC));
			builder.newVertex();

			return;
		}

		const auto vectorAB = (vectorA + vectorB).normalize();
		const auto vectorBC = (vectorB + vectorC).normalize();
		const auto vectorCA = (vectorC + vectorA).normalize();
		const auto newDepth = depth - 1;

		subdivide(builder, vectorA, vectorAB, vectorCA, newDepth);
		subdivide(builder, vectorB, vectorBC, vectorAB, newDepth);
		subdivide(builder, vectorC, vectorCA, vectorBC, newDepth);
		subdivide(builder, vectorAB, vectorBC, vectorCA, newDepth);
	}
	/**
	 * @brief Generates a geodesic sphere shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the sphere shape. Default 1.
	 * @param depth Depth precision of the sphere shape. Default 2.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateGeodesicSphere (vertex_data_t radius = 1, index_data_t depth = 2, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		/* Clamp depth to prevent exponential explosion (depth 8 = ~1.3M triangles, ~4M vertices). */
		constexpr index_data_t maxDepth = 8;

		if ( depth > maxDepth )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", depth " << depth << " clamped to " << maxDepth << " to prevent excessive computation." "\n";

			depth = maxDepth;
		}

		/* Reserve exact triangle count: 20 faces * 4^depth subdivisions. */
		const auto triangleCount = static_cast< index_data_t >(20U << (2U * depth));
		Shape< vertex_data_t, index_data_t > shape{triangleCount};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto positionX = static_cast< vertex_data_t >(0.525731112119133606);
		const auto positionY = static_cast< vertex_data_t >(0.0);
		const auto positionZ = static_cast< vertex_data_t >(0.850650808352039932);

		const std::array< Math::Vector< 3, vertex_data_t >, 12 > vertices{{
			{-positionX,  positionY,  positionZ},
			{ positionX,  positionY,  positionZ},
			{-positionX,  positionY, -positionZ},
			{ positionX,  positionY, -positionZ},
			{ positionY,  positionZ,  positionX},
			{ positionY,  positionZ, -positionX},
			{ positionY, -positionZ,  positionX},
			{ positionY, -positionZ, -positionX},
			{ positionZ,  positionX,  positionY},
			{-positionZ,  positionX,  positionY},
			{ positionZ, -positionX,  positionY},
			{-positionZ, -positionX,  positionY}
		}};

		constexpr std::array< std::array< index_data_t , 3 >, 20 > indices{{
			{{0, 4, 1}},
			{{0, 9, 4}},
			{{9, 5, 4}},
			{{4, 5, 8}},
			{{4, 8, 1}},
			{{8, 10, 1}},
			{{8, 3, 10}},
			{{5, 3, 8}},
			{{5, 2, 3}},
			{{2, 7, 3}},
			{{7, 10, 3}},
			{{7, 6, 10}},
			{{7, 11, 6}},
			{{11, 0, 6}},
			{{0, 1, 6}},
			{{6, 1, 10}},
			{{9, 0, 11}},
			{{9, 11, 2}},
			{{9, 2, 5}},
			{{7, 2, 11}}
		}};

		builder.beginConstruction(ConstructionMode::Triangles);

		for ( const auto & index : indices )
		{
			subdivide(builder, vertices[index[0]], vertices[index[1]], vertices[index[2]], depth);
		}

		builder.endConstruction();

		shape.transform(Math::Matrix< 4, vertex_data_t >::scaling(radius));

		return shape;
	}

	/**
	 * @brief Generates a cylinder shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param baseRadius The bottom radius of the cylinder shape. Default 1.
	 * @param topRadius The top radius of the cylinder shape. Default 1.
	 * @param length The length of the cylinder shape. Default 1.
	 * @param slices Slices precision of the cylinder shape. Default 8.
	 * @param stacks Stack precision of the cylinder shape. Default 1.
	 * @param capMapping UV mapping mode for caps. None = no caps, Planar = flat projection, PerSegment = tiled. Default None.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCylinder (vertex_data_t baseRadius = 1, vertex_data_t topRadius = 1, vertex_data_t length = 1, index_data_t slices = 8, index_data_t stacks = 1, CapUVMapping capMapping = CapUVMapping::None, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		const auto capped = capMapping != CapUVMapping::None;
		const auto baseHasCap = capped && !Utility::isZero(baseRadius);
		const auto topHasCap = capped && !Utility::isZero(topRadius);
		const auto capTriangles = (baseHasCap ? slices * 2 : 0) + (topHasCap ? slices * 2 : 0);

		Shape< vertex_data_t, index_data_t > shape{slices * stacks * 2 + capTriangles};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto radiusStep = (topRadius - baseRadius) / static_cast< vertex_data_t >(stacks);
		const auto stepSizeSlice = (2 * std::numbers::pi_v< vertex_data_t >) / static_cast< vertex_data_t >(slices);

		/* Texture coordinates */
		const auto deltaU = one / static_cast< vertex_data_t >(slices);
		const auto deltaV = one / static_cast< vertex_data_t >(stacks);

		/* Outward surface normal Y component for a truncated cone.
		 * Derived from cross product of circumferential tangent and generatrix:
		 * N = (-sin(θ), 0, cos(θ)) × ((topR-baseR)·cos(θ), -length, (topR-baseR)·sin(θ))
		 *   = (length·cos(θ), topR-baseR, length·sin(θ))
		 * For a straight cylinder (baseR == topR), yNormal = 0 (purely radial). */
		const auto yNormal = Utility::isZero(baseRadius - topRadius) ? static_cast< vertex_data_t >(0) : topRadius - baseRadius;

		std::array< Math::Vector< 3, vertex_data_t >, 4 > positions{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > normals{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > textureCoordinates{};

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		for ( index_data_t stackIndex = 0; stackIndex < stacks; ++stackIndex )
		{
			const auto stackIndexF = static_cast< vertex_data_t >(stackIndex);
			const auto stackIndexPlusOneF = static_cast< vertex_data_t >(stackIndex + 1);

			/* Texture coordinates.
			 * NOTE: Inverted for Vulkan. */
			const auto texCoordV = one - (stackIndex == 0 ? 0 : deltaV * stackIndexF);
			const auto nextV = one - (stackIndex == stacks - 1 ? 1 : deltaV * stackIndexPlusOneF);

			const auto currentRadius = baseRadius + (radiusStep * stackIndexF);
			const auto nextRadius = baseRadius + (radiusStep * stackIndexPlusOneF);

			const auto currentY = -(length / stacks) * stackIndexF;
			const auto nextY = -(length / stacks) * stackIndexPlusOneF;

			for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex )
			{
				const auto sliceIndexF = static_cast< vertex_data_t >(sliceIndex);
				const auto sliceIndexPlusOneF = static_cast< vertex_data_t >(sliceIndex + 1);

				/* Texture coordinates.
				 * NOTE : Inverted for Vulkan. */
				const auto texCoordU = one - (sliceIndex == 0 ? 0 : deltaU * sliceIndexF);
				const auto nextTexCoordU = one - (sliceIndex == slices - 1 ? 1 : deltaU * sliceIndexPlusOneF);

				const auto theta = stepSizeSlice * sliceIndexF;
				const auto thetaNext = sliceIndex == slices - 1 ? 0 : stepSizeSlice * sliceIndexPlusOneF;

				const auto cosTheta = std::cos(theta);
				const auto sinTheta = std::sin(theta);
				const auto cosThetaNext = std::cos(thetaNext);
				const auto sinThetaNext = std::sin(thetaNext);

				/* Inner First */
				positions[0] = {cosTheta * currentRadius, currentY, sinTheta * currentRadius};
				textureCoordinates[0] = {texCoordU, texCoordV, 0};
				normals[0] = {length * cosTheta, yNormal, length * sinTheta};
				normals[0].normalize();

				/* Outer First (same normal as Inner First — constant along generatrix) */
				positions[1] = {cosTheta * nextRadius, nextY, sinTheta * nextRadius};
				textureCoordinates[1] = {texCoordU, nextV, 0};
				normals[1] = normals[0];

				/* Inner second */
				positions[2] = {cosThetaNext * currentRadius, currentY, sinThetaNext * currentRadius};
				textureCoordinates[2] = {nextTexCoordU, texCoordV, 0};
				normals[2] = {length * cosThetaNext, yNormal, length * sinThetaNext};
				normals[2].normalize();

				/* Outer second (same normal as Inner second) */
				positions[3] = {cosThetaNext * nextRadius, nextY, sinThetaNext * nextRadius};
				textureCoordinates[3] = {nextTexCoordU, nextV, 0};
				normals[3] = normals[2];

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setNormal(normals[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.setVertexColor((normals[0][Math::X] + one) * half, (normals[0][Math::Y] + one) * half, (normals[0][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setNormal(normals[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.setVertexColor((normals[1][Math::X] + one) * half, (normals[1][Math::Y] + one) * half, (normals[1][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setNormal(normals[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.setVertexColor((normals[2][Math::X] + one) * half, (normals[2][Math::Y] + one) * half, (normals[2][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setNormal(normals[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
				builder.setVertexColor((normals[3][Math::X] + one) * half, (normals[3][Math::Y] + one) * half, (normals[3][Math::Z] + one) * half);
				builder.newVertex();

				builder.resetCurrentTriangle();
			}
		}

		if ( baseHasCap || topHasCap )
		{
			/* Switch to cap UV multiplier for cap geometry. */
			const auto savedMultiplier = builder.options().textureCoordinatesMultiplier();
			builder.options().setTextureCoordinatesMultiplier(builder.options().capTextureCoordinatesMultiplier());

			const auto volumetricColor = [half, one] (const Math::Vector< 3, vertex_data_t > & p) {
				return Math::Vector< 4, vertex_data_t >{
					(p[Math::X] + one) * half,
					(p[Math::Y] + one) * half,
					(p[Math::Z] + one) * half,
					one
				};
			};

			const auto emitDiskCap = [&] (vertex_data_t y, vertex_data_t capRadius, bool faceUp) {
				const Math::Vector< 3, vertex_data_t > normal{0, faceUp ? -one : one, 0};
				const Math::Vector< 3, vertex_data_t > center{0, y, 0};

				for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex )
				{
					const auto theta = stepSizeSlice * static_cast< vertex_data_t >(sliceIndex);
					const auto thetaNext = sliceIndex + 1 == slices ? static_cast< vertex_data_t >(0) : stepSizeSlice * static_cast< vertex_data_t >(sliceIndex + 1);

					const auto cosA = std::cos(theta);
					const auto sinA = std::sin(theta);
					const auto cosB = std::cos(thetaNext);
					const auto sinB = std::sin(thetaNext);

					const Math::Vector< 3, vertex_data_t > edgeA{cosA * capRadius, y, sinA * capRadius};
					const Math::Vector< 3, vertex_data_t > edgeB{cosB * capRadius, y, sinB * capRadius};

					vertex_data_t texAU, texAV, texBU, texBV, texCU, texCV;

					if ( capMapping == CapUVMapping::Planar )
					{
						/* Planar UV projection: map XZ position [-r, +r] to [0, 1]. */
						texAU = cosA * half + half; texAV = sinA * half + half;
						texBU = cosB * half + half; texBV = sinB * half + half;
						texCU = half; texCV = half;
					}
					else /* PerSegment */
					{
						/* Each triangle gets its own [0,1] UV space. */
						texAU = 0; texAV = 0;
						texBU = one; texBV = 0;
						texCU = half; texCV = one;
					}

					if ( faceUp )
					{
						builder.setNormal(normal); builder.setPosition(edgeB); builder.setTextureCoordinates(texBU, texBV); builder.setVertexColor(volumetricColor(edgeB)); builder.newVertex();
						builder.setNormal(normal); builder.setPosition(edgeA); builder.setTextureCoordinates(texAU, texAV); builder.setVertexColor(volumetricColor(edgeA)); builder.newVertex();
						builder.setNormal(normal); builder.setPosition(center); builder.setTextureCoordinates(texCU, texCV); builder.setVertexColor(volumetricColor(center)); builder.newVertex();
					}
					else
					{
						builder.setNormal(normal); builder.setPosition(edgeA); builder.setTextureCoordinates(texAU, texAV); builder.setVertexColor(volumetricColor(edgeA)); builder.newVertex();
						builder.setNormal(normal); builder.setPosition(edgeB); builder.setTextureCoordinates(texBU, texBV); builder.setVertexColor(volumetricColor(edgeB)); builder.newVertex();
						builder.setNormal(normal); builder.setPosition(center); builder.setTextureCoordinates(texCU, texCV); builder.setVertexColor(volumetricColor(center)); builder.newVertex();
					}

					builder.resetCurrentTriangle();
				}
			};

			if ( topHasCap )
			{
				/* Top cap at Y = -length (topRadius), facing up (Y-). */
				emitDiskCap(-length, topRadius, true);
			}

			if ( baseHasCap )
			{
				/* Base cap at Y = 0 (baseRadius), facing down (Y+). */
				emitDiskCap(static_cast< vertex_data_t >(0), baseRadius, false);
			}

			/* Restore surface UV multiplier. */
			builder.options().setTextureCoordinatesMultiplier(savedMultiplier);
		}

		builder.endConstruction();

		/* Fix tangent vectors analytically for this revolution surface.
		 * Surface vertices: circumferential tangent (-z, 0, x) / r fixes the UV seam.
		 * Cap vertices: uniform tangent so TBN is identical across the whole flat cap,
		 * regardless of UV mapping mode (Planar or PerSegment). */
		{
			constexpr auto epsilon = static_cast< vertex_data_t >(1e-6);

			for ( auto & vertex : shape.vertices() )
			{
				const auto & normal = vertex.normal();
				const auto absNY = std::abs(normal[Math::Y]);

				if ( absNY > one - epsilon )
				{
					/* Cap vertex: uniform tangent so that cross(N, T) = (0,0,1). */
					const auto tx = normal[Math::Y] < 0 ? one : -one;
					vertex.setTangent(Math::Vector< 3, vertex_data_t >{tx, 0, 0});
				}
				else
				{
					/* Surface vertex: circumferential tangent for UV seam fix. */
					const auto & pos = vertex.position();
					const auto r = std::sqrt(pos[Math::X] * pos[Math::X] + pos[Math::Z] * pos[Math::Z]);

					if ( r > epsilon )
					{
						vertex.setTangent(Math::Vector< 3, vertex_data_t >{-pos[Math::Z] / r, 0, pos[Math::X] / r});
					}
				}
			}
		}

		return shape;
	}

	/**
	 * @brief Generates a cone shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the base cylinder. Default 1.
	 * @param length The length of the cone shape. Default 1.
	 * @param slices Slices precision of the cone shape. Default 8.
	 * @param stacks Stack precision of the cone shape. Default 1.
	 * @param capMapping UV mapping mode for the base cap. Default None.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCone (vertex_data_t radius = 1, vertex_data_t length = 1, index_data_t slices = 8, index_data_t stacks = 1, CapUVMapping capMapping = CapUVMapping::None, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateCylinder(radius, static_cast< vertex_data_t >(0), length, slices, stacks, capMapping, options);
	}

	/**
	 * @brief Generates a disk shape facing the sky (Y-).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param outerRadius The radius of the outer part of the disk. Default 1.
	 * @param innerRadius The radius of the inner part of the disk. Default 0.5.
	 * @param slices Slices precision of the disk shape. Default 8.
	 * @param stacks Stack precision of the disk shape. Default 1.
	 * @param uvMapping UV mapping mode: Planar projects texture over the whole disk, PerSegment tiles each quad. Default Planar.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateDisk (vertex_data_t outerRadius = 1, vertex_data_t innerRadius = 0.5, index_data_t slices = 8, index_data_t stacks = 1, CapUVMapping uvMapping = CapUVMapping::Planar, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{slices * stacks * 2};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		if ( innerRadius > outerRadius )
		{
			std::swap(innerRadius, outerRadius);
		}

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto stepSizeRadial = (outerRadius - innerRadius) / stacks;
		const auto stepSizeSlice = (2 * std::numbers::pi_v< vertex_data_t >) / slices;
		const auto radialScale = one / outerRadius;

		std::array< Math::Vector< 3, vertex_data_t >, 4 > positions{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > textureCoordinates{};

		builder.beginConstruction(ConstructionMode::TriangleStrip);
		builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeY());

		for ( index_data_t stackIndex = 0; stackIndex < stacks; ++stackIndex )
		{
			for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex )
			{
				const auto inner = innerRadius + stackIndex * stepSizeRadial;
				const auto outer = innerRadius + (stackIndex + 1) * stepSizeRadial;

				const auto theta = stepSizeSlice * sliceIndex;
				const auto thetaNext = sliceIndex == slices - 1 ? zero : stepSizeSlice * (sliceIndex + 1);

				const auto cosTheta = std::cos(theta);
				const auto sinTheta = std::sin(theta);
				const auto cosThetaNext = std::cos(thetaNext);
				const auto sinThetaNext = std::sin(thetaNext);

				/* Inner First */
				positions[0] = {cosTheta * inner, 0, sinTheta * inner};

				/* Inner Second */
				positions[1] = {cosThetaNext * inner, 0, sinThetaNext * inner};

				/* Outer First */
				positions[2] = {cosTheta * outer, 0, sinTheta * outer};

				/* Outer Second */
				positions[3] = {cosThetaNext * outer, 0, sinThetaNext * outer};

				if ( uvMapping == CapUVMapping::PerSegment )
				{
					/* Each quad gets its own [0,1]x[0,1] UV space. */
					textureCoordinates[0] = {zero, zero, zero};
					textureCoordinates[1] = {one, zero, zero};
					textureCoordinates[2] = {zero, one, zero};
					textureCoordinates[3] = {one, one, zero};
				}
				else
				{
					/* Planar UV: map XZ position [-outerR, +outerR] to [0, 1]. */
					textureCoordinates[0] = {(positions[0][Math::X] * radialScale + one) * half, (positions[0][Math::Z] * radialScale + one) * half, zero};
					textureCoordinates[1] = {(positions[1][Math::X] * radialScale + one) * half, (positions[1][Math::Z] * radialScale + one) * half, zero};
					textureCoordinates[2] = {(positions[2][Math::X] * radialScale + one) * half, (positions[2][Math::Z] * radialScale + one) * half, zero};
					textureCoordinates[3] = {(positions[3][Math::X] * radialScale + one) * half, (positions[3][Math::Z] * radialScale + one) * half, zero};
				}

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.setVertexColor((positions[0][Math::X] / outerRadius + one) * half, half, (positions[0][Math::Z] / outerRadius + one) * half);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.setVertexColor((positions[1][Math::X] / outerRadius + one) * half, half, (positions[1][Math::Z] / outerRadius + one) * half);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.setVertexColor((positions[2][Math::X] / outerRadius + one) * half, half, (positions[2][Math::Z] / outerRadius + one) * half);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
				builder.setVertexColor((positions[3][Math::X] / outerRadius + one) * half, half, (positions[3][Math::Z] / outerRadius + one) * half);
				builder.newVertex();

				builder.resetCurrentTriangle();
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a torus shape.
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param majorRadius The major radius of the torus shape. Default 1.
	 * @param minorRadius The minor radius of the torus shape. Default 0.5.
	 * @param slices Slices precision of the torus shape. Default 8.
	 * @param stacks Stack precision of the torus shape. Default 8.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateTorus (vertex_data_t majorRadius = 1, vertex_data_t minorRadius = 0.5, index_data_t slices = 8, index_data_t stacks = 8, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{stacks * slices * 2};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto stacksF = static_cast< vertex_data_t >(stacks);
		const auto slicesF = static_cast< vertex_data_t >(slices);

		const auto majorStep = (2 * std::numbers::pi_v< vertex_data_t >) / stacksF;
		const auto minorStep = (2 * std::numbers::pi_v< vertex_data_t >) / slicesF;

		std::array< Math::Vector< 3, vertex_data_t >, 4 > positions{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > normals{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > textureCoordinates{};

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		for ( index_data_t stackIndex = 0; stackIndex < stacks; ++stackIndex )
		{
			const auto stackIndexF = static_cast< vertex_data_t >(stackIndex);

			const auto tempXZA = stackIndexF * majorStep;
			const auto positionXA = std::cos(tempXZA);
			const auto positionZA = std::sin(tempXZA);

			const auto tempXZB = tempXZA + majorStep;
			const auto positionXB = std::cos(tempXZB);
			const auto positionZB = std::sin(tempXZB);

			/* Texture coordinates U.
			 * NOTE: Inverted for Vulkan. */
			const auto texCoordUA = stackIndexF / stacksF;
			const auto texCoordUB = (stackIndexF + one) / stacksF;

			for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex )
			{
				const auto sliceIndexF = static_cast< vertex_data_t >(sliceIndex);

				const auto stepB = sliceIndexF * minorStep;
				const auto norm = std::cos(stepB);
				const auto radius = minorRadius * norm + majorRadius;
				const auto positionY = minorRadius * std::sin(stepB);

				const auto nextStepB = (sliceIndexF + one) * minorStep;
				const auto nextNorm = std::cos(nextStepB);
				const auto nextRadius = minorRadius * nextNorm + majorRadius;
				const auto nextPositionY = minorRadius * std::sin(nextStepB);

				/* Texture coordinates V.
				 * NOTE: V is inverted for Vulkan. */
				const auto texCoordV = one - (sliceIndexF / slicesF);
				const auto nextTexCoordV = one - ((sliceIndexF + one) / slicesF);

				/* First point (stack A, current slice) */
				positions[0] = {positionXA * radius, positionY, positionZA * radius};
				textureCoordinates[0] = {texCoordUA, texCoordV, 0};
				normals[0] = {positionXA * norm, positionY / minorRadius, positionZA * norm};

				/* Second point (stack B, current slice) */
				positions[1] = {positionXB * radius, positionY, positionZB * radius};
				textureCoordinates[1] = {texCoordUB, texCoordV, 0};
				normals[1] = {positionXB * norm, positionY / minorRadius, positionZB * norm};

				/* Third point (stack A, next slice) */
				positions[2] = {positionXA * nextRadius, nextPositionY, positionZA * nextRadius};
				textureCoordinates[2] = {texCoordUA, nextTexCoordV, 0};
				normals[2] = {positionXA * nextNorm, nextPositionY / minorRadius, positionZA * nextNorm};

				/* Fourth point (stack B, next slice) */
				positions[3] = {positionXB * nextRadius, nextPositionY, positionZB * nextRadius};
				textureCoordinates[3] = {texCoordUB, nextTexCoordV, 0};
				normals[3] = {positionXB * nextNorm, nextPositionY / minorRadius, positionZB * nextNorm};

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setNormal(normals[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.setVertexColor((normals[0][Math::X] + one) * half, (normals[0][Math::Y] + one) * half, (normals[0][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setNormal(normals[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.setVertexColor((normals[1][Math::X] + one) * half, (normals[1][Math::Y] + one) * half, (normals[1][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setNormal(normals[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.setVertexColor((normals[2][Math::X] + one) * half, (normals[2][Math::Y] + one) * half, (normals[2][Math::Z] + one) * half);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setNormal(normals[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
				builder.setVertexColor((normals[3][Math::X] + one) * half, (normals[3][Math::Y] + one) * half, (normals[3][Math::Z] + one) * half);
				builder.newVertex();

				builder.resetCurrentTriangle();
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a tetrahedron shape (Fire symbol).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The circumscribed sphere radius. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateTetrahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{4};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		const auto oneThird = one / static_cast< vertex_data_t >(3);

		/* Regular tetrahedron inscribed in a sphere of the given radius.
		 * All 4 vertices lie on the sphere at distance radius from origin.
		 * Edge length: a = radius · 2√6/3 ≈ radius · 1.633 */
		const auto sqrt2 = std::sqrt(static_cast< vertex_data_t >(2));
		const auto sqrt6 = std::sqrt(static_cast< vertex_data_t >(6));
		const auto sqrt2Over3 = sqrt2 * oneThird;
		const auto sqrt6Over3 = sqrt6 * oneThird;
		const auto twoSqrt2Over3 = static_cast< vertex_data_t >(2) * sqrt2Over3;

		/* Vertices (Y- is up in Vulkan convention). */
		const Math::Vector< 3, vertex_data_t > top       {0,                       -radius,              0};
		const Math::Vector< 3, vertex_data_t > back      {0,                        radius * oneThird,    radius * twoSqrt2Over3};
		const Math::Vector< 3, vertex_data_t > frontLeft {-radius * sqrt6Over3,     radius * oneThird,   -radius * sqrt2Over3};
		const Math::Vector< 3, vertex_data_t > frontRight{ radius * sqrt6Over3,     radius * oneThird,   -radius * sqrt2Over3};

		/* Face normals (outward-pointing, already unit length). */
		const Math::Vector< 3, vertex_data_t > normalFront {0,            -oneThird,  -twoSqrt2Over3};
		const Math::Vector< 3, vertex_data_t > normalRight { sqrt6Over3,  -oneThird,   sqrt2Over3};
		const Math::Vector< 3, vertex_data_t > normalLeft  {-sqrt6Over3,  -oneThird,   sqrt2Over3};
		const Math::Vector< 3, vertex_data_t > normalBottom{0,             one,         0};

		/* UV mapping: each face mapped to the same equilateral triangle. */
		const Math::Vector< 3, vertex_data_t > uvA{0, one, 0};
		const Math::Vector< 3, vertex_data_t > uvB{one, one, 0};
		const Math::Vector< 3, vertex_data_t > uvC{half, 0, 0};

		/* Volumetric vertex color: position [-radius, radius] → RGB [0, 1]. */
		const auto invRadius = one / radius;
		const auto volumetricColor = [half, one, invRadius](const Math::Vector< 3, vertex_data_t > & pos) {
			return Math::Vector< 4, vertex_data_t >{
				(pos[Math::X] * invRadius + one) * half,
				(pos[Math::Y] * invRadius + one) * half,
				(pos[Math::Z] * invRadius + one) * half,
				one
			};
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* Front face (frontLeft, top, frontRight) */
		builder.setPosition(frontLeft);
		builder.setNormal(normalFront);
		builder.setTextureCoordinates(uvA);
		builder.setVertexColor(volumetricColor(frontLeft));
		builder.newVertex();
		builder.setPosition(top);
		builder.setNormal(normalFront);
		builder.setTextureCoordinates(uvB);
		builder.setVertexColor(volumetricColor(top));
		builder.newVertex();
		builder.setPosition(frontRight);
		builder.setNormal(normalFront);
		builder.setTextureCoordinates(uvC);
		builder.setVertexColor(volumetricColor(frontRight));
		builder.newVertex();

		/* Right face (frontRight, top, back) */
		builder.setPosition(frontRight);
		builder.setNormal(normalRight);
		builder.setTextureCoordinates(uvA);
		builder.setVertexColor(volumetricColor(frontRight));
		builder.newVertex();
		builder.setPosition(top);
		builder.setNormal(normalRight);
		builder.setTextureCoordinates(uvB);
		builder.setVertexColor(volumetricColor(top));
		builder.newVertex();
		builder.setPosition(back);
		builder.setNormal(normalRight);
		builder.setTextureCoordinates(uvC);
		builder.setVertexColor(volumetricColor(back));
		builder.newVertex();

		/* Left face (back, top, frontLeft) */
		builder.setPosition(back);
		builder.setNormal(normalLeft);
		builder.setTextureCoordinates(uvA);
		builder.setVertexColor(volumetricColor(back));
		builder.newVertex();
		builder.setPosition(top);
		builder.setNormal(normalLeft);
		builder.setTextureCoordinates(uvB);
		builder.setVertexColor(volumetricColor(top));
		builder.newVertex();
		builder.setPosition(frontLeft);
		builder.setNormal(normalLeft);
		builder.setTextureCoordinates(uvC);
		builder.setVertexColor(volumetricColor(frontLeft));
		builder.newVertex();

		/* Bottom face (frontRight, back, frontLeft) */
		builder.setPosition(frontRight);
		builder.setNormal(normalBottom);
		builder.setTextureCoordinates(uvA);
		builder.setVertexColor(volumetricColor(frontRight));
		builder.newVertex();
		builder.setPosition(back);
		builder.setNormal(normalBottom);
		builder.setTextureCoordinates(uvB);
		builder.setVertexColor(volumetricColor(back));
		builder.newVertex();
		builder.setPosition(frontLeft);
		builder.setNormal(normalBottom);
		builder.setTextureCoordinates(uvC);
		builder.setVertexColor(volumetricColor(frontLeft));
		builder.newVertex();

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a hexahedron shape (Earth symbol).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The circumscribed sphere radius. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateHexahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		/* Regular cube inscribed in a sphere of the given radius.
		 * Half-edge h = r/√3, so full side = 2r/√3.
		 * Edge length: a = 2r·√3/3 ≈ r · 1.155 */
		const auto side = static_cast< vertex_data_t >(2) * radius / std::sqrt(static_cast< vertex_data_t >(3));

		return generateCuboid(side, side, side, options);
	}

	/**
	 * @brief Generates an octahedron shape (Air symbol).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The circumscribed sphere radius. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateOctahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{8};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		/* Regular octahedron inscribed in a sphere of the given radius.
		 * 6 axis-aligned vertices, all at distance radius from origin.
		 * Edge length: a = radius · √2 ≈ radius · 1.414 */
		const Math::Vector< 3, vertex_data_t > top  {0,        -radius, 0};
		const Math::Vector< 3, vertex_data_t > bot  {0,         radius, 0};
		const Math::Vector< 3, vertex_data_t > front{0,         0,      radius};
		const Math::Vector< 3, vertex_data_t > back {0,         0,     -radius};
		const Math::Vector< 3, vertex_data_t > left {-radius,   0,      0};
		const Math::Vector< 3, vertex_data_t > right{ radius,   0,      0};

		/* Face normals: (±1, ±1, ±1)/√3, already unit length. */
		const auto s = one / std::sqrt(static_cast< vertex_data_t >(3));

		/* UV mapping: each face mapped to the same triangle. */
		const Math::Vector< 3, vertex_data_t > uvA{0, one, 0};
		const Math::Vector< 3, vertex_data_t > uvB{one, one, 0};
		const Math::Vector< 3, vertex_data_t > uvC{half, 0, 0};

		/* Volumetric vertex color: position [-radius, radius] → RGB [0, 1]. */
		const auto invRadius = one / radius;
		const auto volumetricColor = [half, one, invRadius](const Math::Vector< 3, vertex_data_t > & pos) {
			return Math::Vector< 4, vertex_data_t >{
				(pos[Math::X] * invRadius + one) * half,
				(pos[Math::Y] * invRadius + one) * half,
				(pos[Math::Z] * invRadius + one) * half,
				one
			};
		};

		/* Helper to emit one triangular face with flat normal. */
		const auto emitFace = [&](
			const Math::Vector< 3, vertex_data_t > & vA,
			const Math::Vector< 3, vertex_data_t > & vB,
			const Math::Vector< 3, vertex_data_t > & vC,
			const Math::Vector< 3, vertex_data_t > & normal)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(uvA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(uvB);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(uvC);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* Top pyramid (Y- is up) */
		emitFace(right, front, top, { s, -s,  s});  /* Front-right */
		emitFace(back,  right, top, { s, -s, -s});  /* Back-right */
		emitFace(left,  back,  top, {-s, -s, -s});  /* Back-left */
		emitFace(front, left,  top, {-s, -s,  s});  /* Front-left */

		/* Bottom pyramid */
		emitFace(front, right, bot, { s,  s,  s});  /* Front-right */
		emitFace(right, back,  bot, { s,  s, -s});  /* Back-right */
		emitFace(back,  left,  bot, {-s,  s, -s});  /* Back-left */
		emitFace(left,  front, bot, {-s,  s,  s});  /* Front-left */

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a dodecahedron shape (Ether, Universe symbol).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The circumscribed sphere radius. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateDodecahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{36};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		/* Golden ratio and derived constants for vertex coordinates. */
		const auto phi = (one + std::sqrt(static_cast< vertex_data_t >(5))) * half;
		const auto invPhi = one / phi;
		const auto s = one / std::sqrt(static_cast< vertex_data_t >(3));

		/* Regular dodecahedron inscribed in a sphere of the given radius.
		 * 20 vertices from a cube (±1,±1,±1)/√3 and 3 mutually orthogonal
		 * golden rectangles (0,±1/φ,±φ)/√3, (±1/φ,±φ,0)/√3, (±φ,0,±1/φ)/√3.
		 * Edge length: a = radius · 4/(φ²√3) ≈ radius · 0.714 */
		const auto a = radius * s;
		const auto b = radius * phi * s;
		const auto c = radius * invPhi * s;

		/* 8 cube vertices */
		const Math::Vector< 3, vertex_data_t > v0 { a,  a,  a};
		const Math::Vector< 3, vertex_data_t > v1 { a,  a, -a};
		const Math::Vector< 3, vertex_data_t > v2 { a, -a,  a};
		const Math::Vector< 3, vertex_data_t > v3 { a, -a, -a};
		const Math::Vector< 3, vertex_data_t > v4 {-a,  a,  a};
		const Math::Vector< 3, vertex_data_t > v5 {-a,  a, -a};
		const Math::Vector< 3, vertex_data_t > v6 {-a, -a,  a};
		const Math::Vector< 3, vertex_data_t > v7 {-a, -a, -a};

		/* 4 vertices on YZ golden rectangle */
		const Math::Vector< 3, vertex_data_t > v8  {0,  c,  b};
		const Math::Vector< 3, vertex_data_t > v9  {0,  c, -b};
		const Math::Vector< 3, vertex_data_t > v10 {0, -c,  b};
		const Math::Vector< 3, vertex_data_t > v11 {0, -c, -b};

		/* 4 vertices on XY golden rectangle */
		const Math::Vector< 3, vertex_data_t > v12 { c,  b, 0};
		const Math::Vector< 3, vertex_data_t > v13 { c, -b, 0};
		const Math::Vector< 3, vertex_data_t > v14 {-c,  b, 0};
		const Math::Vector< 3, vertex_data_t > v15 {-c, -b, 0};

		/* 4 vertices on XZ golden rectangle */
		const Math::Vector< 3, vertex_data_t > v16 { b, 0,  c};
		const Math::Vector< 3, vertex_data_t > v17 { b, 0, -c};
		const Math::Vector< 3, vertex_data_t > v18 {-b, 0,  c};
		const Math::Vector< 3, vertex_data_t > v19 {-b, 0, -c};

		/* Volumetric vertex color: position [-radius, radius] → RGB [0, 1]. */
		const auto invRadius = one / radius;
		const auto volumetricColor = [half, one, invRadius](const Math::Vector< 3, vertex_data_t > & pos) {
			return Math::Vector< 4, vertex_data_t >{
				(pos[Math::X] * invRadius + one) * half,
				(pos[Math::Y] * invRadius + one) * half,
				(pos[Math::Z] * invRadius + one) * half,
				one
			};
		};

		/* Emit one triangle with flat normal, per-vertex UV, and volumetric vertex color. */
		const auto emitTriangle = [&](
			const Math::Vector< 3, vertex_data_t > & vA,
			const Math::Vector< 3, vertex_data_t > & vB,
			const Math::Vector< 3, vertex_data_t > & vC,
			const Math::Vector< 3, vertex_data_t > & normal,
			const Math::Vector< 3, vertex_data_t > & tcA,
			const Math::Vector< 3, vertex_data_t > & tcB,
			const Math::Vector< 3, vertex_data_t > & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		/* Emit a pentagonal face as 3 triangles (fan from p0).
		 * The outward normal is the normalized sum of vertex positions
		 * (valid for any centrosymmetric solid centered at the origin).
		 * UV coordinates are projected onto the face plane and remapped
		 * to [0,1]x[0,1] so each face frames a full texture. */
		const auto emitPentagon = [&](
			const Math::Vector< 3, vertex_data_t > & p0,
			const Math::Vector< 3, vertex_data_t > & p1,
			const Math::Vector< 3, vertex_data_t > & p2,
			const Math::Vector< 3, vertex_data_t > & p3,
			const Math::Vector< 3, vertex_data_t > & p4)
		{
			using Vec3 = Math::Vector< 3, vertex_data_t >;

			const auto sum = p0 + p1 + p2 + p3 + p4;
			const auto normal = sum.normalized();

			/* Centroid of the pentagon. */
			const auto center = sum / static_cast< vertex_data_t >(5);

			/* Build local tangent frame on the face plane. */
			const auto T = (p0 - center).normalized();
			const auto B = Vec3::crossProduct(normal, T);

			/* Project each vertex into 2D local coordinates (u along T, v along B). */
			const auto lu0 = Vec3::dotProduct(p0 - center, T), lv0 = Vec3::dotProduct(p0 - center, B);
			const auto lu1 = Vec3::dotProduct(p1 - center, T), lv1 = Vec3::dotProduct(p1 - center, B);
			const auto lu2 = Vec3::dotProduct(p2 - center, T), lv2 = Vec3::dotProduct(p2 - center, B);
			const auto lu3 = Vec3::dotProduct(p3 - center, T), lv3 = Vec3::dotProduct(p3 - center, B);
			const auto lu4 = Vec3::dotProduct(p4 - center, T), lv4 = Vec3::dotProduct(p4 - center, B);

			/* Find bounding box and remap to [0,1]. */
			const auto uMin = std::min({lu0, lu1, lu2, lu3, lu4});
			const auto uMax = std::max({lu0, lu1, lu2, lu3, lu4});
			const auto vMin = std::min({lv0, lv1, lv2, lv3, lv4});
			const auto vMax = std::max({lv0, lv1, lv2, lv3, lv4});

			const auto invU = one / (uMax - uMin);
			const auto invV = one / (vMax - vMin);

			const Vec3 tc0{(lu0 - uMin) * invU, (lv0 - vMin) * invV, 0};
			const Vec3 tc1{(lu1 - uMin) * invU, (lv1 - vMin) * invV, 0};
			const Vec3 tc2{(lu2 - uMin) * invU, (lv2 - vMin) * invV, 0};
			const Vec3 tc3{(lu3 - uMin) * invU, (lv3 - vMin) * invV, 0};
			const Vec3 tc4{(lu4 - uMin) * invU, (lv4 - vMin) * invV, 0};

			emitTriangle(p0, p1, p2, normal, tc0, tc1, tc2);
			emitTriangle(p0, p2, p3, normal, tc0, tc2, tc3);
			emitTriangle(p0, p3, p4, normal, tc0, tc3, tc4);
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* 12 pentagonal faces (CCW winding for outward-facing normals). */
		emitPentagon(v0, v12, v14, v4,  v8);   /* Top-front */
		emitPentagon(v0, v16, v17, v1,  v12);  /* Right-top */
		emitPentagon(v0, v8,  v10, v2,  v16);  /* Front-right */
		emitPentagon(v1, v9,  v5,  v14, v12);  /* Top-back */
		emitPentagon(v1, v17, v3,  v11, v9);   /* Back-right */
		emitPentagon(v2, v13, v3,  v17, v16);  /* Bottom-right */
		emitPentagon(v2, v10, v6,  v15, v13);  /* Front-bottom */
		emitPentagon(v4, v18, v6,  v10, v8);   /* Front-left */
		emitPentagon(v4, v14, v5,  v19, v18);  /* Left-top */
		emitPentagon(v5, v9,  v11, v7,  v19);  /* Back-left */
		emitPentagon(v6, v18, v19, v7,  v15);  /* Bottom-left */
		emitPentagon(v3, v13, v15, v7,  v11);  /* Bottom-back */

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates an icosahedron shape (Water symbol).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The circumscribed sphere radius. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateIcosahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{20};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		/* Regular icosahedron inscribed in a sphere of the given radius.
		 * 12 vertices from 3 mutually orthogonal golden rectangles:
		 *   (0, ±1, ±φ)/√(1+φ²),  (±φ, 0, ±1)/√(1+φ²),  (±1, ±φ, 0)/√(1+φ²)
		 * scaled by radius so all vertices lie on the circumscribed sphere.
		 * Edge length: a = radius · 2/√(1+φ²) ≈ radius · 1.0515 */
		const auto phi = (one + std::sqrt(static_cast< vertex_data_t >(5))) * half;
		const auto s = radius / std::sqrt(one + phi * phi);

		const auto p = phi * s; /* long half-axis of golden rectangle */

		/* YZ golden rectangle (4 vertices) */
		const Math::Vector< 3, vertex_data_t > v0  {0,  s,  p};
		const Math::Vector< 3, vertex_data_t > v1  {0,  s, -p};
		const Math::Vector< 3, vertex_data_t > v2  {0, -s,  p};
		const Math::Vector< 3, vertex_data_t > v3  {0, -s, -p};

		/* XZ golden rectangle (4 vertices) */
		const Math::Vector< 3, vertex_data_t > v4  { p, 0,  s};
		const Math::Vector< 3, vertex_data_t > v5  { p, 0, -s};
		const Math::Vector< 3, vertex_data_t > v6  {-p, 0,  s};
		const Math::Vector< 3, vertex_data_t > v7  {-p, 0, -s};

		/* XY golden rectangle (4 vertices) */
		const Math::Vector< 3, vertex_data_t > v8  { s,  p, 0};
		const Math::Vector< 3, vertex_data_t > v9  {-s,  p, 0};
		const Math::Vector< 3, vertex_data_t > v10 { s, -p, 0};
		const Math::Vector< 3, vertex_data_t > v11 {-s, -p, 0};

		/* UV mapping: each triangle gets the same coordinates. */
		const Math::Vector< 3, vertex_data_t > uvA{0, one, 0};
		const Math::Vector< 3, vertex_data_t > uvB{one, one, 0};
		const Math::Vector< 3, vertex_data_t > uvC{half, 0, 0};

		/* Volumetric vertex color: position [-radius, radius] → RGB [0, 1]. */
		const auto invRadius = one / radius;
		const auto volumetricColor = [half, one, invRadius](const Math::Vector< 3, vertex_data_t > & pos) {
			return Math::Vector< 4, vertex_data_t >{
				(pos[Math::X] * invRadius + one) * half,
				(pos[Math::Y] * invRadius + one) * half,
				(pos[Math::Z] * invRadius + one) * half,
				one
			};
		};

		/* Emit one triangle with flat normal and volumetric vertex color.
		 * The outward normal is the normalized sum of the 3 vertex positions
		 * (valid for any centrosymmetric solid centered at the origin). */
		const auto emitFace = [&](
			const Math::Vector< 3, vertex_data_t > & vA,
			const Math::Vector< 3, vertex_data_t > & vB,
			const Math::Vector< 3, vertex_data_t > & vC)
		{
			auto normal = vA + vB + vC;
			normal.normalize();

			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(uvA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(uvB);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(uvC);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* 20 triangular faces (CCW winding for outward-facing normals).
		 *
		 * Connectivity: each vertex is shared by 5 faces.
		 * The 20 faces split into groups around the top cap (Y+),
		 * upper band, lower band, and bottom cap (Y-). */

		/* 5 faces around top vertex cap (v8, v9 are the Y+ pair) */
		emitFace(v0, v4, v8);
		emitFace(v0, v8, v9);
		emitFace(v0, v9, v6);
		emitFace(v1, v8, v5);
		emitFace(v1, v9, v8);

		/* 5 faces upper-middle band */
		emitFace(v1, v7, v9);
		emitFace(v4, v5, v8);
		emitFace(v0, v2, v4);
		emitFace(v0, v6, v2);
		emitFace(v6, v9, v7);

		/* 5 faces lower-middle band */
		emitFace(v2, v10, v4);
		emitFace(v4, v10, v5);
		emitFace(v5, v10, v3);
		emitFace(v2, v6, v11);
		emitFace(v6, v7, v11);

		/* 5 faces around bottom vertex cap (v10, v11 are the Y- pair) */
		emitFace(v3, v10, v11);
		emitFace(v2, v11, v10);
		emitFace(v3, v1, v5);
		emitFace(v3, v7, v1);
		emitFace(v3, v11, v7);

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a subdivided plane on the XZ plane.
	 * @note Ready for vulkan default world axis. Normal points Y- (up).
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param width The width (X axis) of the plane. Default 1.
	 * @param depth The depth (Z axis) of the plane. Default 1.
	 * @param subdivisionsX Number of subdivisions along X. Default 1.
	 * @param subdivisionsZ Number of subdivisions along Z. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generatePlane (vertex_data_t width = 1, vertex_data_t depth = 1, index_data_t subdivisionsX = 1, index_data_t subdivisionsZ = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{subdivisionsX * subdivisionsZ * 2};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto halfWidth = width * half;
		const auto halfDepth = depth * half;
		const auto invSubX = one / static_cast< vertex_data_t >(subdivisionsX);
		const auto invSubZ = one / static_cast< vertex_data_t >(subdivisionsZ);
		const auto stepX = width * invSubX;
		const auto stepZ = depth * invSubZ;

		/* Normal pointing Y- (up in Vulkan). */
		builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeY());

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		for ( index_data_t zIdx = 0; zIdx < subdivisionsZ; ++zIdx )
		{
			const auto z0 = -halfDepth + static_cast< vertex_data_t >(zIdx) * stepZ;
			const auto z1 = -halfDepth + static_cast< vertex_data_t >(zIdx + 1) * stepZ;
			const auto texV0 = static_cast< vertex_data_t >(zIdx) * invSubZ;
			const auto texV1 = static_cast< vertex_data_t >(zIdx + 1) * invSubZ;

			/* Volumetric vertex color: G = 0.5 (Y=0), R and B map X and Z. */
			const auto colorB0 = (z0 / halfDepth + one) * half;
			const auto colorB1 = (z1 / halfDepth + one) * half;

			for ( index_data_t xIdx = 0; xIdx < subdivisionsX; ++xIdx )
			{
				const auto x0 = -halfWidth + static_cast< vertex_data_t >(xIdx) * stepX;
				const auto x1 = -halfWidth + static_cast< vertex_data_t >(xIdx + 1) * stepX;
				const auto texU0 = static_cast< vertex_data_t >(xIdx) * invSubX;
				const auto texU1 = static_cast< vertex_data_t >(xIdx + 1) * invSubX;

				const auto colorR0 = (x0 / halfWidth + one) * half;
				const auto colorR1 = (x1 / halfWidth + one) * half;

				builder.setPosition(x0, 0, z0);
				builder.setTextureCoordinates(texU0, texV0);
				builder.setVertexColor(colorR0, half, colorB0);
				builder.newVertex();

				builder.setPosition(x0, 0, z1);
				builder.setTextureCoordinates(texU0, texV1);
				builder.setVertexColor(colorR0, half, colorB1);
				builder.newVertex();

				builder.setPosition(x1, 0, z0);
				builder.setTextureCoordinates(texU1, texV0);
				builder.setVertexColor(colorR1, half, colorB0);
				builder.newVertex();

				builder.setPosition(x1, 0, z1);
				builder.setTextureCoordinates(texU1, texV1);
				builder.setVertexColor(colorR1, half, colorB1);
				builder.newVertex();

				builder.resetCurrentTriangle();
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a capsule shape (cylinder with hemisphere caps).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the capsule cross-section. Default 1.
	 * @param length The length of the cylindrical section (0 gives a sphere). Default 1.
	 * @param slices Longitudinal subdivisions. Default 16.
	 * @param stacks Latitudinal subdivisions per hemisphere. Default 8.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCapsule (vertex_data_t radius = 1, vertex_data_t length = 1, index_data_t slices = 16, index_data_t stacks = 8, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto halfLen = length * half;
		const auto hasCylinder = length > 0;
		const auto totalRows = static_cast< index_data_t >(2) * stacks + (hasCylinder ? 1 : 0);

		Shape< vertex_data_t, index_data_t > shape{totalRows * slices * 2};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto halfPi = pi * half;
		const auto dAngle = halfPi / static_cast< vertex_data_t >(stacks);
		const auto dPhi = (static_cast< vertex_data_t >(2) * pi) / static_cast< vertex_data_t >(slices);

		/* UV V mapping based on arc length. */
		const auto totalArc = pi * radius + length;
		const auto invTotalArc = one / totalArc;
		const auto topHemiArc = halfPi * radius;

		/* Volumetric vertex color extents. */
		const auto halfHeight = halfLen + radius;
		const auto invHalfHeight = one / halfHeight;
		const auto invRadius = one / radius;

		/* Emit one quad (4 vertices) for a strip row segment. */
		const auto emitQuad = [&](
			const Math::Vector< 3, vertex_data_t > & pA,
			const Math::Vector< 3, vertex_data_t > & nA,
			vertex_data_t uA, vertex_data_t vA,
			const Math::Vector< 3, vertex_data_t > & pB,
			const Math::Vector< 3, vertex_data_t > & nB,
			vertex_data_t uB, vertex_data_t vB,
			const Math::Vector< 3, vertex_data_t > & pC,
			const Math::Vector< 3, vertex_data_t > & nC,
			vertex_data_t uC, vertex_data_t vC,
			const Math::Vector< 3, vertex_data_t > & pD,
			const Math::Vector< 3, vertex_data_t > & nD,
			vertex_data_t uD, vertex_data_t vD)
		{
			const auto color = [&](const Math::Vector< 3, vertex_data_t > & p) {
				return Math::Vector< 4, vertex_data_t >{
					(p[Math::X] * invRadius + one) * half,
					(p[Math::Y] * invHalfHeight + one) * half,
					(p[Math::Z] * invRadius + one) * half,
					one
				};
			};

			builder.setPosition(pA);
			builder.setNormal(nA);
			builder.setTextureCoordinates(uA, vA);
			builder.setVertexColor(color(pA));
			builder.newVertex();

			builder.setPosition(pC);
			builder.setNormal(nC);
			builder.setTextureCoordinates(uC, vC);
			builder.setVertexColor(color(pC));
			builder.newVertex();

			builder.setPosition(pB);
			builder.setNormal(nB);
			builder.setTextureCoordinates(uB, vB);
			builder.setVertexColor(color(pB));
			builder.newVertex();

			builder.setPosition(pD);
			builder.setNormal(nD);
			builder.setTextureCoordinates(uD, vD);
			builder.setVertexColor(color(pD));
			builder.newVertex();

			builder.resetCurrentTriangle();
		};

		const auto deltaU = one / static_cast< vertex_data_t >(slices);

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		/* --- Top hemisphere (north pole to equator) --- */
		for ( index_data_t stack = 0; stack < stacks; ++stack )
		{
			/* beta goes from PI/2 (pole) down to 0 (equator). */
			const auto betaA = halfPi - static_cast< vertex_data_t >(stack) * dAngle;
			const auto betaB = halfPi - static_cast< vertex_data_t >(stack + 1) * dAngle;

			const auto cosBetaA = std::cos(betaA);
			const auto sinBetaA = std::sin(betaA);
			const auto cosBetaB = std::cos(betaB);
			const auto sinBetaB = std::sin(betaB);

			const auto yA = -halfLen - radius * sinBetaA;
			const auto yB = -halfLen - radius * sinBetaB;
			const auto rA = radius * cosBetaA;
			const auto rB = radius * cosBetaB;

			/* Arc-length UV V. */
			const auto arcA = (halfPi - betaA) * radius;
			const auto arcB = (halfPi - betaB) * radius;
			const auto texVA = arcA * invTotalArc;
			const auto texVB = arcB * invTotalArc;

			for ( index_data_t slice = 0; slice < slices; ++slice )
			{
				const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
				const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
				const auto cosPhi = std::cos(phi);
				const auto sinPhi = std::sin(phi);
				const auto cosPhiN = std::cos(phiNext);
				const auto sinPhiN = std::sin(phiNext);

				const auto texUA = static_cast< vertex_data_t >(slice) * deltaU;
				const auto texUB = static_cast< vertex_data_t >(slice + 1) * deltaU;

				emitQuad(
					{cosPhi * rA, yA, sinPhi * rA}, {cosPhi * cosBetaA, -sinBetaA, sinPhi * cosBetaA}, texUA, texVA,
					{cosPhi * rB, yB, sinPhi * rB}, {cosPhi * cosBetaB, -sinBetaB, sinPhi * cosBetaB}, texUA, texVB,
					{cosPhiN * rA, yA, sinPhiN * rA}, {cosPhiN * cosBetaA, -sinBetaA, sinPhiN * cosBetaA}, texUB, texVA,
					{cosPhiN * rB, yB, sinPhiN * rB}, {cosPhiN * cosBetaB, -sinBetaB, sinPhiN * cosBetaB}, texUB, texVB
				);
			}
		}

		/* --- Cylinder section (top equator to bottom equator) --- */
		if ( hasCylinder )
		{
			const auto texVTop = topHemiArc * invTotalArc;
			const auto texVBot = (topHemiArc + length) * invTotalArc;

			for ( index_data_t slice = 0; slice < slices; ++slice )
			{
				const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
				const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
				const auto cosPhi = std::cos(phi);
				const auto sinPhi = std::sin(phi);
				const auto cosPhiN = std::cos(phiNext);
				const auto sinPhiN = std::sin(phiNext);

				const auto texUA = static_cast< vertex_data_t >(slice) * deltaU;
				const auto texUB = static_cast< vertex_data_t >(slice + 1) * deltaU;

				emitQuad(
					{cosPhi * radius, -halfLen, sinPhi * radius}, {cosPhi, 0, sinPhi}, texUA, texVTop,
					{cosPhi * radius, halfLen, sinPhi * radius}, {cosPhi, 0, sinPhi}, texUA, texVBot,
					{cosPhiN * radius, -halfLen, sinPhiN * radius}, {cosPhiN, 0, sinPhiN}, texUB, texVTop,
					{cosPhiN * radius, halfLen, sinPhiN * radius}, {cosPhiN, 0, sinPhiN}, texUB, texVBot
				);
			}
		}

		/* --- Bottom hemisphere (equator to south pole) --- */
		for ( index_data_t stack = 0; stack < stacks; ++stack )
		{
			/* alpha goes from 0 (equator) to PI/2 (south pole). */
			const auto alphaA = static_cast< vertex_data_t >(stack) * dAngle;
			const auto alphaB = static_cast< vertex_data_t >(stack + 1) * dAngle;

			const auto cosAlphaA = std::cos(alphaA);
			const auto sinAlphaA = std::sin(alphaA);
			const auto cosAlphaB = std::cos(alphaB);
			const auto sinAlphaB = std::sin(alphaB);

			const auto yA = halfLen + radius * sinAlphaA;
			const auto yB = halfLen + radius * sinAlphaB;
			const auto rA = radius * cosAlphaA;
			const auto rB = radius * cosAlphaB;

			/* Arc-length UV V. */
			const auto arcA = topHemiArc + length + alphaA * radius;
			const auto arcB = topHemiArc + length + alphaB * radius;
			const auto texVA = arcA * invTotalArc;
			const auto texVB = arcB * invTotalArc;

			for ( index_data_t slice = 0; slice < slices; ++slice )
			{
				const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
				const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
				const auto cosPhi = std::cos(phi);
				const auto sinPhi = std::sin(phi);
				const auto cosPhiN = std::cos(phiNext);
				const auto sinPhiN = std::sin(phiNext);

				const auto texUA = static_cast< vertex_data_t >(slice) * deltaU;
				const auto texUB = static_cast< vertex_data_t >(slice + 1) * deltaU;

				emitQuad(
					{cosPhi * rA, yA, sinPhi * rA}, {cosPhi * cosAlphaA, sinAlphaA, sinPhi * cosAlphaA}, texUA, texVA,
					{cosPhi * rB, yB, sinPhi * rB}, {cosPhi * cosAlphaB, sinAlphaB, sinPhi * cosAlphaB}, texUA, texVB,
					{cosPhiN * rA, yA, sinPhiN * rA}, {cosPhiN * cosAlphaA, sinAlphaA, sinPhiN * cosAlphaA}, texUB, texVA,
					{cosPhiN * rB, yB, sinPhiN * rB}, {cosPhiN * cosAlphaB, sinAlphaB, sinPhiN * cosAlphaB}, texUB, texVB
				);
			}
		}

		builder.endConstruction();

		/* Fix tangent vectors analytically for this revolution surface.
		 * The UV-based tangent computation produces a seam artifact at the
		 * phi=0/2*PI boundary because vertices on each side of the UV seam
		 * are not merged (different U coordinates). The analytical tangent
		 * for any revolution surface is simply the circumferential direction:
		 * dP/dPhi = (-sin(phi), 0, cos(phi)), which equals (-z, 0, x) / r. */
		{
			constexpr auto epsilon = static_cast< vertex_data_t >(1e-6);

			for ( auto & vertex : shape.vertices() )
			{
				const auto & pos = vertex.position();
				const auto r = std::sqrt(pos[Math::X] * pos[Math::X] + pos[Math::Z] * pos[Math::Z]);

				if ( r > epsilon )
				{
					vertex.setTangent(Math::Vector< 3, vertex_data_t >{-pos[Math::Z] / r, 0, pos[Math::X] / r});
				}
			}
		}

		return shape;
	}

	/**
	 * @brief Generates a hemisphere shape (half sphere with disk cap).
	 * @note Ready for vulkan default world axis. Dome points Y- (up).
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the hemisphere. Default 1.
	 * @param slices Longitudinal subdivisions. Default 16.
	 * @param stacks Latitudinal subdivisions (pole to equator). Default 8.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateHemisphere (vertex_data_t radius = 1, index_data_t slices = 16, index_data_t stacks = 8, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		/* Dome surface + equator disk cap. */
		Shape< vertex_data_t, index_data_t > shape{(stacks + 1) * slices * 2};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto halfPi = pi * half;
		const auto dAngle = halfPi / static_cast< vertex_data_t >(stacks);
		const auto dPhi = (static_cast< vertex_data_t >(2) * pi) / static_cast< vertex_data_t >(slices);
		const auto deltaU = one / static_cast< vertex_data_t >(slices);
		const auto invRadius = one / radius;

		/* Volumetric vertex color. Y range: [-radius, 0]. */
		const auto volumetricColor = [half, one, invRadius](const Math::Vector< 3, vertex_data_t > & p) {
			return Math::Vector< 4, vertex_data_t >{
				(p[Math::X] * invRadius + one) * half,
				(p[Math::Y] * invRadius + one) * half,
				(p[Math::Z] * invRadius + one) * half,
				one
			};
		};

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		/* --- Dome surface (pole to equator) --- */
		for ( index_data_t stack = 0; stack < stacks; ++stack )
		{
			/* beta goes from PI/2 (pole, Y = -radius) to 0 (equator, Y = 0). */
			const auto betaA = halfPi - static_cast< vertex_data_t >(stack) * dAngle;
			const auto betaB = halfPi - static_cast< vertex_data_t >(stack + 1) * dAngle;

			const auto cosBetaA = std::cos(betaA);
			const auto sinBetaA = std::sin(betaA);
			const auto cosBetaB = std::cos(betaB);
			const auto sinBetaB = std::sin(betaB);

			const auto yA = -radius * sinBetaA;
			const auto yB = -radius * sinBetaB;
			const auto rA = radius * cosBetaA;
			const auto rB = radius * cosBetaB;

			const auto texVA = static_cast< vertex_data_t >(stack) / static_cast< vertex_data_t >(stacks + 1);
			const auto texVB = static_cast< vertex_data_t >(stack + 1) / static_cast< vertex_data_t >(stacks + 1);

			for ( index_data_t slice = 0; slice < slices; ++slice )
			{
				const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
				const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
				const auto cosPhi = std::cos(phi);
				const auto sinPhi = std::sin(phi);
				const auto cosPhiN = std::cos(phiNext);
				const auto sinPhiN = std::sin(phiNext);

				const auto texUA = static_cast< vertex_data_t >(slice) * deltaU;
				const auto texUB = static_cast< vertex_data_t >(slice + 1) * deltaU;

				const Math::Vector< 3, vertex_data_t > p0{cosPhi * rA, yA, sinPhi * rA};
				const Math::Vector< 3, vertex_data_t > p1{cosPhi * rB, yB, sinPhi * rB};
				const Math::Vector< 3, vertex_data_t > p2{cosPhiN * rA, yA, sinPhiN * rA};
				const Math::Vector< 3, vertex_data_t > p3{cosPhiN * rB, yB, sinPhiN * rB};

				const Math::Vector< 3, vertex_data_t > n0{cosPhi * cosBetaA, -sinBetaA, sinPhi * cosBetaA};
				const Math::Vector< 3, vertex_data_t > n1{cosPhi * cosBetaB, -sinBetaB, sinPhi * cosBetaB};
				const Math::Vector< 3, vertex_data_t > n2{cosPhiN * cosBetaA, -sinBetaA, sinPhiN * cosBetaA};
				const Math::Vector< 3, vertex_data_t > n3{cosPhiN * cosBetaB, -sinBetaB, sinPhiN * cosBetaB};

				builder.setPosition(p0); builder.setNormal(n0); builder.setTextureCoordinates(texUA, texVA); builder.setVertexColor(volumetricColor(p0)); builder.newVertex();
				builder.setPosition(p2); builder.setNormal(n2); builder.setTextureCoordinates(texUB, texVA); builder.setVertexColor(volumetricColor(p2)); builder.newVertex();
				builder.setPosition(p1); builder.setNormal(n1); builder.setTextureCoordinates(texUA, texVB); builder.setVertexColor(volumetricColor(p1)); builder.newVertex();
				builder.setPosition(p3); builder.setNormal(n3); builder.setTextureCoordinates(texUB, texVB); builder.setVertexColor(volumetricColor(p3)); builder.newVertex();
				builder.resetCurrentTriangle();
			}
		}

		/* --- Equator disk cap (Y = 0, normal Y+, facing down in Vulkan) --- */
		{
			const Math::Vector< 3, vertex_data_t > capNormal = Math::Vector< 3, vertex_data_t >::positiveY();
			const Math::Vector< 3, vertex_data_t > pCenter{0, 0, 0};
			const Math::Vector< 3, vertex_data_t > texCenter{half, half, 0};

			for ( index_data_t slice = 0; slice < slices; ++slice )
			{
				const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
				const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
				const auto cosPhi = std::cos(phi);
				const auto sinPhi = std::sin(phi);
				const auto cosPhiN = std::cos(phiNext);
				const auto sinPhiN = std::sin(phiNext);

				/* Outer edge at equator → center at origin. */
				const Math::Vector< 3, vertex_data_t > pEdgeA{cosPhi * radius, 0, sinPhi * radius};
				const Math::Vector< 3, vertex_data_t > pEdgeB{cosPhiN * radius, 0, sinPhiN * radius};

				/* Planar UV projection: map XZ position [-r, +r] to [0, 1]. */
				const Math::Vector< 3, vertex_data_t > texA{cosPhi * half + half, sinPhi * half + half, 0};
				const Math::Vector< 3, vertex_data_t > texB{cosPhiN * half + half, sinPhiN * half + half, 0};

				builder.setPosition(pEdgeA); builder.setNormal(capNormal); builder.setTextureCoordinates(texA); builder.setVertexColor(volumetricColor(pEdgeA)); builder.newVertex();
				builder.setPosition(pEdgeB); builder.setNormal(capNormal); builder.setTextureCoordinates(texB); builder.setVertexColor(volumetricColor(pEdgeB)); builder.newVertex();
				builder.setPosition(pCenter); builder.setNormal(capNormal); builder.setTextureCoordinates(texCenter); builder.setVertexColor(volumetricColor(pCenter)); builder.newVertex();
				builder.resetCurrentTriangle();
			}
		}

		builder.endConstruction();

		/* Fix tangent vectors analytically for this revolution surface.
		 * Same UV seam fix as generateCapsule: (-z, 0, x) / r.
		 * Equator cap vertices (normal purely along Y) get a planar tangent
		 * aligned with the U axis of their planar UV projection. */
		{
			constexpr auto epsilon = static_cast< vertex_data_t >(1e-6);

			for ( auto & vertex : shape.vertices() )
			{
				const auto & normal = vertex.normal();
				const auto absNY = std::abs(normal[Math::Y]);

				if ( absNY > one - epsilon )
				{
					/* Cap vertex: tangent aligned with planar U axis (X+). */
					vertex.setTangent(Math::Vector< 3, vertex_data_t >{one, 0, 0});
				}
				else
				{
					/* Dome vertex: circumferential tangent. */
					const auto & pos = vertex.position();
					const auto r = std::sqrt(pos[Math::X] * pos[Math::X] + pos[Math::Z] * pos[Math::Z]);

					if ( r > epsilon )
					{
						vertex.setTangent(Math::Vector< 3, vertex_data_t >{-pos[Math::Z] / r, 0, pos[Math::X] / r});
					}
				}
			}
		}

		return shape;
	}

	/**
	 * @brief Generates an arrow shape (cylinder shaft + cone head).
	 * @note Ready for vulkan default world axis. Points along -Y (up).
	 * Base at Y = 0, tip at Y = -(shaftLength + headLength).
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param shaftRadius The radius of the shaft cylinder. Default 0.05.
	 * @param headRadius The radius of the cone base. Default 0.15.
	 * @param shaftLength The length of the shaft. Default 0.7.
	 * @param headLength The length of the cone head. Default 0.3.
	 * @param slices Circumferential subdivisions. Default 16.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateArrow (vertex_data_t shaftRadius = 0.05, vertex_data_t headRadius = 0.15, vertex_data_t shaftLength = 0.7, vertex_data_t headLength = 0.3, index_data_t slices = 16, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		/* Components: base cap + shaft body + head base annulus + cone surface.
		 * Triangle count: slices + slices*2 + slices*2 + slices = slices * 6. */
		Shape< vertex_data_t, index_data_t > shape{slices * 6};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto dPhi = (static_cast< vertex_data_t >(2) * pi) / static_cast< vertex_data_t >(slices);

		const auto totalLength = shaftLength + headLength;
		const auto invTotalLen = one / totalLength;
		const auto maxR = std::max(shaftRadius, headRadius);
		const auto invMaxR = one / maxR;

		/* Arrow geometry: base at Y=0, tip at Y=-totalLength.
		 * Shaft: Y=0 to Y=-shaftLength.
		 * Head:  Y=-shaftLength to Y=-totalLength. */

		/* Volumetric vertex color. */
		const auto volumetricColor = [half, one, invMaxR, invTotalLen](const Math::Vector< 3, vertex_data_t > & p) {
			return Math::Vector< 4, vertex_data_t >{
				(p[Math::X] * invMaxR + one) * half,
				(-p[Math::Y] * invTotalLen + one) * half,
				(p[Math::Z] * invMaxR + one) * half,
				one
			};
		};

		/* Cone surface normal Y component: derived from generatrix slope.
		 * The cone surface normal has components (headRadius·cos, headLength, headRadius·sin)
		 * after normalization. */
		const auto coneSlant = std::sqrt(headRadius * headRadius + headLength * headLength);
		const auto coneNY = -headRadius / coneSlant;
		const auto coneNR = headLength / coneSlant;

		builder.beginConstruction(ConstructionMode::Triangles);

		for ( index_data_t slice = 0; slice < slices; ++slice )
		{
			const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
			const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
			const auto cosPhi = std::cos(phi);
			const auto sinPhi = std::sin(phi);
			const auto cosPhiN = std::cos(phiNext);
			const auto sinPhiN = std::sin(phiNext);

			/* --- 1. Shaft base cap (Y = 0, normal +Y, facing down) --- */
			{
				const Math::Vector< 3, vertex_data_t > normal{0, one, 0};
				const Math::Vector< 3, vertex_data_t > center{0, 0, 0};
				const Math::Vector< 3, vertex_data_t > edgeA{cosPhi * shaftRadius, 0, sinPhi * shaftRadius};
				const Math::Vector< 3, vertex_data_t > edgeB{cosPhiN * shaftRadius, 0, sinPhiN * shaftRadius};

				builder.setNormal(normal);

				builder.setPosition(center); builder.setTextureCoordinates(half, half); builder.setVertexColor(volumetricColor(center)); builder.newVertex();
				builder.setPosition(edgeA); builder.setTextureCoordinates(cosPhi * half + half, sinPhi * half + half); builder.setVertexColor(volumetricColor(edgeA)); builder.newVertex();
				builder.setPosition(edgeB); builder.setTextureCoordinates(cosPhiN * half + half, sinPhiN * half + half); builder.setVertexColor(volumetricColor(edgeB)); builder.newVertex();
			}

			/* --- 2. Shaft body (cylinder from Y=0 to Y=-shaftLength) --- */
			{
				const Math::Vector< 3, vertex_data_t > nA{cosPhi, 0, sinPhi};
				const Math::Vector< 3, vertex_data_t > nB{cosPhiN, 0, sinPhiN};

				const Math::Vector< 3, vertex_data_t > topA{cosPhi * shaftRadius, 0, sinPhi * shaftRadius};
				const Math::Vector< 3, vertex_data_t > topB{cosPhiN * shaftRadius, 0, sinPhiN * shaftRadius};
				const Math::Vector< 3, vertex_data_t > botA{cosPhi * shaftRadius, -shaftLength, sinPhi * shaftRadius};
				const Math::Vector< 3, vertex_data_t > botB{cosPhiN * shaftRadius, -shaftLength, sinPhiN * shaftRadius};

				builder.setPosition(topA); builder.setNormal(nA); builder.setTextureCoordinates(0, 0); builder.setVertexColor(volumetricColor(topA)); builder.newVertex();
				builder.setPosition(botA); builder.setNormal(nA); builder.setTextureCoordinates(0, one); builder.setVertexColor(volumetricColor(botA)); builder.newVertex();
				builder.setPosition(topB); builder.setNormal(nB); builder.setTextureCoordinates(one, 0); builder.setVertexColor(volumetricColor(topB)); builder.newVertex();

				builder.setPosition(topB); builder.setNormal(nB); builder.setTextureCoordinates(one, 0); builder.setVertexColor(volumetricColor(topB)); builder.newVertex();
				builder.setPosition(botA); builder.setNormal(nA); builder.setTextureCoordinates(0, one); builder.setVertexColor(volumetricColor(botA)); builder.newVertex();
				builder.setPosition(botB); builder.setNormal(nB); builder.setTextureCoordinates(one, one); builder.setVertexColor(volumetricColor(botB)); builder.newVertex();
			}

			/* --- 3. Head base annulus (Y = -shaftLength, normal +Y) --- */
			{
				const Math::Vector< 3, vertex_data_t > normal{0, one, 0};

				const Math::Vector< 3, vertex_data_t > innerA{cosPhi * shaftRadius, -shaftLength, sinPhi * shaftRadius};
				const Math::Vector< 3, vertex_data_t > innerB{cosPhiN * shaftRadius, -shaftLength, sinPhiN * shaftRadius};
				const Math::Vector< 3, vertex_data_t > outerA{cosPhi * headRadius, -shaftLength, sinPhi * headRadius};
				const Math::Vector< 3, vertex_data_t > outerB{cosPhiN * headRadius, -shaftLength, sinPhiN * headRadius};

				builder.setNormal(normal);

				builder.setPosition(innerA); builder.setTextureCoordinates(0, 0); builder.setVertexColor(volumetricColor(innerA)); builder.newVertex();
				builder.setPosition(outerA); builder.setTextureCoordinates(0, one); builder.setVertexColor(volumetricColor(outerA)); builder.newVertex();
				builder.setPosition(innerB); builder.setTextureCoordinates(one, 0); builder.setVertexColor(volumetricColor(innerB)); builder.newVertex();

				builder.setPosition(innerB); builder.setTextureCoordinates(one, 0); builder.setVertexColor(volumetricColor(innerB)); builder.newVertex();
				builder.setPosition(outerA); builder.setTextureCoordinates(0, one); builder.setVertexColor(volumetricColor(outerA)); builder.newVertex();
				builder.setPosition(outerB); builder.setTextureCoordinates(one, one); builder.setVertexColor(volumetricColor(outerB)); builder.newVertex();
			}

			/* --- 4. Cone head (Y = -shaftLength to Y = -totalLength) --- */
			{
				const Math::Vector< 3, vertex_data_t > nA{coneNR * cosPhi, coneNY, coneNR * sinPhi};
				const Math::Vector< 3, vertex_data_t > nB{coneNR * cosPhiN, coneNY, coneNR * sinPhiN};

				const Math::Vector< 3, vertex_data_t > baseA{cosPhi * headRadius, -shaftLength, sinPhi * headRadius};
				const Math::Vector< 3, vertex_data_t > baseB{cosPhiN * headRadius, -shaftLength, sinPhiN * headRadius};
				const Math::Vector< 3, vertex_data_t > tip{0, -totalLength, 0};

				builder.setPosition(baseA); builder.setNormal(nA); builder.setTextureCoordinates(0, one); builder.setVertexColor(volumetricColor(baseA)); builder.newVertex();
				builder.setPosition(tip); builder.setNormal(nA); builder.setTextureCoordinates(half, 0); builder.setVertexColor(volumetricColor(tip)); builder.newVertex();
				builder.setPosition(baseB); builder.setNormal(nB); builder.setTextureCoordinates(one, one); builder.setVertexColor(volumetricColor(baseB)); builder.newVertex();
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a tube shape (hollow cylinder with optional annular caps).
	 * @note Ready for vulkan default world axis. Centered at origin along Y.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param outerRadius The outer radius of the tube. Default 1.
	 * @param innerRadius The inner radius (hole). Default 0.5.
	 * @param length The length of the tube along Y. Default 1.
	 * @param slices Circumferential subdivisions. Default 16.
	 * @param stacks Longitudinal subdivisions. Default 1.
	 * @param capMapping UV mapping mode for annular caps. Default Planar.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateTube (vertex_data_t outerRadius = 1, vertex_data_t innerRadius = 0.5, vertex_data_t length = 1, index_data_t slices = 16, index_data_t stacks = 1, CapUVMapping capMapping = CapUVMapping::Planar, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		if ( innerRadius > outerRadius )
		{
			std::swap(innerRadius, outerRadius);
		}

		const auto capped = capMapping != CapUVMapping::None;

		/* Outer surface + inner surface + (optional) top cap + bottom cap.
		 * Each surface: slices * stacks * 2 triangles (caps have 1 stack). */
		const auto capTriangles = capped ? slices * 4 : 0;
		Shape< vertex_data_t, index_data_t > shape{slices * stacks * 4 + capTriangles};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);

		const auto halfLen = length * half;
		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto dPhi = (static_cast< vertex_data_t >(2) * pi) / static_cast< vertex_data_t >(slices);
		const auto deltaU = one / static_cast< vertex_data_t >(slices);
		const auto deltaV = one / static_cast< vertex_data_t >(stacks);
		const auto invOuterR = one / outerRadius;
		const auto invHalfLen = one / halfLen;

		/* Volumetric vertex color. */
		const auto volumetricColor = [half, one, invOuterR, invHalfLen](const Math::Vector< 3, vertex_data_t > & p) {
			return Math::Vector< 4, vertex_data_t >{
				(p[Math::X] * invOuterR + one) * half,
				(p[Math::Y] * invHalfLen + one) * half,
				(p[Math::Z] * invOuterR + one) * half,
				one
			};
		};

		builder.beginConstruction(ConstructionMode::TriangleStrip);

		/* Helper: emit a cylindrical surface band. */
		const auto emitSurface = [&](vertex_data_t radius, bool outward) {
			for ( index_data_t stack = 0; stack < stacks; ++stack )
			{
				const auto yA = -halfLen + length * static_cast< vertex_data_t >(stack) / static_cast< vertex_data_t >(stacks);
				const auto yB = -halfLen + length * static_cast< vertex_data_t >(stack + 1) / static_cast< vertex_data_t >(stacks);
				const auto texVA = static_cast< vertex_data_t >(stack) * deltaV;
				const auto texVB = static_cast< vertex_data_t >(stack + 1) * deltaV;

				for ( index_data_t slice = 0; slice < slices; ++slice )
				{
					const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
					const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
					const auto cosPhi = std::cos(phi);
					const auto sinPhi = std::sin(phi);
					const auto cosPhiN = std::cos(phiNext);
					const auto sinPhiN = std::sin(phiNext);

					const auto texUA = static_cast< vertex_data_t >(slice) * deltaU;
					const auto texUB = static_cast< vertex_data_t >(slice + 1) * deltaU;

					const auto sign = outward ? one : -one;
					const Math::Vector< 3, vertex_data_t > nA{sign * cosPhi, 0, sign * sinPhi};
					const Math::Vector< 3, vertex_data_t > nB{sign * cosPhiN, 0, sign * sinPhiN};

					const Math::Vector< 3, vertex_data_t > p0{cosPhi * radius, yA, sinPhi * radius};
					const Math::Vector< 3, vertex_data_t > p1{cosPhi * radius, yB, sinPhi * radius};
					const Math::Vector< 3, vertex_data_t > p2{cosPhiN * radius, yA, sinPhiN * radius};
					const Math::Vector< 3, vertex_data_t > p3{cosPhiN * radius, yB, sinPhiN * radius};

					if ( outward )
					{
						builder.setPosition(p0); builder.setNormal(nA); builder.setTextureCoordinates(texUA, texVA); builder.setVertexColor(volumetricColor(p0)); builder.newVertex();
						builder.setPosition(p2); builder.setNormal(nB); builder.setTextureCoordinates(texUB, texVA); builder.setVertexColor(volumetricColor(p2)); builder.newVertex();
						builder.setPosition(p1); builder.setNormal(nA); builder.setTextureCoordinates(texUA, texVB); builder.setVertexColor(volumetricColor(p1)); builder.newVertex();
						builder.setPosition(p3); builder.setNormal(nB); builder.setTextureCoordinates(texUB, texVB); builder.setVertexColor(volumetricColor(p3)); builder.newVertex();
					}
					else
					{
						builder.setPosition(p2); builder.setNormal(nB); builder.setTextureCoordinates(texUB, texVA); builder.setVertexColor(volumetricColor(p2)); builder.newVertex();
						builder.setPosition(p0); builder.setNormal(nA); builder.setTextureCoordinates(texUA, texVA); builder.setVertexColor(volumetricColor(p0)); builder.newVertex();
						builder.setPosition(p3); builder.setNormal(nB); builder.setTextureCoordinates(texUB, texVB); builder.setVertexColor(volumetricColor(p3)); builder.newVertex();
						builder.setPosition(p1); builder.setNormal(nA); builder.setTextureCoordinates(texUA, texVB); builder.setVertexColor(volumetricColor(p1)); builder.newVertex();
					}
					builder.resetCurrentTriangle();
				}
			}
		};

		/* Helper: emit an annular cap at the given Y position. */
		const auto emitCap = [&](vertex_data_t y, bool faceUp) {
			const Math::Vector< 3, vertex_data_t > normal{0, faceUp ? -one : one, 0};
			const auto innerScale = innerRadius / outerRadius;

			for ( index_data_t slice = 0; slice < slices; ++slice )
			{
				const auto phi = static_cast< vertex_data_t >(slice) * dPhi;
				const auto phiNext = slice + 1 == slices ? 0 : static_cast< vertex_data_t >(slice + 1) * dPhi;
				const auto cosPhi = std::cos(phi);
				const auto sinPhi = std::sin(phi);
				const auto cosPhiN = std::cos(phiNext);
				const auto sinPhiN = std::sin(phiNext);

				const Math::Vector< 3, vertex_data_t > outerA{cosPhi * outerRadius, y, sinPhi * outerRadius};
				const Math::Vector< 3, vertex_data_t > outerB{cosPhiN * outerRadius, y, sinPhiN * outerRadius};
				const Math::Vector< 3, vertex_data_t > innerA{cosPhi * innerRadius, y, sinPhi * innerRadius};
				const Math::Vector< 3, vertex_data_t > innerB{cosPhiN * innerRadius, y, sinPhiN * innerRadius};

				vertex_data_t texOAU, texOAV, texOBU, texOBV, texIAU, texIAV, texIBU, texIBV;

				if ( capMapping == CapUVMapping::Planar )
				{
					/* Planar UV projection: map XZ position [-r, +r] to [0, 1].
					 * Outer ring maps to the full [0,1] circle, inner ring scales proportionally. */
					texOAU = cosPhi * half + half;  texOAV = sinPhi * half + half;
					texOBU = cosPhiN * half + half; texOBV = sinPhiN * half + half;
					texIAU = cosPhi * innerScale * half + half;  texIAV = sinPhi * innerScale * half + half;
					texIBU = cosPhiN * innerScale * half + half; texIBV = sinPhiN * innerScale * half + half;
				}
				else /* PerSegment */
				{
					/* Each annular quad gets its own [0,1]x[0,1] UV space. */
					texOAU = 0; texOAV = one;
					texOBU = one; texOBV = one;
					texIAU = 0; texIAV = 0;
					texIBU = one; texIBV = 0;
				}

				if ( faceUp )
				{
					builder.setNormal(normal); builder.setPosition(outerB); builder.setTextureCoordinates(texOBU, texOBV); builder.setVertexColor(volumetricColor(outerB)); builder.newVertex();
					builder.setNormal(normal); builder.setPosition(outerA); builder.setTextureCoordinates(texOAU, texOAV); builder.setVertexColor(volumetricColor(outerA)); builder.newVertex();
					builder.setNormal(normal); builder.setPosition(innerB); builder.setTextureCoordinates(texIBU, texIBV); builder.setVertexColor(volumetricColor(innerB)); builder.newVertex();
					builder.setNormal(normal); builder.setPosition(innerA); builder.setTextureCoordinates(texIAU, texIAV); builder.setVertexColor(volumetricColor(innerA)); builder.newVertex();
				}
				else
				{
					builder.setNormal(normal); builder.setPosition(outerA); builder.setTextureCoordinates(texOAU, texOAV); builder.setVertexColor(volumetricColor(outerA)); builder.newVertex();
					builder.setNormal(normal); builder.setPosition(outerB); builder.setTextureCoordinates(texOBU, texOBV); builder.setVertexColor(volumetricColor(outerB)); builder.newVertex();
					builder.setNormal(normal); builder.setPosition(innerA); builder.setTextureCoordinates(texIAU, texIAV); builder.setVertexColor(volumetricColor(innerA)); builder.newVertex();
					builder.setNormal(normal); builder.setPosition(innerB); builder.setTextureCoordinates(texIBU, texIBV); builder.setVertexColor(volumetricColor(innerB)); builder.newVertex();
				}
				builder.resetCurrentTriangle();
			}
		};

		/* Outer surface (normals outward). */
		emitSurface(outerRadius, true);

		/* Inner surface (normals inward). */
		emitSurface(innerRadius, false);

		if ( capped )
		{
			/* Switch to cap UV multiplier for cap geometry. */
			const auto savedMultiplier = builder.options().textureCoordinatesMultiplier();
			builder.options().setTextureCoordinatesMultiplier(builder.options().capTextureCoordinatesMultiplier());

			/* Top cap (Y = -halfLen, face up / Y-). */
			emitCap(-halfLen, true);

			/* Bottom cap (Y = +halfLen, face down / Y+). */
			emitCap(halfLen, false);

			/* Restore surface UV multiplier. */
			builder.options().setTextureCoordinatesMultiplier(savedMultiplier);
		}

		builder.endConstruction();

		/* Fix tangent vectors analytically for this revolution surface.
		 * Surface vertices: circumferential tangent (-z, 0, x) / r fixes the UV seam.
		 * Cap vertices: uniform tangent so TBN is identical across the whole flat cap,
		 * regardless of UV mapping mode (Planar or PerSegment). */
		{
			constexpr auto epsilon = static_cast< vertex_data_t >(1e-6);

			for ( auto & vertex : shape.vertices() )
			{
				const auto & normal = vertex.normal();
				const auto absNY = std::abs(normal[Math::Y]);

				if ( absNY > one - epsilon )
				{
					/* Cap vertex: uniform tangent so that cross(N, T) = (0,0,1). */
					const auto tx = normal[Math::Y] < 0 ? one : -one;
					vertex.setTangent(Math::Vector< 3, vertex_data_t >{tx, 0, 0});
				}
				else
				{
					/* Surface vertex: circumferential tangent for UV seam fix. */
					const auto & pos = vertex.position();
					const auto r = std::sqrt(pos[Math::X] * pos[Math::X] + pos[Math::Z] * pos[Math::Z]);

					if ( r > epsilon )
					{
						vertex.setTangent(Math::Vector< 3, vertex_data_t >{-pos[Math::Z] / r, 0, pos[Math::X] / r});
					}
				}
			}
		}

		return shape;
	}

	/**
	 * @brief Generates a brilliant-cut gem shape (diamond approximation).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The girdle radius (widest point). Default 1.
	 * @param facets The number of crown/pavilion facets. Default 8.
	 * @param tableRatio The table radius as fraction of girdle (0-1). Default 0.55.
	 * @param crownAngle The crown slope from girdle plane in degrees. Default 35.
	 * @param pavilionAngle The pavilion slope from girdle plane in degrees. Default 41.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateDiamondCutGem (
		vertex_data_t radius = 1,
		size_t facets = 8,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.55),
		vertex_data_t crownAngle = 35,
		vertex_data_t pavilionAngle = 41,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		const auto triangleCount = 4 * n - 2;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		/* Convert angles to radians. */
		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto degToRad = pi / static_cast< vertex_data_t >(180);
		const auto crownAngleRad = crownAngle * degToRad;
		const auto pavilionAngleRad = pavilionAngle * degToRad;

		/* Compute key dimensions. */
		const auto tableRadius = radius * tableRatio;
		const auto crownHeight = (radius - tableRadius) * std::tan(crownAngleRad);
		const auto pavilionDepth = radius * std::tan(pavilionAngleRad);

		/* Precompute ring vertices. */
		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		std::vector< Vec3 > tableRing(n);
		std::vector< Vec3 > girdleRing(n);

		for ( size_t i = 0; i < n; ++i )
		{
			const auto theta = static_cast< vertex_data_t >(i) * angleStep;
			const auto cosT = std::cos(theta);
			const auto sinT = std::sin(theta);

			tableRing[i] = Vec3(tableRadius * cosT, -crownHeight, tableRadius * sinT);
			girdleRing[i] = Vec3(radius * cosT, zero, radius * sinT);
		}

		const Vec3 culet(zero, pavilionDepth, zero);

		/* Volumetric vertex color: position [-radius, radius] -> RGB [0, 1].
		 * Y range is [-pavilionDepth, crownHeight], so normalize by the max extent. */
		const auto maxExtent = std::max(radius, std::max(crownHeight, pavilionDepth));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		/* Emit one triangle with flat normal, per-vertex UV, and volumetric vertex color.
		 * B/C are swapped for winding convention (same as dodecahedron). */
		const auto emitTriangle = [&](
			const Vec3 & vA,
			const Vec3 & vB,
			const Vec3 & vC,
			const Vec3 & normal,
			const Vec3 & tcA,
			const Vec3 & tcB,
			const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		/* Compute per-face tangent-frame UV for an arbitrary polygon.
		 * Projects vertices onto the face plane using a local tangent frame,
		 * then remaps to [0,1]x[0,1] (same technique as dodecahedron). */
		const auto computeFaceUV = [&one](
			const Vec3 & normal,
			const Vec3 & center,
			const Vec3 * positions,
			Vec3 * uvs,
			size_t count)
		{
			/* Build local tangent frame on the face plane. */
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			/* Project each vertex into 2D local coordinates. */
			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table (flat n-gon at Y = -crownHeight, facing sky Y-) === */
		{
			/* Compute flat normal from cross product (sky-facing = Y-). */
			const auto edge1 = tableRing[1] - tableRing[0];
			const auto edge2 = tableRing[n - 1] - tableRing[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			/* Planar UV for table: U = (cosθ + 1) * 0.5, V = (sinθ + 1) * 0.5 */
			std::vector< Vec3 > tableUV(n);

			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				tableUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}

			/* Fan from T_0: triangles (T_0, T_i, T_{i+1}) for i=1..n-2 */
			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(
					tableRing[0], tableRing[i], tableRing[i + 1],
					tableNormal,
					tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown (n quads from table ring to girdle ring) === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;

			const auto & t0 = tableRing[i];
			const auto & t1 = tableRing[i1];
			const auto & g0 = girdleRing[i];
			const auto & g1 = girdleRing[i1];

			/* Flat normal per quad (outward-pointing). */
			const auto center = (t0 + t1 + g0 + g1) / static_cast< vertex_data_t >(4);
			const auto normal = Vec3::crossProduct(t1 - t0, g0 - t0).normalized();

			/* Per-face tangent-frame UV for the quad. */
			const Vec3 positions[4] = {t0, t1, g1, g0};
			Vec3 uvs[4];
			computeFaceUV(normal, center, positions, uvs, 4);

			/* Two triangles: (T_i, G_i, G_{i+1}) and (T_i, G_{i+1}, T_{i+1}) */
			emitTriangle(t0, g0, g1, normal, uvs[0], uvs[3], uvs[2]);
			emitTriangle(t0, g1, t1, normal, uvs[0], uvs[2], uvs[1]);
		}

		/* === Pavilion (n triangles from girdle to culet) === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;

			const auto & g0 = girdleRing[i];
			const auto & g1 = girdleRing[i1];

			/* Flat normal per triangle (outward-pointing). */
			const auto center = (g0 + g1 + culet) / static_cast< vertex_data_t >(3);
			const auto normal = Vec3::crossProduct(culet - g0, g1 - g0).normalized();

			/* Per-face tangent-frame UV. */
			const Vec3 positions[3] = {g0, g1, culet};
			Vec3 uvs[3];
			computeFaceUV(normal, center, positions, uvs, 3);

			emitTriangle(g0, culet, g1, normal, uvs[0], uvs[2], uvs[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates an emerald-cut gem shape (step-cut rectangular gem with beveled corners).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param length The X-axis dimension (longer axis). Default 1.5.
	 * @param width The Z-axis dimension (shorter axis). Default 1.
	 * @param depth The total Y depth of the gem. Default 1.
	 * @param tableRatio The table size as fraction of length/width. Default 0.6.
	 * @param cornerBevel The corner bevel size as fraction of half-width. Default 0.25.
	 * @param steps The number of step facets on crown and pavilion. Default 3.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateEmeraldCutGem (
		vertex_data_t length = static_cast< vertex_data_t >(1.5),
		vertex_data_t width = 1,
		vertex_data_t depth = 1,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.6),
		vertex_data_t cornerBevel = static_cast< vertex_data_t >(0.25),
		size_t steps = 3,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto triangleCount = 12 + 32 * steps;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		/* Crown/pavilion split. */
		const auto crownHeight = depth * static_cast< vertex_data_t >(0.25);
		const auto pavilionDepth = depth * static_cast< vertex_data_t >(0.75);
		const auto culetRatio = static_cast< vertex_data_t >(0.1);

		/* Build an octagonal ring at a given scale and height.
		 * The octagon is a rectangle with beveled corners. */
		const auto makeOctagonRing = [&](vertex_data_t s, vertex_data_t y) -> std::array< Vec3, 8 > {
			const auto two = static_cast< vertex_data_t >(2);
			const auto halfL = (length / two) * s;
			const auto halfW = (width / two) * s;
			const auto b = cornerBevel * (width / two) * s;

			return {{
				Vec3( halfL - b,  y, -halfW),
				Vec3( halfL,      y, -halfW + b),
				Vec3( halfL,      y,  halfW - b),
				Vec3( halfL - b,  y,  halfW),
				Vec3(-halfL + b,  y,  halfW),
				Vec3(-halfL,      y,  halfW - b),
				Vec3(-halfL,      y, -halfW + b),
				Vec3(-halfL + b,  y, -halfW)
			}};
		};

		/* Volumetric vertex color: position -> RGB [0, 1]. */
		const auto maxExtent = std::max(length * half, std::max(width * half, std::max(crownHeight, pavilionDepth)));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		/* Emit one triangle with flat normal, per-vertex UV, and volumetric vertex color.
		 * B/C are swapped for winding convention (same as generateDiamondCutGem). */
		const auto emitTriangle = [&](
			const Vec3 & vA,
			const Vec3 & vB,
			const Vec3 & vC,
			const Vec3 & normal,
			const Vec3 & tcA,
			const Vec3 & tcB,
			const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		/* Compute per-face tangent-frame UV for an arbitrary polygon. */
		const auto computeFaceUV = [&one](
			const Vec3 & normal,
			const Vec3 & center,
			const Vec3 * positions,
			Vec3 * uvs,
			size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table (flat octagon at Y = -crownHeight, facing sky Y-) === */
		{
			const auto table = makeOctagonRing(tableRatio, -crownHeight);

			const auto edge1 = table[1] - table[0];
			const auto edge2 = table[7] - table[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			/* Planar UV for table: remap XZ to [0,1]. */
			const auto halfL = (length * half) * tableRatio;
			const auto halfW = (width * half) * tableRatio;
			std::array< Vec3, 8 > tableUV;

			for ( size_t i = 0; i < 8; ++i )
			{
				tableUV[i] = Vec3(
					(table[i][Math::X] + halfL) / (halfL * static_cast< vertex_data_t >(2)),
					(table[i][Math::Z] + halfW) / (halfW * static_cast< vertex_data_t >(2)),
					zero);
			}

			/* Fan from v0: 6 triangles (v0, vi, vi+1) for i=1..6 */
			for ( size_t i = 1; i + 1 < 8; ++i )
			{
				emitTriangle(
					table[0], table[i], table[i + 1],
					tableNormal,
					tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown steps (bands between table ring and girdle ring) === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto innerScale = tableRatio + t0 * (one - tableRatio);
			const auto outerScale = tableRatio + t1 * (one - tableRatio);
			const auto innerY = -crownHeight + t0 * crownHeight;
			const auto outerY = -crownHeight + t1 * crownHeight;

			const auto inner = makeOctagonRing(innerScale, innerY);
			const auto outer = makeOctagonRing(outerScale, outerY);

			for ( size_t i = 0; i < 8; ++i )
			{
				const auto next = (i + 1) % 8;

				const auto normal = Vec3::crossProduct(inner[next] - inner[i], outer[i] - inner[i]).normalized();

				const Vec3 positions[4] = {inner[i], outer[i], outer[next], inner[next]};
				Vec3 uvs[4];
				const auto center = (inner[i] + outer[i] + outer[next] + inner[next]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(inner[i], outer[i], outer[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(inner[i], outer[next], inner[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Pavilion steps (bands between girdle ring and culet ring) === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto outerScale = one + t0 * (culetRatio - one);
			const auto innerScale = one + t1 * (culetRatio - one);
			const auto outerY = t0 * pavilionDepth;
			const auto innerY = t1 * pavilionDepth;

			const auto outer = makeOctagonRing(outerScale, outerY);
			const auto inner = makeOctagonRing(innerScale, innerY);

			for ( size_t i = 0; i < 8; ++i )
			{
				const auto next = (i + 1) % 8;

				const auto normal = Vec3::crossProduct(outer[next] - outer[i], inner[i] - outer[i]).normalized();

				const Vec3 positions[4] = {outer[i], inner[i], inner[next], outer[next]};
				Vec3 uvs[4];
				const auto center = (outer[i] + outer[next] + inner[next] + inner[i]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(outer[i], inner[i], inner[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(outer[i], inner[next], outer[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Culet (flat octagon at Y = +pavilionDepth, opposite winding) === */
		{
			const auto culet = makeOctagonRing(culetRatio, pavilionDepth);

			const auto edge1 = culet[1] - culet[0];
			const auto edge2 = culet[7] - culet[0];
			const auto culetNormal = Vec3::crossProduct(edge2, edge1).normalized();

			const auto halfL = (length * half) * culetRatio;
			const auto halfW = (width * half) * culetRatio;
			std::array< Vec3, 8 > culetUV;

			for ( size_t i = 0; i < 8; ++i )
			{
				culetUV[i] = Vec3(
					(culet[i][Math::X] + halfL) / (halfL * static_cast< vertex_data_t >(2)),
					(culet[i][Math::Z] + halfW) / (halfW * static_cast< vertex_data_t >(2)),
					zero);
			}

			/* Fan from v0: 6 triangles, opposite winding (v0, vi+1, vi) */
			for ( size_t i = 1; i + 1 < 8; ++i )
			{
				emitTriangle(
					culet[0], culet[i + 1], culet[i],
					culetNormal,
					culetUV[0], culetUV[i + 1], culetUV[i]);
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates an Asscher-cut gem shape (square step-cut with beveled corners).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The side length of the square. Default 1.
	 * @param depth The total Y depth of the gem. Default 1.
	 * @param tableRatio The table size as fraction of side length. Default 0.65.
	 * @param cornerBevel The corner bevel size as fraction of half-side. Default 0.15.
	 * @param steps The number of step facets on crown and pavilion. Default 3.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateAssscherCutGem (
		vertex_data_t size = 1,
		vertex_data_t depth = 1,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.65),
		vertex_data_t cornerBevel = static_cast< vertex_data_t >(0.15),
		size_t steps = 3,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateEmeraldCutGem< vertex_data_t, index_data_t >(size, size, depth, tableRatio, cornerBevel, steps, options);
	}

	/**
	 * @brief Generates a baguette-cut gem shape (pure rectangular step-cut, no corner bevel).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param length The X-axis dimension. Default 1.5.
	 * @param width The Z-axis dimension. Default 0.5.
	 * @param depth The total Y depth of the gem. Default 0.6.
	 * @param tableRatio The table size as fraction of dimensions. Default 0.65.
	 * @param steps The number of step facets on crown and pavilion. Default 2.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateBaguetteCutGem (
		vertex_data_t length = static_cast< vertex_data_t >(1.5),
		vertex_data_t width = static_cast< vertex_data_t >(0.5),
		vertex_data_t depth = static_cast< vertex_data_t >(0.6),
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.65),
		size_t steps = 2,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto triangleCount = 4 + 16 * steps;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto crownHeight = depth * static_cast< vertex_data_t >(0.25);
		const auto pavilionDepth = depth * static_cast< vertex_data_t >(0.75);
		const auto culetRatio = static_cast< vertex_data_t >(0.1);

		/* Build a rectangular ring (4 vertices) at a given scale and height. */
		const auto makeRectRing = [&](vertex_data_t s, vertex_data_t y) -> std::array< Vec3, 4 > {
			const auto two = static_cast< vertex_data_t >(2);
			const auto halfL = (length / two) * s;
			const auto halfW = (width / two) * s;

			return {{
				Vec3( halfL, y, -halfW),
				Vec3( halfL, y,  halfW),
				Vec3(-halfL, y,  halfW),
				Vec3(-halfL, y, -halfW)
			}};
		};

		const auto maxExtent = std::max(length * half, std::max(width * half, std::max(crownHeight, pavilionDepth)));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA,
			const Vec3 & vB,
			const Vec3 & vC,
			const Vec3 & normal,
			const Vec3 & tcA,
			const Vec3 & tcB,
			const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal,
			const Vec3 & center,
			const Vec3 * positions,
			Vec3 * uvs,
			size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table (flat rectangle at Y = -crownHeight) === */
		{
			const auto table = makeRectRing(tableRatio, -crownHeight);

			const auto edge1 = table[1] - table[0];
			const auto edge2 = table[3] - table[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			const auto halfL = (length * half) * tableRatio;
			const auto halfW = (width * half) * tableRatio;
			std::array< Vec3, 4 > tableUV;

			for ( size_t i = 0; i < 4; ++i )
			{
				tableUV[i] = Vec3(
					(table[i][Math::X] + halfL) / (halfL * static_cast< vertex_data_t >(2)),
					(table[i][Math::Z] + halfW) / (halfW * static_cast< vertex_data_t >(2)),
					zero);
			}

			/* Fan from v0: 2 triangles */
			emitTriangle(table[0], table[1], table[2], tableNormal, tableUV[0], tableUV[1], tableUV[2]);
			emitTriangle(table[0], table[2], table[3], tableNormal, tableUV[0], tableUV[2], tableUV[3]);
		}

		/* === Crown steps === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto innerScale = tableRatio + t0 * (one - tableRatio);
			const auto outerScale = tableRatio + t1 * (one - tableRatio);
			const auto innerY = -crownHeight + t0 * crownHeight;
			const auto outerY = -crownHeight + t1 * crownHeight;

			const auto inner = makeRectRing(innerScale, innerY);
			const auto outer = makeRectRing(outerScale, outerY);

			for ( size_t i = 0; i < 4; ++i )
			{
				const auto next = (i + 1) % 4;

				const auto normal = Vec3::crossProduct(inner[next] - inner[i], outer[i] - inner[i]).normalized();

				const Vec3 positions[4] = {inner[i], outer[i], outer[next], inner[next]};
				Vec3 uvs[4];
				const auto center = (inner[i] + outer[i] + outer[next] + inner[next]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(inner[i], outer[i], outer[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(inner[i], outer[next], inner[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Pavilion steps === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto outerScale = one + t0 * (culetRatio - one);
			const auto innerScale = one + t1 * (culetRatio - one);
			const auto outerY = t0 * pavilionDepth;
			const auto innerY = t1 * pavilionDepth;

			const auto outer = makeRectRing(outerScale, outerY);
			const auto inner = makeRectRing(innerScale, innerY);

			for ( size_t i = 0; i < 4; ++i )
			{
				const auto next = (i + 1) % 4;

				const auto normal = Vec3::crossProduct(outer[next] - outer[i], inner[i] - outer[i]).normalized();

				const Vec3 positions[4] = {outer[i], inner[i], inner[next], outer[next]};
				Vec3 uvs[4];
				const auto center = (outer[i] + outer[next] + inner[next] + inner[i]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(outer[i], inner[i], inner[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(outer[i], inner[next], outer[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Culet (flat rectangle at Y = +pavilionDepth, opposite winding) === */
		{
			const auto culet = makeRectRing(culetRatio, pavilionDepth);

			const auto edge1 = culet[1] - culet[0];
			const auto edge2 = culet[3] - culet[0];
			const auto culetNormal = Vec3::crossProduct(edge2, edge1).normalized();

			const auto halfL = (length * half) * culetRatio;
			const auto halfW = (width * half) * culetRatio;
			std::array< Vec3, 4 > culetUV;

			for ( size_t i = 0; i < 4; ++i )
			{
				culetUV[i] = Vec3(
					(culet[i][Math::X] + halfL) / (halfL * static_cast< vertex_data_t >(2)),
					(culet[i][Math::Z] + halfW) / (halfW * static_cast< vertex_data_t >(2)),
					zero);
			}

			emitTriangle(culet[0], culet[3], culet[2], culetNormal, culetUV[0], culetUV[3], culetUV[2]);
			emitTriangle(culet[0], culet[2], culet[1], culetNormal, culetUV[0], culetUV[2], culetUV[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a princess-cut gem shape (square with chevron pavilion facets).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The side length of the square. Default 1.
	 * @param depth The total Y depth of the gem. Default 1.
	 * @param tableRatio The table size as fraction of side length. Default 0.6.
	 * @param chevronDepth The chevron depth factor (0-1) controlling V-groove depth. Default 0.4.
	 * @param steps The number of step/chevron facets on crown and pavilion. Default 3.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generatePrincessCutGem (
		vertex_data_t size = 1,
		vertex_data_t depth = 1,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.6),
		vertex_data_t chevronDepth = static_cast< vertex_data_t >(0.4),
		size_t steps = 3,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		/* 8-vertex rings: 4 edge midpoints + 4 corners.
		 * Crown: uniform scaling (step cut).
		 * Pavilion: midpoints contract slower, corners faster → chevron V-grooves. */
		const auto triangleCount = 12 + 32 * steps;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto crownHeight = depth * static_cast< vertex_data_t >(0.25);
		const auto pavilionDepth = depth * static_cast< vertex_data_t >(0.75);
		const auto culetRatio = static_cast< vertex_data_t >(0.05);
		const auto halfS = size * half;

		/* Build a princess ring with 8 vertices: midpoints and corners scale independently.
		 * v0,v2,v4,v6 = edge midpoints; v1,v3,v5,v7 = corners. */
		const auto makePrincessRing = [&](vertex_data_t ms, vertex_data_t cs, vertex_data_t y) -> std::array< Vec3, 8 > {
			return {{
				Vec3(zero,       y, -halfS * ms),
				Vec3( halfS * cs, y, -halfS * cs),
				Vec3( halfS * ms, y, zero),
				Vec3( halfS * cs, y,  halfS * cs),
				Vec3(zero,       y,  halfS * ms),
				Vec3(-halfS * cs, y,  halfS * cs),
				Vec3(-halfS * ms, y, zero),
				Vec3(-halfS * cs, y, -halfS * cs)
			}};
		};

		const auto maxExtent = std::max(halfS * static_cast< vertex_data_t >(1.42), std::max(crownHeight, pavilionDepth));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table (flat octagon at Y = -crownHeight) === */
		{
			const auto table = makePrincessRing(tableRatio, tableRatio, -crownHeight);

			const auto edge1 = table[1] - table[0];
			const auto edge2 = table[7] - table[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			std::array< Vec3, 8 > tableUV;

			for ( size_t i = 0; i < 8; ++i )
			{
				tableUV[i] = Vec3(
					(table[i][Math::X] / (halfS * tableRatio) + one) * half,
					(table[i][Math::Z] / (halfS * tableRatio) + one) * half,
					zero);
			}

			for ( size_t i = 1; i + 1 < 8; ++i )
			{
				emitTriangle(table[0], table[i], table[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown steps (uniform scaling, step-cut style) === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto innerS = tableRatio + t0 * (one - tableRatio);
			const auto outerS = tableRatio + t1 * (one - tableRatio);
			const auto innerY = -crownHeight + t0 * crownHeight;
			const auto outerY = -crownHeight + t1 * crownHeight;

			const auto inner = makePrincessRing(innerS, innerS, innerY);
			const auto outer = makePrincessRing(outerS, outerS, outerY);

			for ( size_t i = 0; i < 8; ++i )
			{
				const auto next = (i + 1) % 8;
				const auto normal = Vec3::crossProduct(inner[next] - inner[i], outer[i] - inner[i]).normalized();
				const Vec3 positions[4] = {inner[i], outer[i], outer[next], inner[next]};
				Vec3 uvs[4];
				const auto center = (inner[i] + outer[i] + outer[next] + inner[next]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(inner[i], outer[i], outer[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(inner[i], outer[next], inner[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Pavilion steps (chevron: midpoints contract slower, corners faster) === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			/* Midpoint scale: normal contraction. */
			const auto outerMs = one + t0 * (culetRatio - one);
			const auto innerMs = one + t1 * (culetRatio - one);

			/* Corner scale: faster contraction via chevronDepth. */
			const auto cornerCulet = culetRatio * (one - chevronDepth);
			const auto outerCs = one + t0 * (cornerCulet - one);
			const auto innerCs = one + t1 * (cornerCulet - one);

			const auto outerY = t0 * pavilionDepth;
			const auto innerY = t1 * pavilionDepth;

			const auto outer = makePrincessRing(outerMs, outerCs, outerY);
			const auto inner = makePrincessRing(innerMs, innerCs, innerY);

			for ( size_t i = 0; i < 8; ++i )
			{
				const auto next = (i + 1) % 8;
				const auto normal = Vec3::crossProduct(outer[next] - outer[i], inner[i] - outer[i]).normalized();
				const Vec3 positions[4] = {outer[i], inner[i], inner[next], outer[next]};
				Vec3 uvs[4];
				const auto center = (outer[i] + outer[next] + inner[next] + inner[i]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(outer[i], inner[i], inner[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(outer[i], inner[next], outer[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Culet (flat octagon at Y = +pavilionDepth, opposite winding) === */
		{
			const auto cornerCulet = culetRatio * (one - chevronDepth);
			const auto culet = makePrincessRing(culetRatio, cornerCulet, pavilionDepth);

			const auto edge1 = culet[1] - culet[0];
			const auto edge2 = culet[7] - culet[0];
			const auto culetNormal = Vec3::crossProduct(edge2, edge1).normalized();

			std::array< Vec3, 8 > culetUV;

			for ( size_t i = 0; i < 8; ++i )
			{
				culetUV[i] = Vec3(
					(culet[i][Math::X] / (halfS * culetRatio) + one) * half,
					(culet[i][Math::Z] / (halfS * culetRatio) + one) * half,
					zero);
			}

			for ( size_t i = 1; i + 1 < 8; ++i )
			{
				emitTriangle(culet[0], culet[i + 1], culet[i], culetNormal, culetUV[0], culetUV[i + 1], culetUV[i]);
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a trillion-cut gem shape (triangular with beveled corners).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The circumscribed circle radius. Default 1.
	 * @param depth The total Y depth of the gem. Default 0.8.
	 * @param tableRatio The table size as fraction of size. Default 0.55.
	 * @param cornerBevel The corner bevel amount (0-1). Default 0.2.
	 * @param steps The number of step facets on crown and pavilion. Default 3.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateTrillionCutGem (
		vertex_data_t size = 1,
		vertex_data_t depth = static_cast< vertex_data_t >(0.8),
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.55),
		vertex_data_t cornerBevel = static_cast< vertex_data_t >(0.2),
		size_t steps = 3,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		/* 6 vertices per ring: 3 corners beveled into 2 vertices each. */
		constexpr size_t RingVerts = 6;
		const auto triangleCount = 2 * (RingVerts - 2) + 4 * RingVerts * steps;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto crownHeight = depth * static_cast< vertex_data_t >(0.25);
		const auto pavilionDepth = depth * static_cast< vertex_data_t >(0.75);
		const auto culetRatio = static_cast< vertex_data_t >(0.1);
		const auto pi = std::numbers::pi_v< vertex_data_t >;

		/* Equilateral triangle corners at 90°, 210°, 330° (pointing +X). */
		const std::array< Vec3, 3 > cornerDirs = {{
			Vec3(std::cos(pi * half), zero, std::sin(pi * half)),
			Vec3(std::cos(pi * half + pi * static_cast< vertex_data_t >(2) / static_cast< vertex_data_t >(3)), zero,
				 std::sin(pi * half + pi * static_cast< vertex_data_t >(2) / static_cast< vertex_data_t >(3))),
			Vec3(std::cos(pi * half + pi * static_cast< vertex_data_t >(4) / static_cast< vertex_data_t >(3)), zero,
				 std::sin(pi * half + pi * static_cast< vertex_data_t >(4) / static_cast< vertex_data_t >(3)))
		}};

		/* Build a beveled triangle ring at scale s and height y. */
		const auto makeTriRing = [&](vertex_data_t s, vertex_data_t y) -> std::array< Vec3, 6 > {
			std::array< Vec3, 6 > ring;

			for ( size_t c = 0; c < 3; ++c )
			{
				const auto & curr = cornerDirs[c];
				const auto & prev = cornerDirs[(c + 2) % 3];
				const auto & next = cornerDirs[(c + 1) % 3];

				const auto cornerPos = curr * size * s;
				const auto toPrev = (prev * size * s - cornerPos).normalized();
				const auto toNext = (next * size * s - cornerPos).normalized();

				const auto bevelDist = cornerBevel * size * s;

				ring[c * 2]     = Vec3(cornerPos[Math::X] + toPrev[Math::X] * bevelDist, y,
									   cornerPos[Math::Z] + toPrev[Math::Z] * bevelDist);
				ring[c * 2 + 1] = Vec3(cornerPos[Math::X] + toNext[Math::X] * bevelDist, y,
									   cornerPos[Math::Z] + toNext[Math::Z] * bevelDist);
			}

			return ring;
		};

		const auto maxExtent = std::max(size, std::max(crownHeight, pavilionDepth));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table === */
		{
			const auto table = makeTriRing(tableRatio, -crownHeight);

			const auto edge1 = table[1] - table[0];
			const auto edge2 = table[RingVerts - 1] - table[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			std::array< Vec3, RingVerts > tableUV;

			for ( size_t i = 0; i < RingVerts; ++i )
			{
				tableUV[i] = Vec3(
					(table[i][Math::X] / (size * tableRatio) + one) * half,
					(table[i][Math::Z] / (size * tableRatio) + one) * half,
					zero);
			}

			for ( size_t i = 1; i + 1 < RingVerts; ++i )
			{
				emitTriangle(table[0], table[i], table[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown steps === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto innerScale = tableRatio + t0 * (one - tableRatio);
			const auto outerScale = tableRatio + t1 * (one - tableRatio);
			const auto innerY = -crownHeight + t0 * crownHeight;
			const auto outerY = -crownHeight + t1 * crownHeight;

			const auto inner = makeTriRing(innerScale, innerY);
			const auto outer = makeTriRing(outerScale, outerY);

			for ( size_t i = 0; i < RingVerts; ++i )
			{
				const auto next = (i + 1) % RingVerts;
				const auto normal = Vec3::crossProduct(inner[next] - inner[i], outer[i] - inner[i]).normalized();
				const Vec3 positions[4] = {inner[i], outer[i], outer[next], inner[next]};
				Vec3 uvs[4];
				const auto center = (inner[i] + outer[i] + outer[next] + inner[next]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(inner[i], outer[i], outer[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(inner[i], outer[next], inner[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Pavilion steps === */
		for ( size_t step = 0; step < steps; ++step )
		{
			const auto t0 = static_cast< vertex_data_t >(step) / static_cast< vertex_data_t >(steps);
			const auto t1 = static_cast< vertex_data_t >(step + 1) / static_cast< vertex_data_t >(steps);

			const auto outerScale = one + t0 * (culetRatio - one);
			const auto innerScale = one + t1 * (culetRatio - one);
			const auto outerY = t0 * pavilionDepth;
			const auto innerY = t1 * pavilionDepth;

			const auto outer = makeTriRing(outerScale, outerY);
			const auto inner = makeTriRing(innerScale, innerY);

			for ( size_t i = 0; i < RingVerts; ++i )
			{
				const auto next = (i + 1) % RingVerts;
				const auto normal = Vec3::crossProduct(outer[next] - outer[i], inner[i] - outer[i]).normalized();
				const Vec3 positions[4] = {outer[i], inner[i], inner[next], outer[next]};
				Vec3 uvs[4];
				const auto center = (outer[i] + outer[next] + inner[next] + inner[i]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(outer[i], inner[i], inner[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(outer[i], inner[next], outer[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Culet === */
		{
			const auto culet = makeTriRing(culetRatio, pavilionDepth);

			const auto edge1 = culet[1] - culet[0];
			const auto edge2 = culet[RingVerts - 1] - culet[0];
			const auto culetNormal = Vec3::crossProduct(edge2, edge1).normalized();

			std::array< Vec3, RingVerts > culetUV;

			for ( size_t i = 0; i < RingVerts; ++i )
			{
				culetUV[i] = Vec3(
					(culet[i][Math::X] / (size * culetRatio) + one) * half,
					(culet[i][Math::Z] / (size * culetRatio) + one) * half,
					zero);
			}

			for ( size_t i = 1; i + 1 < RingVerts; ++i )
			{
				emitTriangle(culet[0], culet[i + 1], culet[i], culetNormal, culetUV[0], culetUV[i + 1], culetUV[i]);
			}
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates an oval-cut gem shape (elliptical brilliant cut).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param length The X-axis radius (longer axis). Default 1.
	 * @param width The Z-axis radius (shorter axis). Default 0.65.
	 * @param facets The number of facets around the gem. Default 16.
	 * @param tableRatio The table size as fraction (0-1). Default 0.55.
	 * @param crownAngle The crown slope in degrees. Default 35.
	 * @param pavilionAngle The pavilion slope in degrees. Default 41.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateOvalCutGem (
		vertex_data_t length = 1,
		vertex_data_t width = static_cast< vertex_data_t >(0.65),
		size_t facets = 16,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.55),
		vertex_data_t crownAngle = 35,
		vertex_data_t pavilionAngle = 41,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		const auto triangleCount = 4 * n - 2;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto degToRad = pi / static_cast< vertex_data_t >(180);
		const auto crownAngleRad = crownAngle * degToRad;
		const auto pavilionAngleRad = pavilionAngle * degToRad;

		const auto avgRadius = (length + width) * half;
		const auto tableRadius = avgRadius * tableRatio;
		const auto crownHeight = (avgRadius - tableRadius) * std::tan(crownAngleRad);
		const auto pavilionDepth = avgRadius * std::tan(pavilionAngleRad);

		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		std::vector< Vec3 > tableRing(n);
		std::vector< Vec3 > girdleRing(n);

		for ( size_t i = 0; i < n; ++i )
		{
			const auto theta = static_cast< vertex_data_t >(i) * angleStep;
			const auto cosT = std::cos(theta);
			const auto sinT = std::sin(theta);

			tableRing[i] = Vec3(length * tableRatio * cosT, -crownHeight, width * tableRatio * sinT);
			girdleRing[i] = Vec3(length * cosT, zero, width * sinT);
		}

		const Vec3 culet(zero, pavilionDepth, zero);

		const auto maxExtent = std::max(length, std::max(width, std::max(crownHeight, pavilionDepth)));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table === */
		{
			const auto edge1 = tableRing[1] - tableRing[0];
			const auto edge2 = tableRing[n - 1] - tableRing[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			std::vector< Vec3 > tableUV(n);

			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				tableUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}

			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(tableRing[0], tableRing[i], tableRing[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & t0 = tableRing[i];
			const auto & t1 = tableRing[i1];
			const auto & g0 = girdleRing[i];
			const auto & g1 = girdleRing[i1];

			const auto center = (t0 + t1 + g0 + g1) / static_cast< vertex_data_t >(4);
			const auto normal = Vec3::crossProduct(t1 - t0, g0 - t0).normalized();

			const Vec3 positions[4] = {t0, t1, g1, g0};
			Vec3 uvs[4];
			computeFaceUV(normal, center, positions, uvs, 4);

			emitTriangle(t0, g0, g1, normal, uvs[0], uvs[3], uvs[2]);
			emitTriangle(t0, g1, t1, normal, uvs[0], uvs[2], uvs[1]);
		}

		/* === Pavilion === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & g0 = girdleRing[i];
			const auto & g1 = girdleRing[i1];

			const auto center = (g0 + g1 + culet) / static_cast< vertex_data_t >(3);
			const auto normal = Vec3::crossProduct(culet - g0, g1 - g0).normalized();

			const Vec3 positions[3] = {g0, g1, culet};
			Vec3 uvs[3];
			computeFaceUV(normal, center, positions, uvs, 3);

			emitTriangle(g0, culet, g1, normal, uvs[0], uvs[2], uvs[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a cushion-cut gem shape (rounded rectangle brilliant cut).
	 * @note Ready for vulkan default world axis. Uses superellipse outline.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param length The X-axis radius. Default 1.
	 * @param width The Z-axis radius. Default 0.85.
	 * @param power The superellipse exponent (2=ellipse, higher=more square). Default 2.5.
	 * @param facets The number of facets around the gem. Default 16.
	 * @param tableRatio The table size as fraction (0-1). Default 0.55.
	 * @param crownAngle The crown slope in degrees. Default 35.
	 * @param pavilionAngle The pavilion slope in degrees. Default 41.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCushionCutGem (
		vertex_data_t length = 1,
		vertex_data_t width = static_cast< vertex_data_t >(0.85),
		vertex_data_t power = static_cast< vertex_data_t >(2.5),
		size_t facets = 16,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.55),
		vertex_data_t crownAngle = 35,
		vertex_data_t pavilionAngle = 41,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		const auto triangleCount = 4 * n - 2;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto degToRad = pi / static_cast< vertex_data_t >(180);
		const auto crownAngleRad = crownAngle * degToRad;
		const auto pavilionAngleRad = pavilionAngle * degToRad;

		const auto avgRadius = (length + width) * half;
		const auto tableAvg = avgRadius * tableRatio;
		const auto crownHeight = (avgRadius - tableAvg) * std::tan(crownAngleRad);
		const auto pavilionDepth = avgRadius * std::tan(pavilionAngleRad);

		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		/* Superellipse point: |x/a|^p + |z/b|^p = 1
		 * x = a * sign(cos) * |cos|^(2/p), z = b * sign(sin) * |sin|^(2/p) */
		const auto invP = static_cast< vertex_data_t >(2) / power;
		const auto superEllipsePoint = [invP](vertex_data_t a, vertex_data_t b, vertex_data_t theta) -> std::pair< vertex_data_t, vertex_data_t > {
			const auto cosT = std::cos(theta);
			const auto sinT = std::sin(theta);
			const auto signCos = cosT < 0 ? static_cast< vertex_data_t >(-1) : static_cast< vertex_data_t >(1);
			const auto signSin = sinT < 0 ? static_cast< vertex_data_t >(-1) : static_cast< vertex_data_t >(1);

			return {
				a * signCos * std::pow(std::abs(cosT), invP),
				b * signSin * std::pow(std::abs(sinT), invP)
			};
		};

		std::vector< Vec3 > tableRingVec(n);
		std::vector< Vec3 > girdleRingVec(n);

		for ( size_t i = 0; i < n; ++i )
		{
			const auto theta = static_cast< vertex_data_t >(i) * angleStep;
			const auto [tx, tz] = superEllipsePoint(length * tableRatio, width * tableRatio, theta);
			const auto [gx, gz] = superEllipsePoint(length, width, theta);

			tableRingVec[i] = Vec3(tx, -crownHeight, tz);
			girdleRingVec[i] = Vec3(gx, zero, gz);
		}

		const Vec3 culet(zero, pavilionDepth, zero);

		const auto maxExtent = std::max(length, std::max(width, std::max(crownHeight, pavilionDepth)));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table === */
		{
			const auto edge1 = tableRingVec[1] - tableRingVec[0];
			const auto edge2 = tableRingVec[n - 1] - tableRingVec[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			std::vector< Vec3 > tableUV(n);

			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				tableUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}

			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(tableRingVec[0], tableRingVec[i], tableRingVec[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & t0 = tableRingVec[i];
			const auto & t1 = tableRingVec[i1];
			const auto & g0 = girdleRingVec[i];
			const auto & g1 = girdleRingVec[i1];

			const auto center = (t0 + t1 + g0 + g1) / static_cast< vertex_data_t >(4);
			const auto normal = Vec3::crossProduct(t1 - t0, g0 - t0).normalized();

			const Vec3 positions[4] = {t0, t1, g1, g0};
			Vec3 uvs[4];
			computeFaceUV(normal, center, positions, uvs, 4);

			emitTriangle(t0, g0, g1, normal, uvs[0], uvs[3], uvs[2]);
			emitTriangle(t0, g1, t1, normal, uvs[0], uvs[2], uvs[1]);
		}

		/* === Pavilion === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & g0 = girdleRingVec[i];
			const auto & g1 = girdleRingVec[i1];

			const auto center = (g0 + g1 + culet) / static_cast< vertex_data_t >(3);
			const auto normal = Vec3::crossProduct(culet - g0, g1 - g0).normalized();

			const Vec3 positions[3] = {g0, g1, culet};
			Vec3 uvs[3];
			computeFaceUV(normal, center, positions, uvs, 3);

			emitTriangle(g0, culet, g1, normal, uvs[0], uvs[2], uvs[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a marquise-cut gem shape (pointed elliptical brilliant cut).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param length The X-axis radius (longer axis with points). Default 1.2.
	 * @param width The Z-axis radius (widest point). Default 0.5.
	 * @param sharpness The exponent controlling tip pointedness (1=ellipse, higher=sharper). Default 1.5.
	 * @param facets The number of facets. Default 16.
	 * @param tableRatio The table size fraction. Default 0.5.
	 * @param crownAngle The crown slope in degrees. Default 33.
	 * @param pavilionAngle The pavilion slope in degrees. Default 42.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateMarquiseCutGem (
		vertex_data_t length = static_cast< vertex_data_t >(1.2),
		vertex_data_t width = static_cast< vertex_data_t >(0.5),
		vertex_data_t sharpness = static_cast< vertex_data_t >(1.5),
		size_t facets = 16,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.5),
		vertex_data_t crownAngle = 33,
		vertex_data_t pavilionAngle = 42,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		const auto triangleCount = 4 * n - 2;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto degToRad = pi / static_cast< vertex_data_t >(180);
		const auto crownAngleRad = crownAngle * degToRad;
		const auto pavilionAngleRad = pavilionAngle * degToRad;

		const auto avgRadius = (length + width) * half;
		const auto tableAvg = avgRadius * tableRatio;
		const auto crownHeight = (avgRadius - tableAvg) * std::tan(crownAngleRad);
		const auto pavilionDepth = avgRadius * std::tan(pavilionAngleRad);

		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		/* Marquise outline: x = length * cos(θ), z = width * sign(sin(θ)) * |sin(θ)|^sharpness */
		const auto marquisePoint = [&](vertex_data_t a, vertex_data_t b, vertex_data_t theta) -> std::pair< vertex_data_t, vertex_data_t > {
			const auto cosT = std::cos(theta);
			const auto sinT = std::sin(theta);
			const auto signSin = sinT < zero ? static_cast< vertex_data_t >(-1) : one;

			return {
				a * cosT,
				b * signSin * std::pow(std::abs(sinT), sharpness)
			};
		};

		std::vector< Vec3 > tableRingVec(n);
		std::vector< Vec3 > girdleRingVec(n);

		for ( size_t i = 0; i < n; ++i )
		{
			const auto theta = static_cast< vertex_data_t >(i) * angleStep;
			const auto [tx, tz] = marquisePoint(length * tableRatio, width * tableRatio, theta);
			const auto [gx, gz] = marquisePoint(length, width, theta);

			tableRingVec[i] = Vec3(tx, -crownHeight, tz);
			girdleRingVec[i] = Vec3(gx, zero, gz);
		}

		const Vec3 culet(zero, pavilionDepth, zero);

		const auto maxExtent = std::max(length, std::max(width, std::max(crownHeight, pavilionDepth)));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcA);
			builder.setVertexColor(volumetricColor(vA));
			builder.newVertex();

			builder.setPosition(vC);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcC);
			builder.setVertexColor(volumetricColor(vC));
			builder.newVertex();

			builder.setPosition(vB);
			builder.setNormal(normal);
			builder.setTextureCoordinates(tcB);
			builder.setVertexColor(volumetricColor(vB));
			builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);

			auto uMin = std::numeric_limits< vertex_data_t >::max();
			auto uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max();
			auto vMax = std::numeric_limits< vertex_data_t >::lowest();

			std::vector< vertex_data_t > lu(count), lv(count);

			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]);
				uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]);
				vMax = std::max(vMax, lv[i]);
			}

			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);

			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table === */
		{
			const auto edge1 = tableRingVec[1] - tableRingVec[0];
			const auto edge2 = tableRingVec[n - 1] - tableRingVec[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();

			std::vector< Vec3 > tableUV(n);

			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				tableUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}

			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(tableRingVec[0], tableRingVec[i], tableRingVec[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & t0 = tableRingVec[i];
			const auto & t1 = tableRingVec[i1];
			const auto & g0 = girdleRingVec[i];
			const auto & g1 = girdleRingVec[i1];

			const auto center = (t0 + t1 + g0 + g1) / static_cast< vertex_data_t >(4);
			const auto normal = Vec3::crossProduct(t1 - t0, g0 - t0).normalized();

			const Vec3 positions[4] = {t0, t1, g1, g0};
			Vec3 uvs[4];
			computeFaceUV(normal, center, positions, uvs, 4);

			emitTriangle(t0, g0, g1, normal, uvs[0], uvs[3], uvs[2]);
			emitTriangle(t0, g1, t1, normal, uvs[0], uvs[2], uvs[1]);
		}

		/* === Pavilion === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & g0 = girdleRingVec[i];
			const auto & g1 = girdleRingVec[i1];

			const auto center = (g0 + g1 + culet) / static_cast< vertex_data_t >(3);
			const auto normal = Vec3::crossProduct(culet - g0, g1 - g0).normalized();

			const Vec3 positions[3] = {g0, g1, culet};
			Vec3 uvs[3];
			computeFaceUV(normal, center, positions, uvs, 3);

			emitTriangle(g0, culet, g1, normal, uvs[0], uvs[2], uvs[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a pear-cut gem shape (teardrop brilliant cut).
	 * @note Ready for vulkan default world axis. One end rounded, one end pointed.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param length The X-axis radius (tip-to-round axis). Default 1.
	 * @param width The Z-axis radius (widest point). Default 0.6.
	 * @param sharpness The tip pointedness exponent (higher = sharper). Default 1.6.
	 * @param facets The number of facets. Default 16.
	 * @param tableRatio The table size fraction. Default 0.5.
	 * @param crownAngle The crown slope in degrees. Default 34.
	 * @param pavilionAngle The pavilion slope in degrees. Default 42.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generatePearCutGem (
		vertex_data_t length = 1,
		vertex_data_t width = static_cast< vertex_data_t >(0.6),
		vertex_data_t sharpness = static_cast< vertex_data_t >(1.6),
		size_t facets = 16,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.5),
		vertex_data_t crownAngle = 34,
		vertex_data_t pavilionAngle = 42,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		const auto triangleCount = 4 * n - 2;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto degToRad = pi / static_cast< vertex_data_t >(180);
		const auto crownAngleRad = crownAngle * degToRad;
		const auto pavilionAngleRad = pavilionAngle * degToRad;

		const auto avgRadius = (length + width) * half;
		const auto tableAvg = avgRadius * tableRatio;
		const auto crownHeight = (avgRadius - tableAvg) * std::tan(crownAngleRad);
		const auto pavilionDepth = avgRadius * std::tan(pavilionAngleRad);

		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		/* Pear outline: asymmetric — pointed at +X, rounded at -X.
		 * Smoothly blend sharpness from max at θ=0 (pointed end) to 1 at θ=π (round end). */
		const auto pearPoint = [&](vertex_data_t a, vertex_data_t b, vertex_data_t theta) -> std::pair< vertex_data_t, vertex_data_t > {
			const auto cosT = std::cos(theta);
			const auto sinT = std::sin(theta);
			const auto blendFactor = (one + cosT) * half;
			const auto localSharpness = one + (sharpness - one) * blendFactor;
			const auto signSin = sinT < zero ? static_cast< vertex_data_t >(-1) : one;

			return {
				a * cosT,
				b * signSin * std::pow(std::abs(sinT), localSharpness)
			};
		};

		std::vector< Vec3 > tableRingVec(n);
		std::vector< Vec3 > girdleRingVec(n);

		for ( size_t i = 0; i < n; ++i )
		{
			const auto theta = static_cast< vertex_data_t >(i) * angleStep;
			const auto [tx, tz] = pearPoint(length * tableRatio, width * tableRatio, theta);
			const auto [gx, gz] = pearPoint(length, width, theta);

			tableRingVec[i] = Vec3(tx, -crownHeight, tz);
			girdleRingVec[i] = Vec3(gx, zero, gz);
		}

		const Vec3 culet(zero, pavilionDepth, zero);

		const auto maxExtent = std::max(length, std::max(width, std::max(crownHeight, pavilionDepth)));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA); builder.setNormal(normal); builder.setTextureCoordinates(tcA); builder.setVertexColor(volumetricColor(vA)); builder.newVertex();
			builder.setPosition(vC); builder.setNormal(normal); builder.setTextureCoordinates(tcC); builder.setVertexColor(volumetricColor(vC)); builder.newVertex();
			builder.setPosition(vB); builder.setNormal(normal); builder.setTextureCoordinates(tcB); builder.setVertexColor(volumetricColor(vB)); builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);
			auto uMin = std::numeric_limits< vertex_data_t >::max(), uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max(), vMax = std::numeric_limits< vertex_data_t >::lowest();
			std::vector< vertex_data_t > lu(count), lv(count);
			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]); uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]); vMax = std::max(vMax, lv[i]);
			}
			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);
			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table === */
		{
			const auto edge1 = tableRingVec[1] - tableRingVec[0];
			const auto edge2 = tableRingVec[n - 1] - tableRingVec[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();
			std::vector< Vec3 > tableUV(n);
			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				tableUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}
			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(tableRingVec[0], tableRingVec[i], tableRingVec[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & t0 = tableRingVec[i]; const auto & t1 = tableRingVec[i1];
			const auto & g0 = girdleRingVec[i]; const auto & g1 = girdleRingVec[i1];
			const auto center = (t0 + t1 + g0 + g1) / static_cast< vertex_data_t >(4);
			const auto normal = Vec3::crossProduct(t1 - t0, g0 - t0).normalized();
			const Vec3 positions[4] = {t0, t1, g1, g0};
			Vec3 uvs[4];
			computeFaceUV(normal, center, positions, uvs, 4);
			emitTriangle(t0, g0, g1, normal, uvs[0], uvs[3], uvs[2]);
			emitTriangle(t0, g1, t1, normal, uvs[0], uvs[2], uvs[1]);
		}

		/* === Pavilion === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & g0 = girdleRingVec[i]; const auto & g1 = girdleRingVec[i1];
			const auto center = (g0 + g1 + culet) / static_cast< vertex_data_t >(3);
			const auto normal = Vec3::crossProduct(culet - g0, g1 - g0).normalized();
			const Vec3 positions[3] = {g0, g1, culet};
			Vec3 uvs[3];
			computeFaceUV(normal, center, positions, uvs, 3);
			emitTriangle(g0, culet, g1, normal, uvs[0], uvs[2], uvs[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a heart-cut gem shape (heart-shaped brilliant cut).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param size The overall size of the heart. Default 1.
	 * @param facets The number of facets. Default 24.
	 * @param tableRatio The table size fraction. Default 0.5.
	 * @param crownAngle The crown slope in degrees. Default 34.
	 * @param pavilionAngle The pavilion slope in degrees. Default 42.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateHeartCutGem (
		vertex_data_t size = 1,
		size_t facets = 24,
		vertex_data_t tableRatio = static_cast< vertex_data_t >(0.5),
		vertex_data_t crownAngle = 34,
		vertex_data_t pavilionAngle = 42,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		const auto triangleCount = 4 * n - 2;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto degToRad = pi / static_cast< vertex_data_t >(180);
		const auto crownAngleRad = crownAngle * degToRad;
		const auto pavilionAngleRad = pavilionAngle * degToRad;

		const auto crownHeight = size * static_cast< vertex_data_t >(0.15) * std::tan(crownAngleRad);
		const auto pavilionDepth = size * static_cast< vertex_data_t >(0.5) * std::tan(pavilionAngleRad);

		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		/* Heart curve parametric: x = sin(t)³, z = (13cos(t) - 5cos(2t) - 2cos(3t) - cos(4t)) / 16
		 * Rotated so bottom tip points to +X (along length axis). */
		const auto heartPoint = [&](vertex_data_t scale, vertex_data_t t) -> std::pair< vertex_data_t, vertex_data_t > {
			const auto sinT = std::sin(t);
			const auto cosT = std::cos(t);
			const auto cos2T = std::cos(static_cast< vertex_data_t >(2) * t);
			const auto cos3T = std::cos(static_cast< vertex_data_t >(3) * t);
			const auto cos4T = std::cos(static_cast< vertex_data_t >(4) * t);

			const auto hx = sinT * sinT * sinT;
			const auto hz = (static_cast< vertex_data_t >(13) * cosT
							- static_cast< vertex_data_t >(5) * cos2T
							- static_cast< vertex_data_t >(2) * cos3T
							- cos4T) / static_cast< vertex_data_t >(16);

			/* Rotate 90°: heart tip points +X, top lobes point -X → z=hx, x=hz (CCW ring) */
			return {
				hz * scale * size,
				hx * scale * size
			};
		};

		std::vector< Vec3 > tableRingVec(n);
		std::vector< Vec3 > girdleRingVec(n);

		for ( size_t i = 0; i < n; ++i )
		{
			const auto theta = static_cast< vertex_data_t >(i) * angleStep;
			const auto [tx, tz] = heartPoint(tableRatio, theta);
			const auto [gx, gz] = heartPoint(one, theta);

			tableRingVec[i] = Vec3(tx, -crownHeight, tz);
			girdleRingVec[i] = Vec3(gx, zero, gz);
		}

		const Vec3 culet(zero, pavilionDepth, zero);

		const auto maxExtent = std::max(size, std::max(crownHeight, pavilionDepth));
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA); builder.setNormal(normal); builder.setTextureCoordinates(tcA); builder.setVertexColor(volumetricColor(vA)); builder.newVertex();
			builder.setPosition(vC); builder.setNormal(normal); builder.setTextureCoordinates(tcC); builder.setVertexColor(volumetricColor(vC)); builder.newVertex();
			builder.setPosition(vB); builder.setNormal(normal); builder.setTextureCoordinates(tcB); builder.setVertexColor(volumetricColor(vB)); builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);
			auto uMin = std::numeric_limits< vertex_data_t >::max(), uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max(), vMax = std::numeric_limits< vertex_data_t >::lowest();
			std::vector< vertex_data_t > lu(count), lv(count);
			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]); uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]); vMax = std::max(vMax, lv[i]);
			}
			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);
			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Table === */
		{
			const auto edge1 = tableRingVec[1] - tableRingVec[0];
			const auto edge2 = tableRingVec[n - 1] - tableRingVec[0];
			const auto tableNormal = Vec3::crossProduct(edge1, edge2).normalized();
			std::vector< Vec3 > tableUV(n);
			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				tableUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}
			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(tableRingVec[0], tableRingVec[i], tableRingVec[i + 1], tableNormal, tableUV[0], tableUV[i], tableUV[i + 1]);
			}
		}

		/* === Crown === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & t0 = tableRingVec[i]; const auto & t1 = tableRingVec[i1];
			const auto & g0 = girdleRingVec[i]; const auto & g1 = girdleRingVec[i1];
			const auto center = (t0 + t1 + g0 + g1) / static_cast< vertex_data_t >(4);
			const auto normal = Vec3::crossProduct(t1 - t0, g0 - t0).normalized();
			const Vec3 positions[4] = {t0, t1, g1, g0};
			Vec3 uvs[4];
			computeFaceUV(normal, center, positions, uvs, 4);
			emitTriangle(t0, g0, g1, normal, uvs[0], uvs[3], uvs[2]);
			emitTriangle(t0, g1, t1, normal, uvs[0], uvs[2], uvs[1]);
		}

		/* === Pavilion === */
		for ( size_t i = 0; i < n; ++i )
		{
			const auto i1 = (i + 1) % n;
			const auto & g0 = girdleRingVec[i]; const auto & g1 = girdleRingVec[i1];
			const auto center = (g0 + g1 + culet) / static_cast< vertex_data_t >(3);
			const auto normal = Vec3::crossProduct(culet - g0, g1 - g0).normalized();
			const Vec3 positions[3] = {g0, g1, culet};
			Vec3 uvs[3];
			computeFaceUV(normal, center, positions, uvs, 3);
			emitTriangle(g0, culet, g1, normal, uvs[0], uvs[2], uvs[1]);
		}

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a rose-cut gem shape (domed top with triangular facets, flat bottom).
	 * @note Ready for vulkan default world axis.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The base radius. Default 1.
	 * @param height The dome height. Default 0.6.
	 * @param rings The number of concentric rings from base to apex. Default 3.
	 * @param facets The number of facets around the gem. Default 12.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateRoseCutGem (
		vertex_data_t radius = 1,
		vertex_data_t height = static_cast< vertex_data_t >(0.6),
		size_t rings = 3,
		size_t facets = 12,
		const ShapeBuilderOptions< vertex_data_t > & options = {}
	) noexcept
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		using Vec3 = Math::Vector< 3, vertex_data_t >;
		using Vec4 = Math::Vector< 4, vertex_data_t >;

		constexpr auto half = static_cast< vertex_data_t >(0.5);
		constexpr auto one = static_cast< vertex_data_t >(1);
		constexpr auto zero = static_cast< vertex_data_t >(0);

		const auto n = facets;
		/* Flat base: n-2 triangles.
		 * Dome rings: (rings-1) bands of n quads (2n tris each) + top cap of n triangles. */
		const auto triangleCount = (n - 2) + 2 * n * (rings - 1) + n;

		Shape< vertex_data_t, index_data_t > shape{static_cast< uint32_t >(triangleCount)};
		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto pi = std::numbers::pi_v< vertex_data_t >;
		const auto twoPi = static_cast< vertex_data_t >(2) * pi;
		const auto angleStep = twoPi / static_cast< vertex_data_t >(n);

		const auto maxExtent = std::max(radius, height);
		const auto invExtent = one / maxExtent;
		const auto volumetricColor = [half, one, invExtent](const Vec3 & pos) {
			return Vec4(
				(pos[Math::X] * invExtent + one) * half,
				(pos[Math::Y] * invExtent + one) * half,
				(pos[Math::Z] * invExtent + one) * half,
				one
			);
		};

		const auto emitTriangle = [&](
			const Vec3 & vA, const Vec3 & vB, const Vec3 & vC,
			const Vec3 & normal, const Vec3 & tcA, const Vec3 & tcB, const Vec3 & tcC)
		{
			builder.setPosition(vA); builder.setNormal(normal); builder.setTextureCoordinates(tcA); builder.setVertexColor(volumetricColor(vA)); builder.newVertex();
			builder.setPosition(vC); builder.setNormal(normal); builder.setTextureCoordinates(tcC); builder.setVertexColor(volumetricColor(vC)); builder.newVertex();
			builder.setPosition(vB); builder.setNormal(normal); builder.setTextureCoordinates(tcB); builder.setVertexColor(volumetricColor(vB)); builder.newVertex();
		};

		const auto computeFaceUV = [&one](
			const Vec3 & normal, const Vec3 & center,
			const Vec3 * positions, Vec3 * uvs, size_t count)
		{
			/* Project world-Y onto face plane for consistent texture orientation. */
			const Vec3 upDir(static_cast< vertex_data_t >(0), -one, static_cast< vertex_data_t >(0));
			auto rawT = upDir - normal * Vec3::dotProduct(upDir, normal);
			if ( Vec3::dotProduct(rawT, rawT) < static_cast< vertex_data_t >(0.0001) )
			{
				const Vec3 rightDir(one, static_cast< vertex_data_t >(0), static_cast< vertex_data_t >(0));
				rawT = rightDir - normal * Vec3::dotProduct(rightDir, normal);
			}
			const auto T = rawT.normalized();
			const auto B = Vec3::crossProduct(normal, T);
			auto uMin = std::numeric_limits< vertex_data_t >::max(), uMax = std::numeric_limits< vertex_data_t >::lowest();
			auto vMin = std::numeric_limits< vertex_data_t >::max(), vMax = std::numeric_limits< vertex_data_t >::lowest();
			std::vector< vertex_data_t > lu(count), lv(count);
			for ( size_t i = 0; i < count; ++i )
			{
				lu[i] = Vec3::dotProduct(positions[i] - center, T);
				lv[i] = Vec3::dotProduct(positions[i] - center, B);
				uMin = std::min(uMin, lu[i]); uMax = std::max(uMax, lu[i]);
				vMin = std::min(vMin, lv[i]); vMax = std::max(vMax, lv[i]);
			}
			const auto invScale = one / std::max(uMax - uMin, vMax - vMin);
			for ( size_t i = 0; i < count; ++i )
			{
				uvs[i] = Vec3((lu[i] - uMin) * invScale, (lv[i] - vMin) * invScale, 0);
			}
		};

		/* Precompute dome rings. Ring 0 = base (radius, Y=0), ring `rings` = apex.
		 * Dome points downward (+Y) like a pavilion; flat base at Y=0 faces sky. */
		std::vector< std::vector< Vec3 > > domeRings(rings + 1);

		for ( size_t r = 0; r <= rings; ++r )
		{
			const auto t = static_cast< vertex_data_t >(r) / static_cast< vertex_data_t >(rings);
			const auto ringRadius = radius * (one - t);
			/* Dome profile: parabolic height = h * t² for gentle dome (downward). */
			const auto ringY = height * t * t;

			domeRings[r].resize(n);

			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				domeRings[r][i] = Vec3(ringRadius * std::cos(theta), ringY, ringRadius * std::sin(theta));
			}
		}

		const Vec3 apex(zero, height, zero);

		builder.beginConstruction(ConstructionMode::Triangles);

		/* === Flat base (Y = 0, facing sky = -Y) === */
		{
			const auto & baseRing = domeRings[0];
			const auto edge1 = baseRing[1] - baseRing[0];
			const auto edge2 = baseRing[n - 1] - baseRing[0];
			const auto baseNormal = Vec3::crossProduct(edge1, edge2).normalized();

			std::vector< Vec3 > baseUV(n);

			for ( size_t i = 0; i < n; ++i )
			{
				const auto theta = static_cast< vertex_data_t >(i) * angleStep;
				baseUV[i] = Vec3((std::cos(theta) + one) * half, (std::sin(theta) + one) * half, zero);
			}

			for ( size_t i = 1; i + 1 < n; ++i )
			{
				emitTriangle(baseRing[0], baseRing[i], baseRing[i + 1], baseNormal, baseUV[0], baseUV[i], baseUV[i + 1]);
			}
		}

		/* === Dome bands (pavilion-style: outer=top/large, inner=bottom/small) === */
		for ( size_t r = 0; r + 1 < rings; ++r )
		{
			const auto & outer = domeRings[r];
			const auto & inner = domeRings[r + 1];

			for ( size_t i = 0; i < n; ++i )
			{
				const auto next = (i + 1) % n;
				const auto normal = Vec3::crossProduct(inner[i] - outer[i], outer[next] - outer[i]).normalized();
				const Vec3 positions[4] = {outer[i], inner[i], inner[next], outer[next]};
				Vec3 uvs[4];
				const auto center = (outer[i] + outer[next] + inner[next] + inner[i]) / static_cast< vertex_data_t >(4);
				computeFaceUV(normal, center, positions, uvs, 4);

				emitTriangle(outer[i], inner[i], inner[next], normal, uvs[0], uvs[1], uvs[2]);
				emitTriangle(outer[i], inner[next], outer[next], normal, uvs[0], uvs[2], uvs[3]);
			}
		}

		/* === Apex cap (last ring to apex point) === */
		{
			const auto & lastRing = domeRings[rings - 1];

			for ( size_t i = 0; i < n; ++i )
			{
				const auto next = (i + 1) % n;
				const auto center = (lastRing[i] + lastRing[next] + apex) / static_cast< vertex_data_t >(3);
				const auto normal = Vec3::crossProduct(apex - lastRing[i], lastRing[next] - lastRing[i]).normalized();

				const Vec3 positions[3] = {lastRing[i], lastRing[next], apex};
				Vec3 uvs[3];
				computeFaceUV(normal, center, positions, uvs, 3);

				emitTriangle(lastRing[i], apex, lastRing[next], normal, uvs[0], uvs[2], uvs[1]);
			}
		}

		builder.endConstruction();

		return shape;
	}
}
