/*
 * src/Graphics/Geometry/ResourceGenerator.cpp
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

#include "ResourceGenerator.hpp"

/* Local inclusions. */
#include "Libs/Hash/Hash.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/VertexFactory/ShapeAssembler.hpp"
#include "Libs/VertexFactory/ShapeGenerator.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Libs::VertexFactory;

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::shape (const Shape< float > & shape, const std::string & resourceName) const noexcept
	{
		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [&shape] (IndexedVertexResource & newGeometry) {
			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< VertexResource >
	ResourceGenerator::triangle (float size, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("ShapeTriangle", (std::stringstream{} << size).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< VertexResource >()->getOrCreateResourceAsync(resourceName, [size, parameters] (VertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateTriangle< float, uint32_t >(size, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::quad (float width, float height, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Quad", (std::stringstream{} << width << ',' << height).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [width, height, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateQuad< float, uint32_t >(width, height, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cuboid (float width, float height, float depth, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Cuboid", (std::stringstream{} << width << ',' << height << ',' << depth).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [width, height, depth, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateCuboid< float, uint32_t >(width, height, depth, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cuboid (const Vector< 3, float > & max, const Vector< 3, float > & min, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Cuboid", (std::stringstream{} << max << ',' << min).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [max, min, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateCuboid< float, uint32_t >(max, min, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hollowedCube (float size, float borderSize, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("HollowedCube", (std::stringstream{} << size << ',' << borderSize).str());
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [size, borderSize, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateHollowedCube< float, uint32_t >(size, borderSize, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::sphere (float radius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Sphere", (std::stringstream{} << radius << ',' << slices << ',' << stacks).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, slices, stacks, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateSphere< float, uint32_t >(radius, slices, stacks, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::geodesicSphere (float radius, uint32_t depth, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("GeodesicSphere", (std::stringstream{} << radius << ',' << depth).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, depth, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateGeodesicSphere< float, uint32_t >(radius, depth, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cylinder (float baseRadius, float topRadius, float length, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Cylinder", (std::stringstream{} << baseRadius << ',' << topRadius << ',' << length << ',' << slices << ',' << stacks).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [baseRadius, topRadius, length, slices, stacks, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateCylinder< float, uint32_t >(baseRadius, topRadius, length, slices, stacks, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::disk (float outerRadius, float innerRadius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Disk", (std::stringstream{} << outerRadius << ',' << innerRadius << ',' << slices << ',' << stacks).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [outerRadius, innerRadius, slices, stacks, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateDisk< float, uint32_t >(outerRadius, innerRadius, slices, stacks, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::torus (float majorRadius, float minorRadius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Torus", (std::stringstream{} << majorRadius << ',' << minorRadius << ',' << slices << ',' << stacks).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [majorRadius, minorRadius, slices, stacks, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateTorus< float, uint32_t >(majorRadius, minorRadius, slices, stacks, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::tetrahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Tetrahedron", (std::stringstream{} << radius).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateTetrahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hexahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Hexahedron", (std::stringstream{} << radius).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateHexahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::octahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Octahedron", (std::stringstream{} << radius).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateOctahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::dodecahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Dodecahedron", (std::stringstream{} << radius).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateDodecahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::icosahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Icosahedron", (std::stringstream{} << radius).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [radius, parameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateIcosahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions());

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::arrow (float size, PointTo pointTo, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Arrow", (std::stringstream{} << size << ',' << static_cast< int >(pointTo)).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [size, pointTo, parameters] (IndexedVertexResource & newGeometry) {
			const auto arrowLength = 1.0F * size;
			const auto arrowThickness = 0.015F * size;
			const auto arrowCapLength = arrowLength * 0.2F;
			const auto arrowCapThickness = 0.06F * size;
			constexpr auto quality = 8U;
			const auto gape = arrowCapThickness * 0.5F;

			Shape< float, uint32_t > shape;

			ShapeAssembler< float, uint32_t > assembler{shape};

			ShapeBuilderOptions< float > options{};
			options.enableGlobalVertexColor(parameters.globalVertexColor());
			options.setTextureCoordinatesMultiplier(parameters.textureCoordinatesMultiplier());
			options.setCenterAtBottom(parameters.isCenteredAtBottom());

			const auto translate = Matrix< 4, float >::translation(0.0F, -(gape + arrowLength), 0.0F);

			/* Base arrow. */
			{
				auto chunk = ShapeGenerator::generateCylinder< float, uint32_t >(arrowThickness, arrowThickness, arrowLength, quality, 1, options);

				assembler.merge(chunk, Matrix< 4, float >::translation(0.0F, -gape, 0.0F));
			}

			/* Arrow cap. */
			{
				auto chunk = ShapeGenerator::generateCone< float, uint32_t >(arrowCapThickness, arrowCapLength, quality, 1, options);

				assembler.merge(chunk, translate);
			}

			/* Arrow cap end. */
			{
				options.enableGeometryFlipping(true);

				auto chunk = ShapeGenerator::generateDisk< float, uint32_t >(0.0F, arrowCapThickness, quality, 1, options);

				assembler.merge(chunk, translate);

				options.enableGeometryFlipping(false);
			}

			/* Arrow origin. */
			{
				options.enableGlobalVertexColor(White);

				auto chunk = ShapeGenerator::generateSphere< float, uint32_t >(arrowCapThickness * 0.75F, quality, quality, options);

				assembler.merge(chunk);
			}

			switch ( pointTo )
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

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::axis (float size, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Axis", (std::stringstream{} << size).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [size, parameters] (IndexedVertexResource & newGeometry) {
			const auto arrowLength = 1.0F * size;
			const auto arrowThickness = 0.015F * size;
			const auto arrowCapLength = arrowLength * 0.2F;
			const auto arrowCapThickness = 0.06F * size;
			const auto quality = 8U;
			const auto gape = arrowCapThickness * 0.5F;

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
						auto chunk = ShapeGenerator::generateCylinder< float, uint32_t >(arrowThickness, arrowThickness, arrowLength, quality, 1, options);

						arrowAssembler.merge(chunk, Matrix< 4, float >::translation(0.0F, -gape, 0.0F));
					}

					/* Arrow cap. */
					{
						auto chunk = ShapeGenerator::generateCone< float, uint32_t >(arrowCapThickness, arrowCapLength, quality, 1, options);

						arrowAssembler.merge(chunk, translate);
					}

					/* Arrow cap end. */
					{
						options.enableGeometryFlipping(true);

						auto chunk = ShapeGenerator::generateDisk< float, uint32_t >(0.0F, arrowCapThickness, quality, 1, options);

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

			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< VertexGridResource >
	ResourceGenerator::surface (float size, uint32_t division, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{ 
			resourceName = this->generateResourceName("Surface", (std::stringstream{} << size << ',' << division).str()); 
		}

		const auto & parameters = m_generationParameters;

		return m_resources->container< VertexGridResource >()->getOrCreateResourceAsync(resourceName, [size, division, parameters] (VertexGridResource & newGeometry) {
			return newGeometry.load(size, division, parameters.textureCoordinatesMultiplier()[X], parameters.vertexColorGenMode(), parameters.globalVertexColor());
		}, m_generationParameters.geometryFlags());
	}

	std::string
	ResourceGenerator::generateResourceName (const char * type, const std::string & values) const noexcept
	{
		std::stringstream output;

		output << '+' << type << '(' << Hash::md5(m_generationParameters.uniqueIdentifier() + '-' + values) << ')';

		return output.str();
	}
}
