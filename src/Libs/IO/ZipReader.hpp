/*
 * src/Libs/IO/ZipReader.hpp
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
#include <filesystem>
#include <vector>
#include <string>

/* Third-party inclusions. */
#include <zip.h>

namespace EmEn::Libs::IO
{
	/**
	 * @brief Class to open a ZIP archive.
	 */
	class ZipReader final
	{
		public:

			static constexpr auto ClassId{"ZipReader"};

			/**
			 * @brief Constructs a zip archive reader.
			 * @param filepath A reference to a path.
			 */
			explicit ZipReader (std::filesystem::path filepath) noexcept;

			/**
			 * @brief Destructs the zip archive reader.
			 */
			~ZipReader ();

			/**
			 * @brief Changes the zip archive filepath.
			 * @param filepath A reference to a path.
			 * @return void
			 */
			void
			setFilepath (const std::filesystem::path & filepath) noexcept
			{
				this->close();

				m_filepath = filepath;
			}

			/**
			 * @brief Opens the archive.
			 * @return bool
			 */
			bool open () noexcept;

			/**
			 * @brief Closes the archive and release LibZip resources.
			 * @return void
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the archive is open.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOpen () const noexcept
			{
				return m_zip != nullptr;
			}

			/**
			 * @brief Returns the filepath of the archive.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filepath () const noexcept
			{
				return m_filepath;
			}

			/**
			 * @brief Returns the list of entry inside the archive.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			entries () const noexcept
			{
				return m_entryNames;
			}

			/**
			 * @brief Extracts a specific entry from the ZIP archive to a buffer.
			 * @param entryName A reference to a string.
			 * @param buffer A reference to a vector.
			 * @return bool
			 */
			bool extract (const std::string & entryName, std::vector< char > & buffer) const noexcept;

			/**
			 * @brief Extracts a specific entry from the ZIP archive to a directory.
			 * @param entryName A reference to a string.
			 * @param destinationPath A reference to a path.
			 * @param overwrite Erase an existing file with the same name in the destination path. Default false.
			 * @return bool
			 */
			[[nodiscard]]
			bool extract (const std::string & entryName, const std::filesystem::path & destinationPath, bool overwrite = false) const noexcept;

			/**
			 * @brief Extracts all files present in the archive to a directory.
			 * @param destinationPath A reference to a path.
			 * @param overwrite Erase every existing file with the same name in the destination path. Default false.
			 * @return bool
			 */
			[[nodiscard]]
			bool extractAll (const std::filesystem::path & destinationPath, bool overwrite = false) const noexcept;

			/**
			 * @brief Returns whether file is an archive file by testing it.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isArchiveFile (const std::filesystem::path & filepath) noexcept;

		private:

			/**
			 * @brief Opens the ZIP archive.
			 * @return bool
			 */
			[[nodiscard]]
			bool openArchive () noexcept;

			/**
			 * @brief Read the information about the archive to get entries.
			 * @return void
			 */
			void readArchive () noexcept;

			zip * m_zip{nullptr};
			std::filesystem::path m_filepath;
			std::vector< std::string > m_entryNames;
	};
}
