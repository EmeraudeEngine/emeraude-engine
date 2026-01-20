/*
 * src/Libs/VertexFactory/FileFormatOBJ.hpp
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
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
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
	 * @brief An OBJ vertex structure.
	 * @tparam index_data_t The precision type of index data.
	 * @note OBJ format use index starting at 1.
	 */
	template< typename index_data_t >
	requires (std::is_unsigned_v< index_data_t > )
	class OBJVertex final
	{
		public:

			explicit
			OBJVertex (index_data_t vIndex) noexcept
				: vIndex{vIndex}
			{

			}

			OBJVertex (index_data_t vIndex, index_data_t vtIndex) noexcept
				: vIndex{vIndex},
				vtIndex{vtIndex}
			{

			}

			OBJVertex (index_data_t vIndex, index_data_t vtIndex, index_data_t vnIndex) noexcept
				: vIndex{vIndex},
				vtIndex{vtIndex},
				vnIndex{vnIndex}
			{

			}
				
			index_data_t vIndex{0};
			index_data_t vtIndex{0};
			index_data_t vnIndex{0};
	};

	/**
	 * @brief An OBJ triangle structure.
	 * @tparam index_data_t The precision type of index data.
	 */
	template< typename index_data_t >
	requires (std::is_unsigned_v< index_data_t > )
	using OBJTriangle = std::array< OBJVertex< index_data_t >, 3 >;

	/**
	 * @brief The FileFormatOBJ class.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @note http://www.fileformat.info/format/wavefrontobj/egff.htm
	 * @extends EmEn::Libs::VertexFactory::FileFormatInterface
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatOBJ final : public FileFormatInterface< vertex_data_t, index_data_t >
	{
		public:

			/**
			 * @brief Constructs an OBJ file format.
			 */
			FileFormatOBJ () noexcept = default;

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & readOptions) noexcept override
			{
				m_readOptions = readOptions;

				/* Opening the file. */
				std::ifstream file{filepath};

				if ( !file.is_open() )
				{
					std::cerr << "FileFormatOBJ::readFile(), unable to read OBJ file '" << filepath << "' !" "\n";

					return false;
				}
				
				/* Read a first time to calculate the space required for the structure. */
				{
					if ( !this->analyseFileContent(file) )
					{
						std::cerr << "FileFormatOBJ::readFile(), step 1 'Reserving space' has failed !" "\n";

						return false;
					}

					file.clear();
					file.seekg(0);
				}

				/* 3. Read a third and final time to assemble faces. */
				const bool buildSuccess = geometry.build([this, &geometry, &file] (std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) {
					switch ( m_faceMode )
					{
						case FaceMode::V :
							if ( !this->parseFaceAssemblyV(file, groups, vertices, triangles) )
							{
								return false;
							}

							if ( m_readOptions.requestNormal )
							{
								if ( !geometry.computeTriangleNormal() )
								{
									return false;
								}

								if ( !geometry.computeVertexNormal() )
								{
									return false;
								}
							}
							break;

						case FaceMode::V_VN :
							if ( !this->parseFaceAssemblyV_VN(file, groups, vertices, triangles) )
							{
								return false;
							}
							break;

						case FaceMode::V_VT :
							if ( !this->parseFaceAssemblyV_VT(file, groups, vertices, triangles) )
							{
								return false;
							}

							if ( m_readOptions.requestTangentSpace )
							{
								if ( !geometry.computeTriangleTBNSpace() )
								{
									return false;
								}

								if ( !geometry.computeVertexTBNSpace() )
								{
									return false;
								}
							}
							else if ( m_readOptions.requestNormal )
							{
								if ( !geometry.computeTriangleNormal() )
								{
									return false;
								}

								if ( !geometry.computeVertexNormal() )
								{
									return false;
								}
							}
							break;

						case FaceMode::V_VT_VN :
							if ( !this->parseFaceAssemblyV_VT_VN(file, groups, vertices, triangles) )
							{
								return false;
							}

							if ( m_readOptions.requestTangentSpace )
							{
								if ( !geometry.computeTriangleTangent() )
								{
									return false;
								}

								if ( !geometry.computeVertexTangent() )
								{
									return false;
								}
							}
							break;

						default:
							return false;
					}

					return true;
				}, !m_vt.empty(), false);

				if ( !buildSuccess )
				{
					return false;
				}

				/* Set the normals flag if normals were read from file. */
				if ( !m_vn.empty() )
				{
					geometry.declareNormalsAvailable();
				}

				return true;
			}

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & filepath, const Shape< vertex_data_t, index_data_t > & geometry) const noexcept override
			{
				if ( !geometry.isValid() )
				{
					std::cerr << "FileFormatOBJ::writeFile(), geometry parameter is invalid !" "\n";

					return false;
				}

				std::ofstream file{filepath, std::ios::out};

				if ( !file.is_open() )
				{
					std::cerr << "FileFormatOBJ::writeFile(), unable to open '" << filepath << "' file to write !" "\n";

					return false;
				}

				/* Header */
				file << "# Exported by Emeraude-Engine" << "\n";
				file << "o Geometry" << "\n";

				/* Write Vertices (v) */
				for ( const auto & vertex : geometry.vertices() )
				{
					const auto & pos = vertex.position();

					/* OBJ does not enforce index, just order. */
					file << "v " << pos.x() << " " << pos.y() << " " << pos.z() << "\n";
				}

				/* Write Texture Coordinates (vt) */
				/* We write them even if zero/invalid if we want to rely on the same index, 
				   OR we write them and keep a mapping. 
				   The simplest approach for indexed geometry in Shape is to assume every vertex has unique attributes combination.
				   So we just dump all attributes in order and reference them by same index.
				*/
				if ( geometry.isTextureCoordinatesAvailable() )
				{
					for ( const auto & vertex : geometry.vertices() )
					{
						const auto & uv = vertex.textureCoordinates();

						file << "vt " << uv.x() << " " << uv.y() << "\n";
					}
				}

				/* Write Normals (vn) - only if available */
				const bool hasNormals = geometry.isNormalsAvailable();

				if ( hasNormals )
				{
					for ( const auto & vertex : geometry.vertices() )
					{
						const auto & n = vertex.normal();

						file << "vn " << n.x() << " " << n.y() << " " << n.z() << "\n";
					}
				}

				/* Write Faces (f) */
				/* OBJ indices are 1-based */
				const bool hasUV = geometry.isTextureCoordinatesAvailable();

				file << "s off" << "\n";

				for ( const auto & triangle : geometry.triangles() )
				{
					file << "f";

					for ( int index = 0; index < 3; ++index )
					{
						const index_data_t idx = triangle.vertexIndex(index) + 1;

						if ( hasUV && hasNormals )
						{
							/* f v/vt/vn */
							file << " " << idx << "/" << idx << "/" << idx;
						}
						else if ( hasUV )
						{
							/* f v/vt */
							file << " " << idx << "/" << idx;
						}
						else if ( hasNormals )
						{
							/* f v//vn */
							file << " " << idx << "//" << idx;
						}
						else
						{
							/* f v */
							file << " " << idx;
						}
					}

					file << "\n";
				}

				file.close();

				return true;
			}

		private:

			/**
			 * @brief How the face are declared in the file.
			 * @note Values are ordered by "completeness" to allow comparison.
			 */
			enum class FaceMode : uint8_t
			{
				Undetermined,
				/** @brief F Line format : "f v1 v2 v3" */
				V,
				/** @brief F Line format : "f v1//vn1 v2//vn2 v3//vn3" */
				V_VN,
				/** @brief F Line format : "f v1/vt1 v2/vt2 v3/vt3" */
				V_VT,
				/** @brief F Line format : "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3" */
				V_VT_VN
			};

			/**
			 * @brief Tells what kind of attributes are predominant in the OBJ file.
			 */
			enum PredominantAttributes
			{
				V,
				VN,
				VT
			};

			/**
			 * @brief Determines how the OBJ format describe a face.
			 * @param fline A reference to a string.
			 * @return FaceMode
			 */
			[[nodiscard]]
			static
			FaceMode
			determineFaceDeclarationMode (const std::string & fline) noexcept
			{
				if ( fline.find("//") != std::string::npos )
				{
					return FaceMode::V_VN;
				}

				const auto dashCount = std::ranges::count(fline, '/');

				if ( dashCount == 0 )
				{
					return FaceMode::V;
				}

				const auto spaceCount = std::ranges::count(fline, ' ');

				if ( dashCount > spaceCount )
				{
					return FaceMode::V_VT_VN;
				}

				return FaceMode::V_VT;
			}

			/**
			 * @brief Reads a first time to calculate the space required for the structure.
			 * @param file A reference to an input stream.
			 * @return bool
			 */
			bool
			analyseFileContent (std::ifstream & file) noexcept
			{
				std::string line{};

				index_data_t positionCount = 0;
				index_data_t textureCoordinatesCount = 0;
				index_data_t normalCount = 0;

				while ( std::getline(file, line) )
				{
					if ( line.starts_with("v ") )
					{
						++positionCount;
					}
					else if ( line.starts_with("vt ") )
					{
						++textureCoordinatesCount;
					}
					else if ( line.starts_with("vn ") )
					{
						++normalCount;
					}
					else if ( line.starts_with("f ") )
					{
						/* NOTE: Detect the most complete face mode present in the file.
						 * Files may mix formats (e.g., V_VN and V_VT_VN), so we track the "highest" mode seen.
						 */
						const auto lineFaceMode = determineFaceDeclarationMode(line);

						if ( static_cast< uint8_t >(lineFaceMode) > static_cast< uint8_t >(m_faceMode) )
						{
							m_faceMode = lineFaceMode;
						}

						const auto vertexCount = countFaceVertices(line);

						if ( vertexCount < 3 )
						{
							std::cerr << "FileFormatOBJ::analyseFileContent(), this OBJ loader requires at least 3 vertices per face (got " << vertexCount << ") !" << '\n';

							return false;
						}

						/* NOTE: Polygons with N vertices are triangulated into (N-2) triangles using fan triangulation. */
						m_faceCount += static_cast< index_data_t >(vertexCount - 2);
					}
				}

				if constexpr ( VertexFactoryDebugEnabled )
				{
					std::cout <<
						"[DEBUG:VERTEX_FACTORY] File Parsing - First pass result." "\n" <<
						"\t" "Vertices : " << positionCount << "\n"
						"\t" "Texture Coordinates : " << textureCoordinatesCount << "\n"
						"\t" "Normals : " << normalCount << "\n"
						"\t" "Faces : " << m_faceCount << "\n\n";
				}

				if ( positionCount == 0 )
				{
					std::cerr << "FileFormatOBJ::analyseFileContent(), there is no vertex definition in the OBJ file. Aborting." "\n";

					return false;
				}

				if ( m_faceCount == 0 )
				{
					std::cerr << "FileFormatOBJ::analyseFileContent(), there is no face definition in the OBJ file. Aborting." "\n";

					return false;
				}

				/* Resize vectors. */
				m_v.resize(positionCount);
				m_vt.resize(textureCoordinatesCount);
				m_vn.resize(normalCount);

				m_vertexCount = static_cast< index_data_t >(std::max(m_v.size(), std::max(m_vt.size(), m_vn.size())));

				if ( m_vertexCount == static_cast< index_data_t >(m_v.size()) )
				{
					m_predominantAttribute = PredominantAttributes::V;
				}
				else if ( m_vertexCount == static_cast< index_data_t >(m_vn.size()) )
				{
					m_predominantAttribute = PredominantAttributes::VN;
				}
				else
				{
					m_predominantAttribute = PredominantAttributes::VT;
				}

				return true;
			}

			/**
			 * @brief Reads a line using OBJ F line format : "f v1 v2 v3"
			 * @param file A reference to the OBJ file.
			 * @param groups A reference to an index list.
			 * @param vertices A reference to the vertex list.
			 * @param triangles A reference to the triangle list.
			 * @return bool
			 */
			bool
			parseFaceAssemblyV (std::ifstream & file, std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) noexcept
			{
				/* NOTE: Resize/reserving memory space to the geometry shape. */
				vertices.resize(m_vertexCount);
				triangles.reserve(m_faceCount);

				/* Keep count on attributes and triangles. */
				index_data_t positionCount = 0;
				index_data_t triangleCount = 0;

				std::string objLine{};

				while ( std::getline(file, objLine) )
				{
					/* Declaring a group. */
					if ( objLine.starts_with("g ") )
					{
						if ( triangleCount > 0 )
						{
							groups.emplace_back(triangleCount, 0);
						}

						continue;
					}

					if ( objLine.starts_with("v ") )
					{
						this->extractPositionAttribute(objLine, positionCount++);
					}
					else if ( objLine.starts_with("f ") )
					{
						std::vector< OBJVertex< index_data_t > > faceIndices{};

						if ( !this->extractFaceIndices(objLine, faceIndices) )
						{
							return false;
						}

						/* NOTE: f line can express a triangle or a polygon ! */
						for ( index_data_t triangleOffset = 0; triangleOffset < faceIndices.size() - 2; ++triangleOffset )
						{
							/* NOTES: "f 0 1 2 3"
							 * Must goes into
							 * "0 1 2"
							 * "0 2 3"
							 */

							ShapeTriangle< vertex_data_t, index_data_t > triangle;

							for ( index_data_t faceVertexIndex = 0; faceVertexIndex < 3; ++faceVertexIndex )
							{
								const auto realFaceVertexIndex = faceVertexIndex == 0 ? faceVertexIndex : faceVertexIndex + triangleOffset;

								/* NOTE: Convert OBJ index to vector index. */
								const auto vIndex = faceIndices.at(realFaceVertexIndex).vIndex - 1;

								/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
								vertices.at(vIndex).setPosition(m_v.at(vIndex));

								/* Declare the vertex index to one of the three vertices of the triangle. */
								triangle.setVertexIndex(faceVertexIndex, vIndex);
							}

							triangles.emplace_back(triangle);

							++triangleCount;

							if ( !groups.empty() )
							{
								++groups.back().second;
							}
						}
					}
				}

				return true;
			}

			/**
			 * @brief Reads a line using OBJ F line format : "f v1//vn1 v2//vn2 v3//vn3".
			 * @param file A reference to the OBJ file.
			 * @param groups A reference to an index list.
			 * @param vertices A reference to the vertex list.
			 * @param triangles A reference to the triangle list.
			 * @return bool
			 */
			bool
			parseFaceAssemblyV_VN (std::ifstream & file, std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) noexcept
			{
				/* NOTE: Resize/reserving memory space to the geometry shape. */
				vertices.resize(m_vertexCount);
				triangles.reserve(m_faceCount);

				/* Keep track of generated shape vertex index. */
				std::set< index_data_t > writtenIndexes{};

				/* Keep count on attributes and triangles. */
				index_data_t positionCount = 0;
				index_data_t normalCount = 0;
				index_data_t triangleCount = 0;

				std::string objLine{};

				while ( std::getline(file, objLine) )
				{
					/* Declaring a group. */
					if ( objLine.starts_with("g ") )
					{
						if ( triangleCount > 0 )
						{
							groups.emplace_back(triangleCount, 0);
						}

						continue;
					}

					if ( objLine.starts_with("v ") )
					{
						this->extractPositionAttribute(objLine, positionCount++);
					}
					else if ( objLine.starts_with("vn ") )
					{
						this->extractNormalAttribute(objLine, normalCount++);
					}
					else if ( objLine.starts_with("f ") )
					{
						std::vector< OBJVertex< index_data_t > > faceIndices;

						if ( !this->extractFaceIndices(objLine, faceIndices) )
						{
							return false;
						}

						/* NOTE: f line can express a triangle or a polygon ! */
						for ( index_data_t triangleOffset = 0; triangleOffset < faceIndices.size() - 2; ++triangleOffset )
						{
							/* NOTES: "f 0 1 2 3"
							 * Must goes into
							 * "0 1 2"
							 * "0 2 3"
							 */

							ShapeTriangle< vertex_data_t, index_data_t > triangle;

							for ( index_data_t faceVertexIndex = 0; faceVertexIndex < 3; ++faceVertexIndex )
							{
								const auto realFaceVertexIndex = faceVertexIndex == 0 ? faceVertexIndex : faceVertexIndex + triangleOffset;
								const auto & OBJVertex = faceIndices.at(realFaceVertexIndex);

								/* NOTE: Convert OBJ index (1-based) to vector index (0-based).
								 * Index 0 means "not defined" in our system. */
								const auto vIndex = OBJVertex.vIndex - 1;
								const bool hasNormal = OBJVertex.vnIndex > 0;
								const auto vnIndex = hasNormal ? OBJVertex.vnIndex - 1 : 0;

								/* NOTE: Get the real shape vertex index in the final shape. */
								index_data_t geometryVertexIndex = 0;

								switch ( m_predominantAttribute )
								{
									case PredominantAttributes::V :
										geometryVertexIndex = vIndex;
										break;

									case PredominantAttributes::VN :
										geometryVertexIndex = hasNormal ? vnIndex : vIndex;
										break;

									default:
										return false;
								}

								if ( writtenIndexes.contains(geometryVertexIndex) )
								{
									const auto & vertex = vertices.at(geometryVertexIndex);

									const auto & position = m_v.at(vIndex);

									/* NOTE: If the position is different, we create a new shape vertex. */
									if ( vertex.position() != position )
									{
										const auto normal = hasNormal ? m_vn.at(vnIndex) : Math::Vector< 3, vertex_data_t >{};

										vertices.emplace_back(position, normal);

										geometryVertexIndex = static_cast< index_data_t >(vertices.size() - 1);
									}
								}
								else
								{
									/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
									auto & vertex = vertices.at(geometryVertexIndex);
									vertex.setPosition(m_v.at(vIndex));

									if ( hasNormal )
									{
										vertex.setNormal(m_vn.at(vnIndex));
									}

									writtenIndexes.emplace(geometryVertexIndex);
								}

								/* Declare the vertex index to one of the three vertices of the triangle. */
								triangle.setVertexIndex(faceVertexIndex, geometryVertexIndex);
							}

							triangles.emplace_back(triangle);

							++triangleCount;

							if ( !groups.empty() )
							{
								++groups.back().second;
							}
						}
					}
				}

				return true;
			}

			/**
			 * @brief Reads a line using OBJ F line format : "f v1/vt1 v2/vt2 v3/vt3"
			 * @param file A reference to the OBJ file.
			 * @param groups A reference to an index list.
			 * @param vertices A reference to the vertex list.
			 * @param triangles A reference to the triangle list.
			 * @return bool
			 */
			bool
			parseFaceAssemblyV_VT (std::ifstream & file, std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) noexcept
			{
				/* NOTE: Resize/reserving memory space to the geometry shape. */
				vertices.resize(m_vertexCount);
				triangles.reserve(m_faceCount);

				/* Keep track of generated shape vertex index. */
				std::set< index_data_t > writtenIndexes{};

				/* Keep count on attributes and triangles. */
				index_data_t positionCount = 0;
				index_data_t textureCoordinatesCount = 0;
				index_data_t triangleCount = 0;

				std::string objLine{};

				while ( std::getline(file, objLine) )
				{
					/* Declaring a group. */
					if ( objLine.starts_with("g ") )
					{
						if ( triangleCount > 0 )
						{
							groups.emplace_back(triangleCount, 0);
						}

						continue;
					}

					if ( objLine.starts_with("v ") )
					{
						this->extractPositionAttribute(objLine, positionCount++);
					}
					else if ( objLine.starts_with("vt ") )
					{
						this->extractTextureCoordinatesAttribute(objLine, textureCoordinatesCount++);
					}
					else if ( objLine.starts_with("f ") )
					{
						std::vector< OBJVertex< index_data_t > > faceIndices;

						if ( !this->extractFaceIndices(objLine, faceIndices) )
						{
							return false;
						}

						/* NOTE: f line can express a triangle or a polygon ! */
						for ( index_data_t triangleOffset = 0; triangleOffset < faceIndices.size() - 2; ++triangleOffset )
						{
							/* NOTES: "f 0 1 2 3"
							 * Must goes into
							 * "0 1 2"
							 * "0 2 3"
							 */

							ShapeTriangle< vertex_data_t, index_data_t > triangle;

							for ( index_data_t faceVertexIndex = 0; faceVertexIndex < 3; ++faceVertexIndex )
							{
								const auto realFaceVertexIndex = faceVertexIndex == 0 ? faceVertexIndex : faceVertexIndex + triangleOffset;
								const auto & OBJVertex = faceIndices.at(realFaceVertexIndex);

								/* NOTE: Convert OBJ index (1-based) to vector index (0-based).
								 * Index 0 means "not defined" in our system. */
								const auto vIndex = OBJVertex.vIndex - 1;
								const bool hasTexCoord = OBJVertex.vtIndex > 0;
								const auto vtIndex = hasTexCoord ? OBJVertex.vtIndex - 1 : 0;

								/* NOTE: Get the real shape vertex index in the final shape. */
								index_data_t geometryVertexIndex = 0;

								switch ( m_predominantAttribute )
								{
									case PredominantAttributes::V :
										geometryVertexIndex = vIndex;
										break;

									case PredominantAttributes::VT :
										geometryVertexIndex = hasTexCoord ? vtIndex : vIndex;
										break;

									default:
										return false;
								}

								if ( writtenIndexes.contains(geometryVertexIndex) )
								{
									const auto & vertex = vertices.at(geometryVertexIndex);

									const auto & position = m_v.at(vIndex);

									/* NOTE: If the position is different, we create a new shape vertex. */
									if ( vertex.position() != position )
									{
										const auto texCoord = hasTexCoord ? m_vt.at(vtIndex) : Math::Vector< 3, vertex_data_t >{};

										vertices.emplace_back(position, Math::Vector< 3, vertex_data_t >{}, texCoord);

										geometryVertexIndex = static_cast< index_data_t >(vertices.size() - 1);
									}
								}
								else
								{
									/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
									auto & vertex = vertices.at(geometryVertexIndex);
									vertex.setPosition(m_v.at(vIndex));

									if ( hasTexCoord )
									{
										vertex.setTextureCoordinates(m_vt.at(vtIndex));
									}

									writtenIndexes.emplace(geometryVertexIndex);
								}

								/* Declare the vertex index to one of the three vertices of the triangle. */
								triangle.setVertexIndex(faceVertexIndex, geometryVertexIndex);
							}

							triangles.emplace_back(triangle);

							++triangleCount;

							if ( !groups.empty() )
							{
								++groups.back().second;
							}
						}
					}
				}

				return true;
			}

			/**
			 * @brief Reads a line using OBJ F line format : "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3"
			 * @param file A reference to the OBJ file.
			 * @param groups A reference to an index list.
			 * @param vertices A reference to the vertex list.
			 * @param triangles A reference to the triangle list.
			 * @return bool
			 */
			bool
			parseFaceAssemblyV_VT_VN (std::ifstream & file, std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) noexcept
			{
				/* NOTE: Resize/reserving memory space to the geometry shape. */
				vertices.resize(m_vertexCount);
				triangles.reserve(m_faceCount);

				/* Keep track of generated shape vertex index. */
				std::set< index_data_t > writtenIndexes{};

				/* Keep count on attributes and triangles. */
				index_data_t positionCount = 0;
				index_data_t textureCoordinatesCount = 0;
				index_data_t normalCount = 0;
				index_data_t triangleCount = 0;

				std::string objLine{};

				while ( std::getline(file, objLine) )
				{
					/* Declaring a group. */
					if ( objLine.starts_with("g ") )
					{
						if ( triangleCount > 0 )
						{
							groups.emplace_back(triangleCount, 0);
						}

						continue;
					}

					if ( objLine.starts_with("v ") )
					{
						this->extractPositionAttribute(objLine, positionCount++);
					}
					else if ( objLine.starts_with("vt ") )
					{
						this->extractTextureCoordinatesAttribute(objLine, textureCoordinatesCount++);
					}
					else if ( objLine.starts_with("vn ") )
					{
						this->extractNormalAttribute(objLine, normalCount++);
					}
					else if ( objLine.starts_with("f ") )
					{
						std::vector< OBJVertex< index_data_t > > faceIndices;

						if ( !this->extractFaceIndices(objLine, faceIndices) )
						{
							return false;
						}

						/* NOTE: f line can express a triangle or a polygon ! */
						for ( index_data_t triangleOffset = 0; triangleOffset < faceIndices.size() - 2; ++triangleOffset )
						{
							/* NOTES: "f 0 1 2 3"
							 * Must goes into
							 * "0 1 2"
							 * "0 2 3"
							 */

							ShapeTriangle< vertex_data_t, index_data_t > triangle;

							for ( index_data_t faceVertexIndex = 0; faceVertexIndex < 3; ++faceVertexIndex )
							{
								const auto realFaceVertexIndex = faceVertexIndex == 0 ? faceVertexIndex : faceVertexIndex + triangleOffset;
								const auto & OBJVertex = faceIndices.at(realFaceVertexIndex);

								/* NOTE: Convert OBJ index (1-based) to vector index (0-based).
								 * Index 0 means "not defined" in our system. */
								const auto vIndex = OBJVertex.vIndex - 1;
								const bool hasNormal = OBJVertex.vnIndex > 0;
								const bool hasTexCoord = OBJVertex.vtIndex > 0;
								const auto vnIndex = hasNormal ? OBJVertex.vnIndex - 1 : 0;
								const auto vtIndex = hasTexCoord ? OBJVertex.vtIndex - 1 : 0;

								/* NOTE: Get the real shape vertex index in the final shape. */
								index_data_t geometryVertexIndex = 0;

								switch ( m_predominantAttribute )
								{
									case PredominantAttributes::V :
										geometryVertexIndex = vIndex;
										break;

									case PredominantAttributes::VN :
										geometryVertexIndex = hasNormal ? vnIndex : vIndex;
										break;

									case PredominantAttributes::VT :
										geometryVertexIndex = hasTexCoord ? vtIndex : vIndex;
										break;
								}

								if ( writtenIndexes.contains(geometryVertexIndex) )
								{
									const auto & vertex = vertices.at(geometryVertexIndex);

									const auto & position = m_v.at(vIndex);

									/* NOTE: If the position is different, we create a new shape vertex. */
									if ( vertex.position() != position )
									{
										const auto normal = hasNormal ? m_vn.at(vnIndex) : Math::Vector< 3, vertex_data_t >{};
										const auto texCoord = hasTexCoord ? m_vt.at(vtIndex) : Math::Vector< 3, vertex_data_t >{};

										vertices.emplace_back(position, normal, texCoord);

										geometryVertexIndex = static_cast< uint32_t >(vertices.size() - 1);
									}
								}
								else
								{
									/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
									auto & vertex = vertices.at(geometryVertexIndex);
									vertex.setPosition(m_v.at(vIndex));

									if ( hasNormal )
									{
										vertex.setNormal(m_vn.at(vnIndex));
									}

									if ( hasTexCoord )
									{
										vertex.setTextureCoordinates(m_vt.at(vtIndex));
									}

									writtenIndexes.emplace(geometryVertexIndex);
								}

								/* Declare the vertex index to one of the three vertices of the triangle. */
								triangle.setVertexIndex(faceVertexIndex, geometryVertexIndex);
							}

							triangles.emplace_back(triangle);

							++triangleCount;

							if ( !groups.empty() )
							{
								++groups.back().second;
							}
						}
					}
				}

				return true;
			}

			/**
			 * @brief Parses a floating-point value from a string view.
			 * @note Uses strtof on macOS (libc++ lacks std::from_chars for floats), std::from_chars elsewhere.
			 * @param str The string view to parse.
			 * @param value Output for the parsed value.
			 * @return const char* Pointer past the parsed value, or nullptr on failure.
			 */
			static
			const char *
			parseFloat (std::string_view str, float & value) noexcept
			{
				if constexpr ( IsMacOS )
				{
					char * end = nullptr;
					value = std::strtof(str.data(), &end);
					return end;
				}
				else
				{
					const auto result = std::from_chars(str.data(), str.data() + str.size(), value);
					return (result.ec == std::errc{}) ? result.ptr : nullptr;
				}
			}

			/**
			 * @brief Skips whitespace in a string view.
			 * @param str The string view.
			 * @param pos Current position.
			 * @return size_t New position after whitespace.
			 */
			[[nodiscard]]
			static
			size_t
			skipWhitespace (std::string_view str, size_t pos) noexcept
			{
				while ( pos < str.size() && (str[pos] == ' ' || str[pos] == '\t') )
				{
					++pos;
				}

				return pos;
			}

			/**
			 * @brief Parses a "v" line to extract the position.
			 * @param vLine A reference to a string.
			 * @param offset The vertex offset.
			 * @return void
			 */
			void
			extractPositionAttribute (const std::string & vLine, index_data_t offset) noexcept
			{
				std::string_view line{vLine};

				/* Skip "v " prefix. */
				size_t pos = skipWhitespace(line, 2);

				float x = 0.0F, y = 0.0F, z = 0.0F;

				if ( const auto * end = parseFloat(line.substr(pos), x); end != nullptr )
				{
					pos = skipWhitespace(line, static_cast< size_t >(end - line.data()));

					if ( const auto * end2 = parseFloat(line.substr(pos), y); end2 != nullptr )
					{
						pos = skipWhitespace(line, static_cast< size_t >(end2 - line.data()));

						parseFloat(line.substr(pos), z);
					}
				}

				auto & vector = m_v[offset];
				vector[Math::X] = m_readOptions.flipXAxis ? -x : x;
				vector[Math::Y] = m_readOptions.flipYAxis ? -y : y;
				vector[Math::Z] = m_readOptions.flipZAxis ? -z : z;

				if ( Utility::different(m_readOptions.scaleFactor, 1.0F) )
				{
					vector.scale(m_readOptions.scaleFactor);
				}
			}

			/**
			 * @brief Parses a "vt" line to extract the texture coordinates.
			 * @param vtLine A reference to a string.
			 * @param offset The vertex offset.
			 * @return void
			 */
			void
			extractTextureCoordinatesAttribute (const std::string & vtLine, index_data_t offset) noexcept
			{
				std::string_view line{vtLine};

				/* Skip "vt " prefix. */
				size_t pos = skipWhitespace(line, 3);

				float u = 0.0F, v = 0.0F;

				if ( const auto * end = parseFloat(line.substr(pos), u); end != nullptr )
				{
					pos = skipWhitespace(line, static_cast< size_t >(end - line.data()));

					parseFloat(line.substr(pos), v);
				}

				auto & vector = m_vt[offset];
				vector[Math::X] = m_readOptions.flipXAxis ? -u : u;
				vector[Math::Y] = m_readOptions.flipYAxis ? -v : v;
			}

			/**
			 * @brief Parses a "vn" line to extract the normal.
			 * @param vnLine A reference to a string.
			 * @param offset The vertex offset.
			 * @return void
			 */
			void
			extractNormalAttribute (const std::string & vnLine, index_data_t offset) noexcept
			{
				std::string_view line{vnLine};

				/* Skip "vn " prefix. */
				size_t pos = skipWhitespace(line, 3);

				float x = 0.0F, y = 0.0F, z = 0.0F;

				if ( const auto * end = parseFloat(line.substr(pos), x); end != nullptr )
				{
					pos = skipWhitespace(line, static_cast< size_t >(end - line.data()));

					if ( const auto * end2 = parseFloat(line.substr(pos), y); end2 != nullptr )
					{
						pos = skipWhitespace(line, static_cast< size_t >(end2 - line.data()));

						parseFloat(line.substr(pos), z);
					}
				}

				auto & vector = m_vn[offset];
				vector[Math::X] = m_readOptions.flipXAxis ? -x : x;
				vector[Math::Y] = m_readOptions.flipYAxis ? -y : y;
				vector[Math::Z] = m_readOptions.flipZAxis ? -z : z;
			}

			/**
			 * @brief Parses a single face vertex token (e.g., "1", "1/2", "1//3", "1/2/3").
			 * @param token The token to parse.
			 * @param vIndex Output for position index.
			 * @param vtIndex Output for texture coordinate index (0 if not present).
			 * @param vnIndex Output for normal index (0 if not present).
			 * @return bool True if parsing succeeded.
			 */
			[[nodiscard]]
			static
			bool
			parseFaceToken (std::string_view token, int32_t & vIndex, int32_t & vtIndex, int32_t & vnIndex) noexcept
			{
				vIndex = 0;
				vtIndex = 0;
				vnIndex = 0;

				const auto firstSlash = token.find('/');

				/* Format: "v" (position only). */
				if ( firstSlash == std::string_view::npos )
				{
					const auto result = std::from_chars(token.data(), token.data() + token.size(), vIndex);
					return result.ec == std::errc{};
				}

				/* Parse position index (before first slash). */
				{
					const auto result = std::from_chars(token.data(), token.data() + firstSlash, vIndex);

					if ( result.ec != std::errc{} )
					{
						return false;
					}
				}

				const auto secondSlash = token.find('/', firstSlash + 1);

				/* Format: "v//vn" (position and normal, no texture). */
				if ( firstSlash + 1 < token.size() && token[firstSlash + 1] == '/' )
				{
					const auto vnStart = firstSlash + 2;
					const auto result = std::from_chars(token.data() + vnStart, token.data() + token.size(), vnIndex);
					return result.ec == std::errc{};
				}

				/* Format: "v/vt" (position and texture, no normal). */
				if ( secondSlash == std::string_view::npos )
				{
					const auto vtStart = firstSlash + 1;
					const auto result = std::from_chars(token.data() + vtStart, token.data() + token.size(), vtIndex);
					return result.ec == std::errc{};
				}

				/* Format: "v/vt/vn" (position, texture, and normal). */
				{
					const auto vtStart = firstSlash + 1;
					const auto vtEnd = secondSlash;

					auto result = std::from_chars(token.data() + vtStart, token.data() + vtEnd, vtIndex);

					if ( result.ec != std::errc{} )
					{
						return false;
					}

					const auto vnStart = secondSlash + 1;
					result = std::from_chars(token.data() + vnStart, token.data() + token.size(), vnIndex);

					return result.ec == std::errc{};
				}
			}

			/**
			 * @brief Converts a potentially negative OBJ index to a positive index.
			 * @note OBJ uses 1-based indexing. Negative indices reference from the end.
			 * @param index The index to convert.
			 * @param listSize The size of the corresponding list.
			 * @return index_data_t The converted positive index (still 1-based).
			 */
			[[nodiscard]]
			static
			index_data_t
			resolveIndex (int32_t index, size_t listSize) noexcept
			{
				if ( index < 0 )
				{
					return static_cast< index_data_t >(static_cast< int32_t >(listSize) + index + 1);
				}

				return static_cast< index_data_t >(index);
			}

			/**
			 * @brief Returns a list of usable vertices from F line.
			 * @param fLine Vertices definition from the F line as indexes.
			 * @param faceIndices A reference to a vector holding the face indices to position, normal or texture coordinates.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			extractFaceIndices (const std::string & fLine, std::vector< OBJVertex< index_data_t > > & faceIndices) const noexcept
			{
				std::string_view line{fLine};

				/* Skip "f " prefix. */
				if ( line.size() < 3 || line[0] != 'f' || line[1] != ' ' )
				{
					std::cerr << "FileFormatOBJ::extractFaceIndices(), invalid face line format: '" << fLine << "'" "\n";

					return false;
				}

				line.remove_prefix(2);

				/* Pre-count vertices for reservation (count spaces + 1). */
				const auto vertexCount = std::ranges::count(line, ' ') + 1;
				faceIndices.reserve(static_cast< size_t >(vertexCount));

				/* Parse each vertex token. */
				size_t pos = 0;

				while ( pos < line.size() )
				{
					/* Skip leading whitespace. */
					while ( pos < line.size() && line[pos] == ' ' )
					{
						++pos;
					}

					if ( pos >= line.size() )
					{
						break;
					}

					/* Find end of token. */
					const auto tokenEnd = line.find(' ', pos);
					const auto tokenLength = (tokenEnd == std::string_view::npos) ? (line.size() - pos) : (tokenEnd - pos);
					const auto token = line.substr(pos, tokenLength);

					/* Parse the token. */
					int32_t vIndex = 0;
					int32_t vtIndex = 0;
					int32_t vnIndex = 0;

					if ( !parseFaceToken(token, vIndex, vtIndex, vnIndex) )
					{
						std::cerr << "FileFormatOBJ::extractFaceIndices(), failed to parse face token '" << token << "' in line: '" << fLine << "'" "\n";

						return false;
					}

					/* Resolve negative indices and add to result. */
					faceIndices.emplace_back(
						resolveIndex(vIndex, m_v.size()),
						resolveIndex(vtIndex, m_vt.size()),
						resolveIndex(vnIndex, m_vn.size())
					);

					pos += tokenLength;
				}

				if ( faceIndices.size() < 3 )
				{
					std::cerr << "FileFormatOBJ::extractFaceIndices(), face must have at least 3 vertices, got " << faceIndices.size() << ": '" << fLine << "'" "\n";

					return false;
				}

				return true;
			}

			/**
			 * @brief Counts the number of vertices in an OBJ face line.
			 * @param line The face line (starting with "f ").
			 * @return size_t The number of vertices.
			 */
			[[nodiscard]]
			static
			size_t
			countFaceVertices (std::string_view line) noexcept
			{
				/* Remove trailing whitespace. */
				while ( !line.empty() && std::isspace(static_cast< unsigned char >(line.back())) )
				{
					line.remove_suffix(1);
				}

				/* Count spaces (vertices = spaces, since "f v1 v2 v3" has 3 spaces for 3 vertices). */
				return static_cast< size_t >(std::ranges::count(line, ' '));
			}

			std::vector< Math::Vector< 3, vertex_data_t > > m_v;
			std::vector< Math::Vector< 3, vertex_data_t > > m_vt;
			std::vector< Math::Vector< 3, vertex_data_t > > m_vn;
			index_data_t m_vertexCount{0}; /* Combined attributes vertex count. */
			index_data_t m_faceCount{0};
			FaceMode m_faceMode{FaceMode::Undetermined};
			PredominantAttributes m_predominantAttribute{PredominantAttributes::V};
			ReadOptions m_readOptions{};
	};
}
