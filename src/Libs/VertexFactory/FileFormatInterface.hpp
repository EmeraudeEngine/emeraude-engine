/*
 * src/Libs/VertexFactory/FileFormatInterface.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <filesystem>

/* Local inclusions for usages. */
#include "Shape.hpp"

namespace EmEn::Libs::VertexFactory
{
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
	 * @brief File format interface for reading and writing a geometry.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class FileFormatInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			FileFormatInterface (const FileFormatInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			FileFormatInterface (FileFormatInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return FileFormatInterface &
			 */
			FileFormatInterface & operator= (const FileFormatInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return FileFormatInterface &
			 */
			FileFormatInterface & operator= (FileFormatInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the pixmap format.
			 */
			virtual ~FileFormatInterface () = default;

			/**
			 * @brief Reads the geometry from a file.
			 * @param filepath A reference to a filesystem path.
			 * @param geometry A reference to the Geometry.
			 * @param readOptions A reference to a read options structure.
			 * @return bool
			 */
			virtual bool readFile (const std::filesystem::path & filepath, Shape< vertex_data_t, index_data_t > & geometry, const ReadOptions & readOptions) noexcept = 0;

			/**
			 * @brief Writes the geometry to a file.
			 * @param filepath A reference to a filesystem path. Should be accessible.
			 * @param geometry A read-only reference to the Geometry.
			 * @return bool
			 */
			virtual bool writeFile (const std::filesystem::path & filepath, const Shape< vertex_data_t, index_data_t > & geometry) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a pixmap format.
			 */
			FileFormatInterface () noexcept = default;
	};
}
