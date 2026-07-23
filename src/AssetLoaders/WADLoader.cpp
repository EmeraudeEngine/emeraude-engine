/*
 * src/AssetLoaders/WADLoader.cpp
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

#include "WADLoader.hpp"

/* STL inclusions. */
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <numbers>
#include <unordered_map>

/* Local inclusions. */
#include "VertexFactory/Shape.hpp"
#include "AssetData.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace
{
	using namespace EmEn;

	constexpr auto TracerTag{"WADLoader"};

	/* ---- Raw WAD structures (little-endian, byte-packed in the file). ---- */

	int16_t
	readInt16 (const uint8_t * data) noexcept
	{
		int16_t value = 0;
		std::memcpy(&value, data, sizeof(value));

		return value;
	}

	uint16_t
	readUInt16 (const uint8_t * data) noexcept
	{
		uint16_t value = 0;
		std::memcpy(&value, data, sizeof(value));

		return value;
	}

	uint32_t
	readUInt32 (const uint8_t * data) noexcept
	{
		uint32_t value = 0;
		std::memcpy(&value, data, sizeof(value));

		return value;
	}

	int32_t
	readInt32 (const uint8_t * data) noexcept
	{
		int32_t value = 0;
		std::memcpy(&value, data, sizeof(value));

		return value;
	}

	/* Decodes a fixed 8-byte, zero-padded, case-sensitive lump/texture name. */
	std::string
	readName8 (const uint8_t * data) noexcept
	{
		size_t length = 0;

		while ( length < 8 && data[length] != 0 )
		{
			++length;
		}

		return {reinterpret_cast< const char * >(data), length};
	}

	struct Lump
	{
		std::string name;
		uint32_t offset{0};
		uint32_t size{0};
	};

	struct MapVertex
	{
		float x{0.0F};
		float y{0.0F};
	};

	struct Sidedef
	{
		float xOffset{0.0F};
		float yOffset{0.0F};
		std::string upperTexture;
		std::string lowerTexture;
		std::string middleTexture;
		int16_t sector{-1};
	};

	struct Linedef
	{
		uint16_t v1{0};
		uint16_t v2{0};
		int16_t rightSide{-1};
		int16_t leftSide{-1};
	};

	struct Sector
	{
		float floorHeight{0.0F};
		float ceilingHeight{0.0F};
		std::string floorFlat;
		std::string ceilingFlat;
		float lightLevel{1.0F};
	};

	struct Seg
	{
		uint16_t v1{0};
		uint16_t v2{0};
		uint16_t linedef{0};
		uint16_t direction{0};
	};

	struct SubSector
	{
		uint16_t segCount{0};
		uint16_t firstSeg{0};
	};

	struct BSPNode
	{
		float x{0.0F};
		float y{0.0F};
		float dx{0.0F};
		float dy{0.0F};
		uint16_t rightChild{0};
		uint16_t leftChild{0};
	};

	/* 2D polygon in Doom map coordinates. */
	using Polygon2D = std::vector< std::pair< float, float > >;

	/* Sutherland-Hodgman: clips the polygon by the line (ax,ay)+(dx,dy), keeping one side.
	 * Doom's side convention (R_PointOnSide): cross = dx*(py-ay) - dy*(px-ax);
	 * cross <= 0 → RIGHT (front, child 0), cross >= 0 → LEFT (back, child 1). */
	Polygon2D
	clipPolygon (const Polygon2D & polygon, float ax, float ay, float dx, float dy, bool keepRight) noexcept
	{
		Polygon2D result;
		result.reserve(polygon.size() + 2);

		const auto sideOf = [&] (const std::pair< float, float > & point) {
			const auto cross = (dx * (point.second - ay)) - (dy * (point.first - ax));

			return keepRight ? -cross : cross;
		};

		for ( size_t index = 0; index < polygon.size(); ++index )
		{
			constexpr auto Epsilon{0.01F};

			const auto & current = polygon[index];
			const auto & next = polygon[(index + 1) % polygon.size()];

			const auto currentSide = sideOf(current);
			const auto nextSide = sideOf(next);

			if ( currentSide >= -Epsilon )
			{
				result.push_back(current);
			}

			if ( (currentSide > Epsilon && nextSide < -Epsilon) || (currentSide < -Epsilon && nextSide > Epsilon) )
			{
				const auto t = currentSide / (currentSide - nextSide);

				result.emplace_back(current.first + (t * (next.first - current.first)), current.second + (t * (next.second - current.second)));
			}
		}

		return result;
	}

	/* One corner of a triangle, in Doom map space (x, y horizontal / z = height, map units). */
	struct Corner
	{
		float x{0.0F};
		float y{0.0F};
		float z{0.0F};
		float u{0.0F};
		float v{0.0F};
		float nx{0.0F};
		float ny{0.0F};
		float nz{0.0F};
		float light{1.0F};
	};

	/* Per-texture triangle bucket: 3 corners per triangle, appended flat. */
	using Buckets = std::map< std::string, std::vector< Corner > >;

	constexpr auto SkyFlatName{"F_SKY1"};

	/* ---- Composite wall texture assembly (TEXTURE1/TEXTURE2 + PNAMES + patches). ---- */

	struct TexturePatch
	{
		int16_t originX{0};
		int16_t originY{0};
		uint16_t patchIndex{0};
	};

	struct TextureDefinition
	{
		uint16_t width{0};
		uint16_t height{0};
		std::vector< TexturePatch > patches;
	};

	/* Draws one Doom picture-format patch into an indexed+coverage canvas. */
	void
	blitPatch (const uint8_t * patch, size_t patchSize, int originX, int originY, uint16_t canvasWidth, uint16_t canvasHeight, std::vector< uint8_t > & indexes, std::vector< uint8_t > & coverage) noexcept
	{
		if ( patchSize < 8 )
		{
			return;
		}

		const auto patchWidth = readUInt16(patch);
		//const auto patchHeight = readUInt16(patch + 2);

		for ( uint16_t column = 0; column < patchWidth; ++column )
		{
			const int canvasX = originX + column;

			if ( canvasX < 0 || canvasX >= canvasWidth )
			{
				continue;
			}

			const auto columnHeaderOffset = 8 + (4 * static_cast< size_t >(column));

			if ( columnHeaderOffset + 4 > patchSize )
			{
				break;
			}

			auto postOffset = static_cast< size_t >(readUInt32(patch + columnHeaderOffset));

			/* Column = a list of posts, terminated by topdelta 0xFF. */
			while ( postOffset + 2 <= patchSize )
			{
				const auto topDelta = patch[postOffset];

				if ( topDelta == 0xFF )
				{
					break;
				}

				const auto length = patch[postOffset + 1];

				/* +3: topdelta, length, unused padding byte before the pixels. */
				const auto pixelsOffset = postOffset + 3;

				if ( pixelsOffset + length > patchSize )
				{
					break;
				}

				for ( uint8_t row = 0; row < length; ++row )
				{
					const int canvasY = originY + topDelta + row;

					if ( canvasY >= 0 && canvasY < canvasHeight )
					{
						const auto canvasIndex = (static_cast< size_t >(canvasY) * canvasWidth) + static_cast< size_t >(canvasX);
						indexes[canvasIndex] = patch[pixelsOffset + row];
						coverage[canvasIndex] = 1;
					}
				}

				/* +4: topdelta, length and the two padding bytes around the pixels. */
				postOffset += static_cast< size_t >(length) + 4;
			}
		}
	}
}

namespace EmEn::AssetLoaders
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;
	using namespace Graphics;

	WADLoader::WADLoader (Resources::Manager & resources) noexcept
		: m_resources{resources}
	{

	}

	bool
	WADLoader::load (const std::filesystem::path & filepath, AssetData & output) noexcept
	{
		/* ---- Read the whole WAD in memory (a full IWAD is ~15 MB). ---- */
		std::vector< uint8_t > wad;

		{
			std::ifstream file{filepath, std::ios::binary | std::ios::ate};

			if ( !file.is_open() )
			{
				TraceError{TracerTag} << "Unable to open the WAD file " << filepath << " !";

				return false;
			}

			wad.resize(static_cast< size_t >(file.tellg()));
			file.seekg(0);
			file.read(reinterpret_cast< char * >(wad.data()), static_cast< std::streamsize >(wad.size()));
		}

		if ( wad.size() < 12 || (std::memcmp(wad.data(), "IWAD", 4) != 0 && std::memcmp(wad.data(), "PWAD", 4) != 0) )
		{
			TraceError{TracerTag} << "The file " << filepath << " is not a WAD !";

			return false;
		}

		/* ---- Directory. ---- */
		const auto lumpCount = readUInt32(wad.data() + 4);
		const auto directoryOffset = readUInt32(wad.data() + 8);

		if ( directoryOffset + (static_cast< size_t >(lumpCount) * 16) > wad.size() )
		{
			TraceError{TracerTag} << "Corrupted WAD directory in " << filepath << " !";

			return false;
		}

		std::vector< Lump > lumps;
		lumps.reserve(lumpCount);

		for ( uint32_t index = 0; index < lumpCount; ++index )
		{
			const auto * entry = wad.data() + directoryOffset + (static_cast< size_t >(index) * 16);

			lumps.emplace_back(Lump{
				.name = readName8(entry + 8),
				.offset = readUInt32(entry),
				.size = readUInt32(entry + 4)
			});
		}

		const auto findLump = [&lumps] (const std::string & name, size_t from = 0, size_t upTo = 0) -> const Lump * {
			const auto end = upTo > 0 ? std::min(upTo, lumps.size()) : lumps.size();

			for ( size_t index = from; index < end; ++index )
			{
				if ( lumps[index].name == name )
				{
					return &lumps[index];
				}
			}

			return nullptr;
		};

		const auto lumpData = [&wad] (const Lump & lump) -> const uint8_t * {
			return wad.data() + lump.offset;
		};

		/* ---- Locate the requested map marker (ExMy or MAPxx, followed by THINGS). ---- */
		const auto isMapMarker = [] (const std::string & name) {
			if ( name.size() == 4 && name[0] == 'E' && name[2] == 'M' && (std::isdigit(name[1]) != 0) && (std::isdigit(name[3]) != 0) )
			{
				return true;
			}

			return name.size() == 5 && name.starts_with("MAP") && (std::isdigit(name[3]) != 0) && (std::isdigit(name[4]) != 0);
		};

		size_t mapMarkerIndex = lumps.size();

		{
			uint32_t mapNumber = 0;

			for ( size_t index = 0; index + 1 < lumps.size(); ++index )
			{
				if ( !isMapMarker(lumps[index].name) || lumps[index + 1].name != "THINGS" )
				{
					continue;
				}

				++mapNumber;

				if ( (!m_mapName.empty() && lumps[index].name == m_mapName) || (m_mapName.empty() && mapNumber == m_mapIndex) )
				{
					mapMarkerIndex = index;

					break;
				}
			}
		}

		if ( mapMarkerIndex >= lumps.size() )
		{
			TraceError{TracerTag} << "Map " << (m_mapName.empty() ? std::to_string(m_mapIndex) : m_mapName) << " not found in " << filepath << " !";

			return false;
		}

		m_loadedMapName = lumps[mapMarkerIndex].name;

		const auto mapLump = [&] (const std::string & name) -> const Lump * {
			return findLump(name, mapMarkerIndex + 1, mapMarkerIndex + 11);
		};

		const auto * verticesLump = mapLump("VERTEXES");
		const auto * linedefsLump = mapLump("LINEDEFS");
		const auto * sidedefsLump = mapLump("SIDEDEFS");
		const auto * sectorsLump = mapLump("SECTORS");
		const auto * segsLump = mapLump("SEGS");
		const auto * subSectorsLump = mapLump("SSECTORS");
		const auto * thingsLump = mapLump("THINGS");

		if ( verticesLump == nullptr || linedefsLump == nullptr || sidedefsLump == nullptr || sectorsLump == nullptr || segsLump == nullptr || subSectorsLump == nullptr )
		{
			TraceError{TracerTag} << "Map " << m_loadedMapName << " is missing geometry lumps !";

			return false;
		}

		/* ---- Decode the map lumps. ---- */
		std::vector< MapVertex > vertices(verticesLump->size / 4);

		for ( size_t index = 0; index < vertices.size(); ++index )
		{
			const auto * entry = lumpData(*verticesLump) + (index * 4);
			vertices[index].x = static_cast< float >(readInt16(entry));
			vertices[index].y = static_cast< float >(readInt16(entry + 2));
		}

		std::vector< Linedef > linedefs(linedefsLump->size / 14);

		for ( size_t index = 0; index < linedefs.size(); ++index )
		{
			const auto * entry = lumpData(*linedefsLump) + (index * 14);
			linedefs[index].v1 = readUInt16(entry);
			linedefs[index].v2 = readUInt16(entry + 2);
			linedefs[index].rightSide = readInt16(entry + 10);
			linedefs[index].leftSide = readInt16(entry + 12);
		}

		std::vector< Sidedef > sidedefs(sidedefsLump->size / 30);

		for ( size_t index = 0; index < sidedefs.size(); ++index )
		{
			const auto * entry = lumpData(*sidedefsLump) + (index * 30);
			sidedefs[index].xOffset = static_cast< float >(readInt16(entry));
			sidedefs[index].yOffset = static_cast< float >(readInt16(entry + 2));
			sidedefs[index].upperTexture = readName8(entry + 4);
			sidedefs[index].lowerTexture = readName8(entry + 12);
			sidedefs[index].middleTexture = readName8(entry + 20);
			sidedefs[index].sector = readInt16(entry + 28);
		}

		std::vector< Sector > sectors(sectorsLump->size / 26);

		for ( size_t index = 0; index < sectors.size(); ++index )
		{
			const auto * entry = lumpData(*sectorsLump) + (index * 26);
			sectors[index].floorHeight = static_cast< float >(readInt16(entry));
			sectors[index].ceilingHeight = static_cast< float >(readInt16(entry + 2));
			sectors[index].floorFlat = readName8(entry + 4);
			sectors[index].ceilingFlat = readName8(entry + 12);
			sectors[index].lightLevel = static_cast< float >(std::clamp< int16_t >(readInt16(entry + 20), 0, 255)) / 255.0F;
		}

		std::vector< Seg > segs(segsLump->size / 12);

		for ( size_t index = 0; index < segs.size(); ++index )
		{
			const auto * entry = lumpData(*segsLump) + (index * 12);
			segs[index].v1 = readUInt16(entry);
			segs[index].v2 = readUInt16(entry + 2);
			segs[index].linedef = readUInt16(entry + 6);
			segs[index].direction = readUInt16(entry + 8);
		}

		std::vector< SubSector > subSectors(subSectorsLump->size / 4);

		for ( size_t index = 0; index < subSectors.size(); ++index )
		{
			const auto * entry = lumpData(*subSectorsLump) + (index * 4);
			subSectors[index].segCount = readUInt16(entry);
			subSectors[index].firstSeg = readUInt16(entry + 2);
		}

		/* ---- Palette (PLAYPAL, first 768 bytes = palette 0). ---- */
		std::array< uint8_t, 768 > palette{};

		if ( const auto * playpal = findLump("PLAYPAL"); playpal != nullptr && playpal->size >= 768 )
		{
			std::memcpy(palette.data(), lumpData(*playpal), 768);
		}
		else
		{
			TraceWarning{TracerTag} << "No PLAYPAL lump: textures will be greyscale.";

			for ( size_t index = 0; index < 256; ++index )
			{
				palette[index * 3] = palette[(index * 3) + 1] = palette[(index * 3) + 2] = static_cast< uint8_t >(index);
			}
		}

		/* ---- Wall texture definitions (PNAMES + TEXTURE1/TEXTURE2). ---- */
		std::vector< std::string > patchNames;

		if ( const auto * pnames = findLump("PNAMES"); pnames != nullptr && pnames->size >= 4 )
		{
			const auto count = readUInt32(lumpData(*pnames));
			patchNames.reserve(count);

			for ( uint32_t index = 0; index < count && 4 + ((index + 1) * 8) <= pnames->size; ++index )
			{
				patchNames.emplace_back(readName8(lumpData(*pnames) + 4 + (static_cast< size_t >(index) * 8)));
			}
		}

		std::unordered_map< std::string, TextureDefinition > textureDefinitions;

		for ( const auto * textureLumpName : {"TEXTURE1", "TEXTURE2"} )
		{
			const auto * textureLump = findLump(textureLumpName);

			if ( textureLump == nullptr || textureLump->size < 4 )
			{
				continue;
			}

			const auto * base = lumpData(*textureLump);
			const auto textureCount = readInt32(base);

			for ( int32_t index = 0; index < textureCount; ++index )
			{
				const auto entryOffset = static_cast< size_t >(readInt32(base + 4 + (static_cast< size_t >(index) * 4)));

				if ( entryOffset + 22 > textureLump->size )
				{
					continue;
				}

				const auto * entry = base + entryOffset;

				TextureDefinition definition;
				definition.width = readUInt16(entry + 12);
				definition.height = readUInt16(entry + 14);

				const auto patchCount = readUInt16(entry + 20);
				definition.patches.reserve(patchCount);

				for ( uint16_t patchIdx = 0; patchIdx < patchCount; ++patchIdx )
				{
					const auto * patchEntry = entry + 22 + (static_cast< size_t >(patchIdx) * 10);

					definition.patches.emplace_back(TexturePatch{
						.originX = readInt16(patchEntry),
						.originY = readInt16(patchEntry + 2),
						.patchIndex = readUInt16(patchEntry + 4)
					});
				}

				textureDefinitions[readName8(entry)] = std::move(definition);
			}
		}

		/* ---- Texture creation helpers (indexed pixels → RGBA → engine resources). ---- */
		const auto wadStem = filepath.stem().string();

		std::unordered_map< std::string, std::shared_ptr< TextureResource::Texture2D > > textureCache;
		std::unordered_map< std::string, std::pair< float, float > > textureSizes;

		const auto createTexture = [&] (const std::string & name, uint16_t width, uint16_t height, const std::vector< uint8_t > & indexes, const std::vector< uint8_t > & coverage) -> std::shared_ptr< TextureResource::Texture2D > {
			std::vector< uint8_t > rgba(static_cast< size_t >(width) * height * 4);

			for ( size_t pixel = 0; pixel < indexes.size(); ++pixel )
			{
				const auto colorIndex = static_cast< size_t >(indexes[pixel]) * 3;
				rgba[pixel * 4] = palette[colorIndex];
				rgba[(pixel * 4) + 1] = palette[colorIndex + 1];
				rgba[(pixel * 4) + 2] = palette[colorIndex + 2];
				rgba[(pixel * 4) + 3] = coverage[pixel] != 0 ? 255 : 0;
			}

			const auto imageName = "WAD:" + wadStem + "/Image/" + name;

			/* NOTE: the resource lambdas may run asynchronously on the resource manager's
			 * loading threads — every buffer MUST be captured by value (moved), never by
			 * reference to a local. */
			auto image = m_resources.container< ImageResource >()->getOrCreateResource(imageName, [pixels = std::move(rgba), width, height] (ImageResource & imageResource) {
				Pixmap< uint8_t > pixmap;

				if ( !pixmap.initialize(width, height, ChannelMode::RGBA, pixels) )
				{
					return false;
				}

				return imageResource.load(std::move(pixmap));
			});

			if ( image == nullptr )
			{
				return nullptr;
			}

			const auto textureName = "WAD:" + wadStem + "/Texture/" + name;

			/* NOT sRGB on purpose: unlit Basic materials on the direct swap-chain path keep the
			 * whole chain in perceptual space, exactly like the original renderer (palette
			 * texel × sector light). Decoding to linear here would darken everything. */
			return m_resources.container< TextureResource::Texture2D >()->getOrCreateResource(textureName, [image] (TextureResource::Texture2D & texture) {
				texture.enableSRGB(false);

				return texture.load(image);
			});
		};

		/* Composes a wall texture (patch assembly) or decodes a flat (64×64 raw), cached. */
		const auto resolveTexture = [&] (const std::string & name) -> std::shared_ptr< TextureResource::Texture2D > {
			if ( const auto cached = textureCache.find(name); cached != textureCache.end() )
			{
				return cached->second;
			}

			std::shared_ptr< TextureResource::Texture2D > texture;

			if ( const auto definition = textureDefinitions.find(name); definition != textureDefinitions.end() )
			{
				/* Composite wall texture. */
				const auto width = definition->second.width;
				const auto height = definition->second.height;

				std::vector< uint8_t > indexes(static_cast< size_t >(width) * height, 0);
				std::vector< uint8_t > coverage(indexes.size(), 0);

				for ( const auto & patchRef : definition->second.patches )
				{
					if ( patchRef.patchIndex >= patchNames.size() )
					{
						continue;
					}

					const auto * patchLump = findLump(patchNames[patchRef.patchIndex]);

					if ( patchLump == nullptr )
					{
						continue;
					}

					blitPatch(lumpData(*patchLump), patchLump->size, patchRef.originX, patchRef.originY, width, height, indexes, coverage);
				}

				texture = createTexture(name, width, height, indexes, coverage);
				textureSizes[name] = {static_cast< float >(width), static_cast< float >(height)};
			}
			else if ( const auto * flatLump = findLump(name); flatLump != nullptr && flatLump->size >= 4096 )
			{
				/* Raw 64×64 flat. */
				std::vector< uint8_t > indexes(4096);
				std::memcpy(indexes.data(), lumpData(*flatLump), 4096);

				const std::vector< uint8_t > coverage(4096, 1);

				texture = createTexture(name, 64, 64, indexes, coverage);
				textureSizes[name] = {64.0F, 64.0F};
			}

			if ( texture == nullptr )
			{
				TraceWarning{TracerTag} << "Texture or flat '" << name << "' not found in the WAD.";
			}

			textureCache[name] = texture;

			return texture;
		};

		/* ---- Sky texture pick (episode-based for ExMy, map-range for MAPxx). ---- */
		std::string skyTextureName{"SKY1"};

		if ( m_loadedMapName.size() == 4 && m_loadedMapName[0] == 'E' )
		{
			skyTextureName = std::string{"SKY"} + m_loadedMapName[1];
		}
		else if ( m_loadedMapName.starts_with("MAP") )
		{
			const auto mapNumber = std::atoi(m_loadedMapName.c_str() + 3);
			skyTextureName = mapNumber <= 11 ? "RSKY1" : (mapNumber <= 20 ? "RSKY2" : "RSKY3");
		}

		if ( !textureDefinitions.contains(skyTextureName) )
		{
			skyTextureName.clear();
		}

		/* ---- Geometry assembly, in Doom map space (x, y horizontal, z = height, map units). ---- */
		Buckets buckets;

		/* Emits a vertical wall quad from (x1,y1) to (x2,y2), z from bottom to top. */
		const auto emitWall = [&buckets] (const std::string & textureName, float texWidth, float texHeight, const MapVertex & from, const MapVertex & to, float bottom, float top, float uOffset, float vOffset, float light) {
			if ( textureName.empty() || textureName == "-" || top <= bottom )
			{
				return;
			}

			const auto lengthX = to.x - from.x;
			const auto lengthY = to.y - from.y;
			const auto wallLength = std::sqrt((lengthX * lengthX) + (lengthY * lengthY));

			if ( wallLength < 0.5F )
			{
				return;
			}

			/* Face normal: perpendicular to the wall, pointing toward the viewer side
			 * (the sidedef faces right of the v1→v2 direction in Doom's convention). */
			const auto normalX = lengthY / wallLength;
			const auto normalY = -lengthX / wallLength;

			const auto u0 = uOffset / texWidth;
			const auto u1 = (uOffset + wallLength) / texWidth;
			const auto v0 = vOffset / texHeight;
			const auto v1 = (vOffset + (top - bottom)) / texHeight;

			auto & bucket = buckets[textureName];

			const Corner bottomLeft{
				.x = from.x,
				.y = from.y,
				.z = bottom,
				.u = u0,
				.v = v1,
				.nx = normalX,
				.ny = normalY,
				.nz = 0.0F,
				.light = light
			};

			const Corner bottomRight{
				.x = to.x,
				.y = to.y,
				.z = bottom,
				.u = u1,
				.v = v1,
				.nx = normalX,
				.ny = normalY,
				.nz = 0.0F,
				.light = light
			};

			const Corner topLeft{
				.x = from.x,
				.y = from.y,
				.z = top,
				.u = u0,
				.v = v0,
				.nx = normalX,
				.ny = normalY,
				.nz = 0.0F,
				.light = light
			};

			const Corner topRight{
				.x = to.x,
				.y = to.y,
				.z = top,
				.u = u1,
				.v = v0,
				.nx = normalX,
				.ny = normalY,
				.nz = 0.0F,
				.light = light
			};

			bucket.push_back(topLeft);
			bucket.push_back(topRight);
			bucket.push_back(bottomRight);

			bucket.push_back(topLeft);
			bucket.push_back(bottomRight);
			bucket.push_back(bottomLeft);
		};

		for ( const auto & linedef : linedefs )
		{
			if ( linedef.v1 >= vertices.size() || linedef.v2 >= vertices.size() )
			{
				continue;
			}

			const auto & vertex1 = vertices[linedef.v1];
			const auto & vertex2 = vertices[linedef.v2];

			const auto * rightSide = linedef.rightSide >= 0 && static_cast< size_t >(linedef.rightSide) < sidedefs.size() ? &sidedefs[linedef.rightSide] : nullptr;
			const auto * leftSide = linedef.leftSide >= 0 && static_cast< size_t >(linedef.leftSide) < sidedefs.size() ? &sidedefs[linedef.leftSide] : nullptr;

			const auto * rightSector = rightSide != nullptr && rightSide->sector >= 0 && static_cast< size_t >(rightSide->sector) < sectors.size() ? &sectors[rightSide->sector] : nullptr;
			const auto * leftSector = leftSide != nullptr && leftSide->sector >= 0 && static_cast< size_t >(leftSide->sector) < sectors.size() ? &sectors[leftSide->sector] : nullptr;

			const auto sizeOf = [&textureSizes, &resolveTexture] (const std::string & name) -> std::pair< float, float > {
				if ( name.empty() || name == "-" )
				{
					return {64.0F, 64.0F};
				}

				static_cast< void >(resolveTexture(name));

				if ( const auto size = textureSizes.find(name); size != textureSizes.end() )
				{
					return size->second;
				}

				return {64.0F, 64.0F};
			};

			if ( rightSector != nullptr && leftSector == nullptr )
			{
				/* One-sided wall: full quad over the front sector opening. */
				const auto [texWidth, texHeight] = sizeOf(rightSide->middleTexture);

				emitWall(rightSide->middleTexture, texWidth, texHeight, vertex1, vertex2, rightSector->floorHeight, rightSector->ceilingHeight, rightSide->xOffset, rightSide->yOffset, rightSector->lightLevel);

				continue;
			}

			if ( rightSector == nullptr || leftSector == nullptr )
			{
				continue;
			}

			/* Two-sided linedef: lower and upper steps, seen from each side.
			 * The transparent middle textures (grates, cables) are skipped on purpose. */
			const auto skyOnBothSides = rightSector->ceilingFlat == SkyFlatName && leftSector->ceilingFlat == SkyFlatName;

			/* Right side, facing the right sector. */
			if ( leftSector->floorHeight > rightSector->floorHeight )
			{
				const auto [texWidth, texHeight] = sizeOf(rightSide->lowerTexture);

				emitWall(rightSide->lowerTexture, texWidth, texHeight, vertex1, vertex2, rightSector->floorHeight, leftSector->floorHeight, rightSide->xOffset, rightSide->yOffset, rightSector->lightLevel);
			}

			if ( leftSector->ceilingHeight < rightSector->ceilingHeight && !skyOnBothSides )
			{
				const auto [texWidth, texHeight] = sizeOf(rightSide->upperTexture);

				emitWall(rightSide->upperTexture, texWidth, texHeight, vertex1, vertex2, leftSector->ceilingHeight, rightSector->ceilingHeight, rightSide->xOffset, rightSide->yOffset, rightSector->lightLevel);
			}

			/* Left side, facing the left sector (vertices swapped so the face points left). */
			if ( rightSector->floorHeight > leftSector->floorHeight )
			{
				const auto [texWidth, texHeight] = sizeOf(leftSide->lowerTexture);

				emitWall(leftSide->lowerTexture, texWidth, texHeight, vertex2, vertex1, leftSector->floorHeight, rightSector->floorHeight, leftSide->xOffset, leftSide->yOffset, leftSector->lightLevel);
			}

			if ( rightSector->ceilingHeight < leftSector->ceilingHeight && !skyOnBothSides )
			{
				const auto [texWidth, texHeight] = sizeOf(leftSide->upperTexture);

				emitWall(leftSide->upperTexture, texWidth, texHeight, vertex2, vertex1, leftSector->ceilingHeight, rightSector->ceilingHeight, leftSide->xOffset, leftSide->yOffset, leftSector->lightLevel);
			}
		}

		/* ---- Floors and ceilings: convex fans from the BSP subsectors (SSECTORS/SEGS). ---- */

		/* Resolves the sector a subsector belongs to, through its first seg's sidedef. */
		const auto subSectorSector = [&] (const SubSector & subSector) -> int16_t {
			for ( uint16_t segIdx = 0; segIdx < subSector.segCount; ++segIdx )
			{
				const auto segIndex = static_cast< size_t >(subSector.firstSeg) + segIdx;

				if ( segIndex >= segs.size() || segs[segIndex].linedef >= linedefs.size() )
				{
					continue;
				}

				const auto & seg = segs[segIndex];
				const auto & linedef = linedefs[seg.linedef];
				const auto sideIndex = seg.direction == 0 ? linedef.rightSide : linedef.leftSide;

				if ( sideIndex >= 0 && static_cast< size_t >(sideIndex) < sidedefs.size() )
				{
					return sidedefs[sideIndex].sector;
				}
			}

			return -1;
		};

		/* Per-subsector polygon, reconstructed EXACTLY by clipping the level bounding box
		 * with the BSP partition half-planes down to each leaf, then with the leaf's own
		 * segs (their right side faces the subsector, Doom convention). Fanning or
		 * angular-sorting the seg vertices leaves holes: the corners created by two
		 * partition lines carry no seg vertex at all. */
		std::vector< Polygon2D > subSectorPolygons(subSectors.size());
		std::vector< int16_t > subSectorSectors(subSectors.size(), -1);

		for ( size_t subIdx = 0; subIdx < subSectors.size(); ++subIdx )
		{
			subSectorSectors[subIdx] = subSectorSector(subSectors[subIdx]);
		}

		{
			const auto * nodesLump = mapLump("NODES");

			std::vector< BSPNode > nodes(nodesLump != nullptr ? nodesLump->size / 28 : 0);

			for ( size_t index = 0; index < nodes.size(); ++index )
			{
				const auto * entry = lumpData(*nodesLump) + (index * 28);
				nodes[index].x = static_cast< float >(readInt16(entry));
				nodes[index].y = static_cast< float >(readInt16(entry + 2));
				nodes[index].dx = static_cast< float >(readInt16(entry + 4));
				nodes[index].dy = static_cast< float >(readInt16(entry + 6));
				nodes[index].rightChild = readUInt16(entry + 24);
				nodes[index].leftChild = readUInt16(entry + 26);
			}

			/* Level bounding box (with a margin), counter-clockwise. */
			auto minX = std::numeric_limits< float >::max();
			auto minY = std::numeric_limits< float >::max();
			auto maxX = std::numeric_limits< float >::lowest();
			auto maxY = std::numeric_limits< float >::lowest();

			for ( const auto & vertex : vertices )
			{
				minX = std::min(minX, vertex.x);
				minY = std::min(minY, vertex.y);
				maxX = std::max(maxX, vertex.x);
				maxY = std::max(maxY, vertex.y);
			}

			const Polygon2D levelBox{{minX - 64.0F, minY - 64.0F}, {maxX + 64.0F, minY - 64.0F}, {maxX + 64.0F, maxY + 64.0F}, {minX - 64.0F, maxY + 64.0F}};

			const auto clipBySegs = [&] (uint16_t subSectorIndex, Polygon2D polygon) {
				if ( subSectorIndex >= subSectors.size() )
				{
					return;
				}

				const auto & subSector = subSectors[subSectorIndex];

				for ( uint16_t segIdx = 0; segIdx < subSector.segCount && polygon.size() >= 3; ++segIdx )
				{
					const auto segIndex = static_cast< size_t >(subSector.firstSeg) + segIdx;

					if ( segIndex >= segs.size() || segs[segIndex].v1 >= vertices.size() || segs[segIndex].v2 >= vertices.size() )
					{
						continue;
					}

					const auto & from = vertices[segs[segIndex].v1];
					const auto & to = vertices[segs[segIndex].v2];

					polygon = clipPolygon(polygon, from.x, from.y, to.x - from.x, to.y - from.y, true);
				}

				if ( polygon.size() >= 3 )
				{
					subSectorPolygons[subSectorIndex] = std::move(polygon);
				}
			};

			if ( nodes.empty() )
			{
				/* Degenerate single-subsector map (no BSP nodes). */
				if ( !subSectors.empty() )
				{
					clipBySegs(0, levelBox);
				}
			}
			else
			{
				/* Iterative descent, carrying the clipped polygon. Bit 15 of a child = leaf. */
				std::vector< std::pair< uint16_t, Polygon2D > > stack;
				stack.emplace_back(static_cast< uint16_t >(nodes.size() - 1), levelBox);

				while ( !stack.empty() )
				{
					const auto [nodeIndex, polygon] = std::move(stack.back());
					stack.pop_back();

					if ( (nodeIndex & 0x8000U) != 0 )
					{
						clipBySegs(nodeIndex & 0x7FFFU, polygon);

						continue;
					}

					if ( nodeIndex >= nodes.size() || polygon.size() < 3 )
					{
						continue;
					}

					const auto & node = nodes[nodeIndex];

					auto rightPolygon = clipPolygon(polygon, node.x, node.y, node.dx, node.dy, true);
					auto leftPolygon = clipPolygon(polygon, node.x, node.y, node.dx, node.dy, false);

					if ( rightPolygon.size() >= 3 )
					{
						stack.emplace_back(node.rightChild, std::move(rightPolygon));
					}

					if ( leftPolygon.size() >= 3 )
					{
						stack.emplace_back(node.leftChild, std::move(leftPolygon));
					}
				}
			}
		}

		const auto emitFlat = [&buckets] (const std::string & textureName, const Polygon2D & polygon, float height, bool isCeiling, float light) {
			if ( textureName.empty() || polygon.size() < 3 )
			{
				return;
			}

			auto & bucket = buckets[textureName];

			const auto normalZ = isCeiling ? -1.0F : 1.0F;

			const auto corner = [&] (size_t pointIndex) {
				const auto & point = polygon[pointIndex];

				/* Flats are 64×64, world-aligned. */
				return Corner{
					.x = point.first,
					.y = point.second,
					.z = height,
					.u = point.first / 64.0F,
					.v = -point.second / 64.0F,
					.nx = 0.0F,
					.ny = 0.0F,
					.nz = normalZ,
					.light = light
				};
			};

			for ( size_t fanIdx = 1; fanIdx + 1 < polygon.size(); ++fanIdx )
			{
				if ( isCeiling )
				{
					bucket.push_back(corner(0));
					bucket.push_back(corner(fanIdx));
					bucket.push_back(corner(fanIdx + 1));
				}
				else
				{
					bucket.push_back(corner(0));
					bucket.push_back(corner(fanIdx + 1));
					bucket.push_back(corner(fanIdx));
				}
			}
		};

		for ( size_t subIdx = 0; subIdx < subSectors.size(); ++subIdx )
		{
			const auto sectorIndex = subSectorSectors[subIdx];

			if ( sectorIndex < 0 || static_cast< size_t >(sectorIndex) >= sectors.size() )
			{
				continue;
			}

			const auto & sector = sectors[sectorIndex];

			static_cast< void >(resolveTexture(sector.floorFlat));
			emitFlat(sector.floorFlat, subSectorPolygons[subIdx], sector.floorHeight, false, sector.lightLevel);

			if ( sector.ceilingFlat == SkyFlatName )
			{
				/* Sky ceiling: use the episode sky texture, fullbright. */
				if ( !skyTextureName.empty() )
				{
					static_cast< void >(resolveTexture(skyTextureName));
					emitFlat(skyTextureName, subSectorPolygons[subIdx], sector.ceilingHeight, true, 1.0F);
				}
			}
			else
			{
				static_cast< void >(resolveTexture(sector.ceilingFlat));
				emitFlat(sector.ceilingFlat, subSectorPolygons[subIdx], sector.ceilingHeight, true, sector.lightLevel);
			}
		}

		if ( buckets.empty() )
		{
			TraceError{TracerTag} << "Map " << m_loadedMapName << " produced no geometry !";

			return false;
		}

		/* ---- Player 1 start (THINGS type 1), floor height via the subsector fans. ---- */
		const auto scale = (1.0F / MapUnitsPerMeter) * (m_options.uniformScale > 0.0F ? m_options.uniformScale : 1.0F);

		if ( thingsLump != nullptr )
		{
			for ( size_t index = 0; index < thingsLump->size / 10; ++index )
			{
				const auto * entry = lumpData(*thingsLump) + (index * 10);

				if ( readInt16(entry + 6) != 1 )
				{
					continue;
				}

				const auto startX = static_cast< float >(readInt16(entry));
				const auto startY = static_cast< float >(readInt16(entry + 2));
				const auto startAngle = static_cast< float >(readInt16(entry + 4)) * (std::numbers::pi_v< float > / 180.0F);

				/* Find the containing subsector (2D point-in-triangle over the fans). */
				float floorHeight = 0.0F;

				const auto pointSide = [] (float ax, float ay, float bx, float by, float px, float py) {
					return ((bx - ax) * (py - ay)) - ((by - ay) * (px - ax));
				};

				for ( size_t subIdx = 0; subIdx < subSectors.size(); ++subIdx )
				{
					if ( subSectorSectors[subIdx] < 0 )
					{
						continue;
					}

					const auto & polygon = subSectorPolygons[subIdx];
					bool found = false;

					for ( size_t fanIdx = 1; fanIdx + 1 < polygon.size(); ++fanIdx )
					{
						const auto & vertexA = polygon[0];
						const auto & vertexB = polygon[fanIdx];
						const auto & vertexC = polygon[fanIdx + 1];

						const auto side1 = pointSide(vertexA.first, vertexA.second, vertexB.first, vertexB.second, startX, startY);
						const auto side2 = pointSide(vertexB.first, vertexB.second, vertexC.first, vertexC.second, startX, startY);
						const auto side3 = pointSide(vertexC.first, vertexC.second, vertexA.first, vertexA.second, startX, startY);

						if ( (side1 >= 0 && side2 >= 0 && side3 >= 0) || (side1 <= 0 && side2 <= 0 && side3 <= 0) )
						{
							floorHeight = sectors[subSectorSectors[subIdx]].floorHeight;
							found = true;

							break;
						}
					}

					if ( found )
					{
						break;
					}
				}

				/* Engine world space (Y-down, after the consumer's 180° X rotation):
				 * Xw = x, Yw = -height, Zw = -y. */
				m_playerStartPosition = {startX * scale, -floorHeight * scale, -startY * scale};
				m_playerStartDirection = {std::cos(startAngle), 0.0F, -std::sin(startAngle)};

				break;
			}
		}

		/* ---- Build the multi-material Shape (loader space: Y-up, meters). ---- */
		auto shape = std::make_shared< Shape< float > >();

		std::vector< std::string > materialOrder;
		materialOrder.reserve(buckets.size());

		const auto built = shape->build([&] (auto & groups, auto & shapeVertices, auto & triangles) {
			std::unordered_map< float, uint32_t > lightColorIndexes;

			auto & vertexColors = shape->vertexColors();

			const auto colorIndexFor = [&] (float light) -> uint32_t {
				if ( const auto existing = lightColorIndexes.find(light); existing != lightColorIndexes.end() )
				{
					return existing->second;
				}

				const auto colorIndex = static_cast< uint32_t >(vertexColors.size());

				/* Doom light levels stay RAW: the whole chain is kept in perceptual space
				 * (non-sRGB textures × light), replicating the original renderer's math. */
				vertexColors.emplace_back(light, light, light, 1.0F);
				lightColorIndexes[light] = colorIndex;

				return colorIndex;
			};

			bool firstBucket = true;

			for ( const auto & [textureName, corners] : buckets )
			{
				materialOrder.push_back(textureName);

				if ( !firstBucket )
				{
					groups.emplace_back(static_cast< uint32_t >(triangles.size()), 0U);
				}

				firstBucket = false;

				const auto triangleCount = corners.size() / 3;

				for ( size_t triangle = 0; triangle < triangleCount; ++triangle )
				{
					const auto baseVertex = static_cast< uint32_t >(shapeVertices.size());
					std::array< uint32_t, 3 > colorIndexes{};

					for ( size_t cornerIdx = 0; cornerIdx < 3; ++cornerIdx )
					{
						const auto & corner = corners[(triangle * 3) + cornerIdx];

						/* Doom map space → loader space (Y-up, right-handed): X = x, Y = height, Z = y. */
						const ShapeVertex< float > vertex{
							Vector< 3, float >{corner.x * scale, corner.z * scale, corner.y * scale},
							Vector< 3, float >{corner.nx, corner.nz, corner.ny},
							Vector< 3, float >{corner.u, corner.v, 0.0F}
						};

						shapeVertices.emplace_back(vertex);
						colorIndexes[cornerIdx] = colorIndexFor(corner.light);
					}

					/* Winding swap (indices 1 and 2): the consumer applies a 180° X rotation
					 * (Y-up → Y-down) which inverts the winding — same convention as GLTF/FBX. */
					auto & shapeTriangle = triangles.emplace_back(baseVertex, baseVertex + 2, baseVertex + 1);
					shapeTriangle.setVertexColorIndex(0, colorIndexes[0]);
					shapeTriangle.setVertexColorIndex(1, colorIndexes[2]);
					shapeTriangle.setVertexColorIndex(2, colorIndexes[1]);
				}

				if ( !groups.empty() )
				{
					groups.back().second += static_cast< uint32_t >(triangleCount);
				}
			}

			return true;
		}, true);

		if ( !built || !shape->isValid() )
		{
			TraceError{TracerTag} << "Unable to build the shape for map " << m_loadedMapName << " !";

			return false;
		}

		shape->declareNormalsAvailable();

		/* ---- Engine resources: geometry, materials, mesh. ---- */
		const auto resourcePrefix = "WAD:" + wadStem + "/" + m_loadedMapName;

		constexpr auto geometryFlags = Geometry::EnableNormal | Geometry::EnablePrimaryTextureCoordinates | Geometry::EnableVertexColor;

		auto geometry = m_resources.container< Geometry::IndexedVertexResource >()->getOrCreateResource(resourcePrefix + "/Geometry/Level", [shape] (Geometry::IndexedVertexResource & geometryResource) {
			return geometryResource.load(*shape);
		}, geometryFlags);

		if ( geometry == nullptr )
		{
			TraceError{TracerTag} << "Unable to create the level geometry for map " << m_loadedMapName << " !";

			return false;
		}

		std::vector< std::shared_ptr< Material::Interface > > materialList;
		materialList.reserve(materialOrder.size());

		for ( const auto & textureName : materialOrder )
		{
			const auto texture = resolveTexture(textureName);

			std::stringstream materialName;
			materialName << "WAD:" << wadStem << "/Material/" << textureName;

			auto material = m_resources.container< Material::BasicResource >()->getOrCreateResource(materialName.str(), [texture] (Material::BasicResource & materialResource) {
				materialResource.enableVertexColor();

				if ( texture != nullptr )
				{
					if ( !materialResource.setTextureResource(texture) )
					{
						return false;
					}
				}
				else if ( !materialResource.setColor(PixelFactory::Color< float >{0.5F, 0.5F, 0.5F, 1.0F}) )
				{
					return false;
				}

				return materialResource.setManualLoadSuccess(true);
			});

			materialList.emplace_back(material);
		}

		/* Double-sided: spares the whole winding-convention debate for a materializer —
		 * a Doom level is watertight from inside, back-faces are almost never visible. */
		std::vector< RasterizationOptions > rasterizationOptions(materialList.size());

		for ( auto & options : rasterizationOptions )
		{
			options.setCullingMode(CullingMode::None);
		}

		auto mesh = m_resources.container< Renderable::MultiLayerMeshResource >()->getOrCreateResource(resourcePrefix + "/Mesh/Level", [geometry, materialList, rasterizationOptions] (Renderable::MultiLayerMeshResource & meshResource) {
			return meshResource.load(std::static_pointer_cast< Geometry::Interface >(geometry), materialList, rasterizationOptions);
		});

		if ( mesh == nullptr )
		{
			TraceError{TracerTag} << "Unable to create the level mesh for map " << m_loadedMapName << " !";

			return false;
		}

		/* ---- AssetData: one mesh, one root node. ---- */
		MeshDescriptor meshDescriptor;
		meshDescriptor.renderable = mesh;
		meshDescriptor.geometry = std::static_pointer_cast< Geometry::Interface >(geometry);
		meshDescriptor.materials = std::move(materialList);

		output.meshes.emplace_back(std::move(meshDescriptor));

		NodeDescriptor nodeDescriptor;
		nodeDescriptor.name = m_loadedMapName;
		nodeDescriptor.meshIndex = 0;

		output.nodes.emplace_back(std::move(nodeDescriptor));
		output.rootNodeIndices.push_back(output.nodes.size() - 1);

		TraceInfo{TracerTag} << "Map " << m_loadedMapName << " materialized: " << buckets.size() << " textures, " << shape->vertexColors().size() << " light levels.";

		return true;
	}
}
