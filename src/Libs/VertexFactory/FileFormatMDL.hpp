/*
 * src/Libs/VertexFactory/FileFormatMDL.hpp
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
	 * @brief 3D format from Quake engine.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @extends EmEn::Libs::VertexFactory::FileFormatInterface
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatMDL final : public FileFormatInterface< vertex_data_t, index_data_t >
	{
		public:

			/**
			 * @brief Constructs an MDL file format.
			 */
			FileFormatMDL () noexcept = default;

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

				file.read(reinterpret_cast< char * >(&m_header), sizeof(mdl_header_t));

				if ( (m_header.version != Version) || m_header.ident != MagicNumber )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", file '" << filepath << "' is not a MDL format !" "\n";

					return false;
				}

				/* memory allocation */
				m_skins.resize(m_header.num_skins);
				m_textureCoordinates.resize(m_header.num_verts);
				m_triangles.resize(m_header.num_tris);
				m_frames.resize(m_header.num_frames);
				m_texID.resize(m_header.num_skins);

				/* read texture data */
				for ( auto & skin : m_skins )
				{
					skin.data.resize(m_header.skinwidth * m_header.skinheight);

					file.read(reinterpret_cast< char * >(&skin.group), sizeof(int));
					file.read(reinterpret_cast< char * >(skin.data.data()), sizeof(unsigned char) * m_header.skinwidth * m_header.skinheight);
				}

				file.read(reinterpret_cast< char * >(m_textureCoordinates.data()), sizeof(mdl_texCoord_t) * m_header.num_verts);
				file.read(reinterpret_cast< char * >(m_triangles.data()), sizeof(mdl_triangle_t) * m_header.num_tris);

				/* Read frames */
				for ( auto & frame : m_frames )
				{
					frame.frame.verts.resize(m_header.num_verts);

					file.read(reinterpret_cast< char * >(&frame.type), sizeof(long));
					file.read(reinterpret_cast< char * >(&frame.frame.bboxmin), sizeof(mdl_vertex_t));
					file.read(reinterpret_cast< char * >(&frame.frame.bboxmax), sizeof(mdl_vertex_t));
					file.read(reinterpret_cast< char * >(frame.frame.name.data()), sizeof(char) * 16);
					file.read(reinterpret_cast< char * >(frame.frame.verts.data()), sizeof(mdl_vertex_t) * m_header.num_verts);
				}

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

			using mdl_vec3_t = std::array< float, 3 >;

			struct mdl_header_t
			{
				int ident = 0; /* magic number: "IDPO" */
				int version = 0; /* version: 6 */

				mdl_vec3_t scale{1, 1, 1}; /* scale factor */
				mdl_vec3_t translate{0, 0, 0}; /* translation vector */
				float boundingRadius = 0;
				mdl_vec3_t eyePosition{0, 0, 0}; /* eyes' position */

				int num_skins = 0; /* number of texture2Ds */
				int skinwidth = 0; /* texture width */
				int skinheight = 0; /* texture height */

				int num_verts = 0; /* number of vertices */
				int num_tris = 0; /* number of triangles */
				int num_frames = 0; /* number of frames */

				int synctype = 0; /* 0 = synchronized, 1 = random */
				int flags = 0; /* state flag */
				float size = 1;
			};

			/* skin */
			struct mdl_skin_t
			{
				int group = 0; /* 0 = single */
				std::vector< unsigned char > data{}; /* texture data */
			};

			/* group of pictures */
			struct mdl_groupskin_t
			{
				int group = 0; /* 1 = group */
				int nb = 0; /* number of pics */
				float * time = nullptr; /* time duration for each pic */
				unsigned char * * data = nullptr; /* texture data */
			};

			/* texture coords */
			struct mdl_texCoord_t
			{
				int onseam = 0;
				int s = 0;
				int t = 0;
			};

			/* triangle info */
			struct mdl_triangle_t
			{
				int facesfront = 0; /* 0 = backface, 1 = frontFace */
				std::array< int, 3 > vertex{0, 0, 0}; /* vertex indices */
			};

			/* compressed vertex */
			struct mdl_vertex_t
			{
				std::array< unsigned char, 3 > v{0, 0, 0};
				unsigned char normalIndex = 0;
			};

			/* simple frame */
			struct mdl_simpleframe_t
			{
				mdl_vertex_t bboxmin; /* bounding box min */
				mdl_vertex_t bboxmax; /* bounding box max */
				std::array< char, 16 > name{0};
				std::vector< mdl_vertex_t > verts{}; /* vertex list of the frame */
			};

			struct mdl_frame_t
			{
				int type = 0;  /* 0 = simple */
				mdl_simpleframe_t frame{};
			};

			struct mdl_groupframe_t
			{
				int type = 0; /* !0 = group */
				mdl_vertex_t min; /* min pos in all simple frames */
				mdl_vertex_t max; /* max pos in all simple frames */
				float * time = nullptr; /* time duration for each frame */
				mdl_simpleframe_t * frames = nullptr; /* simple frame list */
			};

			/* magic number: "IDPO" or 1330660425 */
			static constexpr auto MagicNumber = (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I');
			static constexpr auto Version = 6;
			static constexpr std::array< mdl_vec3_t, 162 > s_anorms{{
				{ -0.525731F,  0.000000F,  0.850651F },
				{ -0.442863F,  0.238856F,  0.864188F },
				{ -0.295242F,  0.000000F,  0.955423F },
				{ -0.309017F,  0.500000F,  0.809017F },
				{ -0.162460F,  0.262866F,  0.951056F },
				{  0.000000F,  0.000000F,  1.000000F },
				{  0.000000F,  0.850651F,  0.525731F },
				{ -0.147621F,  0.716567F,  0.681718F },
				{  0.147621F,  0.716567F,  0.681718F },
				{  0.000000F,  0.525731F,  0.850651F },
				{  0.309017F,  0.500000F,  0.809017F },
				{  0.525731F,  0.000000F,  0.850651F },
				{  0.295242F,  0.000000F,  0.955423F },
				{  0.442863F,  0.238856F,  0.864188F },
				{  0.162460F,  0.262866F,  0.951056F },
				{ -0.681718F,  0.147621F,  0.716567F },
				{ -0.809017F,  0.309017F,  0.500000F },
				{ -0.587785F,  0.425325F,  0.688191F },
				{ -0.850651F,  0.525731F,  0.000000F },
				{ -0.864188F,  0.442863F,  0.238856F },
				{ -0.716567F,  0.681718F,  0.147621F },
				{ -0.688191F,  0.587785F,  0.425325F },
				{ -0.500000F,  0.809017F,  0.309017F },
				{ -0.238856F,  0.864188F,  0.442863F },
				{ -0.425325F,  0.688191F,  0.587785F },
				{ -0.716567F,  0.681718F, -0.147621F },
				{ -0.500000F,  0.809017F, -0.309017F },
				{ -0.525731F,  0.850651F,  0.000000F },
				{  0.000000F,  0.850651F, -0.525731F },
				{ -0.238856F,  0.864188F, -0.442863F },
				{  0.000000F,  0.955423F, -0.295242F },
				{ -0.262866F,  0.951056F, -0.162460F },
				{  0.000000F,  1.000000F,  0.000000F },
				{  0.000000F,  0.955423F,  0.295242F },
				{ -0.262866F,  0.951056F,  0.162460F },
				{  0.238856F,  0.864188F,  0.442863F },
				{  0.262866F,  0.951056F,  0.162460F },
				{  0.500000F,  0.809017F,  0.309017F },
				{  0.238856F,  0.864188F, -0.442863F },
				{  0.262866F,  0.951056F, -0.162460F },
				{  0.500000F,  0.809017F, -0.309017F },
				{  0.850651F,  0.525731F,  0.000000F },
				{  0.716567F,  0.681718F,  0.147621F },
				{  0.716567F,  0.681718F, -0.147621F },
				{  0.525731F,  0.850651F,  0.000000F },
				{  0.425325F,  0.688191F,  0.587785F },
				{  0.864188F,  0.442863F,  0.238856F },
				{  0.688191F,  0.587785F,  0.425325F },
				{  0.809017F,  0.309017F,  0.500000F },
				{  0.681718F,  0.147621F,  0.716567F },
				{  0.587785F,  0.425325F,  0.688191F },
				{  0.955423F,  0.295242F,  0.000000F },
				{  1.000000F,  0.000000F,  0.000000F },
				{  0.951056F,  0.162460F,  0.262866F },
				{  0.850651F, -0.525731F,  0.000000F },
				{  0.955423F, -0.295242F,  0.000000F },
				{  0.864188F, -0.442863F,  0.238856F },
				{  0.951056F, -0.162460F,  0.262866F },
				{  0.809017F, -0.309017F,  0.500000F },
				{  0.681718F, -0.147621F,  0.716567F },
				{  0.850651F,  0.000000F,  0.525731F },
				{  0.864188F,  0.442863F, -0.238856F },
				{  0.809017F,  0.309017F, -0.500000F },
				{  0.951056F,  0.162460F, -0.262866F },
				{  0.525731F,  0.000000F, -0.850651F },
				{  0.681718F,  0.147621F, -0.716567F },
				{  0.681718F, -0.147621F, -0.716567F },
				{  0.850651F,  0.000000F, -0.525731F },
				{  0.809017F, -0.309017F, -0.500000F },
				{  0.864188F, -0.442863F, -0.238856F },
				{  0.951056F, -0.162460F, -0.262866F },
				{  0.147621F,  0.716567F, -0.681718F },
				{  0.309017F,  0.500000F, -0.809017F },
				{  0.425325F,  0.688191F, -0.587785F },
				{  0.442863F,  0.238856F, -0.864188F },
				{  0.587785F,  0.425325F, -0.688191F },
				{  0.688191F,  0.587785F, -0.425325F },
				{ -0.147621F,  0.716567F, -0.681718F },
				{ -0.309017F,  0.500000F, -0.809017F },
				{  0.000000F,  0.525731F, -0.850651F },
				{ -0.525731F,  0.000000F, -0.850651F },
				{ -0.442863F,  0.238856F, -0.864188F },
				{ -0.295242F,  0.000000F, -0.955423F },
				{ -0.162460F,  0.262866F, -0.951056F },
				{  0.000000F,  0.000000F, -1.000000F },
				{  0.295242F,  0.000000F, -0.955423F },
				{  0.162460F,  0.262866F, -0.951056F },
				{ -0.442863F, -0.238856F, -0.864188F },
				{ -0.309017F, -0.500000F, -0.809017F },
				{ -0.162460F, -0.262866F, -0.951056F },
				{  0.000000F, -0.850651F, -0.525731F },
				{ -0.147621F, -0.716567F, -0.681718F },
				{  0.147621F, -0.716567F, -0.681718F },
				{  0.000000F, -0.525731F, -0.850651F },
				{  0.309017F, -0.500000F, -0.809017F },
				{  0.442863F, -0.238856F, -0.864188F },
				{  0.162460F, -0.262866F, -0.951056F },
				{  0.238856F, -0.864188F, -0.442863F },
				{  0.500000F, -0.809017F, -0.309017F },
				{  0.425325F, -0.688191F, -0.587785F },
				{  0.716567F, -0.681718F, -0.147621F },
				{  0.688191F, -0.587785F, -0.425325F },
				{  0.587785F, -0.425325F, -0.688191F },
				{  0.000000F, -0.955423F, -0.295242F },
				{  0.000000F, -1.000000F,  0.000000F },
				{  0.262866F, -0.951056F, -0.162460F },
				{  0.000000F, -0.850651F,  0.525731F },
				{  0.000000F, -0.955423F,  0.295242F },
				{  0.238856F, -0.864188F,  0.442863F },
				{  0.262866F, -0.951056F,  0.162460F },
				{  0.500000F, -0.809017F,  0.309017F },
				{  0.716567F, -0.681718F,  0.147621F },
				{  0.525731F, -0.850651F,  0.000000F },
				{ -0.238856F, -0.864188F, -0.442863F },
				{ -0.500000F, -0.809017F, -0.309017F },
				{ -0.262866F, -0.951056F, -0.162460F },
				{ -0.850651F, -0.525731F,  0.000000F },
				{ -0.716567F, -0.681718F, -0.147621F },
				{ -0.716567F, -0.681718F,  0.147621F },
				{ -0.525731F, -0.850651F,  0.000000F },
				{ -0.500000F, -0.809017F,  0.309017F },
				{ -0.238856F, -0.864188F,  0.442863F },
				{ -0.262866F, -0.951056F,  0.162460F },
				{ -0.864188F, -0.442863F,  0.238856F },
				{ -0.809017F, -0.309017F,  0.500000F },
				{ -0.688191F, -0.587785F,  0.425325F },
				{ -0.681718F, -0.147621F,  0.716567F },
				{ -0.442863F, -0.238856F,  0.864188F },
				{ -0.587785F, -0.425325F,  0.688191F },
				{ -0.309017F, -0.500000F,  0.809017F },
				{ -0.147621F, -0.716567F,  0.681718F },
				{ -0.425325F, -0.688191F,  0.587785F },
				{ -0.162460F, -0.262866F,  0.951056F },
				{  0.442863F, -0.238856F,  0.864188F },
				{  0.162460F, -0.262866F,  0.951056F },
				{  0.309017F, -0.500000F,  0.809017F },
				{  0.147621F, -0.716567F,  0.681718F },
				{  0.000000F, -0.525731F,  0.850651F },
				{  0.425325F, -0.688191F,  0.587785F },
				{  0.587785F, -0.425325F,  0.688191F },
				{  0.688191F, -0.587785F,  0.425325F },
				{ -0.955423F,  0.295242F,  0.000000F },
				{ -0.951056F,  0.162460F,  0.262866F },
				{ -1.000000F,  0.000000F,  0.000000F },
				{ -0.850651F,  0.000000F,  0.525731F },
				{ -0.955423F, -0.295242F,  0.000000F },
				{ -0.951056F, -0.162460F,  0.262866F },
				{ -0.864188F,  0.442863F, -0.238856F },
				{ -0.951056F,  0.162460F, -0.262866F },
				{ -0.809017F,  0.309017F, -0.500000F },
				{ -0.864188F, -0.442863F, -0.238856F },
				{ -0.951056F, -0.162460F, -0.262866F },
				{ -0.809017F, -0.309017F, -0.500000F },
				{ -0.681718F,  0.147621F, -0.716567F },
				{ -0.681718F, -0.147621F, -0.716567F },
				{ -0.850651F,  0.000000F, -0.525731F },
				{ -0.688191F,  0.587785F, -0.425325F },
				{ -0.587785F,  0.425325F, -0.688191F },
				{ -0.425325F,  0.688191F, -0.587785F },
				{ -0.425325F, -0.688191F, -0.587785F },
				{ -0.587785F, -0.425325F, -0.688191F },
				{ -0.688191F, -0.587785F, -0.425325F }
			}};

			/* Flag names */
			static constexpr auto UseMDLNormal{0UL};

			/**
			 * @brief Converts the internal format to the native one.
			 * @param geometry A reference to the native geometry.
			 * @param frameIndex Frame inside the MD2 format.
			 * @return bool
			 */
			bool
			convertToGeometry (Shape< vertex_data_t, index_data_t > & geometry, uint32_t frameIndex = 0) const noexcept
			{
				if ( m_header.num_tris <= 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", there is no data !" "\n";

					return false;
				}

				if ( frameIndex >= static_cast< uint32_t >(m_header.num_frames) )
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

				if ( m_flags[UseMDLNormal] )
				{
					builder.options().enableDataEconomy(false);

					for ( size_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++ )
					{
						for ( size_t vertexIndex = 0; vertexIndex < 3UL; vertexIndex++ )
						{
							const auto & vertex = m_frames[frameIndex].frame.verts[m_triangles[triangleIndex].vertex[vertexIndex]];
							const auto & normal = s_anorms[vertex.normalIndex];
							const auto & textureCoordinate = m_textureCoordinates[m_triangles[triangleIndex].vertex[vertexIndex]];

							auto s = static_cast< vertex_data_t >(textureCoordinate.s);
							auto t = static_cast< vertex_data_t >(textureCoordinate.t);

							if ( !m_triangles[triangleIndex].facesfront && textureCoordinate.onseam )
							{
								s += skinWidth * 0.5F; /* backface */
							}

							builder.setPosition(
								m_header.scale[0] * static_cast< vertex_data_t >(vertex.v[0]) + m_header.translate[0],
								m_header.scale[2] * static_cast< vertex_data_t >(vertex.v[2]) + m_header.translate[2],
								m_header.scale[1] * static_cast< vertex_data_t >(vertex.v[1]) + m_header.translate[1]
							);
							builder.setNormal(
								static_cast< vertex_data_t >(normal[0]),
								static_cast< vertex_data_t >(normal[1]),
								static_cast< vertex_data_t >(normal[2])
							);
							builder.setTextureCoordinates(
								(s + 0.5F) / skinWidth,
								skinHeight - (t + 0.5F) / skinHeight
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
							const auto & vertex = m_frames[frameIndex].frame.verts[m_triangles[triangleIndex].vertex[vertexIndex]];
							const auto & textureCoordinate = m_textureCoordinates[m_triangles[triangleIndex].vertex[vertexIndex]];

							auto s = static_cast< vertex_data_t >(textureCoordinate.s);
							auto t = static_cast< vertex_data_t >(textureCoordinate.t);

							if ( !m_triangles[triangleIndex].facesfront && textureCoordinate.onseam )
							{
								s += skinWidth * 0.5F; /* backface */
							}

							builder.setPosition(
								m_header.scale[0] * static_cast< vertex_data_t >(vertex.v[0]) + m_header.translate[0],
								m_header.scale[2] * static_cast< vertex_data_t >(vertex.v[2]) + m_header.translate[2],
								m_header.scale[1] * static_cast< vertex_data_t >(vertex.v[1]) + m_header.translate[1]
							);
							builder.setTextureCoordinates(
								(s + 0.5F) / skinWidth,
								skinHeight - (t + 0.5F) / skinHeight
							);
							builder.newVertex();
						}
					}
				}

				geometry.transform(Math::Matrix< 4, vertex_data_t >::rotation(Math::Radian< vertex_data_t >(90), 0, 1, 0));

				builder.endConstruction();

				return true;
			}

			mdl_header_t m_header{};
			std::vector< mdl_skin_t > m_skins;
			std::vector< mdl_texCoord_t > m_textureCoordinates;
			std::vector< mdl_triangle_t > m_triangles;
			std::vector< mdl_frame_t > m_frames;
			std::vector< uint32_t > m_texID{};
			int32_t m_iSkin = 0;
			std::array< bool, 4 > m_flags{
				true/*UseMDLNormal*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
			};
	};
}
