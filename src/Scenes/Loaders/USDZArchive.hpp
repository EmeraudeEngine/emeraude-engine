/*
 * src/Scenes/Loaders/USDZArchive.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "lightusd.hh"
#include "asset-resolution.hh"
#include "io-util.hh"

namespace EmEn::Scenes::Loaders
{

	/**
	 * @brief A memory-mapped USDZ archive, its asset table, and the resolution of asset paths
	 * against it.
	 *
	 * @note ⚠️ tinyusdz cannot do this on its own, and the two gaps are QUIET:
	 * `SetupUSDZAssetResolution()` registers handlers for IMAGES ONLY (png/jpg/exr/hdr — its own
	 * TODO says "[ ] USD: usda, usdc, usd"), so a nested `.usd` layer is never read out of the
	 * archive; and `USDZResolveAsset()` compares the table VERBATIM, never joining the asking
	 * layer's directory, so `./Assembly/assembly_Lobby.usd` misses an entry stored as
	 * `Users/.../kit_usdz_tejdxcvk/Assembly/assembly_Lobby.usd`. Hence our own three handlers.
	 *
	 * @note The archive is MAPPED, never extracted: layers are parsed and images decoded straight
	 * out of the mapping. A 1.6 GB archive therefore costs its table, not its bytes.
	 */
	class USDZArchive final
	{
		public:

			USDZArchive () noexcept = default;

			USDZArchive (const USDZArchive &) = delete;

			USDZArchive (USDZArchive &&) = delete;

			USDZArchive & operator= (const USDZArchive &) = delete;

			USDZArchive & operator= (USDZArchive &&) = delete;

			~USDZArchive ()
			{
				if ( m_mapped )
				{
					std::string error;

					lightusd::io::UnmapFile(m_handle, &error);
				}
			}

			/**
			 * @brief Maps the archive and reads its asset table.
			 * @param filepath An absolute path to the .usdz file.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			open (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Reads an archive held in memory (the fuzzer's path: bytes, no file) and its asset table.
			 * @note An archive opened this way has no file: entryRange() answers nothing (no encoded source to re-read).
			 * @param bytes The whole archive, owned from now on.
			 * @param sourceName The name the logs use.
			 * @return bool
			 */
			[[nodiscard]]
			bool openMemory (std::vector< uint8_t > bytes, const std::string & sourceName) noexcept;

			/** @brief Returns the archive-relative name of the root layer. */
			[[nodiscard]]
			const std::string &
			rootAssetName () const noexcept
			{
				return m_rootAssetName;
			}

			/** @brief Returns the directory the root layer sits in, inside the archive. */
			[[nodiscard]]
			const std::string &
			baseDirectory () const noexcept
			{
				return m_baseDirectory;
			}

			/**
			 * @brief Installs this archive as the resolver's source for every asset kind.
			 * @note The resolver is REMEMBERED, because the handler signature does not carry the
			 * current working path and the composition sets it on the resolver alone.
			 * @param resolver A reference to the resolver to equip.
			 */
			void
			equip (lightusd::AssetResolutionResolver & resolver) noexcept;

			/**
			 * @brief Resolves an authored asset path to an archive entry name.
			 *
			 * @note Four steps, weakest last:
			 *  1. verbatim, then lexically normalized;
			 *  2. joined with the resolver's current working path, its search paths, then the
			 *     archive's own root directory;
			 *  3. longest UNIQUE path suffix.
			 *
			 * @warning ⚠️⚠️ STEP 3 MATCHES A SUFFIX, NEVER A BASENAME, AND REFUSES AN AMBIGUOUS ONE.
			 * A Kit export bakes its materials into `Materials/Bake/baked_textures_<hash>/` and
			 * names every file inside identically. Measured on WorldLobby.usdz: 285 images under
			 * only 119 distinct filenames, with `mtl-base_color.jpg` appearing SIXTY times. A
			 * basename match resolves all sixty to whichever entry came first — sixty materials
			 * wearing one skin, no error raised anywhere, and a scene that merely looks "wrong"
			 * rather than broken. The suffix keeps `baked_textures_<hash>/mtl-base_color.jpg`
			 * distinct; when even that is ambiguous the answer is REFUSED, so the material keeps its
			 * flat colour instead of someone else's texture.
			 *
			 * @param wanted The authored asset path.
			 * @param searchPaths The search paths the composition is currently using.
			 * @param resolved A reference to the string receiving the archive entry name.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			resolve (const std::string & wanted, const std::vector< std::string > & searchPaths, std::string & resolved) const noexcept;

			/**
			 * @brief Returns the bytes of an archive entry, in place.
			 * @param resolvedName An archive entry name, as returned by resolve().
			 * @param size A reference to the value receiving the entry size in bytes.
			 * @return const std::byte * Null when the entry is unknown or empty.
			 */
			[[nodiscard]]
			const std::byte *
			bytes (const std::string & resolvedName, size_t & size) const noexcept;

			/**
			 * @brief Returns where an entry lies in the archive FILE: (offset, length). A USDZ stores its entries
			 * uncompressed, and the asset table's offsets are relative to the start of the mapped file.
			 * @param resolvedName The entry name.
			 * @return std::optional< std::pair< uint64_t, uint64_t > >
			 */
			[[nodiscard]]
			std::optional< std::pair< uint64_t, uint64_t > >
			entryRange (const std::string & resolvedName) const noexcept;

			/**
			 * @brief Returns the archive file.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filepath () const noexcept
			{
				return m_filepath;
			}

		private:

			/**
			 * @brief Reads the asset table of the archive at m_base / m_size and finds its root layer.
			 * @param sourceName The name the logs use.
			 * @return bool
			 */
			[[nodiscard]]
			bool readTable (const std::string & sourceName) noexcept;

			/** @brief Whether an archive entry name designates a USD layer. */
			[[nodiscard]]
			static
			bool
			isUSDName (const std::string & name) noexcept;

			/**
			 * @brief Collapses '.', '..' and separators, WITHOUT touching the filesystem.
			 * @note std::filesystem cannot serve here: these are archive names, not paths, and a
			 * canonicalisation would resolve them against the process working directory.
			 * @param path The path to normalize.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string
			normalize (const std::string & path) noexcept;

			static int
			resolveHandler (const char * assetName, const std::vector< std::string > & searchPaths, std::string * resolvedName, std::string * error, void * userdata) noexcept;

			static int
			sizeHandler (const char * resolvedName, uint64_t * byteCount, std::string * error, void * userdata) noexcept;

			static int
			readHandler (const char * resolvedName, uint64_t requestedBytes, uint8_t * output, uint64_t * byteCount, std::string * error, void * userdata) noexcept;

			lightusd::io::MMapFileHandle m_handle{};
			std::vector< uint8_t > m_memory;
			const uint8_t * m_base{nullptr};
			size_t m_size{0};
			lightusd::USDZAsset m_asset;
			std::string m_rootAssetName;
			std::string m_baseDirectory;
			std::filesystem::path m_filepath;
			const lightusd::AssetResolutionResolver * m_resolver{nullptr};
			bool m_mapped{false};
	};
}
