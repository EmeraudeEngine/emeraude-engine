/*
 * src/Libs/VertexFactory/FileFormatMD2.hpp
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
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/VertexFactory/ShapeBuilder.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief 3D format from Quake 2 engine.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @extends EmEn::Libs::VertexFactory::FileFormatInterface
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatMD2 final : public FileFormatInterface< vertex_data_t, index_data_t >
	{
		public:

			/**
			 * @brief Constructs a MD2 file format.
			 */
			FileFormatMD2 () noexcept = default;

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & /*readOptions*/) noexcept override
			{
				std::ifstream file(filepath, std::ios::in | std::ios::binary);

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to open file '" << filepath << "' for reading !" "\n";

					return false;
				}

				file.read(reinterpret_cast< char * >(&m_header), sizeof(md2_header_t));

				if ( (m_header.version != Version) || m_header.ident != MagicNumber )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", file '" << filepath << "' is not a MD2 format !" "\n";

					return false;
				}

				/* memory allocation */
				m_skins.resize(m_header.num_skins);
				m_textureCoordinates.resize(m_header.num_st);
				m_triangles.resize(m_header.num_tris);
				m_frames.resize(m_header.num_frames);
				m_GLCommands.resize(m_header.num_glcmds);

				file.seekg(m_header.offset_skins, std::ios::beg);
				file.read(reinterpret_cast< char * >(m_skins.data()), sizeof(char) * 68 * m_header.num_skins);

				file.seekg(m_header.offset_st, std::ios::beg);
				file.read(reinterpret_cast< char * >(m_textureCoordinates.data()), sizeof(md2_texCoord_t) * m_header.num_st );

				file.seekg(m_header.offset_tris, std::ios::beg);
				file.read(reinterpret_cast< char * >(m_triangles.data()), sizeof(md2_triangle_t) * m_header.num_tris);

				file.seekg(m_header.offset_frames, std::ios::beg);

				/* Read frames */
				for ( auto & frame : m_frames )
				{
					frame.verts.resize(m_header.num_vertices);

					file.read(reinterpret_cast< char * >(&frame.scale), sizeof(md2_vec3_t));
					file.read(reinterpret_cast< char * >(&frame.translate), sizeof(md2_vec3_t));
					file.read(reinterpret_cast< char * >(&frame.name), sizeof(char) * 16);
					file.read(reinterpret_cast< char * >(frame.verts.data()), sizeof(md2_vertex_t) * m_header.num_vertices);
				}

				file.seekg(m_header.offset_glcmds, std::ios::beg);
				file.read(reinterpret_cast< char * >(m_GLCommands.data()), sizeof(int) * m_header.num_glcmds);

				file.close();

				return this->convertToGeometry(geometry);
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

				std::cerr << __PRETTY_FUNCTION__ << ", not yet implemented ! File " << filepath << " is not created." "\n";

				return false;
			}

		private:

			using md2_vec3_t = std::array< float, 3 >;

			struct md2_header_t
			{
				int ident = 0;		  // numéro magique : "IDP2"
				int version = 0;		// version du format : 8

				int skinwidth = 0;	  // largeur texture
				int skinheight = 0;	 // hauteur texture

				int framesize = 0;	  // taille d'une frame en octets

				int num_skins = 0;	  // nombre de skins
				int num_vertices = 0;   // nombre de vertices par frame
				int num_st = 0;		 // nombre de coordonnées de texture
				int num_tris = 0;	   // nombre de triangles
				int num_glcmds = 0;	 // nombre de commandes opengl
				int num_frames = 0;	 // nombre de frames

				int offset_skins = 0;   // offset données skins
				int offset_st = 0;	  // offset données coordonnées de texture
				int offset_tris = 0;	// offset données triangles
				int offset_frames = 0;  // offset données frames
				int offset_glcmds = 0;  // offset données commandes OpenGL
				int offset_end = 0;	 // offset fin de fichier
			};

			struct md2_vertex_t
			{
				std::array< unsigned char, 3 > v{0, 0, 0};
				unsigned char normalIndex = 0;
			};

			struct md2_triangle_t
			{
				std::array< unsigned short, 3 > vertex{0, 0, 0};
				std::array< unsigned short, 3 > st{0, 0, 0};
			};

			struct md2_texCoord_t
			{
				short s = 0;
				short t = 0;
			};

			struct md2_frame_t
			{
				md2_vec3_t scale{1, 1, 1};
				md2_vec3_t translate{0, 0, 0};
				std::array< char, 16 > name{0};
				std::vector< md2_vertex_t > verts{};
			};

			struct md2_skin_t
			{
				std::array< char, 68 > name{0};
			};

			/* Magic number "IDP2" ou 844121161 */
			static constexpr auto MagicNumber = (('2' << 24) + ('P' << 16) + ('D' << 8) + 'I');
			static constexpr auto Version = 8;
			static constexpr std::array< md2_vec3_t, 162 > s_anorms{{
				{-0.525731F, 0.000000F, 0.850651F},
				{-0.442863F, 0.238856F, 0.864188F},
				{-0.295242F, 0.000000F, 0.955423F},
				{-0.309017F, 0.500000F, 0.809017F},
				{-0.162460F, 0.262866F, 0.951056F},
				{0.000000F, 0.000000F, 1.000000F},
				{0.000000F, 0.850651F, 0.525731F},
				{-0.147621F, 0.716567F, 0.681718F},
				{0.147621F, 0.716567F, 0.681718F},
				{0.000000F, 0.525731F, 0.850651F},
				{0.309017F, 0.500000F, 0.809017F},
				{0.525731F, 0.000000F, 0.850651F},
				{0.295242F, 0.000000F, 0.955423F},
				{0.442863F, 0.238856F, 0.864188F},
				{0.162460F, 0.262866F, 0.951056F},
				{-0.681718F, 0.147621F, 0.716567F},
				{-0.809017F, 0.309017F, 0.500000F},
				{-0.587785F, 0.425325F, 0.688191F},
				{-0.850651F, 0.525731F, 0.000000F},
				{-0.864188F, 0.442863F, 0.238856F},
				{-0.716567F, 0.681718F, 0.147621F},
				{-0.688191F, 0.587785F, 0.425325F},
				{-0.500000F, 0.809017F, 0.309017F},
				{-0.238856F, 0.864188F, 0.442863F},
				{-0.425325F, 0.688191F, 0.587785F},
				{-0.716567F, 0.681718F, -0.147621F},
				{-0.500000F, 0.809017F, -0.309017F},
				{-0.525731F, 0.850651F, 0.000000F},
				{0.000000F, 0.850651F, -0.525731F},
				{-0.238856F, 0.864188F, -0.442863F},
				{0.000000F, 0.955423F, -0.295242F},
				{-0.262866F, 0.951056F, -0.162460F},
				{0.000000F, 1.000000F, 0.000000F},
				{0.000000F, 0.955423F, 0.295242F},
				{-0.262866F, 0.951056F, 0.162460F},
				{0.238856F, 0.864188F, 0.442863F},
				{0.262866F, 0.951056F, 0.162460F},
				{0.500000F, 0.809017F, 0.309017F},
				{0.238856F, 0.864188F, -0.442863F},
				{0.262866F, 0.951056F, -0.162460F},
				{0.500000F, 0.809017F, -0.309017F},
				{0.850651F, 0.525731F, 0.000000F},
				{0.716567F, 0.681718F, 0.147621F},
				{0.716567F, 0.681718F, -0.147621F},
				{0.525731F, 0.850651F, 0.000000F},
				{0.425325F, 0.688191F, 0.587785F},
				{0.864188F, 0.442863F, 0.238856F},
				{0.688191F, 0.587785F, 0.425325F},
				{0.809017F, 0.309017F, 0.500000F},
				{0.681718F, 0.147621F, 0.716567F},
				{0.587785F, 0.425325F, 0.688191F},
				{0.955423F, 0.295242F, 0.000000F},
				{1.000000F, 0.000000F, 0.000000F},
				{0.951056F, 0.162460F, 0.262866F},
				{0.850651F, -0.525731F, 0.000000F},
				{0.955423F, -0.295242F, 0.000000F},
				{0.864188F, -0.442863F, 0.238856F},
				{0.951056F, -0.162460F, 0.262866F},
				{0.809017F, -0.309017F, 0.500000F},
				{0.681718F, -0.147621F, 0.716567F},
				{0.850651F, 0.000000F, 0.525731F},
				{0.864188F, 0.442863F, -0.238856F},
				{0.809017F, 0.309017F, -0.500000F},
				{0.951056F, 0.162460F, -0.262866F},
				{0.525731F, 0.000000F, -0.850651F},
				{0.681718F, 0.147621F, -0.716567F},
				{0.681718F, -0.147621F, -0.716567F},
				{0.850651F, 0.000000F, -0.525731F},
				{0.809017F, -0.309017F, -0.500000F},
				{0.864188F, -0.442863F, -0.238856F},
				{0.951056F, -0.162460F, -0.262866F},
				{0.147621F, 0.716567F, -0.681718F},
				{0.309017F, 0.500000F, -0.809017F},
				{0.425325F, 0.688191F, -0.587785F},
				{0.442863F, 0.238856F, -0.864188F},
				{0.587785F, 0.425325F, -0.688191F},
				{0.688191F, 0.587785F, -0.425325F},
				{-0.147621F, 0.716567F, -0.681718F},
				{-0.309017F, 0.500000F, -0.809017F},
				{0.000000F, 0.525731F, -0.850651F},
				{-0.525731F, 0.000000F, -0.850651F},
				{-0.442863F, 0.238856F, -0.864188F},
				{-0.295242F, 0.000000F, -0.955423F},
				{-0.162460F, 0.262866F, -0.951056F},
				{0.000000F, 0.000000F, -1.000000F},
				{0.295242F, 0.000000F, -0.955423F},
				{0.162460F, 0.262866F, -0.951056F},
				{-0.442863F, -0.238856F, -0.864188F},
				{-0.309017F, -0.500000F, -0.809017F},
				{-0.162460F, -0.262866F, -0.951056F},
				{0.000000F, -0.850651F, -0.525731F},
				{-0.147621F, -0.716567F, -0.681718F},
				{0.147621F, -0.716567F, -0.681718F},
				{0.000000F, -0.525731F, -0.850651F},
				{0.309017F, -0.500000F, -0.809017F},
				{0.442863F, -0.238856F, -0.864188F},
				{0.162460F, -0.262866F, -0.951056F},
				{0.238856F, -0.864188F, -0.442863F},
				{0.500000F, -0.809017F, -0.309017F},
				{0.425325F, -0.688191F, -0.587785F},
				{0.716567F, -0.681718F, -0.147621F},
				{0.688191F, -0.587785F, -0.425325F},
				{0.587785F, -0.425325F, -0.688191F},
				{0.000000F, -0.955423F, -0.295242F},
				{0.000000F, -1.000000F, 0.000000F},
				{0.262866F, -0.951056F, -0.162460F},
				{0.000000F, -0.850651F, 0.525731F},
				{0.000000F, -0.955423F, 0.295242F},
				{0.238856F, -0.864188F, 0.442863F},
				{0.262866F, -0.951056F, 0.162460F},
				{0.500000F, -0.809017F, 0.309017F},
				{0.716567F, -0.681718F, 0.147621F},
				{0.525731F, -0.850651F, 0.000000F},
				{-0.238856F, -0.864188F, -0.442863F},
				{-0.500000F, -0.809017F, -0.309017F},
				{-0.262866F, -0.951056F, -0.162460F},
				{-0.850651F, -0.525731F, 0.000000F},
				{-0.716567F, -0.681718F, -0.147621F},
				{-0.716567F, -0.681718F, 0.147621F},
				{-0.525731F, -0.850651F, 0.000000F},
				{-0.500000F, -0.809017F, 0.309017F},
				{-0.238856F, -0.864188F, 0.442863F},
				{-0.262866F, -0.951056F, 0.162460F},
				{-0.864188F, -0.442863F, 0.238856F},
				{-0.809017F, -0.309017F, 0.500000F},
				{-0.688191F, -0.587785F, 0.425325F},
				{-0.681718F, -0.147621F, 0.716567F},
				{-0.442863F, -0.238856F, 0.864188F},
				{-0.587785F, -0.425325F, 0.688191F},
				{-0.309017F, -0.500000F, 0.809017F},
				{-0.147621F, -0.716567F, 0.681718F},
				{-0.425325F, -0.688191F, 0.587785F},
				{-0.162460F, -0.262866F, 0.951056F},
				{0.442863F, -0.238856F, 0.864188F},
				{0.162460F, -0.262866F, 0.951056F},
				{0.309017F, -0.500000F, 0.809017F},
				{0.147621F, -0.716567F, 0.681718F},
				{0.000000F, -0.525731F, 0.850651F},
				{0.425325F, -0.688191F, 0.587785F},
				{0.587785F, -0.425325F, 0.688191F},
				{0.688191F, -0.587785F, 0.425325F},
				{-0.955423F, 0.295242F, 0.000000F},
				{-0.951056F, 0.162460F, 0.262866F},
				{-1.000000F, 0.000000F, 0.000000F},
				{-0.850651F, 0.000000F, 0.525731F},
				{-0.955423F, -0.295242F, 0.000000F},
				{-0.951056F, -0.162460F, 0.262866F},
				{-0.864188F, 0.442863F, -0.238856F},
				{-0.951056F, 0.162460F, -0.262866F},
				{-0.809017F, 0.309017F, -0.500000F},
				{-0.864188F, -0.442863F, -0.238856F},
				{-0.951056F, -0.162460F, -0.262866F},
				{-0.809017F, -0.309017F, -0.500000F},
				{-0.681718F, 0.147621F, -0.716567F},
				{-0.681718F, -0.147621F, -0.716567F},
				{-0.850651F, 0.000000F, -0.525731F},
				{-0.688191F, 0.587785F, -0.425325F},
				{-0.587785F, 0.425325F, -0.688191F},
				{-0.425325F, 0.688191F, -0.587785F},
				{-0.425325F, -0.688191F, -0.587785F},
				{-0.587785F, -0.425325F, -0.688191F},
				{-0.688191F, -0.587785F, -0.425325F}
			}};

			/* Flag names */
			static constexpr auto UseMD2Normal = 0UL;

			/**
			 * Convert the internal format to the native one.
			 *
			 * @param geometry A reference to the native geometry.
			 * @param frameIndex Frame inside the MD2 format.
			 * @return bool
			 */
			bool
			convertToGeometry (Shape< vertex_data_t, index_data_t > & geometry, size_t frameIndex = 0UL) const noexcept
			{
				if ( m_header.num_tris <= 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", there is no data !" "\n";

					return false;
				}

				if ( frameIndex >= static_cast< size_t >(m_header.num_frames) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", frame overflow !" "\n";

					return false;
				}

				const auto triangleCount = static_cast< vertex_data_t >(m_header.num_tris);
				const auto skinWidth = static_cast< vertex_data_t >(m_header.skinwidth);
				const auto skinHeight = static_cast< vertex_data_t >(m_header.skinheight);

				geometry.reserveData(triangleCount * 3, triangleCount * 3, 0U, triangleCount);

				ShapeBuilderOptions< vertex_data_t > options{};
				options.enableGlobalVertexColor(PixelFactory::White);

				ShapeBuilder builder{geometry, options};
				builder.beginConstruction(ConstructionMode::Triangles);

				/* NOTE: Quake 2 coordinates system do not use the OpenGL (RH) defaults.
				 * +X = FRONT = OpenGL Z-
				 * +Y = LEFT = OpenGL X+
				 * +Z = UP = OpenGL Y+
				 * So the triangle and normal a rearranged at insertion in the native format. */

				if ( m_flags[UseMD2Normal] )
				{
					builder.options().enableDataEconomy(false);

					for ( size_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++ )
					{
						for ( size_t vertexIndex = 0; vertexIndex < 3; vertexIndex++ )
						{
							const auto & frame = m_frames[frameIndex];

							const auto & vertex = frame.verts[m_triangles[triangleIndex].vertex[vertexIndex]];
							const auto & normal = s_anorms[vertex.normalIndex];
							const auto & textureCoordinate = m_textureCoordinates[m_triangles[triangleIndex].st[vertexIndex]];

							builder.setPosition(
								frame.scale[0] * static_cast< vertex_data_t >(vertex.v[0]) + frame.translate[0],
								frame.scale[2] * static_cast< vertex_data_t >(vertex.v[2]) + frame.translate[2],
								frame.scale[1] * static_cast< vertex_data_t >(vertex.v[1]) + frame.translate[1]
							);
							builder.setNormal(
								static_cast< vertex_data_t >(normal[0]),
								static_cast< vertex_data_t >(normal[2]),
								static_cast< vertex_data_t >(normal[1])
							);
							builder.setTextureCoordinates(
								static_cast< vertex_data_t >(textureCoordinate.s) / skinWidth,
								skinHeight - (static_cast< vertex_data_t >(textureCoordinate.t) / skinHeight)
							);
							builder.newVertex();
						}
					}
				}
				else
				{
					for ( size_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++ )
					{
						for ( size_t vertexIndex = 0; vertexIndex < 3; vertexIndex++ )
						{
							const auto & frame = m_frames[frameIndex];
							const auto & vertex = frame.verts[m_triangles[triangleIndex].vertex[vertexIndex]];
							const auto & textureCoordinate = m_textureCoordinates[m_triangles[triangleIndex].st[vertexIndex]];

							builder.setPosition(
								frame.scale[0] * static_cast< vertex_data_t >(vertex.v[0]) + frame.translate[0],
								frame.scale[2] * static_cast< vertex_data_t >(vertex.v[2]) + frame.translate[2],
								frame.scale[1] * static_cast< vertex_data_t >(vertex.v[1]) + frame.translate[1]
							);
							builder.setTextureCoordinates(
								static_cast< vertex_data_t >(textureCoordinate.s) / skinWidth,
								skinHeight - (static_cast< vertex_data_t >(textureCoordinate.t) / skinHeight)
							);
							builder.newVertex();
						}
					}
				}

				geometry.transform(Math::Matrix< 4, vertex_data_t >::rotation(Math::Radian< vertex_data_t >(90), 0, 1, 0));

				builder.endConstruction();

				return true;
			}

			md2_header_t m_header{};
			std::vector< md2_skin_t > m_skins;
			std::vector< md2_texCoord_t > m_textureCoordinates;
			std::vector< md2_triangle_t > m_triangles;
			std::vector< md2_frame_t > m_frames;
			std::vector< int > m_GLCommands{};
			std::array< bool, 4 > m_flags{
				true/*UseMD2Normal*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
			};
	};
}
