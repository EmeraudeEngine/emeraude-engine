/*
 * src/Libs/VertexFactory/FileFormatMDx.hpp
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

/* STL inclusions. */
#include <cmath>
#include <cstring>
#include <array>
#include <istream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/VertexFactory/ShapeBuilder.hpp"
#include "Libs/Animation/Joint.hpp"
#include "Libs/Animation/Skeleton.hpp"
#include "Libs/Animation/Skin.hpp"
#include "Libs/Math/Quaternion.hpp"
#include "Libs/Math/TransformUtils.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief Unified 3D format loader for ID Tech engines (MDL, MD2, MD3, MD5).
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @extends EmEn::Libs::VertexFactory::FileFormatInterface
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatMDx final : public FileFormatInterface< vertex_data_t, index_data_t >
	{
		public:

			/**
			 * @brief Constructs a MDx file format.
			 */
			FileFormatMDx () noexcept = default;

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, ShapeLoadResult< vertex_data_t, index_data_t > & result, const ReadOptions & /*readOptions*/) noexcept override
			{
				if ( !stream.isOpen() )
				{
					std::cerr << "[VertexFactory::FileFormatMDx] readStream(), stream is not open !\n";

					return false;
				}

				const auto dataSize = stream.size();
				std::string buffer(dataSize, '\0');

				if ( !stream.read(buffer.data(), dataSize) )
				{
					std::cerr << "[VertexFactory::FileFormatMDx] readStream(), failed to read stream data !\n";

					return false;
				}

				std::istringstream input(buffer, std::ios::binary);

				int ident = 0;
				input.read(reinterpret_cast< char * >(&ident), sizeof(int));
				input.seekg(0, std::ios::beg); // Reset to start

				// Check magic numbers
				// MDL: "IDPO"
				if ( ident == (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I') )
				{
					return this->loadMDL(input, result.shape);
				}

				// MD2: "IDP2"
				if ( ident == (('2' << 24) + ('P' << 16) + ('D' << 8) + 'I') )
				{
					return this->loadMD2(input, result.shape);
				}

				// MD3: "IDP3"
				if ( ident == (('3' << 24) + ('P' << 16) + ('D' << 8) + 'I') )
				{
					return this->loadMD3(input, result.shape);
				}

				// MD5 Check (Text based)
				// Read first line to check for "MD5Version"
				std::string line;
				std::getline(input, line);
				input.seekg(0, std::ios::beg); // Reset

				if ( line.find("MD5Version") != std::string::npos )
				{
					return this->loadMD5(input, result);
				}

				std::cerr << "[VertexFactory::FileFormatMDx] readStream(), unknown format !\n";

				return false;
			}

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::writeStream() */
			[[nodiscard]]
			bool
			writeStream (IO::ByteStream & /*stream*/, const Shape< vertex_data_t, index_data_t > & /*geometry*/, const WriteOptions & /*writeOptions*/) const noexcept override
			{
				std::cerr << "[VertexFactory::FileFormatMDx] writeStream(), the engine is read-only for ID Tech 3D file format.\n";

				return false;
			}

		private:

			/** @brief Scale factor to convert idTech unit system to engine units. idTech models are ~100x too large. */
			static constexpr auto IDTechUnitScale = static_cast< vertex_data_t >(0.01);

			// =========================================================================================================
			// MDL Section
			// =========================================================================================================
			using mdl_vec3_t = std::array< float, 3 >;

			struct mdl_header_t
			{
				int ident = 0;
				int version = 0;
				mdl_vec3_t scale{1, 1, 1};
				mdl_vec3_t translate{0, 0, 0};
				float boundingRadius = 0;
				mdl_vec3_t eyePosition{0, 0, 0};
				int num_skins = 0;
				int skinwidth = 0;
				int skinheight = 0;
				int num_verts = 0;
				int num_tris = 0;
				int num_frames = 0;
				int synctype = 0;
				int flags = 0;
				float size = 1;
			};

			struct mdl_skin_t
			{
				int group = 0;
				std::vector< unsigned char > data{};
			};

			struct mdl_texCoord_t
			{
				int onseam = 0;
				int s = 0;
				int t = 0;
			};

			struct mdl_triangle_t
			{
				int facesfront = 0;
				std::array< int, 3 > vertex{0, 0, 0};
			};

			struct mdl_vertex_t
			{
				std::array< unsigned char, 3 > v{0, 0, 0};
				unsigned char normalIndex = 0;
			};
			
			struct mdl_simpleframe_t
			{
				mdl_vertex_t bboxmin;
				mdl_vertex_t bboxmax;
				std::array< char, 16 > name{0};
				std::vector< mdl_vertex_t > verts{};
			};

			struct mdl_frame_t { int type = 0; mdl_simpleframe_t frame{}; };

			// Precomputed normals for MDL/MD2
			static constexpr std::array< std::array<float, 3>, 162 > s_anorms{{
				{ -0.525731F,  0.000000F,  0.850651F }, { -0.442863F,  0.238856F,  0.864188F }, { -0.295242F,  0.000000F,  0.955423F }, { -0.309017F,  0.500000F,  0.809017F },
				{ -0.162460F,  0.262866F,  0.951056F }, {  0.000000F,  0.000000F,  1.000000F }, {  0.000000F,  0.850651F,  0.525731F }, { -0.147621F,  0.716567F,  0.681718F },
				{  0.147621F,  0.716567F,  0.681718F }, {  0.000000F,  0.525731F,  0.850651F }, {  0.309017F,  0.500000F,  0.809017F }, {  0.525731F,  0.000000F,  0.850651F },
				{  0.295242F,  0.000000F,  0.955423F }, {  0.442863F,  0.238856F,  0.864188F }, {  0.162460F,  0.262866F,  0.951056F }, { -0.681718F,  0.147621F,  0.716567F },
				{ -0.809017F,  0.309017F,  0.500000F }, { -0.587785F,  0.425325F,  0.688191F }, { -0.850651F,  0.525731F,  0.000000F }, { -0.864188F,  0.442863F,  0.238856F },
				{ -0.716567F,  0.681718F,  0.147621F }, { -0.688191F,  0.587785F,  0.425325F }, { -0.500000F,  0.809017F,  0.309017F }, { -0.238856F,  0.864188F,  0.442863F },
				{ -0.425325F,  0.688191F,  0.587785F }, { -0.716567F,  0.681718F, -0.147621F }, { -0.500000F,  0.809017F, -0.309017F }, { -0.525731F,  0.850651F,  0.000000F },
				{  0.000000F,  0.850651F, -0.525731F }, { -0.238856F,  0.864188F, -0.442863F }, {  0.000000F,  0.955423F, -0.295242F }, { -0.262866F,  0.951056F, -0.162460F },
				{  0.000000F,  1.000000F,  0.000000F }, {  0.000000F,  0.955423F,  0.295242F }, { -0.262866F,  0.951056F,  0.162460F }, {  0.238856F,  0.864188F,  0.442863F },
				{  0.262866F,  0.951056F,  0.162460F }, {  0.500000F,  0.809017F,  0.309017F }, {  0.238856F,  0.864188F, -0.442863F }, {  0.262866F,  0.951056F, -0.162460F },
				{  0.500000F,  0.809017F, -0.309017F }, {  0.850651F,  0.525731F,  0.000000F }, {  0.716567F,  0.681718F,  0.147621F }, {  0.716567F,  0.681718F, -0.147621F },
				{  0.525731F,  0.850651F,  0.000000F }, {  0.425325F,  0.688191F,  0.587785F }, {  0.864188F,  0.442863F,  0.238856F }, {  0.688191F,  0.587785F,  0.425325F },
				{  0.809017F,  0.309017F,  0.500000F }, {  0.681718F,  0.147621F,  0.716567F }, {  0.587785F,  0.425325F,  0.688191F }, {  0.955423F,  0.295242F,  0.000000F },
				{  1.000000F,  0.000000F,  0.000000F }, {  0.951056F,  0.162460F,  0.262866F }, {  0.850651F, -0.525731F,  0.000000F }, {  0.955423F, -0.295242F,  0.000000F },
				{  0.864188F, -0.442863F,  0.238856F }, {  0.951056F, -0.162460F,  0.262866F }, {  0.809017F, -0.309017F,  0.500000F }, {  0.681718F, -0.147621F,  0.716567F },
				{  0.850651F,  0.000000F,  0.525731F }, {  0.864188F,  0.442863F, -0.238856F }, {  0.809017F,  0.309017F, -0.500000F }, {  0.951056F,  0.162460F, -0.262866F },
				{  0.525731F,  0.000000F, -0.850651F }, {  0.681718F,  0.147621F, -0.716567F }, {  0.681718F, -0.147621F, -0.716567F }, {  0.850651F,  0.000000F, -0.525731F },
				{  0.809017F, -0.309017F, -0.500000F }, {  0.864188F, -0.442863F, -0.238856F }, {  0.951056F, -0.162460F, -0.262866F }, {  0.147621F,  0.716567F, -0.681718F },
				{  0.309017F,  0.500000F, -0.809017F }, {  0.425325F,  0.688191F, -0.587785F }, {  0.442863F,  0.238856F, -0.864188F }, {  0.587785F,  0.425325F, -0.688191F },
				{  0.688191F,  0.587785F, -0.425325F }, { -0.147621F,  0.716567F, -0.681718F }, { -0.309017F,  0.500000F, -0.809017F }, {  0.000000F,  0.525731F, -0.850651F },
				{ -0.525731F,  0.000000F, -0.850651F }, { -0.442863F,  0.238856F, -0.864188F }, { -0.295242F,  0.000000F, -0.955423F }, { -0.162460F,  0.262866F, -0.951056F },
				{  0.000000F,  0.000000F, -1.000000F }, {  0.295242F,  0.000000F, -0.955423F }, {  0.162460F,  0.262866F, -0.951056F }, { -0.442863F, -0.238856F, -0.864188F },
				{ -0.309017F, -0.500000F, -0.809017F }, { -0.162460F, -0.262866F, -0.951056F }, {  0.000000F, -0.850651F, -0.525731F }, { -0.147621F, -0.716567F, -0.681718F },
				{  0.147621F, -0.716567F, -0.681718F }, {  0.000000F, -0.525731F, -0.850651F }, {  0.309017F, -0.500000F, -0.809017F }, {  0.442863F, -0.238856F, -0.864188F },
				{  0.162460F, -0.262866F, -0.951056F }, {  0.238856F, -0.864188F, -0.442863F }, {  0.500000F, -0.809017F, -0.309017F }, {  0.425325F, -0.688191F, -0.587785F },
				{  0.716567F, -0.681718F, -0.147621F }, {  0.688191F, -0.587785F, -0.425325F }, {  0.587785F, -0.425325F, -0.688191F }, {  0.000000F, -0.955423F, -0.295242F },
				{  0.000000F, -1.000000F,  0.000000F }, {  0.262866F, -0.951056F, -0.162460F }, {  0.000000F, -0.850651F,  0.525731F }, {  0.000000F, -0.955423F,  0.295242F },
				{  0.238856F, -0.864188F,  0.442863F }, {  0.262866F, -0.951056F,  0.162460F }, {  0.500000F, -0.809017F,  0.309017F }, {  0.716567F, -0.681718F,  0.147621F },
				{  0.525731F, -0.850651F,  0.000000F }, { -0.238856F, -0.864188F, -0.442863F }, { -0.500000F, -0.809017F, -0.309017F }, { -0.262866F, -0.951056F, -0.162460F },
				{ -0.850651F, -0.525731F,  0.000000F }, { -0.716567F, -0.681718F, -0.147621F }, { -0.716567F, -0.681718F,  0.147621F }, { -0.525731F, -0.850651F,  0.000000F },
				{ -0.500000F, -0.809017F,  0.309017F }, { -0.238856F, -0.864188F,  0.442863F }, { -0.262866F, -0.951056F,  0.162460F }, { -0.864188F, -0.442863F,  0.238856F },
				{ -0.809017F, -0.309017F,  0.500000F }, { -0.688191F, -0.587785F,  0.425325F }, { -0.681718F, -0.147621F,  0.716567F }, { -0.442863F, -0.238856F,  0.864188F },
				{ -0.587785F, -0.425325F,  0.688191F }, { -0.309017F, -0.500000F,  0.809017F }, { -0.147621F, -0.716567F,  0.681718F }, { -0.425325F, -0.688191F,  0.587785F },
				{ -0.162460F, -0.262866F,  0.951056F }, {  0.442863F, -0.238856F,  0.864188F }, {  0.162460F, -0.262866F,  0.951056F }, {  0.309017F, -0.500000F,  0.809017F },
				{  0.147621F, -0.716567F,  0.681718F }, {  0.000000F, -0.525731F,  0.850651F }, {  0.425325F, -0.688191F,  0.587785F }, {  0.587785F, -0.425325F,  0.688191F },
				{  0.688191F, -0.587785F,  0.425325F }, { -0.955423F,  0.295242F,  0.000000F }, { -0.951056F,  0.162460F,  0.262866F }, { -1.000000F,  0.000000F,  0.000000F },
				{ -0.850651F,  0.000000F,  0.525731F }, { -0.955423F, -0.295242F,  0.000000F }, { -0.951056F, -0.162460F,  0.262866F }, { -0.864188F,  0.442863F, -0.238856F },
				{ -0.951056F,  0.162460F, -0.262866F }, { -0.809017F,  0.309017F, -0.500000F }, { -0.864188F, -0.442863F, -0.238856F }, { -0.951056F, -0.162460F, -0.262866F },
				{ -0.809017F, -0.309017F, -0.500000F }, { -0.681718F,  0.147621F, -0.716567F }, { -0.681718F, -0.147621F, -0.716567F }, { -0.850651F,  0.000000F, -0.525731F },
				{ -0.688191F,  0.587785F, -0.425325F }, { -0.587785F,  0.425325F, -0.688191F }, { -0.425325F,  0.688191F, -0.587785F }, { -0.425325F, -0.688191F, -0.587785F },
				{ -0.587785F, -0.425325F, -0.688191F }, { -0.688191F, -0.587785F, -0.425325F }
			}};

			bool
			loadMDL (std::istream & file, Shape< vertex_data_t, index_data_t > & geometry)
			{
				mdl_header_t header;
				file.read(reinterpret_cast< char * >(&header), sizeof(mdl_header_t));

				std::vector< mdl_skin_t > skins(header.num_skins);
				std::vector< mdl_texCoord_t > textureCoordinates(header.num_verts);
				std::vector< mdl_triangle_t > triangles(header.num_tris);
				std::vector< mdl_frame_t > frames(header.num_frames);

				for ( auto & skin : skins )
				{
					skin.data.resize(header.skinwidth * header.skinheight);
					file.read(reinterpret_cast< char * >(&skin.group), sizeof(int));
					file.read(reinterpret_cast< char * >(skin.data.data()), sizeof(unsigned char) * header.skinwidth * header.skinheight);
				}

				file.read(reinterpret_cast< char * >(textureCoordinates.data()), sizeof(mdl_texCoord_t) * header.num_verts);
				file.read(reinterpret_cast< char * >(triangles.data()), sizeof(mdl_triangle_t) * header.num_tris);

				for ( auto & frame : frames )
				{
					frame.frame.verts.resize(header.num_verts);
					file.read(reinterpret_cast< char * >(&frame.type), sizeof(int));
					file.read(reinterpret_cast< char * >(&frame.frame.bboxmin), sizeof(mdl_vertex_t));
					file.read(reinterpret_cast< char * >(&frame.frame.bboxmax), sizeof(mdl_vertex_t));
					file.read(reinterpret_cast< char * >(frame.frame.name.data()), sizeof(char) * 16);
					file.read(reinterpret_cast< char * >(frame.frame.verts.data()), sizeof(mdl_vertex_t) * header.num_verts);
				}

				if ( header.num_tris <= 0 )
				{
					return false;
				}

				const auto triangleCount = static_cast< uint32_t >(header.num_tris);
				const auto skinWidth = static_cast< vertex_data_t >(header.skinwidth);
				const auto skinHeight = static_cast< vertex_data_t >(header.skinheight);

				geometry.reserveData(triangleCount * 3, triangleCount * 3, 0U, triangleCount);

				ShapeBuilderOptions< vertex_data_t > options{};
				options.enableGlobalVertexColor(PixelFactory::White);

				ShapeBuilder builder{geometry, options};
				builder.beginConstruction(ConstructionMode::Triangles);

				int frameIndex = 0; // Default to first frame

				for ( uint32_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++ )
				{
					/* Reverse vertex order (2,1,0) to fix winding. */
					for ( int vertexIndex = 2; vertexIndex >= 0; vertexIndex-- )
					{
						const auto & vertex = frames[frameIndex].frame.verts[triangles[triangleIndex].vertex[vertexIndex]];
						const auto & normal = s_anorms[vertex.normalIndex];
						const auto & textureCoordinate = textureCoordinates[triangles[triangleIndex].vertex[vertexIndex]];

						auto s = static_cast< vertex_data_t >(textureCoordinate.s);
						auto t = static_cast< vertex_data_t >(textureCoordinate.t);

						if ( !triangles[triangleIndex].facesfront && textureCoordinate.onseam )
						{
							s += skinWidth * 0.5F;
						}

						/* Combined transform: Y/Z swap + negation + rotation -90° Y = (Y, -Z, X) */
						builder.setPosition(
							(header.scale[1] * static_cast< vertex_data_t >(vertex.v[1]) + header.translate[1]) * IDTechUnitScale,
							-(header.scale[2] * static_cast< vertex_data_t >(vertex.v[2]) + header.translate[2]) * IDTechUnitScale,
							(header.scale[0] * static_cast< vertex_data_t >(vertex.v[0]) + header.translate[0]) * IDTechUnitScale
						);
						builder.setNormal(
							static_cast< vertex_data_t >(normal[2]),
							-static_cast< vertex_data_t >(normal[1]),
							static_cast< vertex_data_t >(normal[0])
						);
						builder.setTextureCoordinates(
							(s + 0.5F) / skinWidth,
							(t + 0.5F) / skinHeight
						);
						builder.newVertex();
					}
				}

				builder.endConstruction();

				return true;
			}


			// =========================================================================================================
			// MD2 Section
			// =========================================================================================================
			using md2_vec3_t = std::array< float, 3 >;

			struct md2_header_t
			{
				int ident; int version; int skinwidth; int skinheight; int framesize;
				int num_skins; int num_vertices; int num_st; int num_tris; int num_glcmds; int num_frames;
				int offset_skins; int offset_st; int offset_tris; int offset_frames; int offset_glcmds; int offset_end;
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
				md2_vec3_t scale;
				md2_vec3_t translate;
				std::array< char, 16 > name;
				std::vector< md2_vertex_t > verts;
			};

			bool
			loadMD2 (std::istream & file, Shape< vertex_data_t, index_data_t > & geometry)
			{
				md2_header_t header;
				file.read(reinterpret_cast< char * >(&header), sizeof(md2_header_t));

				std::vector< md2_texCoord_t > textureCoordinates(header.num_st);
				std::vector< md2_triangle_t > triangles(header.num_tris);
				std::vector< md2_frame_t > frames(header.num_frames);

				file.seekg(header.offset_st, std::ios::beg);
				file.read(reinterpret_cast< char * >(textureCoordinates.data()), sizeof(md2_texCoord_t) * header.num_st);

				file.seekg(header.offset_tris, std::ios::beg);
				file.read(reinterpret_cast< char * >(triangles.data()), sizeof(md2_triangle_t) * header.num_tris);

				file.seekg(header.offset_frames, std::ios::beg);

				for ( auto & frame : frames )
				{
					frame.verts.resize(header.num_vertices);
					file.read(reinterpret_cast< char * >(&frame.scale), sizeof(md2_vec3_t));
					file.read(reinterpret_cast< char * >(&frame.translate), sizeof(md2_vec3_t));
					file.read(reinterpret_cast< char * >(&frame.name), sizeof(char) * 16);
					file.read(reinterpret_cast< char * >(frame.verts.data()), sizeof(md2_vertex_t) * header.num_vertices);
				}

				if ( header.num_tris <= 0 )
				{
					return false;
				}

				const auto triangleCount = static_cast< uint32_t >(header.num_tris);
				const auto skinWidth = static_cast< vertex_data_t >(header.skinwidth);
				const auto skinHeight = static_cast< vertex_data_t >(header.skinheight);

				geometry.reserveData(triangleCount * 3, triangleCount * 3, 0U, triangleCount);

				ShapeBuilderOptions< vertex_data_t > options{};
				options.enableGlobalVertexColor(PixelFactory::White);
				ShapeBuilder builder{geometry, options};
				builder.beginConstruction(ConstructionMode::Triangles);

				int frameIndex = 0;

				for ( uint32_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++ )
				{
					/* Reverse vertex order (2,1,0) to fix winding. */
					for ( int vertexIndex = 2; vertexIndex >= 0; vertexIndex-- )
					{
						const auto & frame = frames[frameIndex];
						const auto & vertex = frame.verts[triangles[triangleIndex].vertex[vertexIndex]];
						const auto & normal = s_anorms[vertex.normalIndex];
						const auto & textureCoordinate = textureCoordinates[triangles[triangleIndex].st[vertexIndex]];

						/* Combined transform: Y/Z swap + negation + rotation -90° Y = (Y, -Z, X) */
						builder.setPosition(
							(frame.scale[1] * static_cast< vertex_data_t >(vertex.v[1]) + frame.translate[1]) * IDTechUnitScale,
							-(frame.scale[2] * static_cast< vertex_data_t >(vertex.v[2]) + frame.translate[2]) * IDTechUnitScale,
							(frame.scale[0] * static_cast< vertex_data_t >(vertex.v[0]) + frame.translate[0]) * IDTechUnitScale
						);
						builder.setNormal(
							static_cast< vertex_data_t >(normal[1]),
							-static_cast< vertex_data_t >(normal[2]),
							static_cast< vertex_data_t >(normal[0])
						);
						builder.setTextureCoordinates(
							static_cast< vertex_data_t >(textureCoordinate.s) / skinWidth,
							static_cast< vertex_data_t >(textureCoordinate.t) / skinHeight
						);
						builder.newVertex();
					}
				}

				builder.endConstruction();

				return true;
			}

			// =========================================================================================================
			// MD3 Section
			// =========================================================================================================
			using md3_vec3_t = std::array< float, 3 >;
			using md3_vec2_t = std::array< float, 2 >;

			struct md3_header_t
			{
				int ident; 
				int version; 
				char name[64]; 
				int flags;
				int num_frames; 
				int num_tags; 
				int num_surfaces; 
				int num_skins;
				int offset_frames; 
				int offset_tags; 
				int offset_surfaces; 
				int offset_eof;
			};

			struct md3_surface_t
			{
				int ident; 
				char name[64]; 
				int flags;
				int num_frames; 
				int num_shaders; 
				int num_verts; 
				int num_triangles;
				int offset_triangles; 
				int offset_shaders; 
				int offset_st; 
				int offset_xyzn; 
				int offset_end;
			};

			struct md3_triangle_t
			{
				int indexes[3];
			};
			
			struct md3_shader_t
			{
				char name[64]; 
				int shader_index;
			};
			
			struct md3_texCoord_t
			{
				float s; 
				float t;
			};
			
			struct md3_vertex_t
			{
				short v[3]; 
				unsigned char normal[2];
			};

			bool
			loadMD3 (std::istream & file, Shape< vertex_data_t, index_data_t > & geometry)
			{
				md3_header_t header;
				file.read(reinterpret_cast< char * >(&header), sizeof(md3_header_t));

				geometry.clear();
				
				// Count total size to reserve
				int totalTriangles = 0;
				int currentSurfaceOffset = header.offset_surfaces;
				
				std::vector< md3_surface_t > surfaces(header.num_surfaces);
				
				for ( int surfaceIndex = 0; surfaceIndex < header.num_surfaces; ++surfaceIndex)
				{
					file.seekg(currentSurfaceOffset, std::ios::beg);
					file.read(reinterpret_cast< char * >(&surfaces[surfaceIndex]), sizeof(md3_surface_t));
					totalTriangles += surfaces[surfaceIndex].num_triangles;
					currentSurfaceOffset += surfaces[surfaceIndex].offset_end;
				}

				geometry.reserveData(static_cast< uint32_t >(totalTriangles) * 3, static_cast< uint32_t >(totalTriangles) * 3, 0U, static_cast< uint32_t >(totalTriangles));

				ShapeBuilderOptions< vertex_data_t > options{};
				options.enableGlobalVertexColor(PixelFactory::White);
				
				ShapeBuilder builder{geometry, options};
				builder.beginConstruction(ConstructionMode::Triangles);

				currentSurfaceOffset = header.offset_surfaces;

				for ( int surfaceIndex = 0; surfaceIndex < header.num_surfaces; ++surfaceIndex )
				{
					md3_surface_t surf = surfaces[surfaceIndex];

					// Read data for this surface
					std::vector< md3_triangle_t > tris(surf.num_triangles);
					std::vector< md3_vertex_t > verts(surf.num_verts);
					std::vector< md3_texCoord_t > uvs(surf.num_verts);

					// Triangles
					file.seekg(currentSurfaceOffset + surf.offset_triangles, std::ios::beg);
					file.read(reinterpret_cast< char * >(tris.data()), surf.num_triangles * sizeof(md3_triangle_t));

					// Vertices (Frame 0 only for static shape)
					file.seekg(currentSurfaceOffset + surf.offset_xyzn, std::ios::beg);
					file.read(reinterpret_cast< char * >(verts.data()), surf.num_verts * sizeof(md3_vertex_t));

					// UVs
					file.seekg(currentSurfaceOffset + surf.offset_st, std::ios::beg);
					file.read(reinterpret_cast< char * >(uvs.data()), surf.num_verts * sizeof(md3_texCoord_t));

					// Scale factor for int16 XYZ
					constexpr float MD3_XYZ_SCALE = 1.0f / 64.0f;

					for ( const auto & tri : tris )
					{
						/* Reverse vertex order (2,1,0) to fix winding. */
						for ( int k = 2; k >= 0; --k )
						{
							int idx = tri.indexes[k];
							const auto & v = verts[idx];
							const auto & uv = uvs[idx];

							/* Decode Normal from lat/lng encoding. */
							float lat = (v.normal[0] * (2 * 3.14159265f) / 255.0f);
							float lng = (v.normal[1] * (2 * 3.14159265f) / 255.0f);
							float nx = std::cos(lat) * std::sin(lng);
							float ny = std::sin(lat) * std::sin(lng);
							float nz = std::cos(lng);

							/* Combined transform: Y/Z swap + negation + rotation -90° Y = (Y, -Z, X) */
							builder.setPosition(
								static_cast< vertex_data_t >(v.v[1]) * MD3_XYZ_SCALE * IDTechUnitScale,
								-static_cast< vertex_data_t >(v.v[2]) * MD3_XYZ_SCALE * IDTechUnitScale,
								static_cast< vertex_data_t >(v.v[0]) * MD3_XYZ_SCALE * IDTechUnitScale
							);
							builder.setNormal(
								static_cast< vertex_data_t >(ny),
								-static_cast< vertex_data_t >(nz),
								static_cast< vertex_data_t >(nx)
							);
							builder.setTextureCoordinates(
								static_cast< vertex_data_t >(uv.s),
								static_cast< vertex_data_t >(uv.t)
							);
							builder.newVertex();
						}
					}
					currentSurfaceOffset += surf.offset_end;
				}

				builder.endConstruction();

				return true;
			}

			// =========================================================================================================
			// MD5 Section
			// =========================================================================================================

			struct md5_joint_t
			{
				std::string name;
				int parent;
				std::array< float, 3 > pos;
				std::array< float, 4 > orient; // Quaternion (x, y, z, w)
			};

			struct md5_weight_t
			{
				int jointIndex;
				float bias;
				std::array< float, 3 > pos;
			};

			struct md5_vertex_t
			{
				std::array< float, 2 > uv;
				int startWeight;
				int countWeight;
			};

			struct md5_mesh_t
			{
				std::string shader;
				std::vector< md5_vertex_t > verts;
				std::vector< std::array< int, 3 > > tris;
				std::vector< md5_weight_t > weights;
			};

			// Quaternion helpers
			static
			void
			computeW (std::array< float, 4 > & q)
			{
				float t = 1.0f - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);

				q[3] = (t < 0.0f) ? 0.0f : -std::sqrt(t);
			}

			static
			void
			rotatePoint (const std::array< float, 4 > & q, const std::array< float, 3 > & in, std::array< float, 3 > & out)
			{
				// Quaternion rotation P_out = Q * P_in * Q^-1
				// Optimized:
				float x = q[0], y = q[1], z = q[2], w = q[3];
				float ix = w * in[0] + y * in[2] - z * in[1];
				float iy = w * in[1] + z * in[0] - x * in[2];
				float iz = w * in[2] + x * in[1] - y * in[0];
				float iw = -x * in[0] - y * in[1] - z * in[2];

				out[0] = ix * w + iw * -x + iy * -z - iz * -y;
				out[1] = iy * w + iw * -y + iz * -x - ix * -z;
				out[2] = iz * w + iw * -z + ix * -y - iy * -x;
			}

			/**
			 * @brief Converts an MD5 position vector from ID Tech coordinate space to engine space.
			 * @note MD5 uses right-handed Y-up. Engine uses Y-down.
			 * Combined transform: (md5.y, -md5.z, md5.x) * IDTechUnitScale.
			 * @param md5Pos The position in MD5 coordinate space.
			 * @return Math::Vector< 3, vertex_data_t >
			 */
			static
			Math::Vector< 3, vertex_data_t >
			md5ToEnginePosition (const std::array< float, 3 > & md5Pos) noexcept
			{
				return {
					static_cast< vertex_data_t >(md5Pos[1]) * IDTechUnitScale,
					-static_cast< vertex_data_t >(md5Pos[2]) * IDTechUnitScale,
					static_cast< vertex_data_t >(md5Pos[0]) * IDTechUnitScale
				};
			}

			/**
			 * @brief Converts an MD5 quaternion from ID Tech coordinate space to engine space.
			 * @note Uses the rotation matrix path: quat → matrix → coordinate transform → matrix → quat.
			 * The coordinate change matrix M maps (x,y,z)_md5 → (y,-z,x)_engine.
			 * R_engine = M * R_md5 * M^T.
			 * @param md5Orient The quaternion in MD5 coordinate space (x, y, z, w).
			 * @return Math::Quaternion< vertex_data_t >
			 */
			static
			Math::Quaternion< vertex_data_t >
			md5ToEngineRotation (const std::array< float, 4 > & md5Orient) noexcept
			{
				/* Build the MD5 quaternion. */
				const Math::Quaternion< vertex_data_t > qMD5{
					static_cast< vertex_data_t >(md5Orient[0]),
					static_cast< vertex_data_t >(md5Orient[1]),
					static_cast< vertex_data_t >(md5Orient[2]),
					static_cast< vertex_data_t >(md5Orient[3])
				};

				/* Get proper column-major rotation matrix from MD5 quaternion. */
				const auto rotMD5 = qMD5.toRotationMatrix4();

				/* Coordinate change matrix M: engine = (md5.y, -md5.z, md5.x).
				 * Using row-major constructor: row0=(0,1,0,0), row1=(0,0,-1,0), row2=(1,0,0,0), row3=(0,0,0,1). */
				constexpr auto Zero = static_cast< vertex_data_t >(0);
				constexpr auto One = static_cast< vertex_data_t >(1);
				constexpr auto NegOne = static_cast< vertex_data_t >(-1);

				const Math::Matrix< 4, vertex_data_t > M{
					Zero, One,    Zero, Zero,
					Zero, Zero, NegOne, Zero,
					One,  Zero,   Zero, Zero,
					Zero, Zero,   Zero, One
				};

				/* M^T (transpose). Using row-major constructor. */
				const Math::Matrix< 4, vertex_data_t > MT{
					Zero, Zero, One,  Zero,
					One,  Zero, Zero, Zero,
					Zero, NegOne, Zero, Zero,
					Zero, Zero, Zero, One
				};

				/* R_engine = M * R_md5 * M^T */
				const auto rotEngine = M * rotMD5 * MT;

				return Math::Quaternion< vertex_data_t >{rotEngine};
			}

			bool
			loadMD5 (std::istream & file, ShapeLoadResult< vertex_data_t, index_data_t > & result)
			{
				auto & geometry = result.shape;
				std::string line;
				std::vector< md5_joint_t > joints;
				std::vector< md5_mesh_t > meshes;

				int numJoints = 0;
				int numMeshes = 0;

				/* ---- Phase 1: Parse MD5 file ---- */
				while ( std::getline(file, line) )
				{
					if ( line.find("numJoints") != std::string::npos )
					{
						std::stringstream(line) >> line >> numJoints;
					}
					else if ( line.find("numMeshes") != std::string::npos )
					{
						std::stringstream(line) >> line >> numMeshes;
					}
					else if ( line.find("joints {") != std::string::npos )
					{
						joints.resize(numJoints);
						for ( int i = 0; i < numJoints; ++i )
						{
							std::getline(file, line);

							size_t startQuote = line.find('"');
							size_t endQuote = line.find('"', startQuote + 1);
							joints[i].name = line.substr(startQuote + 1, endQuote - startQuote - 1);

							std::stringstream ss(line.substr(endQuote + 1));

							char trash;

							ss >> joints[i].parent >> trash
							   >> joints[i].pos[0] >> joints[i].pos[1] >> joints[i].pos[2] >> trash >> trash
							   >> joints[i].orient[0] >> joints[i].orient[1] >> joints[i].orient[2];

							computeW(joints[i].orient);
						}
					}
					else if ( line.find("mesh {") != std::string::npos )
					{
						md5_mesh_t mesh;

						while ( std::getline(file, line) && line.find("}") == std::string::npos )
						{
							if ( line.find("shader") != std::string::npos )
							{
								size_t start = line.find('"');
								size_t end = line.find('"', start + 1);
								mesh.shader = line.substr(start + 1, end - start - 1);
							}
							else if ( line.find("numverts") != std::string::npos )
							{
								int num; std::stringstream(line) >> line >> num;

								mesh.verts.resize(num);

								for ( int i = 0; i < num; ++i )
								{
									std::getline(file, line);
									std::stringstream ss(line);
									std::string temp; char trash;

									ss >> temp >> temp >> trash >> mesh.verts[i].uv[0] >> mesh.verts[i].uv[1] >> trash >> mesh.verts[i].startWeight >> mesh.verts[i].countWeight;
								}
							}
							else if ( line.find("numtris") != std::string::npos )
							{
								int num; std::stringstream(line) >> line >> num;

								mesh.tris.resize(num);

								for ( int i = 0; i < num; ++i )
								{
									std::getline(file, line);
									std::stringstream ss(line);
									std::string temp;

									ss >> temp >> temp >> mesh.tris[i][0] >> mesh.tris[i][1] >> mesh.tris[i][2];
								}
							}
							else if ( line.find("numweights") != std::string::npos )
							{
								int num; std::stringstream(line) >> line >> num;
								mesh.weights.resize(num);

								for ( int i = 0; i < num; ++i )
								{
									std::getline(file, line);
									std::stringstream ss(line);
									std::string temp; char trash;

									ss >> temp >> temp >> mesh.weights[i].jointIndex >> mesh.weights[i].bias >> trash >> mesh.weights[i].pos[0] >> mesh.weights[i].pos[1] >> mesh.weights[i].pos[2];
								}
							}
						}

						meshes.push_back(mesh);
					}
				}

				/* ---- Phase 2: Build Skeleton in engine coordinates ---- */
				const auto jointCount = static_cast< size_t >(numJoints);

				/* Compute world-space transforms for each joint in engine coordinates. */
				std::vector< Math::Matrix< 4, vertex_data_t > > jointWorldTransforms(jointCount);
				std::vector< Math::Matrix< 4, vertex_data_t > > inverseBindMatrices(jointCount);

				for ( size_t i = 0; i < jointCount; ++i )
				{
					const auto pos = md5ToEnginePosition(joints[i].pos);
					const auto rot = md5ToEngineRotation(joints[i].orient);

					/* MD5 joints are in world space (not parent-relative). */
					jointWorldTransforms[i] = Math::composeTRS(
						pos, rot, Math::Vector< 3, vertex_data_t >{1, 1, 1}
					);

					inverseBindMatrices[i] = jointWorldTransforms[i].inverse();
				}

				/* Compute parent-relative local transforms and build Joint structs. */
				std::vector< Animation::Joint< vertex_data_t > > engineJoints(jointCount);

				for ( size_t i = 0; i < jointCount; ++i )
				{
					engineJoints[i].name = joints[i].name;
					engineJoints[i].parentIndex = joints[i].parent;
					engineJoints[i].inverseBindMatrix = inverseBindMatrices[i];

					Math::Matrix< 4, vertex_data_t > localTransform;

					if ( joints[i].parent == Animation::NoParent )
					{
						localTransform = jointWorldTransforms[i];
					}
					else
					{
						localTransform = jointWorldTransforms[static_cast< size_t >(joints[i].parent)].inverse() * jointWorldTransforms[i];
					}

					const auto trs = Math::decomposeTRS(localTransform);
					engineJoints[i].translation = trs.translation;
					engineJoints[i].rotation = trs.rotation;
					engineJoints[i].scale = trs.scale;
				}

				Animation::Skeleton< vertex_data_t > skeleton{std::move(engineJoints)};

				/* ---- Phase 3: Build geometry with per-vertex bone influences ---- */

				/* Track per-expanded-vertex influence data. */
				struct VertexInfluence
				{
					Math::Vector< 4, int32_t > indices{-1, -1, -1, -1};
					Math::Vector< 4, vertex_data_t > weights{0, 0, 0, 0};
				};

				std::vector< VertexInfluence > vertexInfluences;

				ShapeBuilderOptions< vertex_data_t > options{};
				options.enableGlobalVertexColor(PixelFactory::White);
				ShapeBuilder builder{geometry, options};
				builder.beginConstruction(ConstructionMode::Triangles);

				for ( const auto & mesh : meshes )
				{
					for ( const auto & tri : mesh.tris )
					{
						/* Reverse vertex order (2,1,0) to fix winding. */
						for ( int k = 2; k >= 0; --k )
						{
							int vIdx = tri[k];
							const auto & vert = mesh.verts[vIdx];

							/* Calculate bind-pose position. */
							std::array< float, 3 > finalPos = {0, 0, 0};

							for ( int w = 0; w < vert.countWeight; ++w )
							{
								const auto & weight = mesh.weights[vert.startWeight + w];
								const auto & joint = joints[weight.jointIndex];

								std::array< float, 3 > rotPos;
								rotatePoint(joint.orient, weight.pos, rotPos);

								finalPos[0] += (joint.pos[0] + rotPos[0]) * weight.bias;
								finalPos[1] += (joint.pos[1] + rotPos[1]) * weight.bias;
								finalPos[2] += (joint.pos[2] + rotPos[2]) * weight.bias;
							}

							/* Combined transform: Y/Z swap + negation + rotation -90° Y = (Y, -Z, X) */
							builder.setPosition(
								static_cast< vertex_data_t >(finalPos[1]) * IDTechUnitScale,
								-static_cast< vertex_data_t >(finalPos[2]) * IDTechUnitScale,
								static_cast< vertex_data_t >(finalPos[0]) * IDTechUnitScale
							);

							builder.setTextureCoordinates(
								static_cast< vertex_data_t >(vert.uv[0]),
								static_cast< vertex_data_t >(vert.uv[1])
							);

							builder.newVertex();

							/* Collect top-4 bone influences sorted by weight (descending). */
							VertexInfluence influence;
							int slotCount = std::min(vert.countWeight, 4);

							if ( vert.countWeight <= 4 )
							{
								/* All weights fit directly. */
								for ( int w = 0; w < slotCount; ++w )
								{
									const auto & weight = mesh.weights[vert.startWeight + w];
									influence.indices[w] = weight.jointIndex;
									influence.weights[w] = static_cast< vertex_data_t >(weight.bias);
								}
							}
							else
							{
								/* More than 4 weights: pick the 4 largest by bias. */
								struct WeightEntry { int jointIndex; float bias; };
								std::vector< WeightEntry > allWeights(vert.countWeight);

								for ( int w = 0; w < vert.countWeight; ++w )
								{
									const auto & weight = mesh.weights[vert.startWeight + w];
									allWeights[w] = {weight.jointIndex, weight.bias};
								}

								std::partial_sort(allWeights.begin(), allWeights.begin() + 4, allWeights.end(),
									[] (const WeightEntry & a, const WeightEntry & b) { return a.bias > b.bias; });

								/* Renormalize the top 4 weights. */
								float totalBias = 0;
								for ( int w = 0; w < 4; ++w )
								{
									totalBias += allWeights[w].bias;
								}

								if ( totalBias > 0 )
								{
									for ( int w = 0; w < 4; ++w )
									{
										influence.indices[w] = allWeights[w].jointIndex;
										influence.weights[w] = static_cast< vertex_data_t >(allWeights[w].bias / totalBias);
									}
								}
							}

							vertexInfluences.push_back(influence);
						}
					}
				}

				builder.endConstruction();

				/* ---- Phase 4: Apply bone influences to vertices ---- */
				auto & vertices = geometry.vertices();

				for ( size_t i = 0; i < vertices.size() && i < vertexInfluences.size(); ++i )
				{
					const auto & inf = vertexInfluences[i];
					vertices[i].setInfluences(inf.indices[0], inf.indices[1], inf.indices[2], inf.indices[3]);
					vertices[i].setWeights(inf.weights[0], inf.weights[1], inf.weights[2], inf.weights[3]);
				}

				/* MD5 format doesn't store normals, compute them from geometry. */
				geometry.computeTriangleNormal();
				geometry.computeVertexNormal();

				/* ---- Phase 5: Build Skin and attach to Shape ---- */
				/* MD5 uses direct joint indexing (skin-local == skeleton-global). */
				std::vector< int32_t > skinJointIndices(jointCount);
				for ( size_t i = 0; i < jointCount; ++i )
				{
					skinJointIndices[i] = static_cast< int32_t >(i);
				}

				Animation::Skin< vertex_data_t > skin{std::move(skinJointIndices), std::move(inverseBindMatrices)};

				result.skeleton = std::move(skeleton);
				result.skin = std::move(skin);

				return true;
			}

	};
}
