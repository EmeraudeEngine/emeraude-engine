/*
 * src/Scenes/Loaders/USDLoader.cpp
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

#include "USDLoader.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include <system_error>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include <tuple>

/* STL inclusions. */
#include <cstring>

/* Third-party inclusions. */
#include "tinyusdz.hh"
#include "usdGeom.hh"
#include "usdLux.hh"
#include "composition.hh"
#include "asset-resolution.hh"
#include "io-util.hh"
#include "tydra/render-data.hh"
#include "tydra/render-data-converter.hh"
#include "tydra/scene-access.hh"

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Resources/Manager.hpp"
#include "SceneData.hpp"
#include "Tracer.hpp"
#include "VertexFactory/Shape.hpp"

namespace EmEn::Scenes::Loaders
{
	/* Above this, a prim tree is noise rather than information. */
	static constexpr size_t PrimTreeReportLimit{80};

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

					tinyusdz::io::UnmapFile(m_handle, &error);
				}
			}

			/**
			 * @brief Maps the archive and reads its asset table.
			 * @param filepath An absolute path to the .usdz file.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			open (const std::filesystem::path & filepath) noexcept
			{
				std::string error;

				if ( !tinyusdz::io::MMapFile(filepath.string(), &m_handle, false, &error) )
				{
					TraceError{USDLoader::ClassId} << "Unable to map the archive '" << filepath.string() << "' : " << error;

					return false;
				}

				m_mapped = true;

				std::string warning;

				/* ⚠️ `assetOnMemory` = true: the table keeps a POINTER into our mapping instead of
				 * copying the whole archive into `USDZAsset::data`. That is the difference between
				 * 21 MB and 1.6 GB resident before a single prim is read. */
				if ( !tinyusdz::ReadUSDZAssetInfoFromMemory(m_handle.addr, static_cast< size_t >(m_handle.size), true, &m_asset, &warning, &error) )
				{
					TraceError{USDLoader::ClassId} << "Unable to read the asset table of '" << filepath.filename().string() << "' : " << error;

					return false;
				}

				if ( !warning.empty() )
				{
					TraceWarning{USDLoader::ClassId} << "While reading the asset table of '" << filepath.filename().string() << "' : " << warning;
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
					TraceError{USDLoader::ClassId} << "'" << filepath.filename().string() << "' holds no USD layer at all.";

					return false;
				}

				if ( const auto separator = m_rootAssetName.find_last_of('/'); separator != std::string::npos )
				{
					m_baseDirectory = m_rootAssetName.substr(0, separator);
				}

				TraceInfo{USDLoader::ClassId} <<
					"Archive '" << filepath.filename().string() << "' mapped: " << m_asset.asset_map.size() <<
					" entries, root layer '" << m_rootAssetName << "'.";

				return true;
			}

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
			equip (tinyusdz::AssetResolutionResolver & resolver) noexcept
			{
				m_resolver = &resolver;

				tinyusdz::AssetResolutionHandler handler;
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
			resolve (const std::string & wanted, const std::vector< std::string > & searchPaths, std::string & resolved) const noexcept
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

					const auto candidate = USDZArchive::normalize(base + '/' + wanted);

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
					TraceWarning{USDLoader::ClassId} <<
						"Asset '" << wanted << "' matches SEVERAL archive entries by suffix; refusing to guess. "
						"The asset is dropped rather than replaced by the wrong one.";
				}

				return false;
			}

			/**
			 * @brief Returns the bytes of an archive entry, in place.
			 * @param resolvedName An archive entry name, as returned by resolve().
			 * @param size A reference to the value receiving the entry size in bytes.
			 * @return const std::byte * Null when the entry is unknown or empty.
			 */
			[[nodiscard]]
			const std::byte *
			bytes (const std::string & resolvedName, size_t & size) const noexcept
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

		private:

			/** @brief Whether an archive entry name designates a USD layer. */
			[[nodiscard]]
			static bool
			isUSDName (const std::string & name) noexcept
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

			/**
			 * @brief Collapses '.', '..' and separators, WITHOUT touching the filesystem.
			 * @note std::filesystem cannot serve here: these are archive names, not paths, and a
			 * canonicalisation would resolve them against the process working directory.
			 * @param path The path to normalize.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string
			normalize (const std::string & path) noexcept
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

			static int
			resolveHandler (const char * assetName, const std::vector< std::string > & searchPaths, std::string * resolvedName, std::string * error, void * userdata) noexcept
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

			static int
			sizeHandler (const char * resolvedName, uint64_t * byteCount, std::string * error, void * userdata) noexcept
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

			static int
			readHandler (const char * resolvedName, uint64_t requestedBytes, uint8_t * output, uint64_t * byteCount, std::string * error, void * userdata) noexcept
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

			tinyusdz::io::MMapFileHandle m_handle{};
			tinyusdz::USDZAsset m_asset;
			std::string m_rootAssetName;
			std::string m_baseDirectory;
			const tinyusdz::AssetResolutionResolver * m_resolver{nullptr};
			bool m_mapped{false};
	};

	USDLoader::USDLoader (Resources::Manager & resources) noexcept
		: m_resources{resources}
	{

	}

	void
	USDLoader::collectInventory (const tinyusdz::Prim & prim, size_t depth, Inventory & inventory) noexcept
	{
		inventory.primCount++;
		inventory.maxDepth = std::max(inventory.maxDepth, depth);

		/* `prim_type_name()` carries the authored schema name ("Mesh", "Xform", …). It is empty
		 * for a typeless prim (a pure "over" or a bare def), which is itself worth counting. */
		auto typeName = prim.prim_type_name();

		if ( typeName.empty() )
		{
			typeName = prim.type_name();
		}

		if ( typeName.empty() )
		{
			typeName = "<typeless>";
		}

		inventory.primTypeCounts[typeName]++;

		if ( typeName == "PointInstancer" )
		{
			inventory.pointInstancerCount++;
		}
		else if ( typeName == "Mesh" )
		{
			inventory.meshCount++;
		}
		else if ( typeName == "Material" )
		{
			inventory.materialCount++;
		}

		for ( const auto & child : prim.children() )
		{
			USDLoader::collectInventory(child, depth + 1, inventory);
		}
	}

	void
	USDLoader::reportPrimTree (const tinyusdz::Prim & prim, size_t depth, size_t & remaining) noexcept
	{
		if ( remaining == 0 )
		{
			return;
		}

		remaining--;

		auto typeName = prim.prim_type_name();

		if ( typeName.empty() )
		{
			typeName = prim.type_name();
		}

		TraceInfo{ClassId} << std::string(depth * 2, ' ') << "/" << prim.element_name() << "  [" << ( typeName.empty() ? "<typeless>" : typeName ) << "]";

		for ( const auto & child : prim.children() )
		{
			USDLoader::reportPrimTree(child, depth + 1, remaining);
		}
	}

	void
	USDLoader::reportInventory (const std::filesystem::path & filepath, const tinyusdz::Stage & stage) noexcept
	{
		Inventory inventory;

		for ( const auto & prim : stage.root_prims() )
		{
			USDLoader::collectInventory(prim, 1, inventory);
		}

		const auto & metas = stage.metas();

		TraceInfo{ClassId} <<
			filepath.filename().string() << " stage composed: " <<
			stage.root_prims().size() << " root prims, " <<
			inventory.primCount << " prims total, depth " << inventory.maxDepth << ", " <<
			inventory.meshCount << " meshes, " <<
			inventory.materialCount << " materials, " <<
			inventory.pointInstancerCount << " point instancers, " <<
			metas.subLayers.size() << " sublayers.";

		/* upAxis and metersPerUnit decide the whole conversion; endTimeCode says whether there
		 * is any animation at all. Reporting them removes three assumptions from every later
		 * diagnosis. */
		const auto upAxisName = [] (tinyusdz::Axis axis) -> const char * {
			switch ( axis )
			{
				case tinyusdz::Axis::X :
					return "X";

				case tinyusdz::Axis::Y :
					return "Y";

				case tinyusdz::Axis::Z :
					return "Z";

				default:
					return "<invalid>";
			}
		};

		TraceInfo{ClassId} <<
			"Stage metrics: upAxis " << upAxisName(metas.upAxis.get_value()) <<
			", metersPerUnit " << metas.metersPerUnit.get_value() <<
			", timeCodes " << metas.startTimeCode.get_value() << " to " << metas.endTimeCode.get_value() <<
			" at " << metas.timeCodesPerSecond.get_value() << " per second.";

		for ( const auto & [typeName, count] : inventory.primTypeCounts )
		{
			TraceInfo{ClassId} << "  " << count << " x " << typeName;
		}

		/* Bounded on purpose: an element is worth dumping whole, a full stage is not. */
		if ( inventory.primCount <= PrimTreeReportLimit )
		{
			auto remaining = PrimTreeReportLimit;

			for ( const auto & prim : stage.root_prims() )
			{
				USDLoader::reportPrimTree(prim, 0, remaining);
			}
		}
	}

	void
	USDLoader::collectEnvironmentLights (const tinyusdz::Prim & prim, const std::filesystem::path & stageDirectory, SceneData & output) noexcept
	{
		/* ⚠️ Tydra's RenderLight carries colour, intensity and exposure but NOT the dome's image
		 * path, so the DomeLight prim is read directly. Without this the asset's own sky — the
		 * 8K HDR that produces every reference render Intel ships — is silently dropped, and the
		 * scene ends up lit by whatever generic sky the demo installed instead. */
		if ( const auto * domeLight = prim.as< tinyusdz::DomeLight >(); domeLight != nullptr )
		{
			LightDescriptor descriptor;
			descriptor.name = prim.element_name();
			descriptor.type = LightType::Environment;

			float intensity = 1.0F;
			float exposure = 0.0F;

			if ( const auto value = domeLight->intensity.get_value(); value.has_value() )
			{
				float scalar = 1.0F;

				if ( value.get_scalar(&scalar) )
				{
					intensity = scalar;
				}
			}

			if ( const auto value = domeLight->exposure.get_value(); value.has_value() )
			{
				float scalar = 0.0F;

				if ( value.get_scalar(&scalar) )
				{
					exposure = scalar;
				}
			}

			/* USD folds exposure into the intensity as a power of two, exactly like a stop. */
			descriptor.intensity = intensity * std::pow(2.0F, exposure);

			if ( const auto value = domeLight->file.get_value(); value.has_value() )
			{
				tinyusdz::value::AssetPath assetPath;

				if ( value.value().get_scalar(&assetPath) )
				{
					const auto & rawPath = assetPath.GetAssetPath();

					if ( !rawPath.empty() )
					{
						std::error_code pathError;
						const auto fullPath = std::filesystem::weakly_canonical(stageDirectory / rawPath, pathError);

						if ( !pathError && std::filesystem::exists(fullPath) )
						{
							descriptor.textureAssetPath = fullPath.string();
						}
						else
						{
							TraceWarning{ClassId} << "Dome light image '" << rawPath << "' not found next to the stage.";
						}
					}
				}
			}

			TraceInfo{ClassId} <<
				"Environment light '" << descriptor.name << "': intensity " << descriptor.intensity <<
				", image '" << ( descriptor.textureAssetPath.empty() ? "<none>" : descriptor.textureAssetPath ) << "'.";

			output.lights.emplace_back(std::move(descriptor));
		}

		for ( const auto & child : prim.children() )
		{
			USDLoader::collectEnvironmentLights(child, stageDirectory, output);
		}
	}

	void
	USDLoader::collectInstancers (const tinyusdz::Prim & prim, const std::string & primPath, float metersPerUnit, std::vector< Instancer > & instancers) noexcept
	{
		using namespace Base::Math;

		if ( const auto * pointInstancer = prim.as< tinyusdz::GeomPointInstancer >(); pointInstancer != nullptr )
		{
			Instancer instancer;
			instancer.path = primPath;

			/* ⚠️ AXIS BAKE, INSTANCE EDITION. The vertices of a prototype went through
			 *
			 *     C : engine = (usd.x, usd.z, -usd.y)
			 *
			 * in buildMeshes(). An instance carries a TRANSFORM, not a point, so it cannot be fed
			 * through C directly: a transform changes basis by CONJUGATION, C·T·C⁻¹. Applied to an
			 * already-baked prototype vertex, that reproduces the source placement exactly:
			 *
			 *     C·(T·p) = (C·T·C⁻¹)·(C·p)
			 *
			 * C is the rotation of +90° about X (it sends y to z and z to -y, determinant +1), so
			 * the conjugation has a closed form on each part of the transform, and no matrix is
			 * needed:
			 *   - a translation is simply carried through C;
			 *   - a rotation quaternion is conjugated by C's own quaternion;
			 *   - a scale, being sign-blind, has its Y and Z factors SWAPPED.
			 *
			 * Skipping the conjugation and merely permuting the position is the trap: the forest
			 * lands in the right places, and every plant is rotated wrong — which reads as a bad
			 * asset rather than a bad conversion. */
			/* ⚠️ The axis flip is COMPOSED INTO the bake — one place, so an instance can never end
			 * up mirrored differently from the prototype it draws. */
			const auto bakePosition = [metersPerUnit] (const auto & position) {
				return Vector< 3, float >{static_cast< float >(position[0]) * metersPerUnit, static_cast< float >(position[2]) * metersPerUnit, -static_cast< float >(position[1]) * metersPerUnit};
			};

			/* The quaternion of C, a +90° rotation about X: (sin(45°), 0, 0, cos(45°)). */
			const Quaternion< float > axisChange{std::numbers::sqrt2_v< float > / 2.0F, 0.0F, 0.0F, std::numbers::sqrt2_v< float > / 2.0F};
			const auto axisChangeInverse = axisChange.conjugated();

			std::vector< tinyusdz::value::point3f > positions;
			std::vector< tinyusdz::value::quath > orientations;
			std::vector< tinyusdz::value::float3 > scales;

			if ( const auto value = pointInstancer->positions.get_value(); value.has_value() )
			{
				if ( !value.value().get_scalar(&positions) )
				{
					TraceWarning{ClassId} << "Instancer '" << primPath << "' has time-sampled positions, which are not read.";
				}
			}

			if ( const auto value = pointInstancer->protoIndices.get_value(); value.has_value() )
			{
				std::vector< int32_t > indices;

				if ( value.value().get_scalar(&indices) )
				{
					instancer.prototypeIndices = std::move(indices);
				}
			}

			if ( const auto value = pointInstancer->orientations.get_value(); value.has_value() )
			{
				(void)value.value().get_scalar(&orientations);
			}

			if ( const auto value = pointInstancer->scales.get_value(); value.has_value() )
			{
				(void)value.value().get_scalar(&scales);
			}

			/* The prototypes relationship is the ONLY thing tying an instance to what it draws:
			 * `protoIndices[i]` indexes INTO this target list, not into anything global. */
			if ( pointInstancer->prototypes.has_value() )
			{
				const auto & relation = pointInstancer->prototypes.value();

				if ( relation.is_path() )
				{
					instancer.prototypePaths.emplace_back(relation.targetPath.full_path_name());
				}
				else if ( relation.is_pathvector() )
				{
					for ( const auto & target : relation.targetPathVector )
					{
						instancer.prototypePaths.emplace_back(target.full_path_name());
					}
				}
			}

			instancer.instances.reserve(positions.size());

			for ( size_t index = 0; index < positions.size(); ++index )
			{
				auto rotation = Quaternion< float >{};

				if ( index < orientations.size() )
				{
					/* USD stores an orientation as a HALF-precision quaternion, real part first.
					 * The engine's constructor takes (x, y, z, w). */
					const auto & source = orientations[index];

					const Quaternion< float > sourceRotation{
						tinyusdz::value::half_to_float(source.imag[0]),
						tinyusdz::value::half_to_float(source.imag[1]),
						tinyusdz::value::half_to_float(source.imag[2]),
						tinyusdz::value::half_to_float(source.real)
					};

					rotation = axisChange * sourceRotation * axisChangeInverse;
					rotation.normalize();
				}

				auto scale = Vector< 3, float >{1.0F, 1.0F, 1.0F};

				if ( index < scales.size() )
				{
					scale[0] = scales[index][0];
					scale[1] = scales[index][2];
					scale[2] = scales[index][1];
				}

				/* ⚠️ The instance rotation is conjugated by the flip AFTER the USD axis change, and
				 * the scale is left alone (M·diag(s)·M = diag(s)). Mirroring the position without
				 * the rotation puts every plant in the right spot facing the wrong way. */
				instancer.instances.emplace_back(CartesianFrame< float >::fromQuaternion(bakePosition(positions[index]), rotation, scale));
			}

			if ( !instancer.instances.empty() )
			{
				instancers.emplace_back(std::move(instancer));
			}
			else
			{
				TraceWarning{ClassId} << "Instancer '" << primPath << "' declares no usable position.";
			}
		}

		for ( const auto & child : prim.children() )
		{
			USDLoader::collectInstancers(child, primPath + "/" + child.element_name(), metersPerUnit, instancers);
		}
	}

	size_t
	USDLoader::buildInstanceSets (const std::vector< Instancer > & instancers, const std::map< std::string, size_t > & builtMeshesByPath, SceneData & output) noexcept
	{
		size_t instanceTotal = 0;

		for ( const auto & instancer : instancers )
		{
			for ( size_t prototypeIndex = 0; prototypeIndex < instancer.prototypePaths.size(); ++prototypeIndex )
			{
				const auto & prototypePath = instancer.prototypePaths[prototypeIndex];

				/* A prototype is a SUBTREE, not a mesh: it may hold several meshes, and each of
				 * them must follow every instance. Matching on the path prefix is what keeps the
				 * two apart without assuming a shape the asset never promised. */
				std::vector< size_t > prototypeMeshIndices;

				for ( const auto & [meshPath, meshIndex] : builtMeshesByPath )
				{
					if ( meshPath == prototypePath || meshPath.starts_with(prototypePath + "/") )
					{
						prototypeMeshIndices.push_back(meshIndex);
					}
				}

				if ( prototypeMeshIndices.empty() )
				{
					TraceWarning{ClassId} << "Instancer '" << instancer.path << "' references prototype '" << prototypePath << "', which produced no mesh.";

					continue;
				}

				/* An instance whose protoIndices entry does not name THIS prototype belongs to
				 * another one. An instancer with a single prototype and no index array is the
				 * common case and means "all of them". */
				std::vector< Base::Math::CartesianFrame< float > > selected;

				if ( instancer.prototypeIndices.empty() )
				{
					selected = instancer.instances;
				}
				else
				{
					selected.reserve(instancer.instances.size());

					for ( size_t index = 0; index < instancer.instances.size(); ++index )
					{
						if ( index < instancer.prototypeIndices.size() && static_cast< size_t >(instancer.prototypeIndices[index]) == prototypeIndex )
						{
							selected.push_back(instancer.instances[index]);
						}
					}
				}

				if ( selected.empty() )
				{
					continue;
				}

				for ( const auto meshIndex : prototypeMeshIndices )
				{
					InstanceSetDescriptor descriptor;
					descriptor.name = prototypePath;
					descriptor.meshIndex = meshIndex;
					descriptor.instances = selected;

					output.instanceSets.emplace_back(std::move(descriptor));
				}

				instanceTotal += selected.size() * prototypeMeshIndices.size();
			}
		}

		return instanceTotal;
	}

	std::filesystem::path
	USDLoader::findCaseInsensitive (const std::filesystem::path & wanted) noexcept
	{
		std::error_code pathError;

		const auto directory = wanted.parent_path();

		if ( !std::filesystem::is_directory(directory, pathError) || pathError )
		{
			return {};
		}

		const auto lowered = [] (std::string text) {
			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		};

		const auto target = lowered(wanted.filename().string());

		const std::filesystem::directory_iterator end;

		for ( auto it = std::filesystem::directory_iterator{directory, std::filesystem::directory_options::skip_permission_denied, pathError}; !pathError && it != end; it.increment(pathError) )
		{
			if ( lowered(it->path().filename().string()) == target )
			{
				return it->path();
			}
		}

		return {};
	}

	void
	USDLoader::collectLightPlacements (const tinyusdz::tydra::XformNode & node, std::map< std::string, LightPlacement > & placements) noexcept
	{
		/* Every node is recorded, not just the lights: the map is keyed by absolute prim path and the
		 * caller looks up whatever Tydra reports, so filtering by prim type here would only be a
		 * second place to keep in sync with UsdLux. A whole stage is a few thousand entries. */
		const auto & world = node.get_world_matrix();

		/* ⚠️ USD is a ROW-VECTOR convention: the translation sits in the LAST ROW (m[3][0..2]) and the
		 * local basis vectors are the ROWS, not the columns. Reading it column-major transposes the
		 * rotation — which does not fail, it just aims every light somewhere plausible and wrong. */
		LightPlacement placement;

		placement.position = {
			static_cast< float >(world.m[3][0]),
			static_cast< float >(world.m[3][1]),
			static_cast< float >(world.m[3][2])
		};

		/* ⚠️ A UsdLux light emits along its own LOCAL -Z. That is the specification for DiskLight,
		 * RectLight, DistantLight and the shaping cone alike — not -Y, and not the stage's up axis
		 * (this stage is Z-up, which makes the two easy to confuse). */
		placement.direction = {
			-static_cast< float >(world.m[2][0]),
			-static_cast< float >(world.m[2][1]),
			-static_cast< float >(world.m[2][2])
		};

		placements.emplace(node.absolute_path.full_path_name(), placement);

		for ( const auto & child : node.children )
		{
			USDLoader::collectLightPlacements(child, placements);
		}
	}

	size_t
	USDLoader::buildLights (const tinyusdz::tydra::RenderScene & renderScene, float metersPerUnit, const std::map< std::string, LightPlacement > & placements, SceneData & output) noexcept
	{
		using namespace Base::Math;

		/* ⚠️ THE ANCHOR. See the header for what was measured and why this factor and not another.
		 * `intensity` is read as a luminance in cd/m², scaled by the emitter's area because
		 * `normalize = false` is what every light of this asset declares, then normalized by 4*pi. */
		const auto toCandela = [] (const tinyusdz::tydra::RenderLight & light, float area) {
			return light.intensity * std::pow(2.0F, light.exposure) * area / (4.0F * std::numbers::pi_v< float >);
		};

		size_t translated = 0;
		size_t unplaced = 0;

		for ( const auto & renderLight : renderScene.lights )
		{
			LightDescriptor descriptor;
			descriptor.name = renderLight.name.empty() ? "usd-light-" + std::to_string(translated) : renderLight.name;
			descriptor.color = {renderLight.color[0], renderLight.color[1], renderLight.color[2], 1.0F};

			/* ⚠️⚠️ THE PLACEMENT COMES FROM THE XFORM TREE, NEVER FROM `renderLight`. Tydra's light
			 * converter fills seventeen fields — colour, intensity, exposure, temperature, cone,
			 * shadows — and touches `transform`, `position` and `direction` in NO code path at all
			 * (`render-light-converter.cc` contains zero occurrences of the three words). They keep
			 * the struct's defaults: position (0,0,0) and direction (0,-1,0).
			 *
			 * Measured on WorldLobby.usdz: 29 fixtures read at their correct 3750 cd, all stacked at
			 * the world ORIGIN twenty metres from the room they belong to, aiming horizontally once
			 * baked. The frame came out PURE BLACK with a light set reporting 25 spots and 4 points,
			 * and not one warning anywhere — the loader had faithfully read fields nobody wrote.
			 *
			 * Joined on `abs_path`, which the converter DOES fill and which is unique by
			 * construction. The element name is NOT usable: all 25 ceiling fixtures of this asset are
			 * named "LightBloomDisc". */
			const auto placement = placements.find(renderLight.abs_path);

			if ( placement == placements.cend() )
			{
				/* Reported, never guessed: a light silently left at the origin is precisely the defect
				 * described above, and it must not be reintroduced by a failed lookup. */
				unplaced++;

				TraceWarning{ClassId} <<
					"Light '" << renderLight.abs_path << "' has no entry in the stage's xform tree; "
					"it is dropped rather than placed at the origin.";

				continue;
			}

			/* Radii and lengths are LENGTHS: they go through `metersPerUnit` like a position. */
			const auto radius = renderLight.radius * metersPerUnit;

			switch ( renderLight.type )
			{
				/* A dome has an image and no position. It is collected as an Environment by
				 * collectEnvironmentLights(), which also reads the image path Tydra drops. */
				case tinyusdz::tydra::RenderLight::Type::Dome :
				case tinyusdz::tydra::RenderLight::Type::Portal :
					continue;

				case tinyusdz::tydra::RenderLight::Type::Distant :
					descriptor.type = LightType::Directional;
					/* A directional light carries an ILLUMINANCE in lux and has no area — the
					 * 4*pi normalisation of a punctual emitter does not apply to a parallel beam. */
					descriptor.intensity = renderLight.intensity * std::pow(2.0F, renderLight.exposure);
					break;

				case tinyusdz::tydra::RenderLight::Type::Point :
				case tinyusdz::tydra::RenderLight::Type::Sphere :
					descriptor.type = LightType::Point;
					/* Sphere: the full 4*pi*r^2. */
					descriptor.intensity = toCandela(renderLight, 4.0F * std::numbers::pi_v< float > * radius * radius);
					break;

				case tinyusdz::tydra::RenderLight::Type::Disk :
				case tinyusdz::tydra::RenderLight::Type::Rect :
				case tinyusdz::tydra::RenderLight::Type::Cylinder :
				case tinyusdz::tydra::RenderLight::Type::Geometry :
				{
					const auto area = renderLight.type == tinyusdz::tydra::RenderLight::Type::Rect
						? renderLight.width * metersPerUnit * renderLight.height * metersPerUnit
						: std::numbers::pi_v< float > * radius * radius;

					/* ⚠️ A cone narrower than the full hemisphere makes it a SPOT, and the engine
					 * needs both angles. USD carries one angle plus a SOFTNESS in [0,1]; the inner
					 * cone is that softness eating into the outer one. A softness of 0 therefore
					 * gives a hard edge (inner == outer), which is what these fixtures declare. */
					if ( renderLight.shapingConeAngle < 180.0F )
					{
						descriptor.type = LightType::Spot;
						descriptor.outerConeAngle = renderLight.shapingConeAngle;
						descriptor.innerConeAngle = renderLight.shapingConeAngle * (1.0F - std::clamp(renderLight.shapingConeSoftness, 0.0F, 1.0F));
					}
					else
					{
						descriptor.type = LightType::Point;
					}

					descriptor.intensity = toCandela(renderLight, area);
				}
					break;
			}

			output.lights.emplace_back(descriptor);

			/* ⚠️ A LIGHT WITHOUT A NODE IS NEVER INSTANTIATED. The descriptor carries no transform
			 * on purpose — the placement belongs to the hierarchy — so a node must be emitted and
			 * declared a root, exactly as buildMeshes() does for a mesh. Forgetting this yields a
			 * scene whose light list is full and whose image is black, with nothing reported. */
			NodeDescriptor node;
			node.name = "usd:light/" + descriptor.name + "-" + std::to_string(output.lights.size() - 1);
			node.lightIndex = output.lights.size() - 1;

			/* Same bake as the geometry — engine = (x, z, -y) * metersPerUnit. A light placed with
			 * any other convention lands somewhere else than the room it is meant to light. */
			node.localFrame.setPosition(placement->second.position[0] * metersPerUnit,
				placement->second.position[2] * metersPerUnit,
				-placement->second.position[1] * metersPerUnit);

			/* A spot and a directional light AIM. The direction goes through the direction bake —
			 * no `metersPerUnit`, a direction has no length to convert. */
			if ( descriptor.type == LightType::Spot || descriptor.type == LightType::Directional )
			{
				/* ⚠️ The aim is mirrored with the placement: a light moved but not re-aimed lights
				 * the wrong wall, and nothing reports it. */
				const auto direction = Vector< 3, float >{
					placement->second.direction[0],
					placement->second.direction[2],
					-placement->second.direction[1]};

				if ( !direction.isZero() )
				{
					/* A frame is oriented toward a TARGET, so the direction becomes a point one
					 * unit ahead of the light. */
					node.localFrame.lookAt(node.localFrame.position() + direction.normalized(), false);
				}
			}

			output.nodes.emplace_back(std::move(node));
			output.rootNodeIndices.push_back(output.nodes.size() - 1);

			translated++;
		}

		if ( translated > 0 )
		{
			TraceInfo{ClassId} << translated << " punctual light(s) translated and placed.";
		}

		if ( unplaced > 0 )
		{
			TraceWarning{ClassId} << unplaced << " light(s) dropped for want of a placement — see the warnings above.";
		}

		return translated;
	}

	std::shared_ptr< Graphics::TextureResource::Abstract >
	USDLoader::archiveTexture (const std::string & assetIdentifier, bool sRGB) noexcept
	{
		using namespace Graphics;
		using namespace Base::PixelFactory;

		std::string entryName;

		if ( !m_archive->resolve(assetIdentifier, {}, entryName) )
		{
			TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' is not in the archive; the material keeps its flat colour.";

			return nullptr;
		}

		/* ⚠️ The format comes from the ENTRY NAME, and only Jpeg, PNG and Targa can be decoded from
		 * memory. A format we cannot read must be refused HERE: handing such an entry to the image
		 * container yields a resource that fails to load, the texture built on it fails in turn, and
		 * the material goes down with it — strictly worse than no texture, which merely falls back
		 * to the shader's flat colour. */
		const auto extension = [&entryName] () {
			const auto dot = entryName.find_last_of('.');

			auto text = dot == std::string::npos ? std::string{} : entryName.substr(dot + 1);

			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		}();

		auto format = Pixmap< uint8_t >::Format::None;

		if ( extension == "jpg" || extension == "jpeg" )
		{
			format = Pixmap< uint8_t >::Format::Jpeg;
		}
		else if ( extension == "png" )
		{
			format = Pixmap< uint8_t >::Format::PNG;
		}
		else if ( extension == "tga" )
		{
			format = Pixmap< uint8_t >::Format::Targa;
		}
		else
		{
			TraceWarning{ClassId} <<
				"Archived texture '" << entryName << "' uses the '" << extension <<
				"' format, which cannot be decoded from memory; the material keeps its flat colour. "
				"StreamIO reads JPEG, PNG and Targa.";

			return nullptr;
		}

		/* ⚠️⚠️ The archive is captured BY VALUE. `getOrCreateResource()` runs this factory on the
		 * THREAD POOL, so it executes AFTER load() has returned and after the loader itself may be
		 * gone; a bare pointer into the mapping would be read from an unmapped range, minutes later,
		 * on a worker. The shared pointer keeps the mapping alive exactly as long as some factory
		 * still needs it, and costs nothing but a refcount. */
		auto image = m_resources.container< ImageResource >()
			->getOrCreateResource(m_resourcePrefix + "/image/" + entryName, [archive = m_archive, entryName, format] (auto & imageResource) {
				size_t size = 0;
				const auto * data = archive->bytes(entryName, size);

				if ( data == nullptr )
				{
					return false;
				}

				Pixmap< uint8_t > pixmap;

				constexpr ReadOptions options{
					.targetChannelMode = TargetChannelMode::RGBA
				};

				/* Decoded IN PLACE from the mapping — no extraction, no intermediate copy. */
				if ( !StreamIO::read(data, size, format, pixmap, options) )
				{
					return false;
				}

				return imageResource.load(std::move(pixmap));
			});

		if ( image == nullptr )
		{
			return nullptr;
		}

		return m_resources.container< TextureResource::Texture2D >()
			->getOrCreateResource(m_resourcePrefix + "/texture/" + entryName + ( sRGB ? "-srgb" : "-data" ), [image, sRGB] (auto & textureResource) {
				/* Set sRGB BEFORE load(): the flag must be in place when the VkImage is created. */
				textureResource.enableSRGB(sRGB);

				return textureResource.load(image);
			});
	}

	std::vector< std::shared_ptr< Graphics::Material::Interface > >
	USDLoader::buildMaterials (const tinyusdz::tydra::RenderScene & renderScene, const std::filesystem::path & stageDirectory) noexcept
	{
		using namespace Graphics;
		using namespace Base::PixelFactory;

		std::vector< std::shared_ptr< Material::Interface > > materials;
		materials.reserve(renderScene.materials.size());

		/* Turns a shader parameter's texture reference into an engine texture.
		 * ⚠️ sRGB is NOT cosmetic: a base colour is authored in sRGB, while roughness, metalness
		 * and normals are DATA and must stay linear. Getting it wrong washes out or darkens
		 * everything in a way that looks like a lighting bug. */
		const auto resolveTexture = [this, &renderScene, &stageDirectory] (int32_t textureId, bool sRGB) -> std::shared_ptr< TextureResource::Abstract > {
			if ( textureId < 0 || static_cast< size_t >(textureId) >= renderScene.textures.size() )
			{
				return nullptr;
			}

			const auto imageId = renderScene.textures[static_cast< size_t >(textureId)].texture_image_id;

			if ( imageId < 0 || static_cast< size_t >(imageId) >= renderScene.images.size() )
			{
				return nullptr;
			}

			const auto & assetIdentifier = renderScene.images[static_cast< size_t >(imageId)].asset_identifier;

			if ( assetIdentifier.empty() )
			{
				return nullptr;
			}

			/* An archive image never exists on disk: it is decoded straight out of the mapping. */
			if ( m_archive != nullptr )
			{
				return this->archiveTexture(assetIdentifier, sRGB);
			}

			std::error_code pathError;
			auto fullPath = std::filesystem::weakly_canonical(stageDirectory / assetIdentifier, pathError);

			if ( pathError || !std::filesystem::exists(fullPath) )
			{
				/* ⚠️⚠️ CASE. An asset authored on Windows or macOS records whatever spelling the
				 * DCC felt like, on a filesystem that does not care. On Linux it does, and the
				 * texture simply is not found. Measured on Jungle Ruins: the very same material
				 * asks for `anthurium_botany_01_BaseColor.tif` while the file on disk is
				 * `Anthurium_Botany_01_BaseColor.tif` — and asks for `..._Normal.png` with the
				 * right case.
				 *
				 * The result is the WORST kind of failure to read: the normal map lands, the base
				 * colour does not, so the geometry is lit, detailed, and pure white. It looks like
				 * a lighting or material bug, and nothing points at a filename.
				 *
				 * So a miss is retried case-insensitively within the directory. Bounded to one
				 * listing of one directory, and only on a path that already failed. */
				fullPath = USDLoader::findCaseInsensitive(fullPath);

				if ( fullPath.empty() )
				{
					TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' not found next to the stage.";

					return nullptr;
				}

				TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' only matched by IGNORING CASE: '" << fullPath.filename().string() << "'. The asset's spelling is wrong for a case-sensitive filesystem.";
			}

			/* ⚠️⚠️ A FORMAT WE CANNOT DECODE MUST NOT REACH THE RESOURCE MANAGER. `PixelFactory`
			 * reads JPEG, PNG, Targa and HDR — and Jungle Ruins ships its plant base colours as
			 * 4096² TIFF. Handing such a path to the image container yields a resource that FAILS
			 * to load, the texture built on it fails in turn, and the material goes down with it:
			 * the plants stopped rendering ENTIRELY.
			 *
			 * That is strictly worse than not finding the file at all, which simply falls back to
			 * the shader's flat colour — and it is exactly the trap this project's "loading never
			 * returns a broken resource" rule exists to prevent. So the extension is checked
			 * FIRST, and an unreadable one is reported and skipped, colour intact. */
			const auto extension = [] (std::string text) {
				std::ranges::transform(text, text.begin(), [] (unsigned char character) {
					return static_cast< char >(std::tolower(character));
				});

				return text;
			}(fullPath.extension().string());

			if ( extension != ".jpg" && extension != ".jpeg" && extension != ".png" && extension != ".tga" && extension != ".hdr" && extension != ".tif" && extension != ".tiff" )
			{
				TraceWarning{ClassId} <<
					"Texture '" << fullPath.filename().string() << "' uses the unsupported '" << extension <<
					"' format; the material keeps its flat colour. PixelFactory reads JPEG, PNG, Targa, TIFF and HDR.";

				return nullptr;
			}

			const auto imageName = m_resourcePrefix + "/image/" + fullPath.filename().string();

			auto image = m_resources.container< ImageResource >()
				->getOrCreateResource(imageName, [fullPath] (auto & imageResource) {
					Pixmap< uint8_t > pixmap;

					constexpr ReadOptions options{
						.targetChannelMode = TargetChannelMode::RGBA
					};

					if ( !FileIO::read(fullPath, pixmap, options) )
					{
						return false;
					}

					return imageResource.load(std::move(pixmap));
				});

			if ( image == nullptr )
			{
				return nullptr;
			}

			return m_resources.container< TextureResource::Texture2D >()
				->getOrCreateResource(m_resourcePrefix + "/texture/" + fullPath.filename().string() + ( sRGB ? "-srgb" : "-data" ), [image, sRGB] (auto & textureResource) {
					/* Set sRGB BEFORE load(): the flag must be in place when the VkImage is created. */
					textureResource.enableSRGB(sRGB);

					return textureResource.load(image);
				});
		};

		size_t materialIndex = 0;

		for ( const auto & renderMaterial : renderScene.materials )
		{
			const auto materialName = m_resourcePrefix + "/material/" + ( renderMaterial.name.empty() ? std::to_string(materialIndex) : renderMaterial.name ) + "-" + std::to_string(materialIndex);

			/* ⚠️ `getOrCreateResource()` runs this factory ON THE THREAD POOL. Every capture must
			 * be BY VALUE: a reference to `renderMaterial` — owned by the render scene, itself a
			 * local of load() — dangles the moment load() returns, and the crash lands far from
			 * here. The textures are therefore resolved BEFORE the lambda, on this thread, and
			 * only the resulting shared pointers and plain values cross over. */
			const auto & shaderSource = renderMaterial.surfaceShader;

			const auto albedoTexture = shaderSource.has_value() ? resolveTexture(shaderSource.value().diffuseColor.texture_id, true) : nullptr;
			const auto roughnessTexture = shaderSource.has_value() ? resolveTexture(shaderSource.value().roughness.texture_id, false) : nullptr;
			const auto metalnessTexture = shaderSource.has_value() ? resolveTexture(shaderSource.value().metallic.texture_id, false) : nullptr;
			const auto normalTexture = shaderSource.has_value() ? resolveTexture(shaderSource.value().normal.texture_id, false) : nullptr;

			/* ⚠️ OPACITY IS PART OF THE SURFACE, and ignoring it does not make glass "a bit wrong":
			 * a UsdPreviewSurface pane declares `inputs:diffuseColor = (0, 0, 0)` with
			 * `inputs:opacity = 0` — black BECAUSE it is meant to be seen through. Read as an opaque
			 * material, a whole curtain wall renders as a solid BLACK BLOCK, and the failure reads as
			 * a lighting or a material bug rather than a missing input.
			 *
			 * Measured on World Lobby: exactly 6 materials carry opacity 0 (`Glass` ×4,
			 * `Clear_Glass`, `Tinted_Glass`), each with a real `ior` (1.20 to 1.52); all 135 others
			 * are opaque. So this is six materials deciding whether the building has windows.
			 *
			 * ⚠️ `opacityThreshold` FIRST: a non-zero threshold is a CUTOUT — an alpha TEST that
			 * keeps the material opaque (opaque list, depth write, no sorting), never blending.
			 * Reaching for blending there would defeat the flag and cost sorting for nothing.
			 *
			 * NOT read yet, and deliberately: `ior`. Refraction and its Fresnel belong to the
			 * transmission path (`setTransmissionComponent()`), which is a separate calibration —
			 * mapping opacity to plain alpha blending already turns the black block into glazing. */
			const auto opacityValue = shaderSource.has_value() ? std::clamp(shaderSource.value().opacity.value, 0.0F, 1.0F) : 1.0F;
			const auto opacityThreshold = shaderSource.has_value() ? shaderSource.value().opacityThreshold.value : 0.0F;
			const auto isCutout = opacityThreshold > 0.0F;
			const auto isTranslucent = !isCutout && opacityValue < 1.0F;

			const auto albedoValue = shaderSource.has_value()
				? Color< float >{shaderSource.value().diffuseColor.value[0], shaderSource.value().diffuseColor.value[1], shaderSource.value().diffuseColor.value[2], opacityValue}
				: Color< float >{0.5F, 0.5F, 0.5F, 1.0F};
			const auto roughnessValue = shaderSource.has_value() ? shaderSource.value().roughness.value : 0.5F;
			const auto metalnessValue = shaderSource.has_value() ? shaderSource.value().metallic.value : 0.0F;

			if ( isTranslucent || isCutout )
			{
				TraceInfo{ClassId} <<
					"Material '" << materialName << "' is " << ( isCutout ? "a CUTOUT (alpha test)" : "TRANSLUCENT" ) <<
					" — opacity " << opacityValue << ", threshold " << opacityThreshold << ".";
			}

			auto material = m_resources.container< Material::StandardResource >()
				->getOrCreateResource(materialName, [albedoTexture, roughnessTexture, metalnessTexture, normalTexture, albedoValue, roughnessValue, metalnessValue, isTranslucent, isCutout, opacityValue, opacityThreshold] (auto & materialResource) {
					if ( albedoTexture != nullptr )
					{
						materialResource.setAlbedoComponent(albedoTexture);
					}
					else
					{
						materialResource.setAlbedoComponent(albedoValue);
					}

					if ( roughnessTexture != nullptr )
					{
						materialResource.setRoughnessComponent(roughnessTexture);
					}
					else
					{
						materialResource.setRoughnessComponent(roughnessValue);
					}

					if ( metalnessTexture != nullptr )
					{
						materialResource.setMetalnessComponent(metalnessTexture);
					}
					else
					{
						materialResource.setMetalnessComponent(metalnessValue);
					}

					if ( normalTexture != nullptr )
					{
						materialResource.setNormalComponent(normalTexture);
					}

					/* USD opacity, following the engine's opacity contract. A CUTOUT
					 * (opacityThreshold > 0) is a binary alpha test on the albedo alpha at the
					 * authored threshold — the material stays opaque, casts cutout shadows and
					 * alpha-tests at RT hit time. A TRANSLUCENT material (opacity < 1, no
					 * threshold) gets the global opacity value — setOpacityComponent() enables
					 * the blending itself.
					 *
					 * No material of the reference asset exercises the cutout branch: all 141
					 * report `opacityThreshold = 0`. */
					if ( isCutout )
					{
						materialResource.enableAlphaTest(opacityThreshold);
					}
					else if ( isTranslucent )
					{
						materialResource.setOpacityComponent(opacityValue);
					}

					return materialResource.setManualLoadSuccess(true);
				});

			materials.emplace_back(std::move(material));

			materialIndex++;
		}

		TraceInfo{ClassId} << materials.size() << " materials translated.";

		return materials;
	}

	size_t
	USDLoader::buildMeshes (const tinyusdz::tydra::RenderScene & renderScene, float metersPerUnit, const std::vector< std::string > & prototypePaths, const std::vector< std::shared_ptr< Graphics::Material::Interface > > & materials, SceneData & output, std::map< std::string, size_t > & builtMeshesByPath) noexcept
	{
		using namespace Graphics;
		using namespace Base::Math;
		using namespace Base::VertexFactory;

		/* ⚠️ AXIS AND UNIT BAKING — the single site where USD's convention is left behind.
		 *
		 * USD here is Z-up right-handed; the engine's law is Y-UP (docs/coordinate-system.md).
		 * A point one unit "up" in USD is (0, 0, 1) and must become (0, +1, 0):
		 *
		 *     engine.x =  usd.x        engine.y =  usd.z        engine.z = -usd.y
		 *
		 * ⚠️ The Z sign is NOT decoration: flipping the vertical axis alone gives a determinant
		 * of -1, which mirrors the geometry instead of turning it. Negating Z as well keeps the
		 * determinant at +1, so handedness — and therefore the winding — survives the bake.
		 * Verified on screen: the first attempt (y = -usd.z, z = usd.y) rendered upside down.
		 *
		 * applied to POSITIONS, NORMALS and TANGENTS alike. Converting positions and forgetting
		 * the vectors is the classic failure of this exercise: the shape looks right and the
		 * lighting is subtly wrong, which then gets blamed on the material system.
		 *
		 * `metersPerUnit` scales positions ONLY — a direction has no length to convert. */
		/* ⚠️ The SAME axis bake feeds positions, normals AND tangents, so they cannot drift apart.
		 * A normal needs no special case: the inverse-transpose of a diagonal ±1 matrix is the
		 * matrix itself. (The per-asset AxisFlip layer that used to compose in here is deleted.) */

		const auto bakePosition = [metersPerUnit] (const auto & p) {
			return Vector< 3, float >{static_cast< float >(p[0]) * metersPerUnit, static_cast< float >(p[2]) * metersPerUnit, -static_cast< float >(p[1]) * metersPerUnit};
		};

		const auto bakeDirection = [] (const auto & d) {
			return Vector< 3, float >{static_cast< float >(d[0]), static_cast< float >(d[2]), -static_cast< float >(d[1])};
		};

		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			return m_resources.container< Material::StandardResource >()->getDefaultResource();
		};

		/* ⚠️ The node hierarchy is where a mesh gets its PLACE and its SIZE. Tydra pre-computes
		 * `global_matrix` for every node, so nothing has to be accumulated here — but ignoring
		 * it altogether leaves every mesh at the origin, at its authoring scale. Measured on
		 * Jungle Ruins, that turned the pyramid into a speck a few pixels across.
		 *
		 * A mesh referenced by several nodes is therefore built once per node: same geometry,
		 * different placement. */
		/* ⚠️ THREE KINDS OF MESH COME OUT OF TYDRA, and only one of them is drawn where it stands.
		 *
		 * 1. `/_class_/…` — USD CLASSES. Abstract templates that exist to be inherited from; they
		 *    are not scene content. Tydra converts them like anything else, and drawing them puts
		 *    a full copy of every prototype at the asset's origin, on top of each other. Measured
		 *    on one Anthurium element: 12 meshes out of Tydra for 6 real prototypes.
		 * 2. A PROTOTYPE subtree of a PointInstancer — built, kept, but NEVER given a node: it is
		 *    drawn by its instances, and a node here would draw it one extra time, alone, at
		 *    whatever place the asset stores its prototypes.
		 * 3. Everything else — ordinary geometry, which gets its node as usual.
		 *
		 * Whether a class ends up drawn is not a matter of taste: `_class_` is USD's own marker
		 * for "not renderable", and honouring it is part of reading the format. */
		std::vector< std::tuple< size_t, tinyusdz::value::matrix4d, std::string > > drawList;

		size_t skippedClassCount = 0;

		std::map< std::string, tinyusdz::value::matrix4d > prototypeRootMatrices;

		const auto collectNodes = [&drawList, &skippedClassCount, &prototypeRootMatrices, &prototypePaths] (auto & self, const tinyusdz::tydra::Node & node) -> void {
			if ( node.abs_path.starts_with("/_class_") )
			{
				skippedClassCount++;

				return;
			}

			if ( std::ranges::find(prototypePaths, node.abs_path) != prototypePaths.end() )
			{
				prototypeRootMatrices.emplace(node.abs_path, node.global_matrix);
			}

			if ( node.nodeType == tinyusdz::tydra::NodeType::Mesh && node.id >= 0 )
			{
				drawList.emplace_back(static_cast< size_t >(node.id), node.global_matrix, node.abs_path);
			}

			for ( const auto & child : node.children )
			{
				self(self, child);
			}
		};

		for ( const auto & node : renderScene.nodes )
		{
			collectNodes(collectNodes, node);
		}

		const auto isPrototype = [&prototypePaths] (const std::string & path) {
			return std::ranges::any_of(prototypePaths, [&path] (const auto & prototypePath) {
				return path == prototypePath || path.starts_with(prototypePath + "/");
			});
		};

		/* ⚠️ A prototype's own placement is NOT its instances' placement. Its mesh is baked in the
		 * prototype's local space and every instance then positions it; folding the prototype's
		 * world matrix in as well would offset the entire forest by wherever the asset happens to
		 * park its prototypes.
		 *
		 * Tydra only hands out ABSOLUTE matrices, so this pass assumes a prototype root sits at
		 * the identity — true of every element of Jungle Ruins. Rather than trust that silently,
		 * the assumption is CHECKED: a prototype root carrying a transform says so in the log
		 * instead of producing a forest quietly shifted off the terrain. */
		for ( const auto & [primPath, matrix] : prototypeRootMatrices )
		{
			bool isIdentity = true;

			for ( size_t row = 0; row < 4 && isIdentity; ++row )
			{
				for ( size_t column = 0; column < 4; ++column )
				{
					const auto expected = ( row == column ) ? 1.0 : 0.0;

					if ( std::abs(matrix.m[row][column] - expected) > 1e-5 )
					{
						isIdentity = false;

						break;
					}
				}
			}

			if ( !isIdentity )
			{
				TraceWarning{ClassId} <<
					"Prototype '" << primPath << "' carries a transform of its own, which this pass does not "
					"factor out: its instances will be offset by it.";
			}
		}

		TraceInfo{ClassId} <<
			drawList.size() << " mesh placements collected from the node hierarchy (" <<
			skippedClassCount << " class subtrees skipped, " << prototypePaths.size() << " instancer prototypes declared).";

		size_t builtCount = 0;
		size_t meshIndex = 0;

		for ( const auto & [sourceMeshIndex, worldMatrix, primPath] : drawList )
		{
			if ( sourceMeshIndex >= renderScene.meshes.size() )
			{
				meshIndex++;

				continue;
			}

			const bool instancedOnly = isPrototype(primPath);

			const auto & renderMesh = renderScene.meshes[sourceMeshIndex];

			/* USD matrices are row-major and applied to ROW vectors: p' = p * M, so the
			 * translation sits in row 3. Getting this transposed is silent: a symmetric
			 * transform still looks plausible. */
			const auto toWorldPosition = [&worldMatrix] (const auto & p) {
				const auto x = static_cast< double >(p[0]);
				const auto y = static_cast< double >(p[1]);
				const auto z = static_cast< double >(p[2]);

				return std::array< double, 3 >{
					x * worldMatrix.m[0][0] + y * worldMatrix.m[1][0] + z * worldMatrix.m[2][0] + worldMatrix.m[3][0],
					x * worldMatrix.m[0][1] + y * worldMatrix.m[1][1] + z * worldMatrix.m[2][1] + worldMatrix.m[3][1],
					x * worldMatrix.m[0][2] + y * worldMatrix.m[1][2] + z * worldMatrix.m[2][2] + worldMatrix.m[3][2]
				};
			};

			/* A direction ignores the translation. The 3x3 block is used as-is rather than its
			 * inverse transpose: correct for rotation and uniform scale, which is what this
			 * asset uses. A non-uniform scale would need the inverse transpose — the day one
			 * shows up, the lighting will look wrong on stretched meshes. */
			const auto toWorldDirection = [&worldMatrix] (const auto & d) {
				const auto x = static_cast< double >(d[0]);
				const auto y = static_cast< double >(d[1]);
				const auto z = static_cast< double >(d[2]);

				return std::array< double, 3 >{
					x * worldMatrix.m[0][0] + y * worldMatrix.m[1][0] + z * worldMatrix.m[2][0],
					x * worldMatrix.m[0][1] + y * worldMatrix.m[1][1] + z * worldMatrix.m[2][1],
					x * worldMatrix.m[0][2] + y * worldMatrix.m[1][2] + z * worldMatrix.m[2][2]
				};
			};

			const auto & indices = renderMesh.faceVertexIndices();
			const auto vertexCount = renderMesh.points.size();

			if ( vertexCount == 0 || indices.size() < 3 )
			{
				meshIndex++;

				continue;
			}

			/* Tydra hands back normals, texture coordinates and tangents as raw byte buffers.
			 * They are only usable when their vertex count matches the point count — anything
			 * else means a per-face or per-face-vertex layout this pass does not unpack. */
			const auto * normals = renderMesh.normals.vertex_count() == vertexCount
				? reinterpret_cast< const float * >(renderMesh.normals.data.data())
				: nullptr;

			const float * texCoords = nullptr;

			if ( const auto it = renderMesh.texcoords.find(0); it != renderMesh.texcoords.end() && it->second.vertex_count() == vertexCount )
			{
				texCoords = reinterpret_cast< const float * >(it->second.data.data());
			}

			auto shape = std::make_shared< Shape< float > >();

			const auto triangleCount = indices.size() / 3;

			const bool buildSuccess = shape->build([&] (auto & groups, auto & vertices, auto & triangles) {
				vertices.resize(vertexCount);

				for ( size_t index = 0; index < vertexCount; ++index )
				{
					vertices[index].setPosition(bakePosition(toWorldPosition(renderMesh.points[index])));

					if ( normals != nullptr )
					{
						vertices[index].setNormal(bakeDirection(toWorldDirection(&normals[index * 3])));
					}

					if ( texCoords != nullptr )
					{
						vertices[index].setTextureCoordinates(Vector< 2, float >{texCoords[index * 2], texCoords[index * 2 + 1]});
					}
				}

				triangles.reserve(triangleCount);
				groups.emplace_back(0U, static_cast< uint32_t >(triangleCount));

				for ( size_t triangle = 0; triangle < triangleCount; ++triangle )
				{
					const auto a = indices[triangle * 3];
					const auto b = indices[triangle * 3 + 1];
					const auto c = indices[triangle * 3 + 2];

					/* The winding is kept as authored: the (a, c, b) swap that used to sit here
					 * compensated the engine's ORIENTATION-REVERSING projection — NOT the axis
					 * bake, whose determinant is deliberately kept at +1 a few lines above — and
					 * that mirror went away with the Y-up convention flip. */
					triangles.emplace_back(a, b, c);
				}

				return true;
			}, texCoords != nullptr);

			if ( !buildSuccess )
			{
				TraceWarning{ClassId} << "Unable to build the shape of mesh '" << renderMesh.prim_name << "'.";

				meshIndex++;

				continue;
			}

			if ( normals != nullptr )
			{
				shape->declareNormalsAvailable();
			}
			else
			{
				/* No inversion: nothing reflects the vertex data any more (the winding swap that
				 * used to force it is gone with the orientation-reversing projection). */
				shape->computeTriangleNormal(false);
				shape->computeVertexNormal();
			}

			/* Where a mesh actually landed after the bake. Without it, "I cannot see anything"
			 * is indistinguishable from "it is behind me", and the camera gets aimed by guesswork. */
			const auto & bounds = shape->boundingBox();

			TraceInfo{ClassId} <<
				"Mesh '" << renderMesh.prim_name << "' baked into [" <<
				bounds.minimum()[0] << ", " << bounds.minimum()[1] << ", " << bounds.minimum()[2] << "] .. [" <<
				bounds.maximum()[0] << ", " << bounds.maximum()[1] << ", " << bounds.maximum()[2] << "].";

			const auto resourceName = m_resourcePrefix + "/mesh/" + ( renderMesh.prim_name.empty() ? std::to_string(meshIndex) : renderMesh.prim_name ) + "-" + std::to_string(meshIndex);

			auto geometry = m_resources.container< Geometry::IndexedVertexResource >()
				->getOrCreateResource(resourceName, [shape] (auto & geometryResource) {
					shape->computeTriangleTangent();
					shape->computeVertexTangent();

					return geometryResource.load(*shape);
				}, Geometry::EnableTangentSpace | Geometry::EnablePrimaryTextureCoordinates);

			if ( geometry == nullptr )
			{
				TraceWarning{ClassId} << "Unable to create the geometry of mesh '" << renderMesh.prim_name << "'.";

				meshIndex++;

				continue;
			}

			/* `material_id` is the whole-mesh binding. GeomSubsets carry per-face bindings and are
			 * not honoured yet — a mesh with several material subsets takes the first one. */
			auto material = ( renderMesh.material_id >= 0 && static_cast< size_t >(renderMesh.material_id) < materials.size() && materials[static_cast< size_t >(renderMesh.material_id)] != nullptr )
				? materials[static_cast< size_t >(renderMesh.material_id)]
				: defaultMaterial();

			/* Two-sidedness is carried by the mesh in USD, not by the material. */
			RasterizationOptions rasterization{};

			if ( renderMesh.doubleSided )
			{
				rasterization.setCullingMode(CullingMode::None);
			}

			auto mesh = m_resources.container< Renderable::MeshResource >()
				->getOrCreateResource(resourceName, [geometry, material, rasterization] (auto & meshResource) {
					return meshResource.load(geometry, material, rasterization);
				});

			if ( mesh == nullptr )
			{
				meshIndex++;

				continue;
			}

			MeshDescriptor descriptor;
			descriptor.renderable = mesh;
			descriptor.geometry = geometry;
			descriptor.materials = {material};

			output.meshes.emplace_back(std::move(descriptor));

			builtMeshesByPath.emplace(primPath, output.meshes.size() - 1);

			/* A prototype stops HERE: it is a resource its instances will draw, never a node.
			 * Giving it one is the classic instancing bug — the whole forest is correct, plus one
			 * stray copy of every species sitting at the prototype's own place. */
			if ( !instancedOnly )
			{
				/* One node per mesh for this pass: the render scene's own hierarchy is not walked
				 * yet, so every mesh sits at the stage origin with its baked coordinates. */
				NodeDescriptor node;
				node.name = resourceName;
				node.meshIndex = output.meshes.size() - 1;

				output.nodes.emplace_back(std::move(node));
				output.rootNodeIndices.push_back(output.nodes.size() - 1);
			}

			builtCount++;
			meshIndex++;
		}

		return builtCount;
	}

	bool
	USDLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		(void)output;
		(void)m_resources;

		if ( !std::filesystem::exists(filepath) )
		{
			TraceError{ClassId} << "File '" << filepath.string() << "' does not exist !";

			return false;
		}

		/* ⚠️ Composition MUST run on an absolute path. tinyusdz stores each layer's working
		 * directory as given, then resolves nested references against it: fed a relative path,
		 * a reference inside `elements/Anthurium/PI_Anthurium.usd` is looked up under
		 * `elements/Anthurium/…` relative to the PROCESS working directory, which is the
		 * executable's, not the asset's. The sublayers still compose — so the failure is
		 * partial and quiet: the hard geometry arrives and every referenced prototype silently
		 * does not. Measured on Jungle Ruins: 84 meshes in, 0 PointInstancer prototypes. */
		std::error_code pathError;
		const auto absoluteFilepath = std::filesystem::weakly_canonical(std::filesystem::absolute(filepath, pathError), pathError);

		if ( pathError )
		{
			TraceError{ClassId} << "Unable to resolve an absolute path for '" << filepath.string() << "' : " << pathError.message();

			return false;
		}

		std::string warning;
		std::string error;

		/* A USDZ is a stage whose layers and images live INSIDE the file. Everything below —
		 * composition, then texture reading — must therefore go through the archive rather than the
		 * filesystem, so the archive is opened first and equips the resolver. */
		const auto extension = [] (std::string text) {
			std::ranges::transform(text, text.begin(), [] (unsigned char character) {
				return static_cast< char >(std::tolower(character));
			});

			return text;
		}(absoluteFilepath.extension().string());

		if ( extension == ".usdz" )
		{
			m_archive = std::make_shared< USDZArchive >();

			if ( !m_archive->open(absoluteFilepath) )
			{
				m_archive.reset();

				return false;
			}
		}

		/* ⚠️ `LoadUSDFromFile()` reads the ROOT LAYER ONLY. It parses the subLayers metadata but
		 * composes nothing, so a 19-sublayer stage comes back holding 2 prims and reports
		 * success. Composition in tinyusdz is an EXPLICIT, separate pipeline, and it is the one
		 * used below:
		 *
		 *   LoadLayerFromFile → CompositeSublayers → CompositeAllArcs → LayerToStage
		 *
		 * ⚠️ `CompositeSublayersInPlace()` is DECLARED in v0.9.4's composition.hh but never
		 * implemented — linking against it fails with an undefined reference. Only the plain
		 * `CompositeSublayers()` exists. `LayerToStageInPlace()` IS implemented and is used
		 * below, because it frees each PrimSpec as it converts instead of holding the layer and
		 * the stage at once — which matters when the composed layer is measured in gigabytes. */

		/* Sublayers, references and payloads are all relative to the root layer's directory —
		 * except that tinyusdz stores each sublayer's working directory as the RAW relative path
		 * it was written with ("elements/Anthurium"), never joined with the root. A reference
		 * made from inside such a sublayer is then looked up relative to the PROCESS working
		 * directory and silently fails, taking its whole prim with it.
		 *
		 * Seeding the resolver with every directory of the stage tree side-steps this without a
		 * second patch to the library: whatever relative name a nested layer asks for, the
		 * directory holding it is already declared. Measured on Jungle Ruins, this is what turns
		 * eleven silently missing `*_classes.usda` prototype layers into resolved ones. */
		tinyusdz::AssetResolutionResolver resolver;

		std::vector< std::string > searchPaths;

		if ( m_archive != nullptr )
		{
			/* Inside an archive there are no directories to walk: the table IS the namespace. */
			m_archive->equip(resolver);

			searchPaths.emplace_back(m_archive->baseDirectory());
		}
		else
		{
			searchPaths.emplace_back(absoluteFilepath.parent_path().string());

			std::error_code walkError;
			const std::filesystem::recursive_directory_iterator end;

			for ( auto it = std::filesystem::recursive_directory_iterator{absoluteFilepath.parent_path(), std::filesystem::directory_options::skip_permission_denied, walkError}; !walkError && it != end; it.increment(walkError) )
			{
				if ( it->is_directory(walkError) )
				{
					searchPaths.emplace_back(it->path().string());
				}
			}

			if ( walkError )
			{
				TraceWarning{ClassId} << "Partial stage directory scan for '" << filepath.filename().string() << "' : " << walkError.message();
			}

			resolver.set_search_paths(searchPaths);
		}

		tinyusdz::Layer layer;

		if ( m_archive != nullptr )
		{
			/* ⚠️ Through the RESOLVER, not the filesystem. `LoadLayerFromFile()` would read the
			 * archive's leading bytes and hand back the first USD it finds with an empty working
			 * directory, so every nested layer would then be looked up against the process working
			 * directory instead of the archive. */
			if ( !tinyusdz::LoadLayerFromAsset(resolver, m_archive->rootAssetName(), &layer, &warning, &error) )
			{
				TraceError{ClassId} << "Unable to read the root layer of '" << filepath.filename().string() << "' : " << error;

				return false;
			}
		}
		else if ( !tinyusdz::LoadLayerFromFile(absoluteFilepath.string(), &layer, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to read layer '" << filepath.string() << "' : " << error;

			return false;
		}

		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While reading '" << filepath.filename().string() << "' : " << warning;
			warning.clear();
		}

		tinyusdz::Layer sublayered;

		if ( !tinyusdz::CompositeSublayers(resolver, layer, &sublayered, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to composite sublayers of '" << filepath.filename().string() << "' : " << error;

			return false;
		}

		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While compositing the sublayers of '" << filepath.filename().string() << "' : " << warning;
			warning.clear();
		}

		/* ⚠️ `CompositeAllArcs()` is DELIBERATELY not called here (owner decision, 2026-08-08).
		 * It resolves references, payloads, inherits and variants EAGERLY, in one pass. On Jungle
		 * Ruins that means ingesting 450 MB of ASCII prototype layers before anything can be
		 * drawn: measured at 24 minutes and 15 GB of resident memory, growing linearly with no
		 * convergence in sight — a cost that never amortises.
		 *
		 * Sublayer composition alone completes in seconds and already yields the whole
		 * non-instanced scene (84 meshes, 87 materials on that asset). Prototype references are
		 * therefore resolved by US, per element, when the element is actually needed — which is
		 * the deferred-loading strategy this project chose from the start.
		 *
		 * Consequence to keep in mind: `inherits` and `variants` are NOT applied on this path.
		 * Variant selection through LoaderOptions lands with the on-demand resolver, not before. */
		auto composited = std::make_unique< tinyusdz::Layer >();

		if ( m_options.resolveReferences )
		{
			if ( !tinyusdz::CompositeAllArcs(resolver, sublayered, composited.get(), &warning, &error) )
			{
				TraceError{ClassId} << "Unable to composite the arcs of '" << filepath.filename().string() << "' : " << error;

				return false;
			}

			if ( !warning.empty() )
			{
				TraceWarning{ClassId} << "While compositing the arcs of '" << filepath.filename().string() << "' : " << warning;
				warning.clear();
			}
		}
		else
		{
			*composited = std::move(sublayered);
		}

		/* The stage metrics must be read BEFORE the layer is consumed by the converter. */
		const auto & metas = composited->metas();
		const auto upAxis = metas.upAxis.get_value();
		const auto metersPerUnit = static_cast< float >(metas.metersPerUnit.get_value());

		if ( upAxis != tinyusdz::Axis::Z )
		{
			TraceWarning{ClassId} << "'" << filepath.filename().string() << "' declares a non-Z up axis; the bake below assumes Z-up and will be wrong.";
		}

		/* ⚠️ Two Tydra front-ends exist and they are NOT equivalent:
		 *   - `LayerToRenderSceneConverter` works from a Layer. Its in-place entry point is
		 *     refused at runtime ("destructive source transfer is not implemented safely yet"),
		 *     and its plain entry point returns the node hierarchy with ZERO meshes. Experimental.
		 *   - `RenderSceneConverter` works from a Stage and is the mature path. It is the one used.
		 *
		 * So the layer is turned into a Stage first, with the in-place variant that frees each
		 * PrimSpec as it converts. */
		tinyusdz::Stage stage;

		if ( !tinyusdz::LayerToStageInPlace(std::move(composited), &stage, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to build a stage from '" << filepath.filename().string() << "' : " << error;

			return false;
		}

		/* ⚠️ THIS IS NOT COSMETIC LOGGING. tinyusdz reports through `warn` what it silently gave
		 * up on, and dropping that string on the floor is what cost a whole session: every
		 * PointInstancer of this asset was being discarded by `LayerToStage` — the prim type was
		 * missing from its reconstruction table — and it said so, here, every single time.
		 * A prim that cannot be reconstructed takes its ENTIRE SUBTREE with it, so a swallowed
		 * warning is not a missing detail, it is a missing half of the scene. */
		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While building the stage of '" << filepath.filename().string() << "' : " << warning;
			warning.clear();
		}

		USDLoader::reportInventory(filepath, stage);

		for ( const auto & prim : stage.root_prims() )
		{
			USDLoader::collectEnvironmentLights(prim, absoluteFilepath.parent_path(), output);
		}

		/* PointInstancers are read from the PRIMS: Tydra has no notion of them whatsoever, so
		 * nothing downstream would ever mention the vegetation. */
		std::vector< Instancer > instancers;

		for ( const auto & prim : stage.root_prims() )
		{
			USDLoader::collectInstancers(prim, "/" + prim.element_name(), metersPerUnit, instancers);
		}

		std::vector< std::string > prototypePaths;

		for ( const auto & instancer : instancers )
		{
			prototypePaths.insert(prototypePaths.end(), instancer.prototypePaths.begin(), instancer.prototypePaths.end());
		}

		std::ranges::sort(prototypePaths);
		prototypePaths.erase(std::ranges::unique(prototypePaths).begin(), prototypePaths.end());

		/* Tydra hands back renderer-ready data: triangulated faces, indexed vertices, resolved
		 * material bindings. Re-deriving that from raw prims would be duplicated work — the
		 * engine's own job starts at the translation into native scene logic. */
		tinyusdz::tydra::RenderSceneConverterEnv env{stage};
		env.usd_filename = m_archive != nullptr ? m_archive->rootAssetName() : absoluteFilepath.string();

		/* ⚠️ Tydra carries its OWN resolver, and it is NOT the one composition used. Left as a
		 * filesystem resolver on an archive, a texture is not merely "unresolved": Tydra drops the
		 * whole UsdUVTexture, so `renderScene.textures` comes back EMPTY and the materials look
		 * like the asset never had any. */
		env.asset_resolver = resolver;
		env.set_search_paths(searchPaths);

		tinyusdz::tydra::RenderSceneConverter converter;
		tinyusdz::tydra::RenderScene renderScene;

		if ( !converter.ConvertToRenderScene(env, &renderScene) )
		{
			TraceError{ClassId} << "Unable to convert '" << filepath.filename().string() << "' to a render scene : " << converter.GetError();

			return false;
		}

		if ( !converter.GetWarning().empty() )
		{
			TraceWarning{ClassId} << "While converting '" << filepath.filename().string() << "' : " << converter.GetWarning();
		}

		m_resourcePrefix = "usd:" + filepath.stem().string();

		std::map< std::string, size_t > builtMeshesByPath;

		const auto materials = this->buildMaterials(renderScene, absoluteFilepath.parent_path());
		const auto builtCount = this->buildMeshes(renderScene, metersPerUnit, prototypePaths, materials, output, builtMeshesByPath);
		const auto instanceCount = USDLoader::buildInstanceSets(instancers, builtMeshesByPath, output);

		/* The asset's own interior lighting. Skipping it leaves a scene that can only be lit from
		 * outside, which on an interior means "lit by nothing plausible".
		 *
		 * ⚠️ The PLACEMENT has to be collected separately, from the stage's xform hierarchy, because
		 * Tydra's light converter never writes a light's transform (see buildLights() for what that
		 * cost). `BuildXformNodeFromStage()` is tinyusdz's own equivalent of pxrUSD's
		 * GetLocalToWorldMatrix, so the parent chain is composed by the library rather than by hand. */
		std::map< std::string, LightPlacement > lightPlacements;

		{
			tinyusdz::tydra::XformNode xformRoot;

			if ( tinyusdz::tydra::BuildXformNodeFromStage(stage, &xformRoot) )
			{
				USDLoader::collectLightPlacements(xformRoot, lightPlacements);
			}
			else
			{
				/* Not fatal, and deliberately loud: the geometry carries its own transforms from
				 * Tydra, so the stage still renders — lit by nothing. Silence here would reproduce
				 * exactly the black-frame-without-an-error the placement pass was written to end. */
				TraceError{ClassId} << "Unable to build the xform hierarchy of '" << filepath.filename().string() << "'; every punctual light will be dropped.";
			}
		}

		const auto lightCount = USDLoader::buildLights(renderScene, metersPerUnit, lightPlacements, output);

		TraceInfo{ClassId} <<
			instancers.size() << " point instancers read: " << instanceCount << " instances over " <<
			output.instanceSets.size() << " sets, from " << prototypePaths.size() << " prototypes, " <<
			lightCount << " punctual lights.";

		TraceInfo{ClassId} <<
			filepath.filename().string() << " converted: " <<
			renderScene.meshes.size() << " source meshes (" << builtCount << " built), " <<
			renderScene.materials.size() << " materials, " <<
			renderScene.textures.size() << " textures, " <<
			renderScene.images.size() << " images, " <<
			renderScene.nodes.size() << " root nodes, " <<
			renderScene.cameras.size() << " cameras, " <<
			renderScene.lights.size() << " lights, " <<
			renderScene.instances.size() << " instances.";

		TraceInfo{ClassId} <<
			"Stage metrics: upAxis " << ( upAxis == tinyusdz::Axis::Z ? "Z" : "non-Z" ) <<
			", metersPerUnit " << metersPerUnit <<
			", timeCodes " << metas.startTimeCode.get_value() << " to " << metas.endTimeCode.get_value() << ".";

		/* An element made entirely of instances builds no drawable node at all, and is still a
		 * complete success. Judging on meshes alone would call the vegetation a failure. */
		return builtCount > 0 || !output.instanceSets.empty();
	}
}
