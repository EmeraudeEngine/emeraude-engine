/*
 * src/Graphics/Geometry/ResourceGenerator.cpp
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

#include "ResourceGenerator.hpp"

/* Local inclusions. */
#include "Libs/Hash/Hash.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/VertexFactory/ShapeAssembler.hpp"
#include "Libs/VertexFactory/ShapeGenerator.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Libs::VertexFactory;

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::shape (const Shape< float > & shape, const std::string & resourceName) const noexcept
	{
		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [&shape] (auto & geometryResource) {
				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< VertexResource >
	ResourceGenerator::triangle (float size, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("ShapeTriangle", std::to_string(size));
		}

		return m_resources.container< VertexResource >()
			->getOrCreateResource(resourceName, [size, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateTriangle< float, uint32_t >(size, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::quad (float width, float height, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Quad", std::to_string(width) + ',' + std::to_string(height));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [width, height, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateQuad< float, uint32_t >(width, height, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cuboid (float width, float height, float depth, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Cuboid", std::to_string(width) + ',' + std::to_string(height) + ',' + std::to_string(depth));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [width, height, depth, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateCuboid< float, uint32_t >(width, height, depth, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cuboid (const Vector< 3, float > & max, const Vector< 3, float > & min, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			std::stringstream ss;
			ss << max << ',' << min;
			resourceName = this->generateResourceName("Cuboid", ss.str());
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [max, min, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateCuboid< float, uint32_t >(max, min, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hollowedCube (float size, float borderSize, VertexFactory::CapUVMapping uvMapping, std::string resourceName) const noexcept
	{
		static constexpr const char * uvMappingNames[] = {"none", "planar", "perSegment"};

		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("HollowedCube", std::to_string(size) + ',' + std::to_string(borderSize) + ',' + uvMappingNames[static_cast< uint8_t >(uvMapping)]);
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, borderSize, uvMapping, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateHollowedCube< float, uint32_t >(size, borderSize, uvMapping, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::sphere (float radius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Sphere", std::to_string(radius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, slices, stacks, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateSphere< float, uint32_t >(radius, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::geodesicSphere (float radius, uint32_t depth, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("GeodesicSphere", std::to_string(radius) + ',' + std::to_string(depth));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, depth, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateGeodesicSphere< float, uint32_t >(radius, depth, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cylinder (float baseRadius, float topRadius, float length, uint32_t slices, uint32_t stacks, VertexFactory::CapUVMapping capMapping, std::string resourceName) const noexcept
	{
		static constexpr const char * capMappingNames[] = {"none", "planar", "perSegment"};

		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Cylinder", std::to_string(baseRadius) + ',' + std::to_string(topRadius) + ',' + std::to_string(length) + ',' + std::to_string(slices) + ',' + std::to_string(stacks) + ',' + capMappingNames[static_cast< uint8_t >(capMapping)]);
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [baseRadius, topRadius, length, slices, stacks, capMapping, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateCylinder< float, uint32_t >(baseRadius, topRadius, length, slices, stacks, capMapping, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::disk (float outerRadius, float innerRadius, uint32_t slices, uint32_t stacks, VertexFactory::CapUVMapping uvMapping, std::string resourceName) const noexcept
	{
		static constexpr const char * uvMappingNames[] = {"none", "planar", "perSegment"};

		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Disk", std::to_string(outerRadius) + ',' + std::to_string(innerRadius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks) + ',' + uvMappingNames[static_cast< uint8_t >(uvMapping)]);
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [outerRadius, innerRadius, slices, stacks, uvMapping, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateDisk< float, uint32_t >(outerRadius, innerRadius, slices, stacks, uvMapping, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::torus (float majorRadius, float minorRadius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Torus", std::to_string(majorRadius) + ',' + std::to_string(minorRadius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [majorRadius, minorRadius, slices, stacks, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateTorus< float, uint32_t >(majorRadius, minorRadius, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::tetrahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Tetrahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateTetrahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hexahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Hexahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateHexahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::octahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Octahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateOctahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::dodecahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Dodecahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateDodecahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::icosahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Icosahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateIcosahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::plane (float width, float depth, uint32_t subdivisionsX, uint32_t subdivisionsZ, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Plane", std::to_string(width) + ',' + std::to_string(depth) + ',' + std::to_string(subdivisionsX) + ',' + std::to_string(subdivisionsZ));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [width, depth, subdivisionsX, subdivisionsZ, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generatePlane< float, uint32_t >(width, depth, subdivisionsX, subdivisionsZ, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::capsule (float radius, float length, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Capsule", std::to_string(radius) + ',' + std::to_string(length) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, length, slices, stacks, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateCapsule< float, uint32_t >(radius, length, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hemisphere (float radius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Hemisphere", std::to_string(radius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, slices, stacks, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateHemisphere< float, uint32_t >(radius, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::tube (float outerRadius, float innerRadius, float length, uint32_t slices, uint32_t stacks, VertexFactory::CapUVMapping capMapping, std::string resourceName) const noexcept
	{
		static constexpr const char * capMappingNames[] = {"none", "planar", "perSegment"};

		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Tube", std::to_string(outerRadius) + ',' + std::to_string(innerRadius) + ',' + std::to_string(length) + ',' + std::to_string(slices) + ',' + std::to_string(stacks) + ',' + capMappingNames[static_cast< uint8_t >(capMapping)]);
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [outerRadius, innerRadius, length, slices, stacks, capMapping, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateTube< float, uint32_t >(outerRadius, innerRadius, length, slices, stacks, capMapping, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::diamondCutGem (float radius, uint32_t facets, float tableRatio, float crownAngle, float pavilionAngle, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("DiamondCutGem", std::to_string(radius) + ',' + std::to_string(facets) + ',' + std::to_string(tableRatio) + ',' + std::to_string(crownAngle) + ',' + std::to_string(pavilionAngle));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, facets, tableRatio, crownAngle, pavilionAngle, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateDiamondCutGem< float, uint32_t >(radius, facets, tableRatio, crownAngle, pavilionAngle, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::emeraldCutGem (float length, float width, float depth, float tableRatio, float cornerBevel, uint32_t steps, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("EmeraldCutGem", std::to_string(length) + ',' + std::to_string(width) + ',' + std::to_string(depth) + ',' + std::to_string(tableRatio) + ',' + std::to_string(cornerBevel) + ',' + std::to_string(steps));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [length, width, depth, tableRatio, cornerBevel, steps, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateEmeraldCutGem< float, uint32_t >(length, width, depth, tableRatio, cornerBevel, steps, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::asscherCutGem (float size, float depth, float tableRatio, float cornerBevel, uint32_t steps, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("AsscherCutGem", std::to_string(size) + ',' + std::to_string(depth) + ',' + std::to_string(tableRatio) + ',' + std::to_string(cornerBevel) + ',' + std::to_string(steps));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, depth, tableRatio, cornerBevel, steps, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateAssscherCutGem< float, uint32_t >(size, depth, tableRatio, cornerBevel, steps, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::baguetteCutGem (float length, float width, float depth, float tableRatio, uint32_t steps, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("BaguetteCutGem", std::to_string(length) + ',' + std::to_string(width) + ',' + std::to_string(depth) + ',' + std::to_string(tableRatio) + ',' + std::to_string(steps));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [length, width, depth, tableRatio, steps, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateBaguetteCutGem< float, uint32_t >(length, width, depth, tableRatio, steps, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::princessCutGem (float size, float depth, float tableRatio, float chevronDepth, uint32_t steps, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("PrincessCutGem", std::to_string(size) + ',' + std::to_string(depth) + ',' + std::to_string(tableRatio) + ',' + std::to_string(chevronDepth) + ',' + std::to_string(steps));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, depth, tableRatio, chevronDepth, steps, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generatePrincessCutGem< float, uint32_t >(size, depth, tableRatio, chevronDepth, steps, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::trillionCutGem (float size, float depth, float tableRatio, float cornerBevel, uint32_t steps, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("TrillionCutGem", std::to_string(size) + ',' + std::to_string(depth) + ',' + std::to_string(tableRatio) + ',' + std::to_string(cornerBevel) + ',' + std::to_string(steps));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, depth, tableRatio, cornerBevel, steps, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateTrillionCutGem< float, uint32_t >(size, depth, tableRatio, cornerBevel, steps, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::ovalCutGem (float length, float width, uint32_t facets, float tableRatio, float crownAngle, float pavilionAngle, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("OvalCutGem", std::to_string(length) + ',' + std::to_string(width) + ',' + std::to_string(facets) + ',' + std::to_string(tableRatio) + ',' + std::to_string(crownAngle) + ',' + std::to_string(pavilionAngle));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [length, width, facets, tableRatio, crownAngle, pavilionAngle, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateOvalCutGem< float, uint32_t >(length, width, facets, tableRatio, crownAngle, pavilionAngle, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cushionCutGem (float length, float width, float power, uint32_t facets, float tableRatio, float crownAngle, float pavilionAngle, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("CushionCutGem", std::to_string(length) + ',' + std::to_string(width) + ',' + std::to_string(power) + ',' + std::to_string(facets) + ',' + std::to_string(tableRatio) + ',' + std::to_string(crownAngle) + ',' + std::to_string(pavilionAngle));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [length, width, power, facets, tableRatio, crownAngle, pavilionAngle, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateCushionCutGem< float, uint32_t >(length, width, power, facets, tableRatio, crownAngle, pavilionAngle, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::marquiseCutGem (float length, float width, float sharpness, uint32_t facets, float tableRatio, float crownAngle, float pavilionAngle, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("MarquiseCutGem", std::to_string(length) + ',' + std::to_string(width) + ',' + std::to_string(sharpness) + ',' + std::to_string(facets) + ',' + std::to_string(tableRatio) + ',' + std::to_string(crownAngle) + ',' + std::to_string(pavilionAngle));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [length, width, sharpness, facets, tableRatio, crownAngle, pavilionAngle, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateMarquiseCutGem< float, uint32_t >(length, width, sharpness, facets, tableRatio, crownAngle, pavilionAngle, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::pearCutGem (float length, float width, float sharpness, uint32_t facets, float tableRatio, float crownAngle, float pavilionAngle, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("PearCutGem", std::to_string(length) + ',' + std::to_string(width) + ',' + std::to_string(sharpness) + ',' + std::to_string(facets) + ',' + std::to_string(tableRatio) + ',' + std::to_string(crownAngle) + ',' + std::to_string(pavilionAngle));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [length, width, sharpness, facets, tableRatio, crownAngle, pavilionAngle, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generatePearCutGem< float, uint32_t >(length, width, sharpness, facets, tableRatio, crownAngle, pavilionAngle, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::heartCutGem (float size, uint32_t facets, float tableRatio, float crownAngle, float pavilionAngle, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("HeartCutGem", std::to_string(size) + ',' + std::to_string(facets) + ',' + std::to_string(tableRatio) + ',' + std::to_string(crownAngle) + ',' + std::to_string(pavilionAngle));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, facets, tableRatio, crownAngle, pavilionAngle, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateHeartCutGem< float, uint32_t >(size, facets, tableRatio, crownAngle, pavilionAngle, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::roseCutGem (float radius, float height, uint32_t rings, uint32_t facets, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("RoseCutGem", std::to_string(radius) + ',' + std::to_string(height) + ',' + std::to_string(rings) + ',' + std::to_string(facets));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [radius, height, rings, facets, parameters = m_generationParameters] (auto & geometryResource) {
				auto shape = ShapeGenerator::generateRoseCutGem< float, uint32_t >(radius, height, rings, facets, parameters.getShapeBuilderOptions(false, false, false));

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::arrow (float size, PointTo pointTo, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Arrow", std::to_string(size) + ',' + std::to_string(static_cast< int >(pointTo)));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, pointTo, parameters = m_generationParameters] (auto & geometryResource) {
				constexpr auto arrowLengthFactor{1.0F};
				constexpr auto arrowThicknessFactor{0.015F};
				constexpr auto arrowCapLengthFactor{0.2F};
				constexpr auto arrowCapThicknessFactor{0.06F};
				constexpr auto quality{8U};
				constexpr auto gapeFactor{0.5F};

				const auto arrowLength = arrowLengthFactor * size;
				const auto arrowThickness = arrowThicknessFactor * size;
				const auto arrowCapLength = arrowLength * arrowCapLengthFactor;
				const auto arrowCapThickness = arrowCapThicknessFactor * size;
				const auto gape = arrowCapThickness * gapeFactor;

				Shape< float, uint32_t > shape;

				ShapeAssembler< float, uint32_t > assembler{shape};

				ShapeBuilderOptions< float > options{};
				options.enableGlobalVertexColor(parameters.globalVertexColor());
				options.setTextureCoordinatesMultiplier(parameters.textureCoordinatesMultiplier());
				options.setCenterAtBottom(parameters.isCenteredAtBottom());

				const auto translate = Matrix< 4, float >::translation(0.0F, -(gape + arrowLength), 0.0F);

				/* Base arrow. */
				{
					auto chunk = ShapeGenerator::generateCylinder< float, uint32_t >(arrowThickness, arrowThickness, arrowLength, quality, 1, VertexFactory::CapUVMapping::Planar, options);

					assembler.merge(chunk, Matrix< 4, float >::translation(0.0F, -gape, 0.0F));
				}

				/* Arrow cap. */
				{
					auto chunk = ShapeGenerator::generateCone< float, uint32_t >(arrowCapThickness, arrowCapLength, quality, 1, VertexFactory::CapUVMapping::Planar, options);

					assembler.merge(chunk, translate);
				}

				/* Arrow cap end. */
				{
					options.enableGeometryFlipping(true);

					auto chunk = ShapeGenerator::generateDisk< float, uint32_t >(0.0F, arrowCapThickness, quality, 1, VertexFactory::CapUVMapping::Planar, options);

					assembler.merge(chunk, translate);

					options.enableGeometryFlipping(false);
				}

				/* Arrow origin. */
				{
					options.enableGlobalVertexColor(White);

					auto chunk = ShapeGenerator::generateSphere< float, uint32_t >(arrowCapThickness * 0.75F, quality, quality, options);

					assembler.merge(chunk);
				}

				switch (pointTo)
				{
					case PointTo::PositiveX :
						shape.transform(Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 0.0F, 0.0, 1.0F));
						break;

					case PointTo::NegativeX :
						shape.transform(Matrix< 4, float >::rotation(Radian(-QuartRevolution< float >), 0.0F, 0.0, 1.0F));
						break;

					case PointTo::PositiveY :
						shape.transform(Matrix< 4, float >::rotation(Radian(HalfRevolution< float >), 1.0F, 0.0, 0.0F));
						break;

					case PointTo::NegativeY :
						/* Nothing to do, the arrow is built pointing to Y- by default. */
						break;

					case PointTo::PositiveZ :
						shape.transform(Matrix< 4, float >::rotation(Radian(-QuartRevolution< float >), 1.0F, 0.0, 0.0F));
						break;

					case PointTo::NegativeZ :
						shape.transform(Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 1.0F, 0.0, 0.0F));
						break;
				}

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::axis (float size, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Axis", std::to_string(size));
		}

		return m_resources.container< IndexedVertexResource >()
			->getOrCreateResource(resourceName, [size, parameters = m_generationParameters] (auto & geometryResource) {
				constexpr auto arrowLengthFactor{1.0F};
				constexpr auto arrowThicknessFactor{0.015F};
				constexpr auto arrowCapLengthFactor{0.2F};
				constexpr auto arrowCapThicknessFactor{0.06F};
				constexpr auto quality{8U};
				constexpr auto gapeFactor{0.5F};

				const auto arrowLength = arrowLengthFactor * size;
				const auto arrowThickness = arrowThicknessFactor * size;
				const auto arrowCapLength = arrowLength * arrowCapLengthFactor;
				const auto arrowCapThickness = arrowCapThicknessFactor * size;
				const auto gape = arrowCapThickness * gapeFactor;

				Shape< float > shape{};

				ShapeAssembler assembler{shape};

				ShapeBuilderOptions< float > options{};
				options.setTextureCoordinatesMultiplier(parameters.textureCoordinatesMultiplier());
				options.setCenterAtBottom(parameters.isCenteredAtBottom());

				{
					Shape< float > arrow{};

					{
						ShapeAssembler arrowAssembler{arrow};

						const auto translate = Matrix< 4, float >::translation(0.0F, -arrowLength, 0.0F);

						/* Base arrow. */
						{
							auto chunk = ShapeGenerator::generateCylinder< float, uint32_t >(arrowThickness, arrowThickness, arrowLength, quality, 1, VertexFactory::CapUVMapping::Planar, options);

							arrowAssembler.merge(chunk, Matrix< 4, float >::translation(0.0F, -gape, 0.0F));
						}

						/* Arrow cap. */
						{
							auto chunk = ShapeGenerator::generateCone< float, uint32_t >(arrowCapThickness, arrowCapLength, quality, 1, VertexFactory::CapUVMapping::Planar, options);

							arrowAssembler.merge(chunk, translate);
						}

						/* Arrow cap end. */
						{
							options.enableGeometryFlipping(true);

							auto chunk = ShapeGenerator::generateDisk< float, uint32_t >(0.0F, arrowCapThickness, quality, 1, VertexFactory::CapUVMapping::Planar, options);

							options.enableGeometryFlipping(false);

							arrowAssembler.merge(chunk, translate);
						}
					}

					/* Adds the Y+ arrow in green. This should point downward. */
					arrow.setGlobalVertexColor(Green);

					assembler.merge(arrow, Matrix< 4, float >::rotation(Radian(HalfRevolution< float >), 1.0F, 0.0F, 0.0F));

					/* Adds the X+ arrow in red. This should point toward the right. */
					arrow.setGlobalVertexColor(Red);

					assembler.merge(arrow, Matrix< 4, float >::rotation(Radian(QuartRevolution< float >), 0.0F, 0.0F, 1.0F));

					/* Adds the Z+ arrow in blue. This should point to camera. */
					arrow.setGlobalVertexColor(Blue);

					assembler.merge(arrow, Matrix< 4, float >::rotation(Radian(-QuartRevolution< float >), 1.0F, 0.0F, 0.0F));
				}

				/* Arrow origin. */
				{
					options.enableGlobalVertexColor(White);

					auto chunk = ShapeGenerator::generateSphere< float, uint32_t >(arrowCapThickness * 0.75F, quality, quality, options);

					assembler.merge(chunk);
				}

				if ( !parameters.transformMatrix().isIdentity() )
				{
					shape.transform(parameters.transformMatrix());
				}

				return geometryResource.load(shape);
			}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< VertexGridResource >
	ResourceGenerator::surface (float gridSize, uint32_t gridDivision, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Surface", std::to_string(gridSize) + ',' + std::to_string(gridDivision));
		}

		return m_resources.container< VertexGridResource >()
			->getOrCreateResource(resourceName, [gridSize, gridDivision, parameters = m_generationParameters] (auto & geometryResource) {
				geometryResource.enableVertexColor(parameters.globalVertexColor());

				return geometryResource.load(gridSize, gridDivision, parameters.textureCoordinatesMultiplier()[X]);
			}, m_generationParameters.geometryFlags());
	}

	std::string
	ResourceGenerator::generateResourceName (std::string_view type, std::string_view values) const noexcept
	{
		std::stringstream output;

		output << '+' << type << '(' << Hash::md5(m_generationParameters.uniqueIdentifier() + '-' + std::string(values)) << ')';

		return output.str();
	}
}
