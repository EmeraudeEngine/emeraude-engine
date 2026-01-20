/*
 * src/Libs/VertexFactory/FileFormatSTL.hpp
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
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "ShapeTriangle.hpp"
#include "ShapeVertex.hpp"
#include "Shape.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The FileFormatSTL class.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @note https://en.wikipedia.org/wiki/STL_(file_format)
	 * @extends EmEn::Libs::VertexFactory::FileFormatInterface
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatSTL final : public FileFormatInterface< vertex_data_t, index_data_t >
	{
		public:

			/**
			 * @brief Constructs an STL file format.
			 */
			FileFormatSTL () noexcept = default;

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & readOptions) noexcept override
			{
				std::ifstream file{filepath, std::ios::in | std::ios::binary};

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to read STL file '" << filepath << "' !" "\n";

					return false;
				}

				if ( this->isAscii(file) )
				{
					return this->readAscii(file, geometry);
				}
				
				return this->readBinary(file, geometry);
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

				/* NOTE: We default to Binary STL for saving as it is more compact. */
				std::ofstream file{filepath, std::ios::out | std::ios::binary};

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to open '" << filepath << "' file to write !" "\n";
					return false;
				}

				/* Write Header (80 bytes) */
				char header[80] = {0};
				const std::string headerStr = "Exported by Emeraude-Engine";
				std::copy_n(headerStr.begin(), std::min(headerStr.size(), sizeof(header)), header);
				file.write(header, 80);

				/* Write Triangle Count (4 bytes) */
				auto triangleCount = static_cast< uint32_t >(geometry.triangles().size());
				file.write(reinterpret_cast< const char * >(&triangleCount), sizeof(uint32_t));

				/* Write Triangles */
				for ( size_t i = 0; i < geometry.triangles().size(); ++i )
				{
					const auto & triangle = geometry.triangles()[i];

					/* Normal */
					Math::Vector< 3, float > normal = {0.0f, 0.0f, 0.0f};
					/* Try to get face normal if possible, otherwise zero */
					/* Since we don't have easy access to computed face normal here without computation,
					 * we can either compute it or write 0. STL standard allows 0. */
					file.write(reinterpret_cast< const char * >(&normal), 3 * sizeof(float));

					/* Vertices */
					for ( size_t v = 0; v < 3; ++v )
					{
						const auto & vertex = geometry.vertex(triangle.vertexIndex(v));
						Math::Vector< 3, float > pos = vertex.position(); /* Assuming float position */
						
						/* Convert to float explicitly if vertex_data_t is double */
						auto x = static_cast< float >(pos.x());
						auto y = static_cast< float >(pos.y());
						auto z = static_cast< float >(pos.z());
						
						file.write(reinterpret_cast< const char * >(&x), sizeof(float));
						file.write(reinterpret_cast< const char * >(&y), sizeof(float));
						file.write(reinterpret_cast< const char * >(&z), sizeof(float));
					}

					/* Attribute Byte Count (2 bytes) - usually 0 */
					uint16_t attributeByteCount = 0;
					file.write(reinterpret_cast< const char * >(&attributeByteCount), sizeof(uint16_t));
				}

				file.close();

				return true;
			}

		private:

			/**
			 * @brief Check if the file is ASCII or Binary.
			 * @param file A reference to the input stream.
			 * @return bool True if ASCII, False if Binary.
			 */
			bool
			isAscii (std::ifstream & file) const noexcept
			{
				/* STL rules:
				 * - ASCII starts with "solid"
				 * - Binary has an 80-byte header, generally not starting with "solid" (but it can!)
				 * - Ideally we check file size vs expected binary size: 80 + 4 + (50 * triangle_count)
				 */

				char header[6]; // "solid" + 1
				file.read(header, 5);
				header[5] = '\0';
				
				bool startsWithSolid = (std::string(header) == "solid");
				
				/* If it does not start with solid, it is binary (or invalid). */
				if ( !startsWithSolid )
				{
					file.seekg(0);
					return false;
				}

				/* If it starts with solid, it MIGHT be ascii, or a binary file with "solid" in header. */
				/* Check file size. */
				file.seekg(0, std::ios::end);
				auto fileSize = file.tellg();
				
				file.seekg(80);
				uint32_t count = 0;
				if ( file.read(reinterpret_cast< char * >(&count), 4) )
				{
					auto expectedSize = 80 + 4 + (static_cast< std::streamoff >(count) * 50);
					if ( fileSize == expectedSize )
					{
						/* It matches binary size perfectly, so assume binary. */
						file.seekg(0);
						return false;
					}
				}

				/* Assume ASCII */
				file.seekg(0);
				return true;
			}

			/**
			 * @brief Reads an ASCII STL file.
			 * @param file A reference to the input stream.
			 * @param geometry A reference to the geometry to fill.
			 * @return bool
			 */
			bool
			readAscii (std::ifstream & file, Shape< vertex_data_t, index_data_t > & geometry) noexcept
			{
				std::string line;
				std::string dummy;
				
				std::vector< ShapeVertex< vertex_data_t > > vertices;
				std::vector< ShapeTriangle< vertex_data_t, index_data_t > > triangles;
				
				/* Reserve some space to avoid too many reallocs */
				vertices.reserve(1000);
				triangles.reserve(500);

				/* Keep track of unique vertices to build indexed geometry */
				/* Simple approach: Linear search or Map. For speed/simplicity in this context, 
				   we will just duplicates vertices for now or use a basic dedup if necessary.
				   Given ShapeBuilder usually handles some stuff, but here we populate vectors manually.
				   Let's use a builder approach similar to OBJ reference if possible, 
				   but here we will just push vertices and triangles. 
				*/

				/* 
				   Shape builder helper is usually better. 
				   However, `Shape` class has `build` method.
				   We will populate vectors and then let the `build` method do the job if we used the lambda approach.
				   Alternatively, we can manually populate.
				*/

				return geometry.build([&file, this] (std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) {
					
					std::string line;
					std::vector< ShapeVertex< vertex_data_t > > faceVertices;
					Math::Vector< 3, float > normal;
					
					while ( std::getline(file, line) )
					{
						/* Trim leading whitespace */
						auto first = line.find_first_not_of(" \t\r");
						if ( first == std::string::npos ) continue;
						
						if ( line.compare(first, 5, "solid") == 0 )
						{
							continue;
						}
						
						if ( line.compare(first, 12, "facet normal") == 0 )
						{
							float nx, ny, nz;
							/* "facet normal ni nj nk" */
							/* scan after "facet normal" */
							sscanf(line.c_str() + first + 12, "%f %f %f", &nx, &ny, &nz);
							normal = {nx, ny, nz};
							faceVertices.clear();
						}
						else if ( line.compare(first, 6, "vertex") == 0 )
						{
							float x, y, z;
							sscanf(line.c_str() + first + 6, "%f %f %f", &x, &y, &z);
							
							ShapeVertex< vertex_data_t > v;
							v.setPosition(Math::Vector< 3, vertex_data_t >{static_cast< vertex_data_t >(x), static_cast< vertex_data_t >(y), static_cast< vertex_data_t >(z)});
							v.setNormal(Math::Vector< 3, vertex_data_t >{static_cast< vertex_data_t >(normal.x()), static_cast< vertex_data_t >(normal.y()), static_cast< vertex_data_t >(normal.z())});
							
							faceVertices.push_back(v);
						}
						else if ( line.compare(first, 7, "endloop") == 0 )
						{
							/* End of a loop (face) */
							if ( faceVertices.size() == 3 )
							{
								/* Add vertices to global list and create triangle */
								/* Simple non-indexed assembly (optimisable later) */
								auto baseIndex = static_cast< index_data_t >(vertices.size());
								
								vertices.push_back(faceVertices[0]);
								vertices.push_back(faceVertices[1]);
								vertices.push_back(faceVertices[2]);
								
								ShapeTriangle< vertex_data_t, index_data_t > tri;
								tri.setVertexIndex(0, baseIndex);
								tri.setVertexIndex(1, baseIndex + 1);
								tri.setVertexIndex(2, baseIndex + 2);
								
								triangles.push_back(tri);
							}
						}
					}
					return true;
				}, false, false);
			}

			/**
			 * @brief Reads a Binary STL file.
			 * @param file A reference to the input stream.
			 * @param geometry A reference to the geometry to fill.
			 * @return bool
			 */
			bool
			readBinary (std::ifstream & file, Shape< vertex_data_t, index_data_t > & geometry) noexcept
			{
				/* Skip header */
				file.seekg(80);

				uint32_t triangleCount = 0;
				if ( !file.read(reinterpret_cast< char * >(&triangleCount), 4) )
				{
					return false;
				}

				return geometry.build([&file, triangleCount] (std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) {
					
					/* Optimize allocation */
					vertices.reserve(triangleCount * 3);
					triangles.reserve(triangleCount);

					for ( uint32_t i = 0; i < triangleCount; ++i )
					{
						float n[3];
						float v[3][3];
						uint16_t attr;

						/* Read Normal (3 floats) */
						file.read(reinterpret_cast< char * >(n), 12);
						
						/* Read Vertices (3 * 3 floats) */
						file.read(reinterpret_cast< char * >(v), 36);
						
						/* Read Attribute Byte Count (2 bytes) */
						file.read(reinterpret_cast< char * >(&attr), 2);

						if ( !file.good() ) return false;

						auto baseIndex = static_cast< index_data_t >(vertices.size());

						Math::Vector< 3, vertex_data_t > normal = {
							static_cast< vertex_data_t >(n[0]),
							static_cast< vertex_data_t >(n[1]),
							static_cast< vertex_data_t >(n[2])
						};

						for ( auto & j : v )
						{
							ShapeVertex< vertex_data_t > sv;
							sv.setPosition(Math::Vector< 3, vertex_data_t >{
								static_cast< vertex_data_t >(j[0]),
								static_cast< vertex_data_t >(j[1]),
								static_cast< vertex_data_t >(j[2])
							});
							sv.setNormal(normal);
							vertices.push_back(sv);
						}

						ShapeTriangle< vertex_data_t, index_data_t > tri;
						tri.setVertexIndex(0, baseIndex);
						tri.setVertexIndex(1, baseIndex + 1);
						tri.setVertexIndex(2, baseIndex + 2);
						triangles.push_back(tri);
					}
					
					return true;
				}, false, false);
			}
	};
}
