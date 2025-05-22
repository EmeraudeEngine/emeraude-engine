/*
 * src/Libs/VertexFactory/FileIO.hpp
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
#include <iostream>
#include <type_traits>

/* Local inclusions for usages. */
#include "Libs/IO/IO.hpp"
#include "FileFormatFBX.hpp"
#include "FileFormatMD2.hpp"
#include "FileFormatMD3.hpp"
#include "FileFormatMD5.hpp"
#include "FileFormatMDL.hpp"
#include "FileFormatNative.hpp"
#include "FileFormatOBJ.hpp"

namespace EmEn::Libs::VertexFactory::FileIO
{
	/**
	 * @brief Reads a file into a shape structure.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param filepath A reference to a filesystem path.
	 * @param shape A writable reference to a shape.
	 * @param readOptions A reference to read options structure. Defaults.
	 * @return bool
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	bool
	read (const std::filesystem::path & filepath, Shape< vertex_data_t, index_data_t > & shape, const ReadOptions & readOptions = {}) requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		if ( !IO::fileExists(filepath) )
		{
			std::cerr << "VertexFactory::FileIO::read(), the file '" << filepath << "' doesn't exist !" "\n";

			return false;
		}

		const auto extension = IO::getFileExtension(filepath, true);

		if ( extension == "emgeo" )
		{
			FileFormatNative< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		if ( extension == "obj" )
		{
			FileFormatOBJ< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		if ( extension == "fbx" )
		{
			FileFormatFBX< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		if ( extension == "mdl" )
		{
			FileFormatMDL< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		if ( extension == "md2" )
		{
			FileFormatMD2< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		if ( extension == "md3" )
		{
			FileFormatMD3< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		if ( extension == "md5mesh" )
		{
			FileFormatMD5< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape, readOptions);
		}

		std::cerr << "VertexFactory::FileIO::read(), the file '" << filepath << "' format is not handled !" "\n";

		return false;
	}

	/**
	 * @briew Writes a shape into a file.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @param shape A reference to the source shape.
	 * @param filepath A reference to a filesystem path.
	 * @param overwrite Overwrite existing file. Default false.
	 * @return bool
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	[[nodiscard]]
	bool
	write (const Shape< vertex_data_t, index_data_t > & shape, const std::filesystem::path & filepath, bool overwrite = false) requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	{
		if ( IO::fileExists(filepath) && !overwrite )
		{
			std::cerr << "VertexFactory::FileIO::write(), the file '" << filepath << "' already exists !" "\n";

			return false;
		}

		const auto extension = IO::getFileExtension(filepath, true);

		if ( extension == "emgeo" )
		{
			FileFormatNative< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		if ( extension == "obj" )
		{
			FileFormatOBJ< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		if ( extension == "fbx" )
		{
			FileFormatFBX< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		if ( extension == "mdl" )
		{
			FileFormatMDL< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		if ( extension == "md2" )
		{
			FileFormatMD2< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		if ( extension == "md3" )
		{
			FileFormatMD3< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		if ( extension == "md5mesh" )
		{
			FileFormatMD5< vertex_data_t, index_data_t > fileFormat{};

			return fileFormat.readFile(filepath, shape);
		}

		std::cerr << "VertexFactory::FileIO::write(), the file '" << filepath << "' format is not handled !" "\n";

		return false;
	}
}
