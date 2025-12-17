/*
 * src/Tool/GeometryDataPrinter.cpp
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

#include "GeometryDataPrinter.hpp"

/* STL inclusions. */
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

/* Local inclusions. */
#include "Libs/VertexFactory/ShapeGenerator.hpp"
#include "Libs/BlobTrait.hpp"
#include "Graphics/Types.hpp"
#include "Arguments.hpp"
#include "Constants.hpp"
#include "Tracer.hpp"

namespace EmEn::Tool
{
	using namespace Libs;
	using namespace Libs::VertexFactory;
	using namespace Graphics;

	GeometryDataPrinter::GeometryDataPrinter (const Arguments & arguments) noexcept
	{
		if ( const auto arg = arguments.get("--output-file", "-o") )
		{
			m_outputFile = arg.value();
		}

		if ( const auto arg = arguments.get("--shape") )
		{
			m_shapeType = to_ShapeType(String::ucfirst(arg.value()));
		}

		if ( const auto arg = arguments.get("--size") )
		{
			m_baseSize = String::toNumber< float >(arg.value());
		}

		if ( const auto arg = arguments.get("--length") )
		{
			m_baseLength = String::toNumber< float >(arg.value());
		}

		if ( const auto arg = arguments.get("--quality") )
		{
			m_quality = String::toNumber< uint32_t >(arg.value());
		}
		
		m_enableNormals = arguments.isSwitchPresent("--enable-normals");
		m_enableTangentSpace = arguments.isSwitchPresent("--enable-tangent-space");
		m_enableTexCoords = arguments.isSwitchPresent("--enable-tex-coords");
		m_enable3DTexCoords = arguments.isSwitchPresent("--enable-3d-tex-coords");
	}

	bool
	GeometryDataPrinter::execute () noexcept
	{
		Tracer::info(ClassId, "Execution shape generation ...");

		switch ( m_shapeType )
		{
			case ShapeType::Triangle :
				m_shape = ShapeGenerator::generateTriangle< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Quad :
				m_shape = ShapeGenerator::generateQuad< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Cube :
				m_shape = ShapeGenerator::generateCuboid< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Sphere :
				m_shape = ShapeGenerator::generateSphere< float, uint32_t >(m_baseSize, m_quality, m_quality);
				break;

			case ShapeType::GeodesicSphere :
				m_shape = ShapeGenerator::generateGeodesicSphere< float, uint32_t >(m_baseSize, m_quality);
				break;

			case ShapeType::Cylinder :
				m_shape = ShapeGenerator::generateCylinder< float, uint32_t >(m_baseSize, m_baseSize, m_baseLength, m_quality);
				break;

			case ShapeType::Cone :
				m_shape = ShapeGenerator::generateCone< float, uint32_t >(m_baseSize, m_baseLength, m_quality);
				break;

			case ShapeType::Disk :
				m_shape = ShapeGenerator::generateDisk< float, uint32_t >(m_baseSize, m_baseSize * Half< float >, m_quality);
				break;

			case ShapeType::Torus :
				m_shape = ShapeGenerator::generateTorus< float, uint32_t >(m_baseSize, m_baseSize * Half< float >, m_quality, m_quality);
				break;

			case ShapeType::Tetrahedron :
				m_shape = ShapeGenerator::generateTetrahedron< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Hexahedron :
				m_shape = ShapeGenerator::generateHexahedron< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Octahedron :
				m_shape = ShapeGenerator::generateOctahedron< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Dodecahedron :
				m_shape = ShapeGenerator::generateDodecahedron< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Icosahedron :
				m_shape = ShapeGenerator::generateIcosahedron< float, uint32_t >(m_baseSize);
				break;

			case ShapeType::Custom :
			case ShapeType::Arrow :
			case ShapeType::Axis :
				TraceWarning{ClassId} << "The shape '" << to_cstring(m_shapeType) << "' is not handled !";
				return false;
		}

		if ( !m_shape.isValid() )
		{
			Tracer::error(ClassId, "Invalid shape produced !");

			return false;
		}

		if ( m_outputFile.empty() )
		{
			std::cout << "\n\n" << this->printData() << "\n\n";
		}
		else
		{
			std::ofstream out{m_outputFile};

			if ( !out.is_open() )
			{
				TraceError{ClassId} << "Unable to open file '" << m_outputFile << "' !";

				return false;
			}

			out <<
				"#pragma once" "\n\n"

				"#include <cstdint>" "\n"
				"#include <array>" "\n\n";

			out << this->printData();

			out.close();
		}

		Tracer::getInstance().disableTracer(true);

		return true;
	}

	std::string
	GeometryDataPrinter::printData () const noexcept
	{
		std::vector< float > vertices;
		std::vector< uint32_t > indices;

		NormalType normals;
		TextureCoordinatesType textureCoordinates;

		if ( m_enableTangentSpace )
		{
			normals = NormalType::TBNSpace;
		}
		else if ( m_enableNormals )
		{
			normals = NormalType::Normal;
		}
		else
		{
			normals = NormalType::None;
		}

		if ( m_enableTexCoords )
		{
			textureCoordinates = m_enable3DTexCoords ?
				TextureCoordinatesType::UVW :
				TextureCoordinatesType::UV;
		}
		else
		{
			textureCoordinates = TextureCoordinatesType::None;
		}

		const auto vertexElementCount = m_shape.createIndexedVertexBuffer(vertices, indices, normals, textureCoordinates);

		std::stringstream out{};

		out <<
			"const uint32_t vertexElementCount = " << vertexElementCount << ";" "\n"
			"const uint32_t vertexCount = " << m_shape.vertexCount() << ";" "\n\n"

			"const std::array< float, " << vertexElementCount * m_shape.vertexCount() << " > vertices{" "\n";

		for ( size_t index = 0; index < vertices.size(); index += vertexElementCount )
		{
			out << '\t';

			for ( size_t elementIndex = 0; elementIndex < vertexElementCount; elementIndex++ )
			{
				out << vertices[index + elementIndex] << ", ";
			}

			out << '\n';
		}

		out << "};" "\n\n";

		out << "const std::array< uint32_t, " << indices.size() << " > indices{" "\n";

		for ( size_t index = 0; index < indices.size(); index += 3 )
		{
			out << '\t';

			for ( size_t elementIndex = 0; elementIndex < 3; elementIndex++ )
			{
				out << indices[index + elementIndex] << ", ";
			}

			out << '\n';
		}

		out << "};" "\n\n";

		return out.str();
	}
}
