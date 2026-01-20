/*
 * src/Libs/VertexFactory/FileFormatNative.hpp
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

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief Emeraude engine native geometry format.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @extends EmEn::Libs::VertexFactory::FileFormatInterface
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatNative final : public FileFormatInterface< vertex_data_t, index_data_t >
	{
		public:

			static constexpr auto Magic{"EE3D_V1"};

			/**
			 * @brief Constructs a native file format.
			 */
			FileFormatNative () noexcept = default;

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & /*readOptions*/) noexcept override
			{
				geometry.clear();

				/* Open file in binary mode */
				std::ifstream file{filepath, std::ios::in | std::ios::binary};
				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to open '" << filepath << "' !" "\n";

					return false;
				}

				/* 1. Read Header (32 bytes) */
				char header[32] = {0};
				file.read(header, 32);

				/* Check Magic "EE3D_V1" (8 bytes including null) */
				if ( std::string(header, 7) != Magic )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", invalid magic in '" << filepath << "' !" "\n";
					return false;
				}

				/* Check Version (2 bytes) at offset 8 */
				uint16_t version = *reinterpret_cast< uint16_t * >(&header[8]);
				if ( version != 1 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unsupported version " << version << " in '" << filepath << "' !" "\n";
					return false;
				}

				/* Check Precision (2 bytes) at offset 10 & 11 */
				const auto vPrecision = static_cast< uint8_t >(header[10]);
				const auto iPrecision = static_cast< uint8_t >(header[11]);

				if ( vPrecision != sizeof(vertex_data_t) || iPrecision != sizeof(index_data_t) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", precision mismatch in '" << filepath << "' !" "\n";
					return false;
				}

				/* 2. Read Metadata (Counts) - 3 * 8 bytes = 24 bytes */
				uint64_t counts[3] = {0, 0, 0}; /* Vertices, Triangles, Colors */
				file.read(reinterpret_cast< char * >(counts), 3 * sizeof(uint64_t));
				
				uint64_t vertexCount = counts[0];
				uint64_t triangleCount = counts[1];
				uint64_t colorCount = counts[2];

				/* 3. Resize Geometry */
				/* Access mutable containers directly to resize and then read into data() */
				auto & vertices = geometry.vertices();
				auto & triangles = geometry.triangles();
				auto & colors = geometry.vertexColors();

				vertices.resize(vertexCount);
				triangles.resize(triangleCount);
				colors.resize(colorCount);

				/* 4. Read Data Blobs */
				/* Vertices */
				if ( vertexCount > 0 )
				{
					file.read(reinterpret_cast< char * >(vertices.data()), static_cast< std::streamsize >(vertexCount * sizeof(ShapeVertex< vertex_data_t >)));
				}

				/* Triangles */
				if ( triangleCount > 0 )
				{
					file.read(reinterpret_cast< char * >(triangles.data()), static_cast< std::streamsize >(triangleCount * sizeof(ShapeTriangle< vertex_data_t, index_data_t >)));
				}

				/* Vertex Colors */
				if ( colorCount > 0 )
				{
					file.read(reinterpret_cast< char * >(colors.data()), static_cast< std::streamsize >(colorCount * sizeof(Math::Vector< 4, vertex_data_t >)));
				}

				if ( !file.good() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", error while reading data from '" << filepath << "' !" "\n";

					return false;
				}

				/* Update bounding box/sphere which are not serialized to save space/time and are easily recomputed */
				/* Actually, Shape doesn't update properties automatically on simple resize/fill. 
				   We need to manually trigger property updates or recompute.
				   Shape::transform uses 'updateProperties' flag.
				   We can call a method to update bounds if available, or just leave it to the user.
				   For now we leave as is, raw data loaded.
				*/

				return true;
			}

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & filepath, const Shape< vertex_data_t, index_data_t > & geometry) const noexcept override
			{
				if ( !geometry.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", geometry parameter is invalid !" "\n";

					return false;
				}

				std::ofstream file{filepath, std::ios::out | std::ios::binary};
				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to open '" << filepath << "' file to write !" "\n";

					return false;
				}

				/* 1. Header (32 bytes) */
				char header[32] = {0};
				
				/* Magic "EE3D_V1" */
				std::ranges::copy(std::string{Magic}, header);
				
				/* Version 1 at offset 8 */
				uint16_t version = 1;
				*reinterpret_cast< uint16_t * >(&header[8]) = version;
				
				/* Precision at offset 10, 11 */
				header[10] = static_cast< char >(sizeof(vertex_data_t));
				header[11] = static_cast< char >(sizeof(index_data_t));
				
				/* Write Header */
				file.write(header, 32);

				/* 2. Metadata (Counts) */
				uint64_t counts[3];
				counts[0] = static_cast< uint64_t >(geometry.vertices().size());
				counts[1] = static_cast< uint64_t >(geometry.triangles().size());
				counts[2] = static_cast< uint64_t >(geometry.vertexColors().size());

				file.write(reinterpret_cast< const char * >(counts), 3 * sizeof(uint64_t));

				/* 3. Data Blobs */
				/* Vertices */
				if ( counts[0] > 0 )
				{
					file.write(reinterpret_cast< const char * >(geometry.vertices().data()), static_cast< std::streamsize >(counts[0] * sizeof(ShapeVertex< vertex_data_t >)));
				}

				/* Triangles */
				if ( counts[1] > 0 )
				{
					/* Note: ShapeTriangle is a template class. We assume standard layout. */
					file.write(reinterpret_cast< const char * >(geometry.triangles().data()), static_cast< std::streamsize >(counts[1] * sizeof(ShapeTriangle< vertex_data_t, index_data_t >)));
				}

				/* Colors */
				if ( counts[2] > 0 )
				{
					file.write(reinterpret_cast< const char * >(geometry.vertexColors().data()), static_cast< std::streamsize >(counts[2] * sizeof(Math::Vector< 4, vertex_data_t >)));
				}

				file.close();

				return true;
			}
	};
}
