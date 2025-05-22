/*
 * src/Graphics/Geometry/ResourceGenerator.hpp
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
#include <string>
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Libs/VertexFactory/Shape.hpp"
#include "Resources/Manager.hpp"
#include "VertexResource.hpp"
#include "IndexedVertexResource.hpp"
#include "VertexGridResource.hpp"
#include "GenerationParameters.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief Geometry resources generator using threads.
	 * @note This will use the vertex factory shape builder.
	 */
	class ResourceGenerator final
	{
		public:

			/** @brief Class identification. */
			static constexpr auto ClassId{"ResourceGenerator"};

			/**
			 * @brief Constructs the geometry resource generator.
			 * @param resources A reference to the resource manager.
			 * @param geometryFlagBits Flags value from GeometryFlagBits. Default 0.
			 */
			explicit ResourceGenerator (Resources::Manager & resources, uint32_t geometryFlagBits = 0) noexcept;

			/**
			 * @brief Returns the generation parameters.
			 * @return const GenerationParameters &
			 */
			[[nodiscard]]
			const GenerationParameters &
			parameters () const noexcept
			{
				return m_generationParameters;
			}

			/**
			 * @brief Returns the generation parameters.
			 * @return GenerationParameters &
			 */
			[[nodiscard]]
			GenerationParameters &
			parameters () noexcept
			{
				return m_generationParameters;
			}

			/**
			 * @brief Generates a geometry from a custom vertex factory shape.
			 * @param shape A reference to a shape.
			 * @param resourceName A reference to a string.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > shape (const Libs::VertexFactory::Shape< float > & shape, const std::string & resourceName) const noexcept;

			/**
			 * @brief Generates a triangle geometry.
			 * @param size The base size of the triangle.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< VertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< VertexResource > triangle (float size, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a quad geometry.
			 * @param width The width of the quad.
			 * @param height The height of the quad.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > quad (float width, float height, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a square geometry.
			 * @param size The size of the square.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< VertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource >
			square (float size, const std::string & resourceName = {}) const noexcept
			{
				return this->quad(size, size, resourceName);
			}

			/**
			 * @brief Generates a cuboid geometry.
			 * @param width The width of the cuboid.
			 * @param height The height of the cuboid.
			 * @param depth The depth of the cuboid.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > cuboid (float width, float height, float depth, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a cuboid geometry.
			 * @param size A reference to a vector.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource >
			cuboid (const Libs::Math::Vector< 3, float > & size, const std::string & resourceName = {}) const noexcept
			{
				using namespace EmEn::Libs::Math;

				return this->cuboid(size[X], size[Y], size[Z], resourceName);
			}

			/**
			 * @brief Generates a cuboid geometry.
			 * @param max A reference to a vector for the maximum point.
			 * @param min A reference to a vector for the minimum point.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > cuboid (const Libs::Math::Vector< 3, float > & max, const Libs::Math::Vector< 3, float > & min, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a hollowed cuboid geometry.
			 * @param size The size of the cube.
			 * @param borderSize The border size.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > hollowedCube (float size, float borderSize, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a cube geometry.
			 * @param size The size of the cube.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource >
			cube (float size, const std::string & resourceName = {}) const noexcept
			{
				return this->cuboid(size, size, size, resourceName);
			}

			/**
			 * @brief Generates a sphere geometry.
			 * @param radius The radius of the sphere.
			 * @param slices The number of slices in the sphere. Default 16.
			 * @param stacks The number of stacks in the sphere. Default 8.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > sphere (float radius, uint32_t slices = 16, uint32_t stacks = 8, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a geodesic sphere geometry.
			 * @param radius The radius of the geodesic sphere.
			 * @param depth The depth of the geodesic sphere. Default 2.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > geodesicSphere (float radius, uint32_t depth = 2, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a cylinder geometry.
			 * @param baseRadius The bottom radius of the cylinder.
			 * @param topRadius The top radius of the cylinder.
			 * @param length The length of the cylinder.
			 * @param slices The number of slices in the cylinder. Default 8.
			 * @param stacks The number of stacks in the cylinder. Default 1.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > cylinder (float baseRadius, float topRadius, float length, uint32_t slices = 8, uint32_t stacks = 1, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a cone geometry.
			 * @param radius The radius of the cone.
			 * @param length The length of the cone.
			 * @param slices The number of slices in the cone. Default 8.
			 * @param stacks The number of stacks in the cone. Default 1.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource >
			cone (float radius, float length, uint32_t slices = 8, uint32_t stacks = 1, const std::string & resourceName = {}) const noexcept
			{
				return this->cylinder(radius, 0, length, slices, stacks, resourceName);
			}

			/**
			 * @brief Generates a disk geometry.
			 * @param outerRadius The outer radius of the disk.
			 * @param innerRadius The inner radius of the disk.
			 * @param slices The number of slices in the disk. Default 8.
			 * @param stacks The number of stacks in the disk. Default 1.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > disk (float outerRadius, float innerRadius, uint32_t slices = 8, uint32_t stacks = 1, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a torus geometry.
			 * @param majorRadius The major radius of the torus.
			 * @param minorRadius The minor radius of the torus.
			 * @param slices The number of slices in the torus. Default 8.
			 * @param stacks The number of stacks in the torus. Default 8.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > torus (float majorRadius, float minorRadius, uint32_t slices = 8, uint32_t stacks = 8, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a octahedron geometry.
			 * @param radius The radius.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > octahedron (float radius, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a hexahedron geometry.
			 * @param radius The radius.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > hexahedron (float radius, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a tetrahedron geometry.
			 * @param radius The radius.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > tetrahedron (float radius, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a dodecahedron geometry.
			 * @param radius The radius.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > dodecahedron (float radius, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a icosahedron geometry.
			 * @param radius The radius.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > icosahedron (float radius, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates an arrow geometry.
			 * @param size The size of the arrow.
			 * @param pointTo The direction of the arrow.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > arrow (float size, PointTo pointTo, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates an axis geometry.
			 * @note The vertex color from options will be ignored with this geometry.
			 * @param size The size of the axis shape.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > axis (float size, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a grid geometry.
			 * @param size The size of the whole size of one dimension of the grid. i.e. If the size is 1024, the grid will be from +512 to -512.
			 * @param division How many cell in one dimension.
			 * @param resourceName A string. Default auto generated name.
			 * @return shared_ptr< VertexGridResource >
			 */
			[[nodiscard]]
			std::shared_ptr< VertexGridResource > surface (float size, uint32_t division, std::string resourceName = {}) const noexcept;

		private:

			/**
			 * @brief Returns a unique name for a resource based on parameters.
			 * @param type A string for the basic type of the shape.
			 * @param values A reference to a string for the concatenated values.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generateResourceName (const char * type, const std::string & values) const noexcept;

			Resources::Manager * m_resources{nullptr};
			GenerationParameters m_generationParameters;
	};
}
