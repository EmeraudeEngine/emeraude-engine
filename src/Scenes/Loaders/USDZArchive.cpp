/*
 * src/Scenes/Loaders/USDZArchive.cpp
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

#include "USDZArchive.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cctype>
#include <cstring>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace
{
	/* NOTE: The loader's name: the archive reports under it, as before it had a file of its own. */
	constexpr auto ClassId{"USDLoader"};
}

namespace EmEn::Scenes::Loaders
{
	bool
	USDZArchive::open (const std::filesystem::path & filepath) noexcept
	{
		/* NOTE: The mapping opens the path itself: a FIFO blocked that open forever (2026-10-18). */
		if ( !Base::IO::isReadableRegularFile(filepath) )
		{
			TraceError{ClassId} << "The archive '" << Base::IO::toU8String(filepath) << "' is not a readable file !";

			return false;
		}

		std::string error;

		if ( lightusd::io::MMapFile(Base::IO::toU8String(filepath), &m_handle, false, &error) )
		{
			m_mapped = true;
			m_base = m_handle.addr;
			m_size = static_cast< size_t >(m_handle.size);
		}
		else
		{
			/* NOTE: A platform without tinyusdz's mmap (macOS took the "non posix" branch until the
			 * ext-deps-generator patch of 2026-10-04: TARGET_OS_IPHONE is defined there, as 0) fails here
			 * without a reason: the archive is read into memory instead. */
			TraceWarning{ClassId} << "Unable to map the archive '" << Base::IO::toU8String(filepath) << "' (" << (error.empty() ? std::string{"no reason given: mmap not available in tinyusdz"} : error) << "), reading it into memory.";

			if ( !Base::IO::fileGetContents(filepath, m_memory) || m_memory.empty() )
			{
				TraceError{ClassId} << "Unable to read the archive '" << Base::IO::toU8String(filepath) << "' !";

				return false;
			}

			m_base = m_memory.data();
			m_size = m_memory.size();
		}

		m_filepath = filepath;

		return this->readTable(Base::IO::toU8String(filepath.filename()));
	}

	void
	USDZArchive::equip (lightusd::AssetResolutionResolver & resolver) noexcept
	{
		m_resolver = &resolver;

		lightusd::AssetResolutionHandler handler;
		handler.resolve_fun = &USDZArchive::resolveHandler;
		handler.size_fun = &USDZArchive::sizeHandler;
		handler.read_fun = &USDZArchive::readHandler;
		handler.write_fun = nullptr;
		handler.userdata = this;

		/* "*" is the wildcard fallback; the explicit extensions come first because
		 * `AssetResolutionResolver::resolve()` looks the exact one up before the wildcard. */
		for ( const auto * extension : {"usd", "usda", "usdc", "USD", "USDA", "USDC", "png", "PNG", "jpg", "JPG", "jpeg", "JPEG", "tga", "TGA", "exr", "EXR", "hdr", "HDR", "mdl", "MDL", "*"} )
		{
			resolver.register_asset_resolution_handler(extension, handler);
		}

		resolver.set_current_working_path(m_baseDirectory);
		resolver.set_search_paths({m_baseDirectory});
	}

	bool
	USDZArchive::resolve (const std::string & wanted, const std::vector< std::string > & searchPaths, std::string & resolved) const noexcept
	{
		if ( m_asset.asset_map.contains(wanted) )
		{
			resolved = wanted;

			return true;
		}

		const auto normalized = USDZArchive::normalize(wanted);

		if ( m_asset.asset_map.contains(normalized) )
		{
			resolved = normalized;

			return true;
		}

		/* Step 2 — every directory the composition could mean, strongest first. */
		std::vector< std::string > bases;
		bases.reserve(searchPaths.size() + 2);

		if ( m_resolver != nullptr && !m_resolver->current_working_path().empty() )
		{
			bases.emplace_back(m_resolver->current_working_path());
		}

		bases.insert(bases.end(), searchPaths.begin(), searchPaths.end());
		bases.emplace_back(m_baseDirectory);

		for ( const auto & base : bases )
		{
			if ( base.empty() )
			{
				continue;
			}

			const auto candidate = USDZArchive::normalize(Base::String::concatenate(base, "/", wanted));

			if ( m_asset.asset_map.contains(candidate) )
			{
				resolved = candidate;

				return true;
			}
		}

		/* Step 3 — unique suffix. */
		const std::string * match = nullptr;
		size_t matchCount = 0;

		for ( const auto & [name, range] : m_asset.asset_map )
		{
			if ( name.size() < normalized.size() )
			{
				continue;
			}

			const auto offset = name.size() - normalized.size();

			if ( name.compare(offset, normalized.size(), normalized) != 0 )
			{
				continue;
			}

			/* Anchored on a separator, so "grass.jpg" cannot match "tallgrass.jpg". */
			if ( offset > 0 && name[offset - 1] != '/' )
			{
				continue;
			}

			match = &name;

			if ( ++matchCount > 1 )
			{
				break;
			}
		}

		if ( matchCount == 1 )
		{
			resolved = *match;

			return true;
		}

		if ( matchCount > 1 )
		{
			TraceWarning{ClassId} <<
				"Asset '" << wanted << "' matches SEVERAL archive entries by suffix; refusing to guess. "
				"The asset is dropped rather than replaced by the wrong one.";
		}

		return false;
	}

	const std::byte *
	USDZArchive::bytes (const std::string & resolvedName, size_t & size) const noexcept
	{
		const auto it = m_asset.asset_map.find(resolvedName);

		if ( it == m_asset.asset_map.cend() || it->second.second <= it->second.first )
		{
			size = 0;

			return nullptr;
		}

		size = it->second.second - it->second.first;

		return reinterpret_cast< const std::byte * >(m_asset.addr) + it->second.first;
	}

	std::optional< std::pair< uint64_t, uint64_t > >
	USDZArchive::entryRange (const std::string & resolvedName) const noexcept
	{
		const auto it = m_asset.asset_map.find(resolvedName);

		/* NOTE: The offsets are file offsets only when the table points at the start of the whole file (mapped or
		 * read). */
		if ( it == m_asset.asset_map.cend() || it->second.second <= it->second.first || m_asset.addr != m_base || m_filepath.empty() )
		{
			return std::nullopt;
		}

		return std::pair< uint64_t, uint64_t >{it->second.first, it->second.second - it->second.first};
	}

	bool
	USDZArchive::isUSDName (const std::string & name) noexcept
	{
		const auto dot = name.find_last_of('.');

		if ( dot == std::string::npos )
		{
			return false;
		}

		auto extension = name.substr(dot + 1);

		std::ranges::transform(extension, extension.begin(), [] (unsigned char character) {
			return static_cast< char >(std::tolower(character));
		});

		return extension == "usd" || extension == "usda" || extension == "usdc";
	}

	std::string
	USDZArchive::normalize (const std::string & path) noexcept
	{
		std::vector< std::string > parts;
		std::string current;

		const auto flush = [&parts, &current] () {
			if ( current == ".." )
			{
				if ( !parts.empty() )
				{
					parts.pop_back();
				}
			}
			else if ( !current.empty() && current != "." )
			{
				parts.emplace_back(current);
			}

			current.clear();
		};

		for ( const auto character : path )
		{
			if ( character == '/' || character == '\\' )
			{
				flush();

				continue;
			}

			current += character;
		}

		flush();

		std::string result;

		for ( const auto & part : parts )
		{
			if ( !result.empty() )
			{
				result += '/';
			}

			result += part;
		}

		return result;
	}

	int
	USDZArchive::resolveHandler (const char * assetName, const std::vector< std::string > & searchPaths, std::string * resolvedName, std::string * error, void * userdata) noexcept
	{
		const auto * archive = static_cast< const USDZArchive * >(userdata);

		if ( archive == nullptr || assetName == nullptr || resolvedName == nullptr )
		{
			return -2;
		}

		if ( !archive->resolve(assetName, searchPaths, *resolvedName) )
		{
			if ( error != nullptr )
			{
				*error += std::string{"not in archive: "} + assetName + "\n";
			}

			return -1;
		}

		return 0;
	}

	int
	USDZArchive::sizeHandler (const char * resolvedName, uint64_t * byteCount, std::string * error, void * userdata) noexcept
	{
		const auto * archive = static_cast< const USDZArchive * >(userdata);

		if ( archive == nullptr || resolvedName == nullptr || byteCount == nullptr )
		{
			return -2;
		}

		size_t size = 0;

		if ( archive->bytes(resolvedName, size) == nullptr )
		{
			if ( error != nullptr )
			{
				*error += std::string{"not in archive: "} + resolvedName + "\n";
			}

			return -1;
		}

		*byteCount = size;

		return 0;
	}

	int
	USDZArchive::readHandler (const char * resolvedName, uint64_t requestedBytes, uint8_t * output, uint64_t * byteCount, std::string * error, void * userdata) noexcept
	{
		const auto * archive = static_cast< const USDZArchive * >(userdata);

		if ( archive == nullptr || resolvedName == nullptr || output == nullptr || byteCount == nullptr )
		{
			return -2;
		}

		size_t size = 0;
		const auto * source = archive->bytes(resolvedName, size);

		if ( source == nullptr )
		{
			if ( error != nullptr )
			{
				*error += std::string{"not in archive: "} + resolvedName + "\n";
			}

			return -1;
		}

		if ( size > requestedBytes )
		{
			if ( error != nullptr )
			{
				*error += std::string{"entry larger than the requested read: "} + resolvedName + "\n";
			}

			return -2;
		}

		std::memcpy(output, source, size);

		*byteCount = size;

		return 0;
	}

	bool
	USDZArchive::openMemory (std::vector< uint8_t > bytes, const std::string & sourceName) noexcept
	{
		if ( bytes.empty() )
		{
			TraceError{ClassId} << "The archive '" << sourceName << "' is empty !";

			return false;
		}

		m_memory = std::move(bytes);
		m_base = m_memory.data();
		m_size = m_memory.size();

		return this->readTable(sourceName);
	}

	bool
	USDZArchive::readTable (const std::string & sourceName) noexcept
	{
		std::string error;
		std::string warning;

		/* ⚠️ `assetOnMemory` = true: the table keeps a POINTER into our mapping instead of
		 * copying the whole archive into `USDZAsset::data`. That is the difference between
		 * 21 MB and 1.6 GB resident before a single prim is read. */
		if ( !lightusd::ReadUSDZAssetInfoFromMemory(m_base, m_size, true, &m_asset, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to read the asset table of '" << sourceName << "' : " << error;

			return false;
		}

		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While reading the asset table of '" << sourceName << "' : " << warning;
		}

		/* The first USD entry in ARCHIVE ORDER is the root layer — that is what the USDZ
		 * specification mandates, and it is what every exporter writes. */
		size_t rootOffset = 0;

		for ( const auto & [name, range] : m_asset.asset_map )
		{
			if ( !isUSDName(name) )
			{
				continue;
			}

			if ( m_rootAssetName.empty() || range.first < rootOffset )
			{
				m_rootAssetName = name;
				rootOffset = range.first;
			}
		}

		if ( m_rootAssetName.empty() )
		{
			TraceError{ClassId} << "'" << sourceName << "' holds no USD layer at all.";

			return false;
		}

		if ( const auto separator = m_rootAssetName.find_last_of('/'); separator != std::string::npos )
		{
			m_baseDirectory = m_rootAssetName.substr(0, separator);
		}

		TraceInfo{ClassId} <<
			"Archive '" << sourceName << "' " << (m_mapped ? "mapped" : "read into memory") << ": " << m_asset.asset_map.size() <<
			" entries, root layer '" << m_rootAssetName << "'.";

		return true;
	}
}
