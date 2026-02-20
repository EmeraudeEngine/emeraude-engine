/*
 * src/Graphics/Geometry/ResourceGenerator.hpp
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
#include <string>
#include <string_view>
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Libs/VertexFactory/Shape.hpp"
#include "Resources/Manager.hpp"
#include "VertexResource.hpp"
#include "IndexedVertexResource.hpp"
#include "VertexGridResource.hpp"
#include "GenerationParameters.hpp"

/* Local inclusions for usages. */
#include "Libs/VertexFactory/CapUVMapping.hpp"

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
			 * @param geometryFlags Flags value from geometry. Default 0.
			 */
			explicit
			ResourceGenerator (Resources::Manager & resources, uint32_t geometryFlags = 0) noexcept
				: m_resources{resources},
				m_generationParameters{geometryFlags}
			{

			}

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
				using namespace Libs::Math;

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
			 * @param uvMapping UV mapping mode. Default PerSegment.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > hollowedCube (float size, float borderSize, Libs::VertexFactory::CapUVMapping uvMapping = Libs::VertexFactory::CapUVMapping::PerSegment, std::string resourceName = {}) const noexcept;

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
			std::shared_ptr< IndexedVertexResource > cylinder (float baseRadius, float topRadius, float length, uint32_t slices = 8, uint32_t stacks = 1, Libs::VertexFactory::CapUVMapping capMapping = {}, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a cone geometry.
			 * @param radius The radius of the cone.
			 * @param length The length of the cone.
			 * @param slices The number of slices in the cone. Default 8.
			 * @param stacks The number of stacks in the cone. Default 1.
			 * @param capMapping UV mapping mode for the base cap. Default None.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource >
			cone (float radius, float length, uint32_t slices = 8, uint32_t stacks = 1, Libs::VertexFactory::CapUVMapping capMapping = {}, const std::string & resourceName = {}) const noexcept
			{
				return this->cylinder(radius, 0, length, slices, stacks, capMapping, resourceName);
			}

			/**
			 * @brief Generates a disk geometry.
			 * @param outerRadius The outer radius of the disk.
			 * @param innerRadius The inner radius of the disk.
			 * @param slices The number of slices in the disk. Default 8.
			 * @param stacks The number of stacks in the disk. Default 1.
			 * @param uvMapping UV mapping mode for the disk surface. Default Planar.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > disk (float outerRadius, float innerRadius, uint32_t slices = 8, uint32_t stacks = 1, Libs::VertexFactory::CapUVMapping uvMapping = Libs::VertexFactory::CapUVMapping::Planar, std::string resourceName = {}) const noexcept;

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
			 * @brief Generates an octahedron geometry.
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
			 * @brief Generates an icosahedron geometry.
			 * @param radius The radius.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > icosahedron (float radius, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a subdivided plane geometry on the XZ plane.
			 * @param width The width (X axis) of the plane.
			 * @param depth The depth (Z axis) of the plane.
			 * @param subdivisionsX Number of subdivisions along X. Default 1.
			 * @param subdivisionsZ Number of subdivisions along Z. Default 1.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > plane (float width, float depth, uint32_t subdivisionsX = 1, uint32_t subdivisionsZ = 1, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a capsule geometry (cylinder with hemisphere caps).
			 * @param radius The radius of the capsule cross-section.
			 * @param length The length of the cylindrical section (0 gives a sphere).
			 * @param slices Longitudinal subdivisions. Default 16.
			 * @param stacks Latitudinal subdivisions per hemisphere. Default 8.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > capsule (float radius, float length, uint32_t slices = 16, uint32_t stacks = 8, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a hemisphere geometry (half sphere with disk cap).
			 * @param radius The radius of the hemisphere.
			 * @param slices Longitudinal subdivisions. Default 16.
			 * @param stacks Latitudinal subdivisions. Default 8.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > hemisphere (float radius, uint32_t slices = 16, uint32_t stacks = 8, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a tube geometry (hollow cylinder with optional annular caps).
			 * @param outerRadius The outer radius of the tube.
			 * @param innerRadius The inner radius (hole).
			 * @param length The length of the tube.
			 * @param slices Circumferential subdivisions. Default 16.
			 * @param stacks Longitudinal subdivisions. Default 1.
			 * @param capped Whether to generate annular caps at both ends. Default true.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > tube (float outerRadius, float innerRadius, float length, uint32_t slices = 16, uint32_t stacks = 1, Libs::VertexFactory::CapUVMapping capMapping = {}, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a brilliant-cut gem geometry (diamond approximation).
			 * @param radius The girdle radius (widest point).
			 * @param facets The number of crown/pavilion facets. Default 8.
			 * @param tableRatio The table radius as fraction of girdle (0-1). Default 0.55.
			 * @param crownAngle The crown slope from girdle plane in degrees. Default 35.
			 * @param pavilionAngle The pavilion slope from girdle plane in degrees. Default 41.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > diamondCutGem (float radius, uint32_t facets = 8, float tableRatio = 0.55F, float crownAngle = 35.0F, float pavilionAngle = 41.0F, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates an emerald-cut gem geometry (step-cut rectangular gem with beveled corners).
			 * @param length The X-axis dimension (longer axis).
			 * @param width The Z-axis dimension (shorter axis).
			 * @param depth The total Y depth of the gem.
			 * @param tableRatio The table size as fraction of length/width. Default 0.6.
			 * @param cornerBevel The corner bevel size as fraction of half-width. Default 0.25.
			 * @param steps The number of step facets on crown and pavilion. Default 3.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > emeraldCutGem (float length, float width, float depth, float tableRatio = 0.6F, float cornerBevel = 0.25F, uint32_t steps = 3, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates an Asscher-cut gem geometry (square step-cut with beveled corners).
			 * @param size The side length of the square.
			 * @param depth The total Y depth of the gem.
			 * @param tableRatio The table size as fraction of side length. Default 0.65.
			 * @param cornerBevel The corner bevel size as fraction of half-side. Default 0.15.
			 * @param steps The number of step facets. Default 3.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > asscherCutGem (float size, float depth, float tableRatio = 0.65F, float cornerBevel = 0.15F, uint32_t steps = 3, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a baguette-cut gem geometry (pure rectangular step-cut, no corner bevel).
			 * @param length The X-axis dimension.
			 * @param width The Z-axis dimension.
			 * @param depth The total Y depth of the gem.
			 * @param tableRatio The table size as fraction of dimensions. Default 0.65.
			 * @param steps The number of step facets. Default 2.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > baguetteCutGem (float length, float width, float depth, float tableRatio = 0.65F, uint32_t steps = 2, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a princess-cut gem geometry (square with chevron pavilion facets).
			 * @param size The side length of the square.
			 * @param depth The total Y depth of the gem.
			 * @param tableRatio The table size as fraction of side length. Default 0.6.
			 * @param chevronDepth The chevron depth factor (0-1). Default 0.4.
			 * @param steps The number of step/chevron facets. Default 3.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > princessCutGem (float size, float depth, float tableRatio = 0.6F, float chevronDepth = 0.4F, uint32_t steps = 3, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a trillion-cut gem geometry (triangular with beveled corners).
			 * @param size The circumscribed circle radius.
			 * @param depth The total Y depth of the gem.
			 * @param tableRatio The table size as fraction of size. Default 0.55.
			 * @param cornerBevel The corner bevel amount (0-1). Default 0.2.
			 * @param steps The number of step facets. Default 3.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > trillionCutGem (float size, float depth, float tableRatio = 0.55F, float cornerBevel = 0.2F, uint32_t steps = 3, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates an oval-cut gem geometry (elliptical brilliant cut).
			 * @param length The X-axis radius (longer axis).
			 * @param width The Z-axis radius (shorter axis).
			 * @param facets The number of facets. Default 16.
			 * @param tableRatio The table size fraction. Default 0.55.
			 * @param crownAngle The crown slope in degrees. Default 35.
			 * @param pavilionAngle The pavilion slope in degrees. Default 41.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > ovalCutGem (float length, float width, uint32_t facets = 16, float tableRatio = 0.55F, float crownAngle = 35.0F, float pavilionAngle = 41.0F, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a cushion-cut gem geometry (rounded rectangle brilliant cut).
			 * @param length The X-axis radius.
			 * @param width The Z-axis radius.
			 * @param power The superellipse exponent. Default 2.5.
			 * @param facets The number of facets. Default 16.
			 * @param tableRatio The table size fraction. Default 0.55.
			 * @param crownAngle The crown slope in degrees. Default 35.
			 * @param pavilionAngle The pavilion slope in degrees. Default 41.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > cushionCutGem (float length, float width, float power = 2.5F, uint32_t facets = 16, float tableRatio = 0.55F, float crownAngle = 35.0F, float pavilionAngle = 41.0F, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a marquise-cut gem geometry (pointed elliptical brilliant cut).
			 * @param length The X-axis radius (pointed axis).
			 * @param width The Z-axis radius (widest).
			 * @param sharpness The tip pointedness exponent. Default 1.5.
			 * @param facets The number of facets. Default 16.
			 * @param tableRatio The table size fraction. Default 0.5.
			 * @param crownAngle The crown slope in degrees. Default 33.
			 * @param pavilionAngle The pavilion slope in degrees. Default 42.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > marquiseCutGem (float length, float width, float sharpness = 1.5F, uint32_t facets = 16, float tableRatio = 0.5F, float crownAngle = 33.0F, float pavilionAngle = 42.0F, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a pear-cut gem geometry (teardrop brilliant cut).
			 * @param length The tip-to-round axis radius.
			 * @param width The widest point radius.
			 * @param sharpness The tip pointedness exponent. Default 1.6.
			 * @param facets The number of facets. Default 16.
			 * @param tableRatio The table size fraction. Default 0.5.
			 * @param crownAngle Crown slope in degrees. Default 34.
			 * @param pavilionAngle Pavilion slope in degrees. Default 42.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > pearCutGem (float length, float width, float sharpness = 1.6F, uint32_t facets = 16, float tableRatio = 0.5F, float crownAngle = 34.0F, float pavilionAngle = 42.0F, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a heart-cut gem geometry (heart-shaped brilliant cut).
			 * @param size The overall size.
			 * @param facets The number of facets. Default 24.
			 * @param tableRatio The table size fraction. Default 0.5.
			 * @param crownAngle Crown slope in degrees. Default 34.
			 * @param pavilionAngle Pavilion slope in degrees. Default 42.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > heartCutGem (float size, uint32_t facets = 24, float tableRatio = 0.5F, float crownAngle = 34.0F, float pavilionAngle = 42.0F, std::string resourceName = {}) const noexcept;

			/**
			 * @brief Generates a rose-cut gem geometry (domed top, flat bottom).
			 * @param radius The base radius.
			 * @param height The dome height.
			 * @param rings The number of concentric rings. Default 3.
			 * @param facets The number of facets around. Default 12.
			 * @param resourceName A string. Default auto generated name.
			 * @return std::shared_ptr< IndexedVertexResource >
			 */
			[[nodiscard]]
			std::shared_ptr< IndexedVertexResource > roseCutGem (float radius, float height, uint32_t rings = 3, uint32_t facets = 12, std::string resourceName = {}) const noexcept;

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
			 * @param gridSize The size of the whole size of one dimension of the grid. i.e. If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cell in one dimension.
			 * @param resourceName A string. Default auto generated name.
			 * @return shared_ptr< VertexGridResource >
			 */
			[[nodiscard]]
			std::shared_ptr< VertexGridResource > surface (float gridSize, uint32_t gridDivision, std::string resourceName = {}) const noexcept;

		private:

			/**
			 * @brief Returns a unique name for a resource based on parameters.
			 * @param type A string for the basic type of the shape.
			 * @param values A reference to a string for the concatenated values.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generateResourceName (std::string_view type, std::string_view values) const noexcept;

			Resources::Manager & m_resources;
			GenerationParameters m_generationParameters;
	};
}
