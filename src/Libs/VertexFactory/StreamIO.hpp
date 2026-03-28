/*
 * src/Libs/VertexFactory/StreamIO.hpp
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
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>

/* Local inclusions for usages. */
#include "Libs/IO/MemoryStream.hpp"
#include "FileFormatNative.hpp"
#include "ShapeLoadResult.hpp"

namespace EmEn::Libs::VertexFactory::StreamIO
{
	/**
	 * @brief Decodes geometry data from a memory buffer into a load result.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param data A reference to the source byte vector.
	 * @param result A writable reference to the destination load result.
	 * @param readOptions A reference to read options. Defaults.
	 * @return bool
	 * @note Only supports the native (ee3d) format for stream-based reading.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	bool
	read (const std::vector< std::byte > & data, ShapeLoadResult< vertex_data_t, index_data_t > & result, const ReadOptions & readOptions = {})
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		if ( data.empty() )
		{
			std::cerr << "[VertexFactory::StreamIO] read(), empty input buffer !\n";

			return false;
		}

		IO::MemoryStream stream{data};

		FileFormatNative< vertex_data_t, index_data_t > fileFormat{};

		return fileFormat.readStream(stream, result, readOptions);
	}

	/**
	 * @brief Encodes a shape into a memory buffer using native format.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param shape A reference to the source shape.
	 * @param output A reference to the destination byte vector. Will be cleared before writing.
	 * @param writeOptions A reference to write options. Defaults.
	 * @return bool
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	bool
	write (const Shape< vertex_data_t, index_data_t > & shape, std::vector< std::byte > & output, const WriteOptions & writeOptions = {})
		requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		output.clear();

		IO::MemoryStream stream{output};

		FileFormatNative< vertex_data_t, index_data_t > fileFormat{};

		return fileFormat.writeStream(stream, shape, writeOptions);
	}
}
