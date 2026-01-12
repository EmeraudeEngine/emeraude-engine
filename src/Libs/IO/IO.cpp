/*
 * src/Libs/IO/IO.cpp
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

#include "IO.hpp"

/* STL inclusions. */
#include <algorithm>

/* Platform libraries. */
#if IS_LINUX || IS_MACOS
	#include <unistd.h>
#endif

#if IS_WINDOWS
	#include <Windows.h>
	#pragma comment(lib, "advapi32.lib")
#endif

namespace EmEn::Libs::IO
{
	bool
	fileExists (const std::filesystem::path & filepath) noexcept
	{
		if ( filepath.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		if ( !std::filesystem::exists(filepath, errorCode) ) [[unlikely]]
		{
			return false;
		}

		const auto result = std::filesystem::is_regular_file(filepath, errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to check the existence of the file " << filepath << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";

			return false;
		}

		return result;
	}

	size_t
	filesize (const std::filesystem::path & filepath) noexcept
	{
		std::error_code errorCode{};

		const auto size = std::filesystem::file_size(filepath, errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to get the size of the file " << filepath << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";
		}

		return size;
	}

	bool
	createFile (const std::filesystem::path & filepath) noexcept
	{
		if ( filepath.empty() ) [[unlikely]]
		{
			return false;
		}

		std::ofstream file{filepath};

		return file.is_open();
	}

	bool
	eraseFile (const std::filesystem::path & filepath) noexcept
	{
		if ( filepath.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		if ( !std::filesystem::is_regular_file(filepath, errorCode) ) [[unlikely]]
		{
			if ( errorCode.value() > 0 ) [[unlikely]]
			{
				std::cerr << __PRETTY_FUNCTION__ << ", unable to check the path " << filepath << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";
			}
			else
			{
				std::cerr << __PRETTY_FUNCTION__ << ", the path " << filepath << " is not a regular file !" << "\n";
			}

			return false;
		}

		std::filesystem::remove(filepath, errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to delete the file " << filepath << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";

			return false;
		}

		return true;
	}

	bool
	directoryExists (const std::filesystem::path & path) noexcept
	{
		if ( path.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		const auto result = std::filesystem::is_directory(path, errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			/* NOTE: We don't need to print the error message if the directory do not exist. */
			if ( errorCode.value() != 2 ) [[unlikely]]
			{
				std::cerr << __PRETTY_FUNCTION__ << ", unable to check if the path " << path << " is a directory (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";
			}

			return false;
		}

		return result;
	}

	bool
	isDirectoryContentEmpty (const std::filesystem::path & path) noexcept
	{
		if ( path.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		const auto result = std::filesystem::is_empty(path, errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to check the content of directory " << path << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";
		}

		return result;
	}

	std::vector< std::filesystem::path >
	directoryEntries (const std::filesystem::path & path) noexcept
	{
		std::vector< std::filesystem::path > entries{};

		std::error_code errorCode{};

		for ( const auto & entry : std::filesystem::directory_iterator(path, errorCode) )
		{
			entries.emplace_back(entry.path());
		}

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", an error occurs when reading path " << path << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";
		}

		return entries;
	}

	bool
	createDirectory (const std::filesystem::path & path, bool removeFileSection) noexcept
	{
		if ( path.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		if ( removeFileSection )
		{
			const auto parentPath = path.parent_path();

			std::filesystem::create_directories(parentPath, errorCode);
		}
		else
		{
			std::filesystem::create_directories(path, errorCode);
		}

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to create the path " << path << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";

			return false;
		}

		return true;
	}

	bool
	eraseDirectory (const std::filesystem::path & path, bool recursive) noexcept
	{
		if ( path.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		if ( !std::filesystem::is_directory(path, errorCode) ) [[unlikely]]
		{
			if ( errorCode.value() > 0 ) [[unlikely]]
			{
				std::cerr << __PRETTY_FUNCTION__ << ", unable to check the path " << path << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";
			}
			else
			{
				std::cerr << __PRETTY_FUNCTION__ << ", the path " << path << " is not a directory !" << "\n";
			}

			return false;
		}

		if ( recursive )
		{
			std::filesystem::remove_all(path, errorCode);
		}
		else
		{
			std::filesystem::remove(path, errorCode);
		}

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", Unable to delete the directory " << path << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";

			return false;
		}

		return true;
	}

	std::filesystem::path
	getCurrentWorkingDirectory () noexcept
	{
		std::error_code errorCode{};

		const auto path = std::filesystem::current_path(errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to get the current working directory (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";

			return {};
		}

		return path;
	}

	bool
	exists (const std::filesystem::path & path) noexcept
	{
		if ( path.empty() ) [[unlikely]]
		{
			return false;
		}

		std::error_code errorCode{};

		const auto result = std::filesystem::exists(path, errorCode);

		if ( errorCode.value() > 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to check the existence of the entry " << path << " (" << errorCode.value() << ':' << errorCode.message() << ")" "\n";

			return false;
		}

		return result;
	}

	bool
	readable (const std::filesystem::path & path) noexcept
	{
		if ( path.empty() )
		{
			return false;
		}

#if IS_LINUX || IS_MACOS
		return access(path.c_str(), R_OK) == 0;
#elif IS_WINDOWS
		return true; // TODO ...
#else
		std::cerr << __PRETTY_FUNCTION__ << ", unable to check permission !" "\n";

		return false;
#endif
	}

	bool
	writable (const std::filesystem::path & path) noexcept
	{
		if ( path.empty() )
		{
			return false;
		}

#if IS_LINUX || IS_MACOS
		return access(path.c_str(), W_OK) == 0;
#elif IS_WINDOWS
		return true; // TODO ...
#else
		std::cerr << __PRETTY_FUNCTION__ << ", unable to check permission !" "\n";

		return false;
#endif
	}

	bool
	executable (const std::filesystem::path & path) noexcept
	{
		if ( path.empty() )
		{
			return false;
		}

#if IS_LINUX || IS_MACOS
		return access(path.c_str(), X_OK) == 0;
#elif IS_WINDOWS
		return true; // TODO ...
#else
		std::cerr << __PRETTY_FUNCTION__ << ", unable to check permission !" "\n";

		return false;
#endif
	}

	std::string
	getFileExtension (const std::filesystem::path & filepath, bool forceToLower) noexcept
	{
		if ( !filepath.has_extension() )
		{
			return {};
		}

		auto extension = filepath.extension().string().substr(1);

		if ( forceToLower )
		{
			std::ranges::transform(extension, extension.begin(), ::tolower);
		}

		return extension;
	}

	bool
	fileGetContents (const std::filesystem::path & filepath, std::string & content) noexcept
	{
		if ( filepath.empty() ) [[unlikely]]
		{
			return false;
		}

		std::ifstream file{filepath, std::ios::binary | std::ios::ate};

		if ( !file.is_open() ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to read " << filepath << " file." "\n";

			return false;
		}

		/* NOTE: Read the file size. */
		const auto bytes = file.tellg();

		if ( bytes < 0 ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to get the size of " << filepath << " file." "\n";

			return false;
		}

		file.seekg(0, std::ifstream::beg);

		content.resize(static_cast< size_t >(bytes));

		file.read(content.data(), static_cast< std::streamsize >(content.size()));

		if ( !file ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", error reading " << filepath << " file." "\n";

			return false;
		}

		return true;
	}

	bool
	filePutContents (const std::filesystem::path & filepath, std::string_view content, bool append, bool createDirectories) noexcept
	{
		if ( filepath.empty() ) [[unlikely]]
		{
			return false;
		}

		if ( createDirectories && !IO::createDirectory(filepath, true) ) [[unlikely]]
		{
			return false;
		}

		std::ofstream file{filepath, std::ios::binary | (append ? std::ios::app : std::ios::trunc)};

		if ( !file.is_open() ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to write into " << filepath << " file." "\n";

			return false;
		}

		file.write(content.data(), static_cast< std::streamsize >(content.size()));

		if ( !file ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", error writing to " << filepath << " file." "\n";

			return false;
		}

		return true;
	}
}
