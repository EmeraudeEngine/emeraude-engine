/*
 * src/Libs/VertexFactory/FileFormatOBJ.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
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
				: vIndex(vIndex)
			{

			}

			OBJVertex (index_data_t vIndex, index_data_t vtIndex) noexcept
				: vIndex(vIndex),
				vtIndex(vtIndex)
			{

			}

			OBJVertex (index_data_t vIndex, index_data_t vtIndex, index_data_t vnIndex) noexcept
				: vIndex(vIndex),
				vtIndex(vtIndex),
				vnIndex(vnIndex)
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
					std::cerr << __PRETTY_FUNCTION__ << ", unable to read OBJ file '" << filepath << "' !" "\n";

					return false;
				}
				
				/* Read a first time to calculate the space required for the structure. */
				{
					if ( !this->analyseFileContent(file) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", step 1 'Reserving space' has failed !" "\n";

						return false;
					}

					file.clear();
					file.seekg(0);
				}

				/* 3. Read a third and final time to assemble faces. */
				return geometry.build([this, &geometry, &file] (std::vector< std::pair< index_data_t, index_data_t > > & groups, std::vector< ShapeVertex< vertex_data_t > > & vertices, std::vector< ShapeTriangle< vertex_data_t, index_data_t > > & triangles) {
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

				std::ofstream file{filepath};

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to open '" << filepath << "' file to write !" "\n";

					return false;
				}

				/* FIXME: TODO ! */

				file.close();

				return true;
			}

		private:

			enum class LineType : uint8_t
			{
				Unknown,
				Comment,
				VertexValues,
				TexCoordValues,
				NormalValues,
				FaceAssembly,
				ObjectName,
				GroupName,
				SmoothShadingState,
				MaterialFile,
				MaterialState
			};

			/**
			 * @brief How the face are declared in the file.
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
						if ( m_faceCount == 0 )
						{
							m_faceMode = determineFaceDeclarationMode(line);
						}

						const auto spaceCount = getSpaceCount(line);

						if ( spaceCount < 3 || spaceCount > 4 )
						{
							std::cerr << __PRETTY_FUNCTION__ << ", this OBJ loader support only triangle or quad face definition !" << '\n';

							return false;
						}

						m_faceCount += spaceCount - 2;
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
					std::cerr << __PRETTY_FUNCTION__ << ", there is no vertex definition in the OBJ file. Aborting." "\n";

					return false;
				}

				if ( m_faceCount == 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", there is no face definition in the OBJ file. Aborting." "\n";

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

								/* NOTE: Convert OBJ index to vector index. */
								const auto vIndex = OBJVertex.vIndex - 1;
								const auto vnIndex = OBJVertex.vnIndex - 1;

								/* NOTE: Get the real shape vertex index in the final shape. */
								index_data_t geometryVertexIndex = 0;

								switch ( m_predominantAttribute )
								{
									case PredominantAttributes::V :
										geometryVertexIndex = vIndex;
										break;

									case PredominantAttributes::VN :
										geometryVertexIndex = vnIndex;
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
										vertices.emplace_back(position, m_vn.at(vnIndex));

										geometryVertexIndex = static_cast< index_data_t >(vertices.size() - 1);
									}
								}
								else
								{
									/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
									auto & vertex = vertices.at(geometryVertexIndex);
									vertex.setPosition(m_v.at(vIndex));
									vertex.setNormal(m_vn.at(vnIndex));

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

								/* NOTE: Convert OBJ index to vector index. */
								const auto vIndex = OBJVertex.vIndex - 1;
								const auto vtIndex = OBJVertex.vtIndex - 1;

								/* NOTE: Get the real shape vertex index in the final shape. */
								index_data_t geometryVertexIndex = 0;

								switch ( m_predominantAttribute )
								{
									case PredominantAttributes::V :
										geometryVertexIndex = vIndex;
										break;

									case PredominantAttributes::VT :
										geometryVertexIndex = vtIndex;
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
										vertices.emplace_back(position, Math::Vector< 3, float >{}, m_vt.at(vtIndex));

										geometryVertexIndex = static_cast< index_data_t >(vertices.size() - 1);
									}
								}
								else
								{
									/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
									auto & vertex = vertices.at(geometryVertexIndex);
									vertex.setPosition(m_v.at(vIndex));
									vertex.setTextureCoordinates(m_vt.at(vtIndex));

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

								/* NOTE: Convert OBJ index to vector index. */
								const auto vIndex = OBJVertex.vIndex - 1;
								const auto vnIndex = OBJVertex.vnIndex - 1;
								const auto vtIndex = OBJVertex.vtIndex - 1;

								/* NOTE: Get the real shape vertex index in the final shape. */
								index_data_t geometryVertexIndex = 0;

								switch ( m_predominantAttribute )
								{
									case PredominantAttributes::V :
										geometryVertexIndex = vIndex;
										break;

									case PredominantAttributes::VN :
										geometryVertexIndex = vnIndex;
										break;

									case PredominantAttributes::VT :
										geometryVertexIndex = vtIndex;
										break;
								}

								if ( writtenIndexes.contains(geometryVertexIndex) )
								{
									const auto & vertex = vertices.at(geometryVertexIndex);

									const auto & position = m_v.at(vIndex);

									/* NOTE: If the position is different, we create a new shape vertex. */
									if ( vertex.position() != position )
									{
										vertices.emplace_back(position, m_vn.at(vnIndex), m_vt.at(vtIndex));

										geometryVertexIndex = static_cast< uint32_t >(vertices.size() - 1);
									}
								}
								else
								{
									/* NOTE: Copy the OBJ extracts values to the final shape vertex. */
									auto & vertex = vertices.at(geometryVertexIndex);
									vertex.setPosition(m_v.at(vIndex));
									vertex.setNormal(m_vn.at(vnIndex));
									vertex.setTextureCoordinates(m_vt.at(vtIndex));

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
			 * @brief Parses and "v" line to extract the position.
			 * @param vLine A reference to a string.
			 * @param offset The vertex offset.
			 * @return void
			 */
			void
			extractPositionAttribute (const std::string & vLine, index_data_t offset) noexcept
			{
#ifdef USE_SAFE_READ
				Math::Vector< 3, data_t > position{vLine, ' ', 1};

				if ( m_readOptions.flipXAxis )
				{
					position[Math::X] = -position[Math::X];
				}

				if ( m_readOptions.flipYAxis )
				{
					position[Math::Y] = -position[Math::Y];
				}

				if ( m_readOptions.flipZAxis )
				{
					position[Math::Z] = -position[Math::Z];
				}

				if ( Utility::different(m_scaleFactor, 1.0F) )
				{
					position.scale(m_scaleFactor);
				}

				m_v.emplace_back(position);
#else
				float x, y, z;

				sscanf(vLine.c_str(), "v %f %f %f",
					&x,
					&y,
					&z
				);

				auto & vector =  m_v[offset];
				vector[Math::X] = m_readOptions.flipXAxis ? -x : x;
				vector[Math::Y] = m_readOptions.flipYAxis ? -y : y;
				vector[Math::Z] = m_readOptions.flipZAxis ? -z : z;

				if ( Utility::different(m_readOptions.scaleFactor, 1.0F) )
				{
					vector.scale(m_readOptions.scaleFactor);
				}
#endif
			}

			/**
			 * @brief Parses and "vt" line to extract the texture coordinates.
			 * @param vtLine A reference to a string.
			 * @param offset The vertex offset.
			 * @return void
			 */
			void
			extractTextureCoordinatesAttribute (const std::string & vtLine, index_data_t offset) noexcept
			{
#ifdef USE_SAFE_READ
				Math::Vector< 2, data_t > textureCoordinates{vtLine, ' ', 1};

				if ( m_readOptions.flipXAxis )
				{
					textureCoordinates[Math::X] = -textureCoordinates[Math::X];
				}

				if ( m_readOptions.flipYAxis )
				{
					textureCoordinates[Math::Y] = -textureCoordinates[Math::Y];
				}

				m_vt.emplace_back(textureCoordinates);
#else
				float u, v;

				sscanf(vtLine.c_str(), "vt %f %f",
					&u,
					&v
				);

				auto & vector =  m_vt[offset];
				vector[Math::X] = m_readOptions.flipXAxis ? -u : u;
				vector[Math::Y] = m_readOptions.flipYAxis ? -v : v;
#endif
			}

			/**
			 * @brief Parses and "vn" line to extract the normal.
			 * @param vnLine A reference to a string.
			 * @param offset The vertex offset.
			 * @return void
			 */
			void
			extractNormalAttribute (const std::string & vnLine, index_data_t offset) noexcept
			{
#ifdef USE_SAFE_READ
				Math::Vector< 3, data_t > normal{vnLine, ' ', 1};

				if ( m_readOptions.flipXAxis )
				{
					normal[Math::X] = -normal[Math::X];
				}

				if ( m_readOptions.flipYAxis )
				{
					normal[Math::Y] = -normal[Math::Y];
				}

				if ( m_readOptions.flipZAxis )
				{
					normal[Math::Z] = -normal[Math::Z];
				}

				m_vn.emplace_back(normal);
#else
				float x, y, z;

				sscanf(vnLine.c_str(), "vn %f %f %f",
					&x,
					&y,
					&z
				);

				auto & vector =  m_vn[offset];
				vector[Math::X] = m_readOptions.flipXAxis ? -x : x;
				vector[Math::Y] = m_readOptions.flipYAxis ? -y : y;
				vector[Math::Z] = m_readOptions.flipZAxis ? -z : z;
#endif
			}

			/**
			 * @brief Returns a list of usable vertices from F line.
			 * @param fLine Vertices definition from the F line as indexes.
			 * @param faceIndices A reference to a vector holding the face indices to position, normal or texture coordinates.
			 * @return bool
			 */
			bool
			extractFaceIndices (const std::string & fLine, std::vector< OBJVertex< index_data_t > > & faceIndices) const noexcept
			{
#ifdef USE_SAFE_READ
				/* Split the face definition into points. */
				const auto points = String::explode(fLine, ' ', false);

				/* NOTE: Including the "f" part ! */
				if ( points.size() < 4 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", an OBJ face must define at least 3 vertices !" "\n" "Faulty line: " << fLine << '\n';

					return false;
				}

				/* NOTE: Reserving the final data excluding the "f" part. */
				faceIndices.reserve(points.size() - 1);

				for ( index_data_t index = 1; index < points.size(); ++index )
				{
					uint32_t vIndex = 0;
					uint32_t vtIndex = 0;
					uint32_t vnIndex = 0;

					switch ( m_faceMode )
					{
						case FaceMode::V :
							vIndex = String::toNumber< uint32_t >(points[index]);
							break;

						case FaceMode::V_VN :
						{
							const auto subCmp = String::explode(String::trim(points[index]), '/', false);

							vIndex = String::toNumber< uint32_t >(subCmp[0]);
							vnIndex = String::toNumber< uint32_t >(subCmp[1]);
						}
							break;

						case FaceMode::V_VT :
						{
							const auto subCmp = String::explode(String::trim(points[index]), '/', false);

							vIndex = String::toNumber< uint32_t >(subCmp[0]);
							vtIndex = String::toNumber< uint32_t >(subCmp[1]);
						}
							break;

						case FaceMode::V_VT_VN :
						{
							const auto subCmp = String::explode(String::trim(points[index]), '/', false);

							vIndex = String::toNumber< uint32_t >(subCmp[0]);
							vtIndex = String::toNumber< uint32_t >(subCmp[1]);
							vnIndex = String::toNumber< uint32_t >(subCmp[2]);
						}
							break;

						default:
							std::cerr << __PRETTY_FUNCTION__ << ", invalid face description mode !" "\n";

							return false;
					}

					/* Checks for outbound with vertices list. */
					if ( vIndex - 1 >= m_v.size() )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", vertex #" << vIndex << " doesn't exist in OBJ file." "\n";

						return false;
					}

					/* Checks for outbound with texture coords list. */
					if ( vtIndex > 0 && vtIndex - 1 >= m_vt.size() )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", texture coordinates #" << vtIndex << " doesn't exist in OBJ file." "\n";

						return false;
					}

					/* Checks for outbound with normals list. */
					if ( vnIndex > 0 && vnIndex - 1 >= m_vn.size() )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", normal #" << vnIndex << " doesn't exist in OBJ file." "\n";

						return false;
					}

					faceIndices.emplace_back(vIndex, vtIndex, vnIndex);
				}

				return true;
#else

				const index_data_t pointCount = getSpaceCount(fLine);

				if ( pointCount == 3 )
				{
					faceIndices.reserve(3);

					switch ( m_faceMode )
					{
						case FaceMode::V :
						{
							uint32_t vIndexA, vIndexB, vIndexC;

							sscanf(fLine.c_str(), "f %u %u %u",
								&vIndexA,
								&vIndexB,
								&vIndexC
							);

							faceIndices.emplace_back(vIndexA);
							faceIndices.emplace_back(vIndexB);
							faceIndices.emplace_back(vIndexC);
						}
							break;

						case FaceMode::V_VN :
						{
							uint32_t vIndexA, vIndexB, vIndexC;
							uint32_t vnIndexA, vnIndexB, vnIndexC;

							sscanf(fLine.c_str(), "f %u//%u %u//%u %u//%u",
								&vIndexA, &vnIndexA,
								&vIndexB, &vnIndexB,
								&vIndexC, &vnIndexC
							);

							faceIndices.emplace_back(vIndexA, 0, vnIndexA);
							faceIndices.emplace_back(vIndexB, 0, vnIndexB);
							faceIndices.emplace_back(vIndexC, 0, vnIndexC);
						}
							break;

						case FaceMode::V_VT :
						{
							uint32_t vIndexA, vIndexB, vIndexC;
							uint32_t vtIndexA, vtIndexB, vtIndexC;

							sscanf(fLine.c_str(), "f %u/%u %u/%u %u/%u",
								&vIndexA, &vtIndexA,
								&vIndexB, &vtIndexB,
								&vIndexC, &vtIndexC
							);

							faceIndices.emplace_back(vIndexA, vtIndexA);
							faceIndices.emplace_back(vIndexB, vtIndexB);
							faceIndices.emplace_back(vIndexC, vtIndexC);
						}
							break;

						case FaceMode::V_VT_VN :
						{
							uint32_t vIndexA, vIndexB, vIndexC;
							uint32_t vtIndexA, vtIndexB, vtIndexC;
							uint32_t vnIndexA, vnIndexB, vnIndexC;

							sscanf(fLine.c_str(), "f %u/%u/%u %u/%u/%u %u/%u/%u",
								&vIndexA, &vtIndexA, &vnIndexA,
								&vIndexB, &vtIndexB, &vnIndexB,
								&vIndexC, &vtIndexC, &vnIndexC
							);

							faceIndices.emplace_back(vIndexA, vtIndexA, vnIndexA);
							faceIndices.emplace_back(vIndexB, vtIndexB, vnIndexB);
							faceIndices.emplace_back(vIndexC, vtIndexC, vnIndexC);
						}
							break;

						default:
							std::cerr << __PRETTY_FUNCTION__ << ", invalid face description mode !" "\n";

							return false;
					}

					return true;
				}

				if ( pointCount == 4 )
				{
					faceIndices.reserve(4);

					switch ( m_faceMode )
					{
						case FaceMode::V :
						{
							uint32_t vIndexA, vIndexB, vIndexC, vIndexD;

							sscanf(fLine.c_str(), "f %u %u %u %u",
								&vIndexA,
								&vIndexB,
								&vIndexC,
								&vIndexD
							);

							faceIndices.emplace_back(vIndexA);
							faceIndices.emplace_back(vIndexB);
							faceIndices.emplace_back(vIndexC);
							faceIndices.emplace_back(vIndexD);
						}
							break;

						case FaceMode::V_VN :
						{
							uint32_t vIndexA, vIndexB, vIndexC, vIndexD;
							uint32_t vnIndexA, vnIndexB, vnIndexC, vnIndexD;

							sscanf(fLine.c_str(), "f %u//%u %u//%u %u//%u %u//%u",
								&vIndexA, &vnIndexA,
								&vIndexB, &vnIndexB,
								&vIndexC, &vnIndexC,
								&vIndexD, &vnIndexD
							);

							faceIndices.emplace_back(vIndexA, 0, vnIndexA);
							faceIndices.emplace_back(vIndexB, 0, vIndexB);
							faceIndices.emplace_back(vIndexC, 0, vIndexC);
							faceIndices.emplace_back(vIndexD, 0, vIndexD);
						}
							break;

						case FaceMode::V_VT :
						{
							uint32_t vIndexA, vIndexB, vIndexC, vIndexD;
							uint32_t vtIndexA, vtIndexB, vtIndexC, vtIndexD;

							sscanf(fLine.c_str(), "f %u/%u %u/%u %u/%u %u/%u",
								&vIndexA, &vtIndexA,
								&vIndexB, &vtIndexB,
								&vIndexC, &vtIndexC,
								&vIndexD, &vtIndexD
							);

							faceIndices.emplace_back(vIndexA, vtIndexA);
							faceIndices.emplace_back(vIndexB, vtIndexB);
							faceIndices.emplace_back(vIndexC, vtIndexC);
							faceIndices.emplace_back(vIndexD, vtIndexD);
						}
							break;

						case FaceMode::V_VT_VN :
						{
							uint32_t vIndexA, vIndexB, vIndexC, vIndexD;
							uint32_t vtIndexA, vtIndexB, vtIndexC, vtIndexD;
							uint32_t vnIndexA, vnIndexB, vnIndexC, vnIndexD;

							sscanf(fLine.c_str(), "f %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
								&vIndexA, &vtIndexA, &vnIndexA,
								&vIndexB, &vtIndexB, &vnIndexB,
								&vIndexC, &vtIndexC, &vnIndexC,
								&vIndexD, &vtIndexD, &vnIndexD
							);

							faceIndices.emplace_back(vIndexA, vtIndexA, vnIndexA);
							faceIndices.emplace_back(vIndexB, vtIndexB, vnIndexB);
							faceIndices.emplace_back(vIndexC, vtIndexC, vnIndexC);
							faceIndices.emplace_back(vIndexD, vtIndexD, vnIndexD);
						}
							break;

						default:
							std::cerr << __PRETTY_FUNCTION__ << ", invalid face description mode !" "\n";

							return false;
					}

					return true;
				}

				std::cerr << __PRETTY_FUNCTION__ << ", this OBJ loader support only triangle and quad !" "\n";

				return false;

#endif
			}

			/**
			 * @brief Returns the number of usable space in the OBJ line.
			 * @param line A reference to a string.
			 * @return index_data_t
			 */
			[[nodiscard]]
			static
			index_data_t
			getSpaceCount (const std::string & line) noexcept
			{
				const auto lastChar = line.find_last_not_of(" \t\f\v\n\r");

				index_data_t count = 0;

				for ( index_data_t index = 0; index < lastChar; ++index )
				{
					if ( line.at(index) == ' ' )
					{
						++count;
					}
				}

				return count;
			}

#if __cplusplus < 202002L /* C++20 feature */
			/**
			 * @brief Determines the type of the OBJ file line.
			 * @param line The current parsed line.
			 * @return LineType
			 */
			[[nodiscard]]
			static
			LineType
			getLineType (const std::string & line) noexcept
			{
				/* # this is a comment */
				if ( String::startsWith(line, "#") )
				{
					return LineType::Comment;
				}

				/* mtllib [external .mtl file name] */
				if ( String::startsWith(line, "mtllib") )
				{
					return LineType::MaterialFile;
				}

				/* usemtl [material name] */
				if ( String::startsWith(line, "usemtl") )
				{
					return LineType::MaterialState;
				}

				/* vt 0.500 -1.352 [0.234] */
				if ( String::startsWith(line, "vt ") )
				{
					return LineType::TexCoordValues;
				}

				/* vn 0.707 0.000 0.707 */
				if ( String::startsWith(line, "vn ") )
				{
					return LineType::NormalValues;
				}

				/* v 0.123 0.234 0.345 1.0 */
				if ( String::startsWith(line, "v ") )
				{
					return LineType::VertexValues;
				}

				/*
				f v1 v2 v3 v4
				f v1/vt1 v2/vt2 v3/vt3
				f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
				f v1//vn1 v2//vn2 v3//vn3
				*/
				if ( String::startsWith(line, "f ") )
				{
					return LineType::FaceAssembly;
				}

				/* o [object name] */
				if ( String::startsWith(line, "o ") )
				{
					return LineType::ObjectName;
				}

				/* g [group name] */
				if ( String::startsWith(line, "g ") )
				{
					return LineType::GroupName;
				}

				/* s 1 OR s off */
				if ( String::startsWith(line, "s ") )
				{
					return LineType::SmoothShadingState;
				}

				/* ??? undetermined line */
				return LineType::Unknown;
			}
#else
		/**
		 * @brief Determines the type of the OBJ file line.
		 * @param line The current parsed line.
		 * @return LineType
		 */
		[[nodiscard]]
		static
		LineType
		getLineType (const std::string & line) noexcept
			{
				/* # this is a comment */
				if ( line.starts_with("#") )
				{
					return LineType::Comment;
				}

				/* mtllib [external .mtl file name] */
				if ( line.starts_with("mtllib") )
				{
					return LineType::MaterialFile;
				}

				/* usemtl [material name] */
				if ( line.starts_with("usemtl") )
				{
					return LineType::MaterialState;
				}

				/* vt 0.500 -1.352 [0.234] */
				if ( line.starts_with("vt ") )
				{
					return LineType::TexCoordValues;
				}

				/* vn 0.707 0.000 0.707 */
				if ( line.starts_with("vn ") )
				{
					return LineType::NormalValues;
				}

				/* v 0.123 0.234 0.345 1.0 */
				if ( line.starts_with("v ") )
				{
					return LineType::VertexValues;
				}

				/*
				f v1 v2 v3 v4
				f v1/vt1 v2/vt2 v3/vt3
				f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
				f v1//vn1 v2//vn2 v3//vn3
				*/
				if ( line.starts_with("f ") )
				{
					return LineType::FaceAssembly;
				}

				/* o [object name] */
				if ( line.starts_with("o ") )
				{
					return LineType::ObjectName;
				}

				/* g [group name] */
				if ( line.starts_with("g ") )
				{
					return LineType::GroupName;
				}

				/* s 1 OR s off */
				if ( line.starts_with("s ") )
				{
					return LineType::SmoothShadingState;
				}

				/* ??? undetermined line */
				return LineType::Unknown;
			}
#endif
			/* Flag names */
			static constexpr auto XAxisFlipEnabled{0UL};
			static constexpr auto YAxisFlipEnabled{1UL};
			static constexpr auto ZAxisFlipEnabled{2UL};

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
