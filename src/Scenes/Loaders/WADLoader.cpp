/*
 * src/Scenes/Loaders/WADLoader.cpp
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

/* Local inclusions for the analysis. */
#include "WADParser.hpp"

/* STL inclusions. */
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <numbers>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <utility>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "VertexFactory/Shape.hpp"
#include "SceneData.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/MovieResource.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Resources/Manager.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace
{
	constexpr auto TracerTag{"WADLoader"};
}

namespace EmEn::Scenes::Loaders
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
	WADLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		/* ---- Read the whole WAD in memory (a full IWAD is ~15 MB). ---- */
		std::vector< uint8_t > wad;

		{
			/* NOTE: Through Base::IO::openInputFile(): a FIFO blocked the open forever (2026-10-18). */
			std::ifstream file;

			if ( !Base::IO::openInputFile(filepath, file) )
			{
				TraceError{TracerTag} << "Unable to open the WAD file " << filepath << " !";

				return false;
			}

			/* NOTE: tellg() answers -1 when the size is unknown: resized to it, the vector asked for 2^64 - 1 bytes (an
			 * abort under -fno-exceptions); and a short read was never checked (zeros parsed as a WAD). */
			const auto bytes = Base::IO::fileSizeFromStream(file);

			if ( bytes < 0 )
			{
				TraceError{TracerTag} << "The size of the WAD file " << filepath << " is unknown !";

				return false;
			}

			wad.resize(static_cast< size_t >(bytes));

			if ( !file.read(reinterpret_cast< char * >(wad.data()), static_cast< std::streamsize >(wad.size())) )
			{
				TraceError{TracerTag} << "Unable to read the WAD file " << filepath << " whole !";

				return false;
			}
		}

		/* ---- Analysis: the whole parse and its validations, without any resource (fuzz_wad covers it). ---- */
		auto level = WAD::parse(wad, WAD::Selection{.mapName = m_mapName, .mapIndex = m_mapIndex}, Base::IO::toU8String(filepath), m_options.uniformScale);

		if ( !level.has_value() )
		{
			return false;
		}

		m_loadedMapName = level->mapName;
		m_playerStartPosition = level->playerStartPosition;
		m_playerStartDirection = level->playerStartDirection;

		const auto wadStem = Base::IO::toU8String(filepath.stem());

		/* ---- Static textures: an image and an sRGB texture per composed texture or flat. ---- */
		std::unordered_map< std::string, std::shared_ptr< TextureResource::Texture2D > > textures;

		for ( auto & [name, record] : level->textures )
		{
			const auto imageName = Base::String::concatenate("WAD:", wadStem, "/Image/", name);
			const auto width = record.width;
			const auto height = record.height;

			/* NOTE: the resource lambdas may run asynchronously on the resource manager's
			 * loading threads — every buffer MUST be captured by value, never by reference. */
			auto image = m_resources.container< ImageResource >()->getOrCreateResource(imageName, [pixels = std::move(record.rgba), width, height] (ImageResource & imageResource) {
				Pixmap< uint8_t > pixmap;

				if ( !pixmap.initialize(width, height, ChannelMode::RGBA, pixels) )
				{
					return false;
				}

				return imageResource.load(std::move(pixmap));
			});

			if ( image == nullptr )
			{
				TraceWarning{TracerTag} << "Texture or flat '" << name << "' could not be created.";

				continue;
			}

			const auto textureName = Base::String::concatenate("WAD:", wadStem, "/Texture/", name);

			/* sRGB, like every colour texture (owner decision 2026-10-14): the texture is the albedo,
			 * whose setter requires it. Until then the loader created it linear on purpose (a perceptual
			 * chain, texel × sector light) and setAlbedoComponent() rewrote it sRGB on the shared
			 * resource, racing the upload on a pool worker: the format depended on which task finished
			 * first. The chain is photometric since (emission × FullBrightLuminance, exposure triad). */
			/* NOTE: Created SYNCHRONOUSLY (the function only links the source): the colour space is set when this returns,
			 * and the material setter checks it — an asynchronous creation would set it later, on a worker (texture identity,
			 * docs/caution-points.md). */
			textures[name] = m_resources.container< TextureResource::Texture2D >()->getOrCreateResourceSync(textureName, [image] (TextureResource::Texture2D & texture) {
				if ( !texture.enableSRGB(true) )
				{
					return false;
				}

				return texture.load(image);
			});
		}

		/* ---- Animated ranges: one movie and one AnimatedTexture2D shared by every name of the range. ---- */

		/* Every name of a resolved range maps to the SAME AnimatedTexture2D. Vanilla additionally
		 * phase-shifts each name by its position in the range (a NUKAGE2 sector runs one frame ahead
		 * of a NUKAGE1 one, the `+i` in P_UpdateSpecials); that offset is deliberately NOT
		 * reproduced — owner decision — so the whole range shares one texture and one upload. */
		std::unordered_map< std::string, std::shared_ptr< TextureResource::AnimatedTexture2D > > animatedTextures;

		for ( auto & range : level->animatedRanges )
		{
			std::vector< MovieResource::Frame > frames;
			frames.reserve(range.frames.size());

			for ( auto & pixmap : range.frames )
			{
				frames.emplace_back(std::move(pixmap), WAD::AnimationFrameDurationMS);
			}

			/* ⚠️ Captured BY VALUE (moved): these lambdas run on the resource manager's loading
			 * threads, so a reference to a local here is a dangling read. */
			auto movie = m_resources.container< MovieResource >()->getOrCreateResource(Base::String::concatenate("WAD:", wadStem, "/Movie/", range.suffix), [movieFrames = std::move(frames)] (MovieResource & movieResource) mutable {
				return movieResource.load(std::move(movieFrames));
			});

			if ( movie == nullptr )
			{
				continue;
			}

			/* sRGB, same reasoning as the static path. */
			/* NOTE: Created SYNCHRONOUSLY (the function only links the source): the colour space is set when this returns,
			 * and the material setter checks it — an asynchronous creation would set it later, on a worker (texture identity,
			 * docs/caution-points.md). */
			auto animated = m_resources.container< TextureResource::AnimatedTexture2D >()->getOrCreateResourceSync(Base::String::concatenate("WAD:", wadStem, "/AnimTexture/", range.suffix), [movie] (TextureResource::AnimatedTexture2D & texture) {
				if ( !texture.enableSRGB(true) )
				{
					return false;
				}

				return texture.load(movie);
			});

			if ( animated == nullptr )
			{
				continue;
			}

			for ( const auto & frameName : range.names )
			{
				animatedTextures[frameName] = animated;
			}
		}

		/* ---- Engine resources: geometry, materials, mesh. ---- */
		const auto resourcePrefix = Base::String::concatenate("WAD:", wadStem, "/", m_loadedMapName);

		constexpr auto geometryFlags = Geometry::EnableNormal | Geometry::EnablePrimaryTextureCoordinates | Geometry::EnableVertexColor;

		auto geometry = m_resources.container< Geometry::IndexedVertexResource >()->getOrCreateResource(resourcePrefix + "/Geometry/Level", [shape = level->shape] (Geometry::IndexedVertexResource & geometryResource) {
			return geometryResource.load(*shape);
		}, geometryFlags);

		if ( geometry == nullptr )
		{
			TraceError{TracerTag} << "Unable to create the level geometry for map " << m_loadedMapName << " !";

			return false;
		}

		std::vector< std::shared_ptr< Material::Interface > > materialList;
		materialList.reserve(level->surfaces.size());

		for ( const auto & surface : level->surfaces )
		{
			const auto isMasked = surface.masked;

			/* An animated range wins over the static texture: every name of the range resolved to the
			 * same AnimatedTexture2D, a Texture2DArray whose layer is selected by the per-draw
			 * frameIndex push constant. Nothing animation-specific is declared on the material —
			 * setAlbedoComponent() raises PrimaryTextureCoordinatesUses3D from the texture's own
			 * request3DTextureCoordinates(), create() raises IsAnimated from duration() > 0, and
			 * Component::Visual ticks the animation time on the logic thread. */
			std::shared_ptr< TextureResource::Abstract > texture;

			if ( const auto animated = animatedTextures.find(surface.textureName); animated != animatedTextures.end() )
			{
				texture = animated->second;
			}
			else if ( const auto still = textures.find(surface.textureName); still != textures.end() )
			{
				texture = still->second;
			}

			/* ⚠️ The surface class is part of the material NAME, not just of the bucket key: the
			 * resource container is keyed by name, so an opaque and a masked variant of the same
			 * texture sharing one name would collide and whichever loaded first would impose its
			 * mode on the other — a grate turning its solid twin see-through, or the reverse. */
			std::stringstream materialName;
			materialName << "WAD:" << wadStem << "/Material/" << surface.textureName << (isMasked ? "/Masked" : "");

			auto material = m_resources.container< Material::StandardResource >()->getOrCreateResource(materialName.str(), [texture, isMasked] (Material::StandardResource & materialResource) {
				materialResource.enableVertexColor();

				if ( texture != nullptr )
				{
					/* A masked surface consumes the alpha channel the composition already wrote
					 * (255 where a patch covers the texel, 0 where none does) and cuts out on it.
					 * ALPHA-TEST, not blending: vanilla writes the texel straight to the framebuffer
					 * and never reads the destination, so the transparency is strictly binary. This
					 * keeps the layer in the opaque list with depth write on — no sorting needed. */
					if ( !materialResource.setAlbedoComponent(texture, isMasked) )
					{
						return false;
					}

					if ( isMasked )
					{
						/* 0.5 EXPLICITLY: vanilla's cutout is binary (alpha is 0 or 255), any
						 * threshold in between behaves identically, and this is the value the
						 * fixed cutoff used before the threshold became configurable. */
						materialResource.enableAlphaTest(0.5F);
					}
				}
				else if ( !materialResource.setAlbedoComponent(PixelFactory::Color< float >{0.5F, 0.5F, 0.5F, 1.0F}) )
				{
					return false;
				}

				/* The surface EMITS, it is not lit — the sector light level is already baked into
				 * the vertex colors, so re-lighting would double-count it. UNLIT declares exactly
				 * that: no light pass runs over the surface and its colour IS its emitted radiance.
				 * ⚠️ Unlit writes fragmentColor().rgb x emissionMultiplier(), so the
				 * AutoIllumination COMPONENT below is MANDATORY — without it the surface writes its
				 * raw [0,1] colour and reads black under the photometric exposure.
				 * Full self-illumination mask, scaled to an absolute luminance: the emitted quantity
				 * is texel x vertexColor x FullBrightLuminance, which anchors a 1993 [0,1] ordinal in
				 * candela per square meter and lets a dim sector emit proportionally less.
				 * ⚠️ The AMOUNT is the mask and is clamped to [0,1]; the luminance belongs to the
				 * STRENGTH. Passing the nits through the amount silently clamps them to 1. */
				materialResource.enableUnlit();
				materialResource.setAutoIlluminationComponent(1.0F);
				materialResource.setEmissiveStrength(FullBrightLuminance);

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

		/* ---- SceneData: one mesh, one root node. ---- */
		MeshDescriptor meshDescriptor;
		meshDescriptor.renderable = mesh;
		meshDescriptor.geometry = std::static_pointer_cast< Geometry::Interface >(geometry);
		meshDescriptor.materials = std::move(materialList);
		/* UNLIT, and it must stay that way: the sector light levels are already baked into the
		 * vertex colors of the materials, which declare themselves UNLIT above through
		 * enableUnlit(), exactly like the original renderer. On the LIT
		 * path the ambient/IBL pass is scaled by the background luminance, so installing a sky
		 * would multiply every surface by the sky brightness and the baked look would be destroyed.
		 * ⚠️ LATENT ONLY, and do not claim otherwise: with the light set disabled — which is the
		 * case for every demo shipping this loader today — the mesh is already off the lit path, so
		 * this flag changes nothing observable. It is armed for the day a scene enables the light
		 * set with a map loaded. Verified by measurement: the map's pixels are BIT-IDENTICAL across
		 * the whole 1-to-31800-nit sky store, so nothing reaches them from the sky today. */
		meshDescriptor.lightingEnabled = false;

		output.meshes.emplace_back(std::move(meshDescriptor));

		NodeDescriptor nodeDescriptor;
		nodeDescriptor.name = m_loadedMapName;
		nodeDescriptor.meshIndex = 0;

		output.nodes.emplace_back(std::move(nodeDescriptor));
		output.rootNodeIndices.push_back(output.nodes.size() - 1);

		TraceInfo{TracerTag} << "Map " << m_loadedMapName << " materialized: " << level->surfaces.size() << " textures, " << level->shape->vertexColors().size() << " light levels.";

		return true;
	}
}
