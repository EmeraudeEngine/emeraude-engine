/*
 * src/Libs/VertexFactory/ShapeGenerator.hpp
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
#include <cmath>
#include <array>
#include <vector>

/* Local inclusions for usages. */
#include "TextureCoordinates.hpp"
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
	generateTriangle (vertex_data_t size = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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
	generateQuad (vertex_data_t xScale, vertex_data_t yScale = 0, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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
	generateCuboid (vertex_data_t width = 1, vertex_data_t height = 0, vertex_data_t depth = 0, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		constexpr auto Half = static_cast< vertex_data_t >(0.5);
		constexpr auto SplitFactor = static_cast< vertex_data_t >(1.0);

		/* Centering value on axis */
		{
			/* X-axis */
			width *= Half;

			/* Y-axis */
			if ( height > 0 )
			{
				height *= Half;
			}
			else
			{
				height = width;
			}

			/* Z-axis */
			if ( depth > 0 )
			{
				depth *= Half;
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

			builder.setPosition(width, -height * SplitFactor, -depth * SplitFactor);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.setPosition(width, -height * SplitFactor,  depth * SplitFactor);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(width,  height * SplitFactor, -depth * SplitFactor);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(width,  height * SplitFactor,  depth * SplitFactor);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Left face (X-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeX());

			builder.setPosition(-width,  height * SplitFactor, -depth * SplitFactor);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition(-width,  height * SplitFactor,  depth * SplitFactor);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition(-width, -height * SplitFactor, -depth * SplitFactor);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(-width, -height * SplitFactor,  depth * SplitFactor);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Top face (Y+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveY());

			builder.setPosition(-width * SplitFactor,  height, -depth * SplitFactor);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition( width * SplitFactor,  height, -depth * SplitFactor);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(-width * SplitFactor,  height,  depth * SplitFactor);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition( width * SplitFactor,  height,  depth * SplitFactor);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Bottom face (Y-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeY());

			builder.setPosition(-width * SplitFactor, -height,  depth * SplitFactor);
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.setPosition( width * SplitFactor, -height,  depth * SplitFactor);
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition(-width * SplitFactor, -height, -depth * SplitFactor);
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 1, 0);
			builder.newVertex();

			builder.setPosition( width * SplitFactor, -height, -depth * SplitFactor);
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Front face (Z+) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());

			builder.setPosition(-width * SplitFactor, -height * SplitFactor,  depth); // Top-left
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(0, 0, 1);
			builder.newVertex();

			builder.setPosition(-width * SplitFactor,  height * SplitFactor,  depth); // Bottom-left
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(0, 1, 1);
			builder.newVertex();

			builder.setPosition( width * SplitFactor, -height * SplitFactor,  depth); // Top-right
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(1, 0, 1);
			builder.newVertex();

			builder.setPosition( width * SplitFactor,  height * SplitFactor,  depth); // Bottom-right
			builder.setTextureCoordinates(1, 1);
			builder.setVertexColor(1, 1, 1);
			builder.newVertex();

			builder.resetCurrentTriangle();
		}

		/* Back face (Z-) */
		{
			builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeZ());

			builder.setPosition( width * SplitFactor, -height * SplitFactor, -depth); // Top-right
			builder.setTextureCoordinates(0, 0);
			builder.setVertexColor(1, 0, 0);
			builder.newVertex();

			builder.setPosition( width * SplitFactor,  height * SplitFactor, -depth); // Bottom-right
			builder.setTextureCoordinates(0, 1);
			builder.setVertexColor(1, 1, 0);
			builder.newVertex();

			builder.setPosition(-width * SplitFactor, -height * SplitFactor, -depth); // Top-left
			builder.setTextureCoordinates(1, 0);
			builder.setVertexColor(0, 0, 0);
			builder.newVertex();

			builder.setPosition(-width * SplitFactor,  height * SplitFactor, -depth); // Bottom-left
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
	generateCuboid (const Math::Vector< 3, vertex_data_t > & size, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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
	generateCuboid (const Math::Vector< 4, vertex_data_t > & size, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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
	generateCuboid (const Math::Vector< 3, vertex_data_t > & max, const Math::Vector< 3, vertex_data_t > & min, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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
	generateCuboid (const Math::Space3D::AACuboid< vertex_data_t > & box, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateCuboid(box.maximum(), box.minimum(), options);
	}

	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateHollowedCube (vertex_data_t size, vertex_data_t borderSize, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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

				builder.beginConstruction(ConstructionMode::TriangleStrip);

				/* Right face (X+) */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveX());
				builder.setPosition(-size + borderSize, -size + borderSize, -size);
				builder.newVertex();
				builder.setPosition(-size + borderSize, -size + borderSize, -size + borderSize);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size + borderSize);
				builder.newVertex();
				builder.resetCurrentTriangle();

				/* Left face (X-) */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeX());
				builder.setPosition(-size,  size, -size); // High point
				builder.newVertex();
				builder.setPosition(-size,  size - borderSize, -size + borderSize);
				builder.newVertex();
				builder.setPosition(-size, -size, -size); // High point
				builder.newVertex();
				builder.setPosition(-size, -size + borderSize, -size + borderSize);
				builder.newVertex();
				builder.resetCurrentTriangle();

				/* Front face (Z+) */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveZ());
				builder.setPosition(-size, -size + borderSize, -size + borderSize);
				builder.newVertex();
				builder.setPosition(-size,  size - borderSize, -size + borderSize);
				builder.newVertex();
				builder.setPosition(-size + borderSize, -size + borderSize, -size + borderSize);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size + borderSize);
				builder.newVertex();
				builder.resetCurrentTriangle();

				/* Back face (Z-) */
				builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::negativeZ());
				builder.setPosition(-size + borderSize, -size + borderSize, -size);
				builder.newVertex();
				builder.setPosition(-size + borderSize,  size - borderSize, -size);
				builder.newVertex();
				builder.setPosition(-size, -size, -size); // High point
				builder.newVertex();
				builder.setPosition(-size,  size, -size); // High point
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
	generateSphere (vertex_data_t radius = 1, index_data_t slices = 16, index_data_t stacks = 8, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
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

		const auto dRHO = std::numbers::pi_v< vertex_data_t > / static_cast< vertex_data_t >(stacks);
		const auto dTheta = (2 * std::numbers::pi_v< vertex_data_t >) / static_cast< vertex_data_t >(slices);

		const auto deltaU = static_cast< vertex_data_t >(1) / static_cast< vertex_data_t >(slices);
		const auto deltaV = static_cast< vertex_data_t >(1) / static_cast< vertex_data_t >(stacks);

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
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setNormal(normals[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setNormal(normals[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setNormal(normals[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
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
	 * @param points A reference to a point list.
	 * @param depth The current depth.
	 * @return void
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	void
	subdivide (ShapeBuilder< vertex_data_t, index_data_t > & builder, const Math::Vector< 3, vertex_data_t > & vectorA, const Math::Vector< 3, vertex_data_t > & vectorB, const Math::Vector< 3, vertex_data_t > & vectorC, std::vector< Math::Vector< 3, vertex_data_t > > & points, index_data_t depth) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		if ( depth == 0 )
		{
			builder.setPosition(vectorA);
			builder.setTextureCoordinates(TextureCoordinates::generateSphericalCoordinates(vectorA));
			builder.newVertex();

			builder.setPosition(vectorB);
			builder.setTextureCoordinates(TextureCoordinates::generateSphericalCoordinates(vectorB));
			builder.newVertex();

			builder.setPosition(vectorC);
			builder.setTextureCoordinates(TextureCoordinates::generateSphericalCoordinates(vectorC));
			builder.newVertex();

			return;
		}

		const auto vectorAB = (vectorA + vectorB).normalize();
		const auto vectorBC = (vectorB + vectorC).normalize();
		const auto vectorCA = (vectorC + vectorA).normalize();
		const auto newDepth = depth - 1;

		subdivide(builder, vectorA, vectorAB, vectorCA, points, newDepth);
		subdivide(builder, vectorB, vectorBC, vectorAB, points, newDepth);
		subdivide(builder, vectorC, vectorCA, vectorBC, points, newDepth);
		subdivide(builder, vectorAB, vectorBC, vectorCA, points, newDepth);
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
	generateGeodesicSphere (vertex_data_t radius = 1, index_data_t depth = 2, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{depth * 20};

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

		std::vector< Math::Vector< 3, vertex_data_t > > points{};

		builder.beginConstruction(ConstructionMode::Triangles);

		for ( const auto & index : indices )
		{
			subdivide(builder, vertices[index[0]], vertices[index[1]], vertices[index[2]], points, depth);
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
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCylinder (vertex_data_t baseRadius = 1, vertex_data_t topRadius = 1, vertex_data_t length = 1, index_data_t slices = 8, index_data_t stacks = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{slices * stacks * 6};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		const auto radiusStep = (topRadius - baseRadius) / static_cast< vertex_data_t >(stacks);
		const auto stepSizeSlice = (2 * std::numbers::pi_v< vertex_data_t >) / static_cast< vertex_data_t >(slices);

		/* Texture coordinates */
		const auto deltaU = static_cast< vertex_data_t >(1.0) / static_cast< vertex_data_t >(slices);
		const auto deltaV = static_cast< vertex_data_t >(1.0) / static_cast< vertex_data_t >(stacks);

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
			const auto texCoordV = static_cast< vertex_data_t >(1.0) - (stackIndex == 0 ? 0 : deltaV * stackIndexF);
			const auto nextV = static_cast< vertex_data_t >(1.0) - (stackIndex == stacks - 1 ? 1 : deltaV * stackIndexPlusOneF);

			const auto currentRadius = baseRadius + (radiusStep * stackIndexF);
			const auto nextRadius = baseRadius + (radiusStep * stackIndexPlusOneF);

			const auto currentY = -(length / stacks) * stackIndexF;
			const auto nextY = -(length / stacks) * stackIndexPlusOneF;

			/* Rise over run... */
			const auto yNormal = Utility::isZero(baseRadius - topRadius) ? 0 : baseRadius - topRadius;

			for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex )
			{
				const auto sliceIndexF = static_cast< vertex_data_t >(sliceIndex);
				const auto sliceIndexPlusOneF = static_cast< vertex_data_t >(sliceIndex + 1);

				/* Texture coordinates.
				 * NOTE : Surface Normal, same for everybody
				 * NOTE² : Inverted for Vulkan. */
				const auto texCoordU = static_cast< vertex_data_t >(1.0) - (sliceIndex == 0 ? 0 : deltaU * sliceIndexF);
				const auto nextTexCoordU = static_cast< vertex_data_t >(1.0) - (sliceIndex == slices - 1 ? 1 : deltaU * sliceIndexPlusOneF);

				const auto theta = stepSizeSlice * sliceIndexF;
				const auto thetaNext = sliceIndex == slices - 1 ? 0 : stepSizeSlice * sliceIndexPlusOneF;

				/* Inner First */
				positions[0] = {std::cos(theta) * currentRadius, currentY, std::sin(theta) * currentRadius};
				textureCoordinates[0] = {texCoordU, texCoordV, 0};
				normals[0] = {positions[0][Math::X], yNormal, positions[0][Math::Z]};
				normals[0].normalize();

				/* Outer First */
				positions[1] = {std::cos(theta) * nextRadius, nextY, std::sin(theta) * nextRadius};
				textureCoordinates[1] = {texCoordU, nextV, 0};

				if ( Utility::isZero(nextRadius) )
				{
					normals[1] = normals[0];
				}
				else
				{
					normals[1] = {positions[1][Math::X], yNormal, positions[1][Math::Z]};
					normals[1].normalize();
				}

				/* Inner second */
				positions[2] = {std::cos(thetaNext) * currentRadius, currentY, std::sin(thetaNext) * currentRadius};
				textureCoordinates[2] = {nextTexCoordU, texCoordV, 0};
				normals[2] = {positions[2][Math::X], yNormal, positions[2][Math::Z]};
				normals[2].normalize();

				/* Outer second */
				positions[3] = {std::cos(thetaNext) * nextRadius, nextY, std::sin(thetaNext) * nextRadius};
				textureCoordinates[3] = {nextTexCoordU, nextV, 0};

				if ( Utility::isZero(nextRadius) )
				{
					normals[3] = normals[2];
				}
				else
				{
					normals[3] = {positions[3][Math::X], yNormal, positions[3][Math::Z]};
					normals[3].normalize();
				}

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setNormal(normals[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setNormal(normals[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setNormal(normals[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setNormal(normals[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
				builder.newVertex();

				builder.resetCurrentTriangle();
			}
		}

		builder.endConstruction();

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
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateCone (vertex_data_t radius = 1, vertex_data_t length = 1, index_data_t slices = 8, index_data_t stacks = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		return generateCylinder(radius, static_cast< vertex_data_t >(0), length, slices, stacks, options);
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
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateDisk (vertex_data_t outerRadius = 1, vertex_data_t innerRadius = 0.5, index_data_t slices = 8, index_data_t stacks = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{slices * stacks * 6};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		if ( innerRadius > outerRadius )
		{
			auto tmp = innerRadius;

			innerRadius = outerRadius;
			outerRadius = tmp;
		}

		const auto stepSizeRadial = std::abs(outerRadius - innerRadius) / stacks;
		const auto stepSizeSlice = (2 * std::numbers::pi_v< vertex_data_t >) / slices;
		const auto radialScale = static_cast< vertex_data_t >(1) / outerRadius;

		std::array< Math::Vector< 3, vertex_data_t >, 4 > positions{};
		std::array< Math::Vector< 3, vertex_data_t >, 4 > textureCoordinates{};

		builder.beginConstruction(ConstructionMode::TriangleStrip);
		builder.options().enableGlobalNormal(Math::Vector< 3, vertex_data_t >::positiveY());

		for ( index_data_t stackIndex = 0; stackIndex < stacks; ++stackIndex )
		{
			for ( index_data_t sliceIndex = 0; sliceIndex < slices; ++sliceIndex )
			{
				const auto inner = innerRadius + stackIndex * stepSizeRadial;
				const auto outer = innerRadius + (stackIndex + 1) * stepSizeRadial;

				const auto theta = stepSizeSlice * sliceIndex;
				const auto thetaNext = sliceIndex == slices - 1 ? 0 : stepSizeSlice * (sliceIndex + 1);

				/* Inner First */
				positions[0] = {std::cos(theta) * inner, 0, std::sin(theta) * inner};
				textureCoordinates[0] = {
					((positions[0][Math::X] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					((positions[0][Math::Z] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					0
				};

				/* Inner Second */
				positions[1] = {std::cos(thetaNext) * inner, 0, std::sin(thetaNext) * inner};
				textureCoordinates[1] = {
					((positions[1][Math::X] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					((positions[1][Math::Z] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					0
				};

				/* Outer First */
				positions[2] = {std::cos(theta) * outer, 0, std::sin(theta) * outer};
				textureCoordinates[2] = {
					((positions[2][Math::X] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					((positions[2][Math::Z] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					0
				};

				/* Outer Second */
				positions[3] = {std::cos(thetaNext) * outer, 0, std::sin(thetaNext) * outer};
				textureCoordinates[3] = {
					((positions[3][Math::X] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					((positions[3][Math::Z] * radialScale) + 1) * static_cast< vertex_data_t >(0.5),
					0
				};

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
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
	generateTorus (vertex_data_t majorRadius = 1, vertex_data_t minorRadius = 0.5, index_data_t slices = 8, index_data_t stacks = 8, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{stacks * (slices + 1) * 6};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

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

			for ( index_data_t sliceIndex = 0; sliceIndex <= slices; ++sliceIndex )
			{
				const auto sliceIndexF = static_cast< vertex_data_t >(sliceIndex);

				auto stepB = sliceIndexF * minorStep;
				auto norm = std::cos(stepB);
				auto radius = minorRadius * norm + majorRadius;
				auto positionY = minorRadius * std::sin(stepB);

				/* Texture coordinates.
				 * NOTE: V is inverted for Vulkan. */
				const auto texCoordUA = stackIndexF / stacksF;
				const auto texCoordVA = static_cast< vertex_data_t >(1.0) - (sliceIndexF / slicesF);
				const auto texCoordUB = (stackIndexF + 1) / stacksF;
				const auto texCoordVB = static_cast< vertex_data_t >(1.0) - ((sliceIndexF + 1) / slicesF);

				/* First point */
				positions[0] = {positionXA * radius, positionY, positionZA * radius};
				textureCoordinates[0] = {texCoordUA, texCoordVA, 0};
				normals[0] = {positionXA * norm, positionY / minorRadius, positionZA * norm};
				normals[0].normalize();

				/* Second point */
				positions[1] = {positionXB * radius, positionY, positionZB * radius};
				textureCoordinates[1] = {texCoordUB, texCoordVA, 0};
				normals[1] = {positionXB * norm, positionY / minorRadius, positionZB * norm};
				normals[1].normalize();

				/* Next one over */
				stepB = (sliceIndexF + 1) * minorStep;
				norm = std::cos(stepB);
				radius = minorRadius * norm + majorRadius;
				positionY = minorRadius * std::sin(stepB);

				/* Third (based on first) */
				positions[2] = {positionXA * radius, positionY, positionZA * radius};
				textureCoordinates[2] = {texCoordUA, texCoordVB, 0};
				normals[2] = {positionXA * norm, positionY / minorRadius, positionZA * norm};
				normals[2].normalize();

				/* Fourth (based on second) */
				positions[3] = {positionXB * radius, positionY, positionZB * radius};
				textureCoordinates[3] = {texCoordUB, texCoordVB, 0};
				normals[3] = {positionXB * norm, positionY / minorRadius, positionZB * norm};
				normals[3].normalize();

				/* Draw quad */
				builder.setPosition(positions[0]);
				builder.setNormal(normals[0]);
				builder.setTextureCoordinates(textureCoordinates[0]);
				builder.newVertex();

				builder.setPosition(positions[1]);
				builder.setNormal(normals[1]);
				builder.setTextureCoordinates(textureCoordinates[1]);
				builder.newVertex();

				builder.setPosition(positions[2]);
				builder.setNormal(normals[2]);
				builder.setTextureCoordinates(textureCoordinates[2]);
				builder.newVertex();

				builder.setPosition(positions[3]);
				builder.setNormal(normals[3]);
				builder.setTextureCoordinates(textureCoordinates[3]);
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
	 * @fixme Mathematically incorrect geometry.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the shape. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateTetrahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{4};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		/* NOTE : a = (2 * √6) / 3 * r */
		const vertex_data_t edgeSize = (static_cast< vertex_data_t >(2) * std::sqrt(static_cast< vertex_data_t >(6))) / static_cast< vertex_data_t >(3) * radius;

		const Math::Vector< 3, vertex_data_t > frontLeft{ edgeSize, edgeSize, -edgeSize};
		const Math::Vector< 3, vertex_data_t > frontRight{-edgeSize, edgeSize, -edgeSize};
		const Math::Vector< 3, vertex_data_t > back{0, edgeSize, edgeSize};
		const Math::Vector< 3, vertex_data_t > top{0, -edgeSize, 0};

		const Math::Vector< 3, vertex_data_t > UVBottomLeft{0, 1, 0};
		const Math::Vector< 3, vertex_data_t > UVBottomRight{1, 1, 0};
		const Math::Vector< 3, vertex_data_t > UVTopMiddle{0.5, 0, 0};

		builder.beginConstruction(ConstructionMode::Triangles);

		builder.setPosition(frontLeft);
		builder.setTextureCoordinates(UVBottomLeft);
		builder.newVertex();
		builder.setPosition(frontRight);
		builder.setTextureCoordinates(UVBottomRight);
		builder.newVertex();
		builder.setPosition(top);
		builder.setTextureCoordinates(UVTopMiddle);
		builder.newVertex();

		builder.setPosition(frontRight);
		builder.setTextureCoordinates(UVBottomLeft);
		builder.newVertex();
		builder.setPosition(back);
		builder.setTextureCoordinates(UVBottomRight);
		builder.newVertex();
		builder.setPosition(top);
		builder.setTextureCoordinates(UVTopMiddle);
		builder.newVertex();

		builder.setPosition(back);
		builder.setTextureCoordinates(UVBottomLeft);
		builder.newVertex();
		builder.setPosition(frontLeft);
		builder.setTextureCoordinates(UVBottomRight);
		builder.newVertex();
		builder.setPosition(top);
		builder.setTextureCoordinates(UVTopMiddle);
		builder.newVertex();

		builder.setPosition(frontRight);
		builder.setTextureCoordinates(1, 0);
		builder.newVertex();
		builder.setPosition(frontLeft);
		builder.setTextureCoordinates(0, 0);
		builder.newVertex();
		builder.setPosition(back);
		builder.setTextureCoordinates(0.5, 1);
		builder.newVertex();

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a hexahedron shape (Earth symbol).
	 * @note Ready for vulkan default world axis.
	 * @fixme Mathematically incorrect geometry.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the shape. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateHexahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		/* NOTE: c = √(2) * r */
		const vertex_data_t edgeSize = std::sqrt(static_cast< vertex_data_t >(2)) * radius;
		const vertex_data_t delta = std::sqrt(static_cast< vertex_data_t >(3)) * edgeSize * 0.5;

		return generateCuboid(delta, delta, delta, options);
	}

	/**
	 * @brief Generates an octahedron shape (Air symbol).
	 * @note Ready for vulkan default world axis.
	 * @fixme Mathematically incorrect geometry.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The volume radius. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateOctahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{8};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		/* NOTE: a = √2 / 2 * r */
		const vertex_data_t edgeSize = std::sqrt(static_cast< vertex_data_t >(2)) / static_cast< vertex_data_t >(2) * radius;

		builder.beginConstruction(ConstructionMode::Triangles);

		/* Top pyramid */
		/* Front */
		builder.setPosition(-edgeSize, 0, edgeSize);
		builder.setTextureCoordinates(0, 0);
		builder.newVertex();
		builder.setPosition(edgeSize, 0, edgeSize);
		builder.setTextureCoordinates(1, 0);
		builder.newVertex();
		builder.setPosition(0, -edgeSize, 0);
		builder.setTextureCoordinates(0.5, -1);
		builder.newVertex();

		/* Back */
		builder.setPosition(edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(0, 0);
		builder.newVertex();
		builder.setPosition(-edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(1, 0);
		builder.newVertex();
		builder.setPosition(0, -edgeSize, 0);
		builder.setTextureCoordinates(0.5, -1);
		builder.newVertex();

		/* Left */
		builder.setPosition(-edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(0, 0);
		builder.newVertex();
		builder.setPosition(-edgeSize, 0,  edgeSize);
		builder.setTextureCoordinates(1, 0);
		builder.newVertex();
		builder.setPosition(0, -edgeSize, 0);
		builder.setTextureCoordinates(0.5, -1);
		builder.newVertex();

		/* Right */
		builder.setPosition(edgeSize, 0,  edgeSize);
		builder.setTextureCoordinates(0, 0);
		builder.newVertex();
		builder.setPosition(edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(1, 0);
		builder.newVertex();
		builder.setPosition(0, -edgeSize, 0);
		builder.setTextureCoordinates(0.5, -1);
		builder.newVertex();

		/* Bottom pyramid */
		/* Front */
		builder.setPosition(edgeSize, 0, edgeSize);
		builder.setTextureCoordinates(1, -1);
		builder.newVertex();
		builder.setPosition(-edgeSize, 0, edgeSize);
		builder.setTextureCoordinates(0, -1);
		builder.newVertex();
		builder.setPosition(0, edgeSize, 0);
		builder.setTextureCoordinates(0.5, 0);
		builder.newVertex();

		/* Back */
		builder.setPosition(-edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(1, -1);
		builder.newVertex();
		builder.setPosition(edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(0, -1);
		builder.newVertex();
		builder.setPosition(0, edgeSize, 0);
		builder.setTextureCoordinates(0.5, 0);
		builder.newVertex();

		/* Left */
		builder.setPosition(-edgeSize, 0,  edgeSize);
		builder.setTextureCoordinates(1, -1);
		builder.newVertex();
		builder.setPosition(-edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(0, -1);
		builder.newVertex();
		builder.setPosition(0,  edgeSize, 0);
		builder.setTextureCoordinates(0.5, 0);
		builder.newVertex();

		/* Right */
		builder.setPosition(edgeSize, 0, -edgeSize);
		builder.setTextureCoordinates(1, -1);
		builder.newVertex();
		builder.setPosition(edgeSize, 0,  edgeSize);
		builder.setTextureCoordinates(0, -1);
		builder.newVertex();
		builder.setPosition(0,  edgeSize, 0);
		builder.setTextureCoordinates(0.5, 0);
		builder.newVertex();

		builder.endConstruction();

		return shape;
	}

	/**
	 * @brief Generates a dodecahedron shape (Ether, Universe symbol).
	 * @TODO Write the algorithm.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the shape. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateDodecahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{36};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		std::cerr << __PRETTY_FUNCTION__ << ", not handle yet ! (Radius: " << radius << ")" "\n";

		return shape;
	}

	/**
	 * @brief Generates an icosahedron shape (Water symbol).
	 * @TODO Write the algorithm.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param radius The radius of the shape. Default 1.
	 * @param options A reference to initial builder options. Default none.
	 * @return Shape< vertex_data_t, index_data_t >
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	Shape< vertex_data_t, index_data_t >
	generateIcosahedron (vertex_data_t radius = 1, const ShapeBuilderOptions< vertex_data_t > & options = {}) noexcept requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		Shape< vertex_data_t, index_data_t > shape{20};

		ShapeBuilder< vertex_data_t, index_data_t > builder{shape, options};

		std::cerr << __PRETTY_FUNCTION__ << ", not handle yet ! (Radius: " << radius << ")" "\n";

		return shape;
	}
}
