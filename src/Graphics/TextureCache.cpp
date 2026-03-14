/*
 * src/Graphics/TextureCache.cpp
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

#include "TextureCache.hpp"

/* STL inclusions. */
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>

/* Local inclusions. */
#include "Libs/Hash/SHA256.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	std::filesystem::path TextureCache::s_cacheDirectory;
	bool TextureCache::s_initialized = false;

	void
	TextureCache::initialize (const std::filesystem::path & baseCacheDirectory) noexcept
	{
		if ( s_initialized )
		{
			return;
		}

		s_cacheDirectory = baseCacheDirectory / "TextureCache";

		std::error_code ec;

		if ( !std::filesystem::exists(s_cacheDirectory, ec) )
		{
			if ( !std::filesystem::create_directories(s_cacheDirectory, ec) )
			{
				TraceError{ClassId} << "Failed to create texture cache directory: " << s_cacheDirectory;

				return;
			}
		}

		s_initialized = true;

		TraceSuccess{ClassId} << "Texture cache directory: " << s_cacheDirectory;
	}

	std::vector< CompressedMipLevel >
	TextureCache::tryLoad (
		const std::string & resourceName,
		uint64_t sourceFileSize,
		uint64_t sourceModTime
	) noexcept
	{
		if ( !s_initialized )
		{
			return {};
		}

		const auto path = cacheFilePath(resourceName, sourceFileSize, sourceModTime);

		std::ifstream file(path, std::ios::binary);

		if ( !file.is_open() )
		{
			return {};
		}

		/* Read and validate header. */
		uint32_t magic = 0;
		uint32_t version = 0;
		uint32_t mipCount = 0;

		file.read(reinterpret_cast< char * >(&magic), sizeof(magic));
		file.read(reinterpret_cast< char * >(&version), sizeof(version));
		file.read(reinterpret_cast< char * >(&mipCount), sizeof(mipCount));

		if ( !file || magic != Magic || version != Version || mipCount == 0 || mipCount > 20 )
		{
			return {};
		}

		/* Read mip levels. */
		std::vector< CompressedMipLevel > result;
		result.reserve(mipCount);

		for ( uint32_t i = 0; i < mipCount; ++i )
		{
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t dataSize = 0;

			file.read(reinterpret_cast< char * >(&width), sizeof(width));
			file.read(reinterpret_cast< char * >(&height), sizeof(height));
			file.read(reinterpret_cast< char * >(&dataSize), sizeof(dataSize));

			if ( !file || width == 0 || height == 0 || dataSize == 0 )
			{
				return {};
			}

			CompressedMipLevel mip;
			mip.width = width;
			mip.height = height;
			mip.data.resize(dataSize);

			file.read(reinterpret_cast< char * >(mip.data.data()), dataSize);

			if ( !file )
			{
				return {};
			}

			result.emplace_back(std::move(mip));
		}

		TraceInfo{ClassId} << "Cache hit: " << resourceName;

		return result;
	}

	bool
	TextureCache::store (
		const std::string & resourceName,
		uint64_t sourceFileSize,
		uint64_t sourceModTime,
		const std::vector< CompressedMipLevel > & mipLevels
	) noexcept
	{
		if ( !s_initialized || mipLevels.empty() )
		{
			return false;
		}

		const auto path = cacheFilePath(resourceName, sourceFileSize, sourceModTime);

		std::ofstream file(path, std::ios::binary | std::ios::trunc);

		if ( !file.is_open() )
		{
			TraceError{ClassId} << "Failed to open cache file for writing: " << path;

			return false;
		}

		/* Write header. */
		const uint32_t magic = Magic;
		const uint32_t version = Version;
		const auto mipCount = static_cast< uint32_t >(mipLevels.size());

		file.write(reinterpret_cast< const char * >(&magic), sizeof(magic));
		file.write(reinterpret_cast< const char * >(&version), sizeof(version));
		file.write(reinterpret_cast< const char * >(&mipCount), sizeof(mipCount));

		/* Write mip levels. */
		for ( const auto & mip : mipLevels )
		{
			const auto dataSize = static_cast< uint32_t >(mip.data.size());

			file.write(reinterpret_cast< const char * >(&mip.width), sizeof(mip.width));
			file.write(reinterpret_cast< const char * >(&mip.height), sizeof(mip.height));
			file.write(reinterpret_cast< const char * >(&dataSize), sizeof(dataSize));
			file.write(reinterpret_cast< const char * >(mip.data.data()), dataSize);
		}

		if ( !file )
		{
			TraceError{ClassId} << "Failed to write cache file: " << path;

			return false;
		}

		return true;
	}

	std::filesystem::path
	TextureCache::cacheFilePath (
		const std::string & resourceName,
		uint64_t sourceFileSize,
		uint64_t sourceModTime
	) noexcept
	{
		/* Build a unique key from resource name + file size + modification time. */
		std::ostringstream key;
		key << resourceName << "|" << sourceFileSize << "|" << sourceModTime;
		const auto keyStr = key.str();

		/* Hash the key with SHA256. */
		Libs::Hash::SHA256 sha;
		sha.update(reinterpret_cast< const uint8_t * >(keyStr.data()), keyStr.size());

		std::array< uint8_t, 32 > digest{};
		sha.final(digest);

		/* Convert to hex string for the filename. */
		static constexpr auto hexChars = "0123456789abcdef";
		std::string hexHash;
		hexHash.reserve(64);

		for ( const auto byte : digest )
		{
			hexHash += hexChars[(byte >> 4) & 0xF];
			hexHash += hexChars[byte & 0xF];
		}

		return s_cacheDirectory / (hexHash + ".bc7cache");
	}
}
