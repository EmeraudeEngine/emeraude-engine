/*
 * src/Libs/VertexFactory/FileFormatInterface.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>

/* Local inclusions for usages. */
#include "Libs/IO/ByteStream.hpp"
#include "Shape.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief Options for reading/decoding geometry data.
	 */
	struct ReadOptions
	{
		float scaleFactor = 1.0F;
		bool flipXAxis = false;
		bool flipYAxis = false;
		bool flipZAxis = false;
		bool requestNormal = false;
		bool requestTangentSpace = false;
		bool requestTextureCoordinates = false;
		bool requestVertexColor = false;
	};

	/**
	 * @brief Options for writing/encoding geometry data.
	 */
	struct WriteOptions
	{

	};

	/**
	 * @brief Stream-based format interface for reading and writing geometry data.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatInterface
	{
		public:

			FileFormatInterface (const FileFormatInterface &) noexcept = default;
			FileFormatInterface (FileFormatInterface &&) noexcept = default;
			FileFormatInterface & operator= (const FileFormatInterface &) noexcept = default;
			FileFormatInterface & operator= (FileFormatInterface &&) noexcept = default;
			virtual ~FileFormatInterface () = default;

			/**
			 * @brief Reads geometry data from a byte stream into a shape.
			 * @param stream A reference to the input byte stream.
			 * @param geometry A reference to the destination shape.
			 * @param readOptions A reference to read options.
			 * @return bool
			 */
			virtual bool readStream (IO::ByteStream & stream, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & readOptions) noexcept = 0;

			/**
			 * @brief Writes geometry data from a shape into a byte stream.
			 * @param stream A reference to the output byte stream.
			 * @param geometry A read-only reference to the source shape.
			 * @param writeOptions A reference to write options.
			 * @return bool
			 */
			virtual bool writeStream (IO::ByteStream & stream, const Shape< vertex_data_t, index_data_t > & geometry, const WriteOptions & writeOptions = {}) const noexcept = 0;

		protected:

			FileFormatInterface () noexcept = default;
	};
}
