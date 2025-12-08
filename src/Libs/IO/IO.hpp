/*
 * src/Libs/IO/IO.hpp
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

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <concepts>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string_view>
#include <vector>

namespace EmEn::Libs::IO
{
#if IS_WINDOWS
	constexpr char Separator = '\\';
#else
	constexpr char Separator = '/';
#endif

	/**
	 * @brief Checks if a file exists on disk.
	 *
	 * Verifies that the specified path exists and is a regular file (not a directory,
	 * symlink, or other special file type).
	 *
	 * @param filepath Path to the file to check.
	 * @return True if the file exists and is a regular file, false otherwise.
	 * @note Returns false if the path is empty or if an error occurs during the check.
	 */
	[[nodiscard]]
	bool fileExists (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Returns the size of a file in bytes.
	 *
	 * Retrieves the size of the specified file using the filesystem API.
	 * Errors are logged to stderr.
	 *
	 * @param filepath Path to the file.
	 * @return Size of the file in bytes, or 0 if an error occurs.
	 * @note Returns 0 if the file does not exist or cannot be accessed.
	 */
	[[nodiscard]]
	size_t filesize (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Creates an empty file at the specified location.
	 *
	 * Creates a new empty file at the given path. If the file already exists,
	 * it will be truncated (emptied).
	 *
	 * @param filepath Path where the file should be created.
	 * @return True if the file was successfully created or opened, false otherwise.
	 * @note Returns false if the path is empty. Parent directories must exist.
	 */
	bool createFile (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Deletes a file from disk.
	 *
	 * Permanently removes the specified file. The path must point to a regular file,
	 * not a directory or special file.
	 *
	 * @param filepath Path to the file to delete.
	 * @return True if the file was successfully deleted, false otherwise.
	 * @warning This is a destructive operation that cannot be undone.
	 * @note Returns false if the path is empty, not a regular file, or deletion fails.
	 */
	bool eraseFile (const std::filesystem::path & filepath) noexcept;

	/**
	 * @brief Checks if a directory exists on disk.
	 *
	 * Verifies that the specified path exists and is a directory.
	 *
	 * @param path Path to the directory to check.
	 * @return True if the path exists and is a directory, false otherwise.
	 * @note Returns false if the path is empty or points to a non-directory.
	 *       Error messages are suppressed for non-existent directories (error code 2).
	 */
	[[nodiscard]]
	bool directoryExists (const std::filesystem::path & path) noexcept;

	/**
	 * @brief Checks whether a directory is empty.
	 *
	 * Determines if the specified directory contains no files or subdirectories.
	 *
	 * @param path Path to the directory to check.
	 * @return True if the directory is empty, false otherwise.
	 * @note Returns false if the path is empty or an error occurs.
	 *       The directory must exist for this function to work correctly.
	 */
	[[nodiscard]]
	bool isDirectoryContentEmpty (const std::filesystem::path & path) noexcept;
	
	/**
	 * @brief Returns a list of all entries in a directory.
	 *
	 * Retrieves all files and subdirectories within the specified directory.
	 * This is a non-recursive operation (does not traverse subdirectories).
	 *
	 * @param path Path to the directory to list.
	 * @return Vector containing paths to all entries in the directory. Returns empty vector if an error occurs.
	 * @note Errors during iteration are logged to stderr. The directory must exist.
	 */
	[[nodiscard]]
	std::vector< std::filesystem::path > directoryEntries (const std::filesystem::path & path) noexcept;

	/**
	 * @brief Creates a directory and all necessary parent directories.
	 *
	 * Creates the specified directory along with any missing parent directories
	 * in the path. If the directory already exists, the operation succeeds.
	 *
	 * @param path Path to the directory to create.
	 * @param removeFileSection If true, treats the last component as a filename and creates
	 *        only the parent directory path. Useful when path includes a filename. Default false.
	 * @return True if the directory was created or already exists, false on error.
	 * @note Returns false if the path is empty or directory creation fails.
	 */
	bool createDirectory (const std::filesystem::path & path, bool removeFileSection = false) noexcept;

	/**
	 * @brief Deletes a directory from disk.
	 *
	 * Removes the specified directory. By default, only empty directories can be removed.
	 * With recursive mode enabled, all contents are deleted.
	 *
	 * @param path Path to the directory to delete.
	 * @param recursive If true, deletes the directory and all its contents recursively.
	 *        If false, only deletes empty directories. Default false.
	 * @return True if the directory was successfully deleted, false otherwise.
	 * @warning This is a destructive operation that cannot be undone. Use recursive mode with caution.
	 * @note Returns false if the path is empty, not a directory, or deletion fails.
	 */
	bool eraseDirectory (const std::filesystem::path & path, bool recursive = false) noexcept;

	/**
	 * @brief Returns the current working directory of the process.
	 *
	 * Retrieves the absolute path of the directory from which the application
	 * was launched or to which it has changed.
	 *
	 * @return Path to the current working directory. Returns empty path if an error occurs.
	 * @note Errors are logged to stderr.
	 */
	[[nodiscard]]
	std::filesystem::path getCurrentWorkingDirectory () noexcept;

	/**
	 * @brief Checks if a path exists on disk.
	 *
	 * Verifies that the specified path exists, regardless of whether it is a file,
	 * directory, symlink, or other filesystem entity.
	 *
	 * @param path Path to check for existence.
	 * @return True if the path exists, false otherwise.
	 * @note Returns false if the path is empty or an error occurs.
	 * @see fileExists, directoryExists
	 */
	[[nodiscard]]
	bool exists (const std::filesystem::path & path) noexcept;

	/**
	 * @brief Checks whether the application has read permission for the path.
	 *
	 * Tests if the current process has permission to read from the specified path.
	 *
	 * @param path Path to check for read permission.
	 * @return True if the path is readable by the application, false otherwise.
	 * @note Returns false if the path is empty. On Linux/macOS, uses access() with R_OK.
	 * @warning Windows implementation always returns true (permission check not implemented).
	 * @todo Implement proper permission checking for Windows platform.
	 * @see writable, executable
	 */
	[[nodiscard]]
	bool readable (const std::filesystem::path & path) noexcept;

	/**
	 * @brief Checks whether the application has write permission for the path.
	 *
	 * Tests if the current process has permission to write to the specified path.
	 *
	 * @param path Path to check for write permission.
	 * @return True if the path is writable by the application, false otherwise.
	 * @note Returns false if the path is empty. On Linux/macOS, uses access() with W_OK.
	 * @warning Windows implementation always returns true (permission check not implemented).
	 * @todo Implement proper permission checking for Windows platform.
	 * @see readable, executable
	 */
	[[nodiscard]]
	bool writable (const std::filesystem::path & path) noexcept;

	/**
	 * @brief Checks whether the application has execute permission for the path.
	 *
	 * Tests if the current process has permission to execute the specified path.
	 *
	 * @param path Path to check for execute permission.
	 * @return True if the path is executable by the application, false otherwise.
	 * @note Returns false if the path is empty. On Linux/macOS, uses access() with X_OK.
	 * @warning Windows implementation always returns true (permission check not implemented).
	 * @todo Implement proper permission checking for Windows platform.
	 * @see readable, writable
	 */
	[[nodiscard]]
	bool executable (const std::filesystem::path & path) noexcept;

	/**
	 * @brief Extracts the file extension from a path.
	 *
	 * Returns the file extension without the leading dot. For example,
	 * "file.txt" returns "txt", "archive.tar.gz" returns "gz".
	 *
	 * @param filepath Path to extract the extension from.
	 * @param forceToLower If true, converts the extension to lowercase. Default false.
	 * @return The file extension as a string, or empty string if no extension exists.
	 * @note The leading dot is automatically removed from the extension.
	 */
	[[nodiscard]]
	std::string getFileExtension (const std::filesystem::path & filepath, bool forceToLower = false) noexcept;

	/**
	 * @brief Reads a file and returns its content as a string.
	 *
	 * Reads the entire file into memory and stores it in the provided string.
	 * The file is opened in binary mode to preserve exact content.
	 *
	 * @param filepath Path to the file to read.
	 * @param[out] content String that will be filled with the file contents.
	 *        The string is resized to match the file size.
	 * @return True if the file was successfully read, false otherwise.
	 * @note Returns false if the path is empty, file cannot be opened, or read fails.
	 *       Errors are logged to stderr.
	 * @see fileGetContents(const std::filesystem::path&, std::vector<data_t>&)
	 */
	bool fileGetContents (const std::filesystem::path & filepath, std::string & content) noexcept;

	/**
	 * @brief Writes a string to a file.
	 *
	 * Writes the provided string content to a file. Can optionally append to existing
	 * content or create parent directories as needed. The file is opened in binary mode.
	 *
	 * @param filepath Path to the file to write.
	 * @param content String view containing the data to write.
	 * @param append If true, appends to the file instead of overwriting. Default false.
	 * @param createDirectories If true, creates parent directories if they don't exist. Default false.
	 * @return True if the content was successfully written, false otherwise.
	 * @note Returns false if the path is empty, directory creation fails (when requested),
	 *       file cannot be opened, or write fails. Errors are logged to stderr.
	 * @see filePutContents(const std::filesystem::path&, const container_t&, bool, bool)
	 */
	bool filePutContents (const std::filesystem::path & filepath, std::string_view content, bool append = false, bool createDirectories = false) noexcept;

	/**
	 * @brief Reads a file and returns its binary content in a vector.
	 *
	 * Template function that reads a file as binary data and stores it in a vector
	 * of the specified type. The vector is automatically resized to accommodate the
	 * file contents, with proper alignment for the data type.
	 *
	 * @tparam data_t The type of data elements to store (e.g., uint32_t, char, std::byte).
	 * @param filepath Path to the file to read.
	 * @param[out] content Vector that will be filled with the file contents.
	 *        The vector is resized to fit the file data, accounting for element size.
	 * @return True if the file was successfully read, false otherwise.
	 * @note Returns false if the path is empty, file cannot be opened, or read fails.
	 *       If file size is not evenly divisible by sizeof(data_t), the vector is
	 *       sized up to accommodate partial elements. Errors are logged to stderr.
	 * @see fileGetContents(const std::filesystem::path&, std::string&)
	 */
	template< typename data_t >
	bool
	fileGetContents (const std::filesystem::path & filepath, std::vector< data_t > & content) noexcept
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

		content.resize(static_cast< size_t >(bytes) / sizeof(data_t) + (static_cast< size_t >(bytes) % sizeof(data_t) ? 1U : 0U));

		file.read(reinterpret_cast< char * >(content.data()), bytes);

		if ( !file ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", error reading " << filepath << " file." "\n";

			return false;
		}

		return true;
	}

	/**
	 * @brief Writes binary data from a container to a file.
	 *
	 * Template function that writes binary data from any contiguous container
	 * (vector, array, span, etc.) to a file. Can optionally append to existing
	 * content or create parent directories as needed.
	 *
	 * @tparam container_t A contiguous range type (std::vector, std::array, std::span, C-array, etc.).
	 * @param filepath Path to the file to write.
	 * @param content Container holding the binary data to write.
	 * @param append If true, appends to the file instead of overwriting. Default false.
	 * @param createDirectories If true, creates parent directories if they don't exist. Default false.
	 * @return True if the data was successfully written, false otherwise.
	 * @note Returns false if the path is empty, directory creation fails (when requested),
	 *       file cannot be opened, or write fails. Errors are logged to stderr.
	 *       The total bytes written equals container size multiplied by element size.
	 * @see filePutContents(const std::filesystem::path&, std::string_view, bool, bool)
	 */
	template< std::ranges::contiguous_range container_t >
	bool
	filePutContents (const std::filesystem::path & filepath, const container_t & content, bool append = false, bool createDirectories = false) noexcept
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

		file.write(reinterpret_cast< const char * >(std::ranges::data(content)), static_cast< std::streamsize >(std::ranges::size(content) * sizeof(std::ranges::range_value_t< container_t >)));

		if ( !file ) [[unlikely]]
		{
			std::cerr << __PRETTY_FUNCTION__ << ", error writing to " << filepath << " file." "\n";

			return false;
		}

		return true;
	}

	/**
	 * @brief Checks if a directory is ready to use and writable.
	 *
	 * Convenience function that verifies a directory exists and is writable,
	 * or creates it if it doesn't exist. Useful for ensuring cache and output
	 * directories are accessible before use.
	 *
	 * @param path Path to the directory to check or create.
	 * @return True if the directory exists and is writable, or was successfully created.
	 *         False if creation fails or the directory is not writable.
	 * @note This function combines directoryExists(), writable(), and createDirectory().
	 *       Particularly useful for cache directories and temporary storage locations.
	 * @see directoryExists, writable, createDirectory
	 */
	[[nodiscard]]
	inline
	bool
	isDirectoryUsable (const std::filesystem::path & path) noexcept
	{
		if ( IO::directoryExists(path) )
		{
			return IO::writable(path);
		}

		return IO::createDirectory(path);
	}
}
