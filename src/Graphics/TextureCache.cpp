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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "TextureCache.hpp"

/* STL inclusions. */
#include <fstream>
#include <ios>
#include <string_view>

/* Local inclusions. */
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "Hash/FNV1a.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base::PixelFactory;

	TextureCache::TextureCache (PrimaryServices & primaryServices, const TextureCompressor & compressor) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices},
		m_compressor{compressor}
	{

	}

	bool
	TextureCache::onInitialize () noexcept
	{
		if ( !m_primaryServices.settings().getOrSetDefault< bool >(GraphicsTextureCacheEnabledKey, DefaultGraphicsTextureCacheEnabled) )
		{
			/* NOTE: An empty directory is what makes every lookup miss, so the service stays usable
			 * and getOrCompress() simply always compresses. */
			Tracer::info(ClassId, "The texture disk cache is disabled by settings. Textures will be compressed at every launch.");

			return true;
		}

		m_cacheDirectory = m_primaryServices.fileSystem().cacheDirectory(CacheDirectoryName);

		if ( std::error_code error; !std::filesystem::exists(m_cacheDirectory, error) )
		{
			if ( !std::filesystem::create_directories(m_cacheDirectory, error) )
			{
				TraceError{ClassId} << "Failed to create the texture cache directory '" << m_cacheDirectory << "' ! The cache is disabled.";

				/* NOTE: A disabled disk cache costs compression time, it is never a reason to bring
				 * the renderer down. An empty directory makes every lookup miss. */
				m_cacheDirectory.clear();

				return true;
			}
		}

		if ( m_primaryServices.arguments().isSwitchPresent("--clear-renderer-cache") )
		{
			this->clearCache();
		}

		return true;
	}

	bool
	TextureCache::onTerminate () noexcept
	{
		m_cacheDirectory.clear();

		return true;
	}

	void
	TextureCache::clearCache () const noexcept
	{
		if ( m_cacheDirectory.empty() )
		{
			return;
		}

		size_t erasedCount = 0;

		std::error_code error;

		for ( const auto & entry : std::filesystem::directory_iterator{m_cacheDirectory, error} )
		{
			if ( !entry.is_regular_file(error) || entry.path().extension() != CacheFileExtension )
			{
				continue;
			}

			if ( std::filesystem::remove(entry.path(), error) )
			{
				++erasedCount;
			}
			else
			{
				TraceError{ClassId} << "Unable to erase the texture cache entry '" << entry.path() << "' !";
			}
		}

		TraceSuccess{ClassId} << "Texture cache cleared (" << erasedCount << " entrie(s) erased).";
	}

	size_t
	TextureCache::cacheKey (const Pixmap< uint8_t > & pixmap) noexcept
	{
		/* NOTE: Content-addressed on purpose. Anything derived from the resource NAME alone lets an
		 * edited texture keep serving its stale blob; hashing the decoded pixels cannot. The
		 * geometry is folded in because two different layouts could otherwise share a byte run. */
		const auto & bytes = pixmap.data();

		auto key = Base::Hash::FNV1a(std::string_view{reinterpret_cast< const char * >(bytes.data()), bytes.size()});

		const auto fold = [&key] (size_t value) noexcept {
			key ^= value + 0x9e3779b9 + (key << 6) + (key >> 2);
		};

		fold(static_cast< size_t >(pixmap.width()));
		fold(static_cast< size_t >(pixmap.height()));
		fold(static_cast< size_t >(pixmap.colorCount()));

		return key;
	}

	std::filesystem::path
	TextureCache::cacheFilePath (size_t key) const noexcept
	{
		static constexpr auto HexChars = "0123456789abcdef";

		std::string filename;
		filename.reserve(sizeof(key) * 2 + 8);

		for ( size_t shift = sizeof(key) * 8; shift > 0; shift -= 4 )
		{
			filename += HexChars[(key >> (shift - 4)) & 0xF];
		}

		filename += CacheFileExtension;

		return m_cacheDirectory / filename;
	}

	std::vector< CompressedMipLevel >
	TextureCache::tryLoad (size_t key) const noexcept
	{
		const auto path = this->cacheFilePath(key);

		std::ifstream file{path, std::ios::binary};

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

		for ( uint32_t index = 0; index < mipCount; ++index )
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

		return result;
	}

	bool
	TextureCache::store (size_t key, const std::vector< CompressedMipLevel > & mipLevels) const noexcept
	{
		if ( mipLevels.empty() )
		{
			return false;
		}

		const auto path = this->cacheFilePath(key);

		std::ofstream file{path, std::ios::binary | std::ios::trunc};

		if ( !file.is_open() )
		{
			TraceError{ClassId} << "Failed to open the cache file '" << path << "' for writing !";

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
			TraceError{ClassId} << "Failed to write the cache file '" << path << "' !";

			return false;
		}

		return true;
	}

	std::vector< CompressedMipLevel >
	TextureCache::getOrCompress (const std::string & resourceName, const Pixmap< uint8_t > & pixmap, uint32_t maxMipLevels) const noexcept
	{
		const auto cacheEnabled = !m_cacheDirectory.empty();
		const auto key = cacheEnabled ? TextureCache::cacheKey(pixmap) : 0;

		if ( cacheEnabled )
		{
			auto cached = this->tryLoad(key);

			if ( !cached.empty() )
			{
				TraceDebug{ClassId} << "Cache hit: " << resourceName;

				return cached;
			}
		}

		auto compressed = m_compressor.compress(pixmap, maxMipLevels);

		if ( compressed.empty() || !cacheEnabled )
		{
			return compressed;
		}

		if ( !this->store(key, compressed) )
		{
			TraceWarning{ClassId} << "Unable to cache the compressed texture '" << resourceName << "' ! It will be compressed again on the next run.";
		}

		return compressed;
	}
}
