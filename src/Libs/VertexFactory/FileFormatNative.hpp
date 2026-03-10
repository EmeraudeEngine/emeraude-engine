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

/* STL inclusions. */
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

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

			FileFormatNative () noexcept = default;

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & /*readOptions*/) noexcept override
			{
				geometry.clear();

				if ( !stream.isOpen() )
				{
					std::cerr << "[VertexFactory::FileFormatNative] readStream(), stream is not open !\n";

					return false;
				}

				/* 1. Read Header (32 bytes) */
				char header[32] = {0};

				if ( !stream.read(header, 32) )
				{
					std::cerr << "[VertexFactory::FileFormatNative] readStream(), unable to read header !\n";

					return false;
				}

				/* Check Magic "EE3D_V1" (8 bytes including null) */
				if ( std::string(header, 7) != Magic )
				{
					std::cerr << "[VertexFactory::FileFormatNative] readStream(), invalid magic !\n";

					return false;
				}

				/* Check Version (2 bytes) at offset 8 */
				uint16_t version = 0;
				std::memcpy(&version, &header[8], sizeof(uint16_t));

				if ( version != 1 )
				{
					std::cerr << "[VertexFactory::FileFormatNative] readStream(), unsupported version " << version << " !\n";

					return false;
				}

				/* Check Precision (2 bytes) at offset 10 & 11 */
				const auto vPrecision = static_cast< uint8_t >(header[10]);
				const auto iPrecision = static_cast< uint8_t >(header[11]);

				if ( vPrecision != sizeof(vertex_data_t) || iPrecision != sizeof(index_data_t) )
				{
					std::cerr << "[VertexFactory::FileFormatNative] readStream(), precision mismatch !\n";

					return false;
				}

				/* 2. Read Metadata (Counts) - 3 * 8 bytes = 24 bytes */
				uint64_t counts[3] = {0, 0, 0};

				if ( !stream.read(counts, 3 * sizeof(uint64_t)) )
				{
					std::cerr << "[VertexFactory::FileFormatNative] readStream(), unable to read metadata !\n";

					return false;
				}

				const uint64_t vertexCount = counts[0];
				const uint64_t triangleCount = counts[1];
				const uint64_t colorCount = counts[2];

				/* 3. Resize Geometry */
				auto & vertices = geometry.vertices();
				auto & triangles = geometry.triangles();
				auto & colors = geometry.vertexColors();

				vertices.resize(vertexCount);
				triangles.resize(triangleCount);
				colors.resize(colorCount);

				/* 4. Read Data Blobs */
				if ( vertexCount > 0 )
				{
					if ( !stream.read(vertices.data(), vertexCount * sizeof(ShapeVertex< vertex_data_t >)) )
					{
						std::cerr << "[VertexFactory::FileFormatNative] readStream(), failed to read vertices !\n";

						return false;
					}
				}

				if ( triangleCount > 0 )
				{
					if ( !stream.read(triangles.data(), triangleCount * sizeof(ShapeTriangle< vertex_data_t, index_data_t >)) )
					{
						std::cerr << "[VertexFactory::FileFormatNative] readStream(), failed to read triangles !\n";

						return false;
					}
				}

				if ( colorCount > 0 )
				{
					if ( !stream.read(colors.data(), colorCount * sizeof(Math::Vector< 4, vertex_data_t >)) )
					{
						std::cerr << "[VertexFactory::FileFormatNative] readStream(), failed to read vertex colors !\n";

						return false;
					}
				}

				return true;
			}

			/** @copydoc EmEn::Libs::VertexFactory::FileFormatInterface::writeStream() */
			[[nodiscard]]
			bool
			writeStream (IO::ByteStream & stream, const Shape< vertex_data_t, index_data_t > & geometry, const WriteOptions & /*writeOptions*/) const noexcept override
			{
				if ( !geometry.isValid() )
				{
					std::cerr << "[VertexFactory::FileFormatNative] writeStream(), geometry is invalid !\n";

					return false;
				}

				if ( !stream.isOpen() )
				{
					std::cerr << "[VertexFactory::FileFormatNative] writeStream(), stream is not open !\n";

					return false;
				}

				/* 1. Header (32 bytes) */
				char header[32] = {0};

				std::memcpy(header, Magic, 7);

				uint16_t version = 1;
				std::memcpy(&header[8], &version, sizeof(uint16_t));

				header[10] = static_cast< char >(sizeof(vertex_data_t));
				header[11] = static_cast< char >(sizeof(index_data_t));

				if ( !stream.write(header, 32) )
				{
					return false;
				}

				/* 2. Metadata (Counts) */
				uint64_t counts[3];
				counts[0] = static_cast< uint64_t >(geometry.vertices().size());
				counts[1] = static_cast< uint64_t >(geometry.triangles().size());
				counts[2] = static_cast< uint64_t >(geometry.vertexColors().size());

				if ( !stream.write(counts, 3 * sizeof(uint64_t)) )
				{
					return false;
				}

				/* 3. Data Blobs */
				if ( counts[0] > 0 )
				{
					if ( !stream.write(geometry.vertices().data(), counts[0] * sizeof(ShapeVertex< vertex_data_t >)) )
					{
						return false;
					}
				}

				if ( counts[1] > 0 )
				{
					if ( !stream.write(geometry.triangles().data(), counts[1] * sizeof(ShapeTriangle< vertex_data_t, index_data_t >)) )
					{
						return false;
					}
				}

				if ( counts[2] > 0 )
				{
					if ( !stream.write(geometry.vertexColors().data(), counts[2] * sizeof(Math::Vector< 4, vertex_data_t >)) )
					{
						return false;
					}
				}

				return true;
			}
	};
}
