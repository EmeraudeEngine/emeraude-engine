/*
 * src/Libs/IO/ZipReader.cpp
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

#include "ZipReader.hpp"

/* STL inclusions. */
#include <iostream>
#include <fstream>
#include <algorithm>

namespace EmEn::Libs::IO
{
	ZipReader::ZipReader (std::filesystem::path filepath) noexcept
		: m_filepath(std::move(filepath))
	{

	}

	ZipReader::~ZipReader ()
	{
		this->close();
	}

	bool
	ZipReader::isArchiveFile (const std::filesystem::path & filepath) noexcept
	{
		if ( !std::filesystem::exists(filepath) )
		{
			return false;
		}

		int errorCode = 0;

		auto * archive = zip_open(filepath.string().data(), ZIP_RDONLY | ZIP_CHECKCONS, &errorCode);

		if ( archive == nullptr )
		{
			return false;
		}

		zip_close(archive);

		return true;
	}

	bool
	ZipReader::open () noexcept
	{
		if ( !std::filesystem::exists(m_filepath) )
		{
			std::cerr << ClassId << " : The archive file " << m_filepath << " doesn't exists !" "\n";

			return false;
		}

		if ( !this->openArchive() )
		{
			std::cerr << ClassId << " : Unable to open the archive " << m_filepath << " !" "\n";

			return false;
		}

		this->readArchive();

		return !m_entryNames.empty();
	}

	void
	ZipReader::close () noexcept
	{
		if ( m_zip != nullptr )
		{
			zip_close(m_zip);

			m_zip = nullptr;
		}

		m_entryNames.clear();
	}

	bool
	ZipReader::extract (const std::string & entryName, std::vector< char > & buffer) const noexcept
	{
		if ( !this->isOpen() )
		{
			return false;
		}

		struct zip_stat stat{};
		zip_stat_init(&stat);
		zip_stat(m_zip, entryName.data(), 0, &stat);

		if ( stat.size == 0 )
		{
			std::cerr << ClassId << " : The entry " << entryName << " doesn't exists !" "\n";

			return false;
		}

		buffer.resize(stat.size);

		zip_file * compressedFile = zip_fopen(m_zip, entryName.data(), 0);
		zip_fread(compressedFile, buffer.data(), stat.size);
		zip_fclose(compressedFile);

		return true;
	}

	bool
	ZipReader::extract (const std::string & entryName, const std::filesystem::path & destinationPath, bool overwrite) const noexcept
	{
		if ( std::ranges::find(m_entryNames, entryName) == m_entryNames.cend() )
		{
			std::cerr << ClassId << " : The entry " << entryName << " doesn't exists !" "\n";

			return false;
		}

		/* NOTE: Complete the final destination. This can hold subdirectories. */
		std::filesystem::path filepath = destinationPath;
		filepath.append(entryName);

		/* NOTE: Check the final destination. */
		const auto finalDirectory = filepath.parent_path();

		if ( std::filesystem::exists(finalDirectory) )
		{
			if ( !is_directory(finalDirectory) )
			{
				std::cerr << ClassId << " : Unable to use the destination directory " << finalDirectory << " !" "\n";

				return false;
			}

			if ( std::filesystem::exists(filepath) && !overwrite )
			{
				std::cerr << ClassId << " : The destination file " << filepath << " already exists !" "\n";

				return false;
			}
		}
		else
		{
			if ( !std::filesystem::create_directories(finalDirectory) )
			{
				std::cerr << ClassId << " : Unable to create the destination directory " << finalDirectory << " !" "\n";

				return false;
			}
		}

		std::ofstream file{filepath.string(), std::ios::binary | std::ios::trunc};

		if ( !file.is_open() )
		{
			std::cerr << ClassId << " : Unable to open the file " << filepath << " for writing !" "\n";

			return false;
		}

		std::vector< char > buffer;

		if ( !this->extract(entryName, buffer) )
		{
			std::cerr << ClassId << " : Unable to extract the content buffer !" "\n";

			return false;
		}

		file.write(buffer.data(), static_cast< std::streamsize >(buffer.size()));

		return true;
	}

	bool
	ZipReader::extractAll (const std::filesystem::path & destinationPath, bool overwrite) const noexcept
	{
		size_t fileExtracted = 0;

		/* NOTE: Check the main destination. */
		if ( std::filesystem::exists(destinationPath) )
		{
			if ( !is_directory(destinationPath) )
			{
				std::cerr << ClassId << " : Unable to use the destination directory " << destinationPath << " !" "\n";

				return false;
			}
		}
		else
		{
			if ( !std::filesystem::create_directories(destinationPath) )
			{
				std::cerr << ClassId << " : Unable to create the destination directory " << destinationPath << " !" "\n";

				return false;
			}
		}

		for ( const auto & entryName : m_entryNames )
		{
			if ( this->extract(entryName, destinationPath, overwrite) )
			{
				fileExtracted++;
			}
		}

		return fileExtracted == m_entryNames.size();
	}

	bool
	ZipReader::openArchive () noexcept
	{
		int errorCode = 0;

		m_zip = zip_open(m_filepath.string().data(), 0, &errorCode);

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
	ZipReader::readArchive () noexcept
	{
		const auto entryCount = zip_get_num_entries(m_zip, 0);

		struct zip_stat stat{};

		for ( zip_int64_t index = 0; index < entryCount; index++)
		{
			if ( zip_stat_index(m_zip, index, 0, &stat ) == 0 )
			{
				m_entryNames.emplace_back(stat.name);
			}
		}
	}
}
