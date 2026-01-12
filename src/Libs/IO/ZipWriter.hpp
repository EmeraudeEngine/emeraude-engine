/*
 * src/Libs/IO/ZipWriter.hpp
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
#include <filesystem>
#include <vector>

/* Third-party inclusions. */
#include <zip.h>

namespace EmEn::Libs::IO
{
	/**
	 * @brief Class to create a ZIP archive.
	 */
	class ZipWriter final
	{
		public:

			static constexpr auto ClassId{"ZipWriter"};

			/**
			 * @brief Constructs a zip archive writer.
			 * @param filepath A reference to a path.
			 */
			explicit ZipWriter (std::filesystem::path filepath) noexcept;

			/**
			 * @brief Changes the zip archive filepath.
			 * @param filepath A reference to a path.
			 * @return void
			 */
			void
			setFilepath (const std::filesystem::path & filepath) noexcept
			{
				this->closeArchive();

				m_filepath = filepath;
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
			 * @brief Adds a file to compress into the archive with a custom entry name.
			 * @param path A reference to a path.
			 * @param entryName A reference to a string.
			 * @return bool
			 */
			bool addFilepathToSources (const std::filesystem::path & path, const std::string & entryName) noexcept;

			/**
			 * @brief Adds a file to compress into the archive.
			 * @param path A reference to a path.
			 * @return bool
			 */
			bool
			addFilepathToSources (const std::filesystem::path & path) noexcept
			{
				return this->addFilepathToSources(path, path.filename().string());
			}

			/**
			 * @brief Adds a directory to compress into the archive.
			 * @param path A reference to a path.
			 * @return bool
			 */
			bool addDirectoryToSources (const std::filesystem::path & path) noexcept;

			/**
			 * @brief Creates the archive.
			 * @return bool
			 */
			bool create () noexcept;

		private:

			/**
			 * @brief Read the date of a source file to get it ready for the zip archive.
			 * @param filepath A reference to a path.
			 * @param fileContent A writable reference to a vector.
			 * @return bool
			 */
			static bool readSource (const std::filesystem::path & filepath, std::vector< char > & fileContent) noexcept;

			/**
			 * @brief Opens the ZIP archive.
			 * @return bool
			 */
			[[nodiscard]]
			bool openArchive () noexcept;

			/**
			 * @brief Closes the archive and release LibZip resources.
			 * @return void
			 */
			void closeArchive () noexcept;

			/**
			 * @brief Checks whether a filepath exists in source.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			[[nodiscard]]
			bool sourceExists (const std::filesystem::path & filepath) const noexcept;

			zip * m_zip{nullptr};
			std::filesystem::path m_filepath;
			std::vector< std::pair< std::filesystem::path, std::string > > m_sources;
	};
}
