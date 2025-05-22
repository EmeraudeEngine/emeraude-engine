/*
 * src/Libs/IO/ZipWriter.cpp
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

#include "ZipWriter.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iostream>
#include <fstream>

namespace EmEn::Libs::IO
{
	ZipWriter::ZipWriter (std::filesystem::path filepath) noexcept
		: m_filepath(std::move(filepath))
	{

	}

	bool
	ZipWriter::sourceExists (const std::filesystem::path & filepath) const noexcept
	{
		return std::ranges::any_of(m_sources, [&filepath] (const std::pair< std::filesystem::path, std::string > & source) {
			return source.first == filepath;
		});
	}

	bool
	ZipWriter::addFilepathToSources (const std::filesystem::path & path, const std::string & entryName) noexcept
	{
		if ( !std::filesystem::is_regular_file(path) && !std::filesystem::exists(path) )
		{
			std::cerr << ClassId << " : The path " << path << " is not a regular file or doesn't exists !" "\n";

			return false;
		}

		if ( ZipWriter::sourceExists(path) )
		{
			std::cerr << ClassId << " : The file " << path << " already exists in source !" "\n";

			return false;
		}

		m_sources.emplace_back(path, entryName);

		return true;
	}

	bool
	ZipWriter::addDirectoryToSources (const std::filesystem::path & path) noexcept
	{
		if ( !std::filesystem::is_directory(path) && !std::filesystem::exists(path) )
		{
			std::cerr << ClassId << " : The path " << path << " is not a directory or doesn't exists !" "\n";

			return false;
		}

		const auto root = path.string();

		for ( const auto & entry : std::filesystem::recursive_directory_iterator(path) )
		{
			if ( entry.is_regular_file() )
			{
				if ( ZipWriter::sourceExists(entry) )
				{
					std::cerr << ClassId << " : The file " << entry << " already exists in source !" "\n";

					return false;
				}

				auto filepathString = entry.path().string();

				const auto entryName = filepathString.replace(filepathString.find(root), root.length(), "");

				m_sources.emplace_back(entry, entryName);
			}
		}

		return true;
	}

	bool
	ZipWriter::create () noexcept
	{
		if ( std::filesystem::exists(m_filepath) )
		{
			std::cerr << ClassId << " : The archive file " << m_filepath << " already exists !" "\n";

			return false;
		}

		if ( !this->openArchive() )
		{
			std::cerr << ClassId << " : Unable to create the archive " << m_filepath << " !" "\n";

			return false;
		}

		bool success = true;
		std::vector< char > buffer;

		for ( const auto & [filepath, entryName] : m_sources )
		{
			/* Read the file content. */
			if ( !ZipWriter::readSource(filepath, buffer) )
			{
				std::cerr << ClassId << " : Unable to open file " << filepath << " !" "\n";

				success = false;

				break;
			}

			/* Set source. */
			zip_source * source = zip_source_buffer(m_zip, buffer.data(), buffer.size(), 0);

			if ( source == nullptr )
			{
				std::cerr << ClassId << " : Failed to create source buffer ! Error : " << zip_strerror(m_zip) << "\n";

				success = false;

				break;
			}

			/* Add the source to archive. */
			const auto index = zip_file_add(m_zip, entryName.data(), source, ZIP_FL_OVERWRITE);

			if ( index < 0 )
			{
				std::cerr << ClassId << " : Failed to add file to archive ! Error : " << zip_strerror(m_zip) << "\n";

				success = false;

				break;
			}
		}

		this->closeArchive();

		return success;
	}

	bool
	ZipWriter::readSource (const std::filesystem::path & filepath, std::vector< char > & fileContent) noexcept
	{
		std::ifstream file{filepath.string(), std::ios::binary};

		if ( !file.is_open() )
		{
			return false;
		}

		/* NOTE: Read the file size. */
		file.seekg(0, std::ifstream::end);
		const auto bytes = file.tellg();
		file.seekg(0, std::ifstream::beg);

		fileContent.resize(bytes);

		file.read(fileContent.data(), static_cast< std::streamsize >(fileContent.size()));
		file.close();

		return true;
	}

	bool
	ZipWriter::openArchive () noexcept
	{
		int errorCode = 0;

		m_zip = zip_open(m_filepath.string().data(), ZIP_CREATE, &errorCode);

		if ( m_zip == nullptr )
		{
			zip_error_t error;
			zip_error_init_with_code(&error, errorCode);

			std::cerr << ClassId << " : Unable to init LibZip. Error : " << zip_error_strerror(&error) << " !" "\n";

			zip_error_fini(&error);

			return false;
		}

		return true;
	}

	void
	ZipWriter::closeArchive () noexcept
	{
		if ( m_zip != nullptr )
		{
			zip_close(m_zip);

			m_zip = nullptr;
		}

		m_sources.clear();
	}
}
