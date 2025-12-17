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
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Libs::VertexFactory;

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::shape (const Shape< float > & shape, const std::string & resourceName) const noexcept
	{
		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(resourceName, [&shape] (IndexedVertexResource & newGeometry) {
			return newGeometry.load(shape);
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< VertexResource >
	ResourceGenerator::triangle (float size, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("ShapeTriangle", std::to_string(size));
		}

		return m_resources.container< VertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [size, parameters = m_generationParameters] (VertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateTriangle< float, uint32_t >(size, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::quad (float width, float height, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Quad", std::to_string(width) + ',' + std::to_string(height));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [width, height, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateQuad< float, uint32_t >(width, height, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cuboid (float width, float height, float depth, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Cuboid", std::to_string(width) + ',' + std::to_string(height) + ',' + std::to_string(depth));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [width, height, depth, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateCuboid< float, uint32_t >(width, height, depth, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
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

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [max, min, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateCuboid< float, uint32_t >(max, min, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hollowedCube (float size, float borderSize, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("HollowedCube", std::to_string(size) + ',' + std::to_string(borderSize));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [size, borderSize, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateHollowedCube< float, uint32_t >(size, borderSize, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::sphere (float radius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Sphere", std::to_string(radius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, slices, stacks, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateSphere< float, uint32_t >(radius, slices, stacks, parameters.getShapeBuilderOptions(false, false, true));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::geodesicSphere (float radius, uint32_t depth, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("GeodesicSphere", std::to_string(radius) + ',' + std::to_string(depth));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, depth, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateGeodesicSphere< float, uint32_t >(radius, depth, parameters.getShapeBuilderOptions(true, true, true));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::cylinder (float baseRadius, float topRadius, float length, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Cylinder", std::to_string(baseRadius) + ',' + std::to_string(topRadius) + ',' + std::to_string(length) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [baseRadius, topRadius, length, slices, stacks, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateCylinder< float, uint32_t >(baseRadius, topRadius, length, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::disk (float outerRadius, float innerRadius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Disk", std::to_string(outerRadius) + ',' + std::to_string(innerRadius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [outerRadius, innerRadius, slices, stacks, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateDisk< float, uint32_t >(outerRadius, innerRadius, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::torus (float majorRadius, float minorRadius, uint32_t slices, uint32_t stacks, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Torus", std::to_string(majorRadius) + ',' + std::to_string(minorRadius) + ',' + std::to_string(slices) + ',' + std::to_string(stacks));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [majorRadius, minorRadius, slices, stacks, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateTorus< float, uint32_t >(majorRadius, minorRadius, slices, stacks, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::tetrahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Tetrahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateTetrahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::hexahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Hexahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateHexahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::octahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Octahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateOctahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::dodecahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Dodecahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateDodecahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::icosahedron (float radius, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Icosahedron", std::to_string(radius));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [radius, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			auto shape = ShapeGenerator::generateIcosahedron< float, uint32_t >(radius, parameters.getShapeBuilderOptions(false, false, false));

			if ( !parameters.transformMatrix().isIdentity() )
			{
				shape.transform(parameters.transformMatrix());
			}

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::arrow (float size, PointTo pointTo, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Arrow", std::to_string(size) + ',' + std::to_string(static_cast< int >(pointTo)));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [size, pointTo, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			constexpr auto arrowLengthFactor = 1.0F;
			constexpr auto arrowThicknessFactor = 0.015F;
			constexpr auto arrowCapLengthFactor = 0.2F;
			constexpr auto arrowCapThicknessFactor = 0.06F;
			constexpr auto quality = 8U;
			constexpr auto gapeFactor = 0.5F;

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

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< IndexedVertexResource >
	ResourceGenerator::axis (float size, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Axis", std::to_string(size));
		}

		return m_resources.container< IndexedVertexResource >()->getOrCreateResourceAsync(std::move(resourceName), [size, parameters = m_generationParameters] (IndexedVertexResource & newGeometry) {
			constexpr auto arrowLengthFactor = 1.0F;
			constexpr auto arrowThicknessFactor = 0.015F;
			constexpr auto arrowCapLengthFactor = 0.2F;
			constexpr auto arrowCapThicknessFactor = 0.06F;
			constexpr auto quality = 8U;
			constexpr auto gapeFactor = 0.5F;

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

			return newGeometry.load(std::move(shape));
		}, m_generationParameters.geometryFlags());
	}

	std::shared_ptr< VertexGridResource >
	ResourceGenerator::surface (float size, uint32_t division, std::string resourceName) const noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = this->generateResourceName("Surface", std::to_string(size) + ',' + std::to_string(division));
		}

		return m_resources.container< VertexGridResource >()->getOrCreateResourceAsync(std::move(resourceName), [size, division, parameters = m_generationParameters] (VertexGridResource & newGeometry) {
			return newGeometry.load(size, division, parameters.textureCoordinatesMultiplier()[X], parameters.vertexColorGenMode(), parameters.globalVertexColor());
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
