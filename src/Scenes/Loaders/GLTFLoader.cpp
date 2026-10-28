/*
 * src/Scenes/Loaders/GLTFLoader.cpp
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

#include "GLTFLoader.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iterator>
#include <limits>
#include <numbers>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

/* Third-party inclusions. */
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "json/json.h"
#include "volk.h"

/* Local inclusions. */
#include "Animation/Skin.hpp"
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletonResource.hpp"
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "GLTFParser.hpp"
#include "Graphics/CompressedImageResource.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/KTX2Decoder.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/TexturePacker.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/Types.hpp"
#include "IO/IO.hpp"
#include "Logging/Severity.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "Network/PercentEncoding.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "PixelFactory/Types.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Container.hpp"
#include "Resources/Manager.hpp"
#include "SceneData.hpp"
#include "Scenes/Loaders/Interface.hpp"
#include "Scenes/Loaders/LoaderOptions.hpp"
#include "String.hpp"
#include "Tracer.hpp"
#include "VertexFactory/Shape.hpp"
#include "VertexFactory/ShapeVertex.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/PhysicalDevice.hpp"

namespace
{
	using namespace EmEn;
	using namespace EmEn::Graphics;
	using namespace EmEn::Base::Math;
	using namespace EmEn::Base::PixelFactory;

	/* Where a glTF buffer lies in a FILE (an image's encoded source, docs/subsystems/resources/03 § CPU Copies). */
	struct BufferLocation final
	{
		std::filesystem::path filepath;
		uint64_t offset{0};
	};

	/* Reads a little-endian 32-bit value (the GLB header's encoding). */
	uint32_t
	readLE32 (const std::vector< std::byte > & bytes, size_t offset) noexcept
	{
		if ( offset + 4 > bytes.size() )
		{
			return 0;
		}

		return std::to_integer< uint32_t >(bytes[offset]) | (std::to_integer< uint32_t >(bytes[offset + 1]) << 8U) | (std::to_integer< uint32_t >(bytes[offset + 2]) << 16U) | (std::to_integer< uint32_t >(bytes[offset + 3]) << 24U);
	}

	/* The model's OWN JSON (fastgltf keeps either a copy or a view of the bytes, depending on its version): the whole
	 * file of a .gltf, the JSON chunk of a .glb, with the offset of its BIN chunk. */
	struct ModelJSON final
	{
		Json::Value root;
		std::optional< uint64_t > binDataOffset;
	};

	/* Reads the model's JSON. nullopt when the file is unreadable or holds no JSON object: fastgltf then reports it. */
	std::optional< ModelJSON >
	readModelJSON (const std::filesystem::path & filepath) noexcept
	{
		constexpr uint32_t GLBMagic{0x46546C67}; /* "glTF" */
		constexpr uint32_t JSONChunk{0x4E4F534A}; /* "JSON" */
		constexpr uint32_t BINChunk{0x004E4942}; /* "BIN\0" */
		constexpr uint64_t GLBHeaderBytes{12};
		constexpr uint64_t ChunkHeaderBytes{8};

		std::string json;
		std::optional< uint64_t > binDataOffset;
		std::vector< std::byte > header;

		if ( Base::IO::fileGetRange(filepath, 0, GLBHeaderBytes + ChunkHeaderBytes, header) && readLE32(header, 0) == GLBMagic )
		{
			const uint64_t jsonLength = readLE32(header, 12);
			std::vector< std::byte > jsonBytes;

			if ( readLE32(header, 16) != JSONChunk || !Base::IO::fileGetRange(filepath, GLBHeaderBytes + ChunkHeaderBytes, jsonLength, jsonBytes) )
			{
				return std::nullopt;
			}

			json.resize(jsonBytes.size());

			std::ranges::transform(jsonBytes, json.begin(), [] (std::byte value) {
				return static_cast< char >(value);
			});

			const auto binHeaderOffset = GLBHeaderBytes + ChunkHeaderBytes + jsonLength;

			if ( std::vector< std::byte > binHeader; Base::IO::fileGetRange(filepath, binHeaderOffset, ChunkHeaderBytes, binHeader) && readLE32(binHeader, 4) == BINChunk )
			{
				binDataOffset = binHeaderOffset + ChunkHeaderBytes;
			}
		}
		else if ( !Base::IO::fileGetContents(filepath, json) )
		{
			return std::nullopt;
		}

		auto root = Base::FastJSON::getRootFromString(json, 64, true);

		if ( !root.has_value() || !root->isObject() )
		{
			return std::nullopt;
		}

		return ModelJSON{.root = std::move(*root), .binDataOffset = binDataOffset};
	}

	/* Returns the first buffers[] or images[] uri that is not strict UTF-8 once percent-decoded, nullopt when all are.
	 * NOTE: A trust-boundary check BEFORE fastgltf (owner decision 2026-10-12, rule C): fastgltf 0.9.1 percent-decodes
	 * a uri and hands it to std::filesystem::u8path(), which THROWS on invalid UTF-8 with MSVC (an abort: "%FF.bin"
	 * killed the engine), while POSIX would only answer "file not found". The JSON itself is valid UTF-8 (the parser
	 * checks it): only a percent-escape can smuggle an invalid byte in. A data: URI is not a path. */
	std::optional< std::string >
	findNonUTF8URI (const Json::Value & root) noexcept
	{
		constexpr std::array< const char *, 2 > Keys{"buffers", "images"};

		for ( const auto * const key : Keys )
		{
			const auto entries = Base::FastJSON::getArray(root, key);

			if ( !entries.has_value() )
			{
				continue;
			}

			for ( const auto & entry : *entries )
			{
				const auto uri = entry.isObject() ? Base::FastJSON::getValue< std::string >(entry, "uri") : std::nullopt;

				if ( !uri.has_value() || uri->starts_with("data:") )
				{
					continue;
				}

				if ( !Base::String::isValidUTF8(Base::Network::PercentEncoding::decode(*uri)) )
				{
					return uri;
				}
			}
		}

		return std::nullopt;
	}

	/* Locates every buffer of a glTF in a file, from the model's OWN JSON: the BIN chunk of a .glb (the buffer without
	 * "uri"), or an external .bin. A data: URI buffer, or anything unreadable, gets no location. */
	std::vector< std::optional< BufferLocation > >
	locateBuffers (const std::filesystem::path & filepath, const std::filesystem::path & basePath) noexcept
	{
		std::vector< std::optional< BufferLocation > > locations;

		const auto model = readModelJSON(filepath);

		if ( !model.has_value() )
		{
			return locations;
		}

		const auto buffers = Base::FastJSON::getArray(model->root, "buffers");

		if ( !buffers.has_value() )
		{
			return locations;
		}

		for ( const auto & buffer : *buffers )
		{
			const auto uri = buffer.isObject() ? Base::FastJSON::getValue< std::string >(buffer, "uri") : std::nullopt;

			if ( !uri.has_value() )
			{
				/* NOTE: Only the FIRST buffer of a .glb may omit its uri: it is the BIN chunk. */
				if ( buffer.isObject() && model->binDataOffset.has_value() && locations.empty() )
				{
					locations.emplace_back(BufferLocation{.filepath = filepath, .offset = *model->binDataOffset});
				}
				else
				{
					locations.emplace_back();
				}

				continue;
			}

			if ( uri->starts_with("data:") )
			{
				locations.emplace_back();

				continue;
			}

			/* NOTE: The same resolution as an image's uri (fastgltf decodes the percent-escapes). */
			const fastgltf::URI parsedURI{std::string_view{*uri}};

			/* NOTE: A glTF URI is UTF-8 (glTF 2.0 § 3.1): IO::u8path(), never the narrow path constructor (the ANSI code
			 * page on Windows). */
			locations.emplace_back(BufferLocation{.filepath = basePath / Base::IO::u8path(std::string{parsedURI.path()}), .offset = 0});
		}

		return locations;
	}

	/* Translates a glTF sampler wrap mode. The three values are the whole of what glTF can
	 * express (GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE), so this conversion is total. */
	constexpr
	TextureResource::WrapMode
	toWrapMode (fastgltf::Wrap wrap) noexcept
	{
		switch ( wrap )
		{
			case fastgltf::Wrap::ClampToEdge :
				return TextureResource::WrapMode::ClampToEdge;

			case fastgltf::Wrap::MirroredRepeat :
				return TextureResource::WrapMode::MirroredRepeat;

			case fastgltf::Wrap::Repeat :
				break;
		}

		return TextureResource::WrapMode::Repeat;
	}

	/* Detect image format from MIME type. */
	Pixmap< uint8_t >::Format
	mimeToPixmapFormat (fastgltf::MimeType mime) noexcept
	{
		switch ( mime )
		{
			case fastgltf::MimeType::JPEG :
				return Pixmap< uint8_t >::Format::Jpeg;

			case fastgltf::MimeType::PNG :
				return Pixmap< uint8_t >::Format::PNG;

			/* ⚠️ Decoded to RGB/RGBA pixels like PNG and JPEG, NOT kept compressed like KTX2:
			 * WebP has no GPU format, so it takes the ordinary Pixmap path and the consumer's
			 * CPU BC7 + texture cache afterwards. */
			case fastgltf::MimeType::WEBP :
				return Pixmap< uint8_t >::Format::WebP;

			default :
				return Pixmap< uint8_t >::Format::None;
		}
	}

	/* A texture's UV transform (KHR_texture_transform) and its texture coordinate SET: glTF's TextureInfo.texCoord,
	 * overridden by the transform's own texCoord. `present` is set when either applies: an identity transform is
	 * harmless, and the set must reach the material (2026-10-03: the set was ignored, every map sampled TEXCOORD_0 —
	 * CarConcept's occlusion, baked on TEXCOORD_1, blotched its paint). */
	struct UVTransform final
	{
		Vector< 2, float > scale{1.0F, 1.0F};
		Vector< 2, float > offset{0.0F, 0.0F};
		float rotation{0.0F};
		uint32_t set{0};
		bool present{false};
	};

	/* Applies a texture's UV transform and its texture coordinate set to its material component. */
	void
	applyTextureCoordinates (Material::StandardResource & material, ComponentType componentType, const UVTransform & transform) noexcept
	{
		material.setComponentUVWTransform(componentType, transform.scale, transform.offset, transform.rotation);

		if ( transform.set == 1 )
		{
			static_cast< void >(material.setComponentUVWChannel(componentType, 1));
		}
	}
}

/* Forward declarations. */
namespace EmEn::Graphics::Geometry
{
}

namespace EmEn::Scenes::Loaders
{
	using namespace Graphics;
	using namespace Graphics::Geometry;
	using namespace Base::Animation;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Base::PixelFactory;

	using GLTF::materialSamplesSecondarySet;
	using GLTF::textureCoordinateSet;

	GLTFLoader::GLTFLoader (Resources::Manager & resources) noexcept
		: m_resources{resources}
	{

	}

	GLTFLoader::~GLTFLoader () = default;

	bool
	GLTFLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		/* Generate a resource prefix from the filename. */
		/* ⚠️⚠️ The PATH, not the stem. Keyed on the stem — as it was until 2026-09-18 — two
		 * different files named alike shared their whole resource namespace: the Khronos corpus
		 * ships `SunglassesKhronos/glTF-Binary/SunglassesKhronos.glb` and
		 * `SunglassesKhronos/glTF-Draco/SunglassesKhronos.gltf`, whose geometry AND texture
		 * encodings differ, under one prefix `glTF:SunglassesKhronos/`. The symptom was never a
		 * crash: the second load served the first's cached resources, and the conformance bench
		 * measured 98.16 % of pixels differing where a clean session reads 6.35 % — a WRONG
		 * MEASUREMENT, which is the worst failure mode. Same lesson as buildResourceKey() one
		 * level down: a NAME IS NOT AN IDENTITY, and here the file name is the name.
		 *
		 * `lexically_normal()` folds `.` and `..` away and `IO::toGenericU8String()` forces forward
		 * slashes, so one file yields one key on every platform. UTF-8, never `generic_string()`:
		 * on Windows that is the ANSI code page, which THROWS (an abort) on a character outside it.
		 *
		 * ⚠️ The key is derived from the path AS GIVEN, deliberately: a caller must be able to
		 * predict it without loading anything — projet-alpha's Fox does exactly that, to skip a
		 * reload it has already done. Canonicalising to an absolute path would make the key
		 * machine-dependent and unwritable by hand. The price is that two SPELLINGS of one file
		 * (a relative and an absolute path) are two keys: pass an asset's path consistently. */
		m_resourcePrefix = "glTF:" + Base::IO::toGenericU8String(std::filesystem::path{filepath}.replace_extension().lexically_normal()) + "/";

		/* stripRootMotion only ever acted on loadAnimationClipsOnly(), which this loader
		 * does not implement (glTF carries its clips inside the asset, so the split-animation
		 * workflow has no glTF equivalent). Say so instead of ignoring the option: a caller
		 * porting FBX code over would otherwise believe the root motion had been stripped. */
		if ( m_options.stripRootMotion )
		{
			Tracer::warning(ClassId, "LoaderOptions::stripRootMotion has no effect on glTF: the option only applies to loadAnimationClipsOnly(), which this loader does not implement.");
		}

		/* NOTE: Trust boundary, BEFORE fastgltf: a uri that is not UTF-8 once percent-decoded would make fastgltf abort
		 * the process on Windows (findNonUTF8URI()). An unreadable model is left to fastgltf, which reports it. */
		if ( const auto model = readModelJSON(filepath); model.has_value() )
		{
			if ( const auto badURI = findNonUTF8URI(model->root); badURI.has_value() )
			{
				TraceError{ClassId} << "The glTF '" << Base::IO::toU8String(filepath) << "' has a uri that is not UTF-8 once percent-decoded ('" << *badURI << "'), refused !";

				return false;
			}
		}

		/* NOTE: fastgltf opens the path itself: a FIFO blocked that open forever (2026-10-18). */
		if ( !Base::IO::isReadableRegularFile(filepath) )
		{
			TraceError{ClassId} << "The glTF '" << Base::IO::toU8String(filepath) << "' is not a readable file !";

			return false;
		}

		/* Parse the glTF/glb asset. */
		auto gltfFile = fastgltf::GltfDataBuffer::FromPath(filepath);

		if ( gltfFile.error() != fastgltf::Error::None )
		{
			TraceError{ClassId} << "Failed to open '" << Base::IO::toU8String(filepath) << "' !";

			return false;
		}

		const auto parentPath = filepath.parent_path();

		/* The extensions this loader declares. Named rather than inlined so the diagnostic below
		 * can say what we support against what the asset demands. */
		/* ---- Analysis: the parse, the validations, the geometry, the skins, the clips, the node tree, without any
		 * resource (fuzz_gltf covers it). ---- */
		const auto sourceName = Base::IO::toU8String(filepath);

		auto analysed = GLTF::parse(gltfFile.get(), parentPath, filepath.extension() == ".glb", sourceName);

		if ( !analysed.has_value() )
		{
			return false;
		}

		if ( !GLTF::analyse(*analysed, m_resourcePrefix, m_options, sourceName) )
		{
			return false;
		}

		const auto & asset = *analysed->gltf;

		/* ---- Images and materials, read from the validated asset (the images through its decoded buffer views). ---- */
		if ( !this->loadImages(*analysed, filepath, parentPath) )
		{
			Tracer::warning(ClassId, "Some images failed to load, continuing with defaults.");
		}

		if ( !this->loadMaterials(asset) )
		{
			Tracer::warning(ClassId, "Some materials failed to load, continuing with defaults.");
		}

		/* ---- Geometries and meshes. ---- */
		if ( !this->buildMeshes(*analysed, output) )
		{
			Tracer::error(ClassId, "Failed to load meshes !");

			return false;
		}

		/* ---- Skeletons, clips, and the skeletal data of every skinned mesh. ---- */
		m_skeletons.clear();
		m_animationClips.clear();
		m_nodeAnimationClips.clear();

		if ( !m_options.skipSkinning )
		{
			for ( auto & description : analysed->skins )
			{
				m_skeletons.push_back(m_resources.container< Animations::SkeletonResource >()
					->getOrCreateResourceSync(description.resourceName, [&description] (auto & resource) {
						return resource.load(std::move(description.skeleton));
					}));
			}

			const auto registerClips = [this] (std::vector< GLTF::Clip > & clips, std::vector< std::shared_ptr< Animations::AnimationClipResource > > & resources) {
				for ( auto & description : clips )
				{
					auto clipResource = m_resources.container< Animations::AnimationClipResource >()
						->getOrCreateResourceSync(description.resourceName, [&description] (auto & resource) {
							return resource.load(std::move(description.clip));
						});

					if ( clipResource != nullptr )
					{
						resources.push_back(std::move(clipResource));
					}
				}
			};

			registerClips(analysed->clips, m_animationClips);
			registerClips(analysed->nodeClips, m_nodeAnimationClips);

			for ( const auto & [meshIdx, skinIdx] : analysed->meshToSkinIndex )
			{
				if ( meshIdx >= m_meshes.size() || m_meshes[meshIdx] == nullptr )
				{
					continue;
				}

				if ( skinIdx >= m_skeletons.size() )
				{
					continue;
				}

				if ( auto * skeletalData = dynamic_cast< Renderable::SkeletalDataTrait * >(m_meshes[meshIdx].get()) )
				{
					/* NOTE: Each mesh gets its copy of the skin, with the root transform of its own node. */
					auto skin = analysed->skins[skinIdx].skin;

					if ( const auto rootTransform = analysed->meshSkinRootTransform.find(meshIdx); rootTransform != analysed->meshSkinRootTransform.end() )
					{
						skin.setRootTransform(rootTransform->second);
					}

					skeletalData->setSkeletalData(m_skeletons[skinIdx], std::move(skin), m_animationClips);
				}
			}
		}

		if ( GLTF::buffersFailed(*analysed) )
		{
			TraceError{ClassId} << "The glTF '" << Base::IO::toU8String(filepath) << "' has a meshopt-compressed buffer view that failed to decode, refused !";

			return false;
		}

		GLTF::releaseBuffers(*analysed);

		output.lights = std::move(analysed->scene.lights);
		output.cameras = std::move(analysed->scene.cameras);
		output.nodes = std::move(analysed->scene.nodes);
		output.rootNodeIndices = std::move(analysed->scene.rootNodeIndices);
		output.skinJointNodeIndices = std::move(analysed->scene.skinJointNodeIndices);
		output.skeletons = m_skeletons;
		output.animationClips = m_animationClips;
		output.nodeAnimationClips = m_nodeAnimationClips;

		TraceDebug{ClassId} <<
			Base::IO::toU8String(filepath.filename()) << " loaded: " <<
			output.nodes.size() << " nodes, " <<
			output.meshes.size() << " meshes, " <<
			output.skeletons.size() << " skeletons, " <<
			output.animationClips.size() << " skeletal clips, " <<
			output.nodeAnimationClips.size() << " node clips, " <<
			output.lights.size() << " lights, " <<
			output.cameras.size() << " cameras.";

		return true;
	}

	bool
	GLTFLoader::loadImages (GLTF::Asset & analysed, const std::filesystem::path & filepath, const std::filesystem::path & basePath) noexcept
	{
		const auto & asset = *analysed.gltf;

		/* Where the buffers lie in files: an image in a buffer view gets its byte range as encoded source. */
		const auto bufferLocations = locateBuffers(filepath, basePath);

		m_images.resize(asset.images.size());
		m_compressedImages.resize(asset.images.size());

		/* A KTX2 payload can only stay block-compressed if the device samples block-compressed
		 * formats. Ask once, the answer holds for the whole asset. */
		const auto blockCompressionSupported =
			m_resources.graphicsRenderer().device()->physicalDevice()->featuresVK10().textureCompressionBC == VK_TRUE;

		const Graphics::KTX2Decoder::Options KTXOptions{
			.maxDimension = CompressedImageResource::maxDimension(m_resources.primaryServices().settings())
		};

		size_t compressedCount = 0;
		bool allSuccess = true;

		for ( size_t imageIndex = 0; imageIndex < asset.images.size(); ++imageIndex )
		{
			const auto & glTFImage = asset.images[imageIndex];

			/* ⚠️ Keyed on the INDEX, not on the name: TransmissionRoughnessTest carries two
			 * different images both named 'RoughnessGrid'. See buildResourceKey(). */
			const auto name = buildResourceKey(m_resourcePrefix, "Image/", {glTFImage.name.data(), glTFImage.name.size()}, imageIndex);

			/* Builds the right kind of resource from an encoded blob : a KTX2 container
			 * (KHR_texture_basisu) goes to a CompressedImageResource and stays block-compressed all
			 * the way to the GPU, anything else is decoded to pixels as before.
			 *
			 * NOTE: The blob is copied into a shared_ptr because the creation function is enqueued
			 * on the thread pool and outlives this scope. That also means the KTX2 transcode of the
			 * whole asset runs in parallel across the workers. */
			const auto buildFromBytes = [&] (const std::byte * data, size_t size, fastgltf::MimeType mime, std::optional< EncodedSource > source) -> bool {
				auto blob = std::make_shared< std::vector< std::byte > >(data, data + size);

				if ( Graphics::KTX2Decoder::isKTX2(*blob) )
				{
					if ( blockCompressionSupported )
					{
						auto compressed = m_resources.container< CompressedImageResource >()
							->getOrCreateResource(name, [blob] (auto & resource) {
								return resource.load(*blob);
							});

						if ( compressed == nullptr )
						{
							return false;
						}

						m_compressedImages[imageIndex] = std::move(compressed);
						++compressedCount;

						return true;
					}

					/* No block compression on this device : transcode to plain pixels so the asset
					 * still renders. Correct, but it throws away the entire point of the KTX2. */
					auto image = m_resources.container< ImageResource >()
						->getOrCreateResource(name, [blob, KTXOptions, name] (auto & resource) {
							Pixmap< uint8_t > pixmap;

							if ( !Graphics::KTX2Decoder::decodeToPixmap(*blob, KTXOptions, name, pixmap) )
							{
								return false;
							}

							return resource.load(std::move(pixmap));
						});

					if ( image == nullptr )
					{
						return false;
					}

					m_images[imageIndex] = std::move(image);

					return true;
				}

				/* Ordinary encoded image (PNG, JPEG). */
				auto image = m_resources.container< ImageResource >()
					->getOrCreateResource(name, [blob, format = mimeToPixmapFormat(mime), encodedSource = std::move(source)] (auto & resource) {
						Pixmap< uint8_t > pixmap;

						constexpr ReadOptions options{
							.targetChannelMode = TargetChannelMode::RGBA
						};

						if ( !StreamIO::read(*blob, format, pixmap, options) )
						{
							return false;
						}

						/* NOTE: Where the encoded bytes lie, so a released CPU copy can come back. */
						if ( encodedSource.has_value() )
						{
							resource.setEncodedSource(*encodedSource);
						}

						return resource.load(std::move(pixmap));
					});

				if ( image == nullptr )
				{
					return false;
				}

				m_images[imageIndex] = std::move(image);

				return true;
			};

			const auto loaded = std::visit(fastgltf::visitor{
				/* File-based image (external .gltf). */
				[&] (const fastgltf::sources::URI & uri) -> bool {
					/* NOTE: Confined to the asset's directory (the analysis refused any other uri; owner decision 2026-10-18). */
					const auto confined = Base::IO::confinedPath(basePath, Base::IO::u8path(std::string{uri.uri.path()}));

					if ( !confined.has_value() )
					{
						return false;
					}

					const std::filesystem::path & fullPath = *confined;

					/* A sidecar .ktx2 is read whole and handed to the KTX2 path ; every other
					 * format keeps going through PixelFactory, which picks its decoder from the
					 * file extension. */
					if ( fullPath.extension() == ".ktx2" )
					{
						std::vector< std::byte > content;

						if ( !Base::IO::fileGetContents(fullPath, content) )
						{
							TraceError{ClassId} << "Unable to read the KTX2 file '" << Base::IO::toU8String(fullPath) << "' !";

							return false;
						}

						return buildFromBytes(content.data(), content.size(), fastgltf::MimeType::KTX2, std::nullopt);
					}

					auto image = m_resources.container< ImageResource >()
						->getOrCreateResource(name, [fullPath] (auto & imageResource) {
							Pixmap< uint8_t > pixmap;

							constexpr ReadOptions options{
								.targetChannelMode = TargetChannelMode::RGBA
							};

							if ( !FileIO::read(fullPath, pixmap, options) )
							{
								return false;
							}

							/* NOTE: The file itself: a released CPU copy comes back from it. */
							imageResource.setEncodedSource(EncodedSource::file(fullPath));

							return imageResource.load(std::move(pixmap));
						});

					if ( image == nullptr )
					{
						return false;
					}

					m_images[imageIndex] = std::move(image);

					return true;
				},

				/* Embedded image (in .glb buffer view). */
				[&] (const fastgltf::sources::BufferView & bufferView) -> bool {
					/* NOTE: Through the meshopt cache, not the raw buffer : an image view is
					 * normally uncompressed, but nothing in the spec forbids compressing it, and
					 * the cache is transparent for the uncompressed case. */
					const auto bytes = GLTF::bufferView(analysed, bufferView.bufferViewIndex);

					if ( bytes.empty() )
					{
						return false;
					}

					/* NOTE: The byte range in the file, unless the view is meshopt-compressed or its buffer is not a
					 * file (a data: URI). */
					std::optional< EncodedSource > source;

					if ( bufferView.bufferViewIndex < asset.bufferViews.size() )
					{
						const auto & view = asset.bufferViews[bufferView.bufferViewIndex];

						if ( view.meshoptCompression == nullptr && view.bufferIndex < bufferLocations.size() && bufferLocations[view.bufferIndex].has_value() && view.byteLength == bytes.size() )
						{
							const auto & location = *bufferLocations[view.bufferIndex];

							source = EncodedSource::fileRange(location.filepath, location.offset + view.byteOffset, view.byteLength, mimeToPixmapFormat(bufferView.mimeType));
						}
					}

					return buildFromBytes(bytes.data(), bytes.size(), bufferView.mimeType, std::move(source));
				},

				/* Inline base64 data (Array source). */
				[&] (const fastgltf::sources::Array & array) -> bool {
					/* NOTE: A base64 data: URI has no byte range in a file: the image stays resident. */
					return buildFromBytes(array.bytes.data(), array.bytes.size_bytes(), array.mimeType, std::nullopt);
				},

				/* Fallback for unhandled source types. */
				[&] (const auto &) -> bool {
					return false;
				}
			}, glTFImage.data);

			if ( !loaded )
			{
				TraceWarning{ClassId} << "Image " << imageIndex << " ('" << name << "') failed to load, using default.";

				m_images[imageIndex] = m_resources.container< ImageResource >()->getDefaultResource();
				allSuccess = false;
			}
		}

		if ( compressedCount > 0 )
		{
			TraceInfo{ClassId} <<
				compressedCount << " of " << asset.images.size() <<
				" image(s) kept block-compressed from the KTX2 payload (no decode, no CPU compression pass).";
		}

		return allSuccess;
	}

	bool
	GLTFLoader::loadMaterials (const fastgltf::Asset & asset) noexcept
	{
		m_materials.resize(asset.materials.size());
		m_materialsVertexColor.resize(asset.materials.size());
		m_materialsPrimarySetOnly.resize(asset.materials.size());
		m_materialSamplesSecondarySet.assign(asset.materials.size(), 0);
		m_textures.resize(asset.textures.size());

		/* Which materials does a COLOR_0 primitive actually use?
		 *
		 * glTF's `COLOR_0` MULTIPLIES the base colour, and the engine implements that by a material
		 * flag whose codegen declares a vertex INPUT ATTRIBUTE. So the flag cannot be set on a
		 * material shared with a colourless primitive: that primitive's geometry carries no such
		 * attribute and its shader would read one. ⚠️ This is the majority case, not an edge — on
		 * the reference Sponza, 17 of the 22 colour-using materials are also used by colourless
		 * primitives. A separate `…-vc` resource is therefore created for those, and only those:
		 * an asset without COLOR_0 pays nothing. */
		std::unordered_set< size_t > materialsUsedWithVertexColor;

		for ( const auto & glTFMesh : asset.meshes )
		{
			for ( const auto & primitive : glTFMesh.primitives )
			{
				if ( primitive.findAttribute("COLOR_0") != primitive.attributes.end() && primitive.materialIndex.has_value() )
				{
					materialsUsedWithVertexColor.insert(primitive.materialIndex.value());
				}
			}
		}

		/* Which materials sample TEXCOORD_1, and which of them a primitive WITHOUT it uses (an invalid asset: they get
		 * the `…-uv0` variant, every map on TEXCOORD_0). */
		std::unordered_set< size_t > materialsNeedingPrimarySetOnly;

		for ( size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex )
		{
			const auto & material = asset.materials[materialIndex];

			m_materialSamplesSecondarySet[materialIndex] = materialSamplesSecondarySet(material) ? 1 : 0;

			/* The maps bound without their set (no UV transform path either): said, not silently mis-sampled. */
			const bool unsupportedSet =
				( material.anisotropy != nullptr && textureCoordinateSet(material.anisotropy->anisotropyTexture) != 0 ) ||
				( material.iridescence != nullptr && ( textureCoordinateSet(material.iridescence->iridescenceTexture) != 0 || textureCoordinateSet(material.iridescence->iridescenceThicknessTexture) != 0 ) ) ||
				( material.volume != nullptr && textureCoordinateSet(material.volume->thicknessTexture) != 0 );

			if ( unsupportedSet )
			{
				TraceWarning{ClassId} << "Material '" << material.name << "': an anisotropy, iridescence or volume map samples a texture coordinate set other than 0, which those maps do not support; they sample set 0.";
			}
		}

		for ( const auto & glTFMesh : asset.meshes )
		{
			for ( const auto & primitive : glTFMesh.primitives )
			{
				if ( !primitive.materialIndex.has_value() || primitive.materialIndex.value() >= asset.materials.size() )
				{
					continue;
				}

				const auto materialIndex = primitive.materialIndex.value();

				if ( m_materialSamplesSecondarySet[materialIndex] != 0 && primitive.findAttribute("TEXCOORD_1") == primitive.attributes.end() )
				{
					if ( materialsNeedingPrimarySetOnly.insert(materialIndex).second )
					{
						TraceWarning{ClassId} << "Material " << materialIndex << " samples TEXCOORD_1, which a primitive of mesh '" << glTFMesh.name << "' lacks (invalid glTF): that primitive samples TEXCOORD_0.";
					}
				}
			}
		}

		bool allSuccess = true;

		for ( size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex )
		{
			const auto & glTFMaterial = asset.materials[materialIndex];

			/* ⚠️ Keyed on the INDEX, not on the name: TransmissionTest carries three genuinely
			 * different materials all named 'BlueTransWithMask'. See buildResourceKey(). */
			const auto name = buildResourceKey(m_resourcePrefix, "Material/", {glTFMaterial.name.data(), glTFMaterial.name.size()}, materialIndex);

			/* Resolve a glTF texture index to a Texture2D resource, creating it on demand. */
			const auto resolveTexture = [&] (size_t textureIndex, const TextureResource::Usage & usage = {}) -> std::shared_ptr< TextureResource::Texture2D > {
				if ( textureIndex >= asset.textures.size() )
				{
					return nullptr;
				}

				/* Return cached texture if already created.
				 * ⚠️ THREE slots per glTF texture, selected by the usage (linear data, sRGB colour, normal map). The
				 * colour space and the normal map kind are baked into the resource at creation and come from the USAGE,
				 * not from the asset: the same image legitimately serves as an sRGB albedo for one material and as a
				 * linear data map for another. Caching on the texture index alone let whichever usage resolved FIRST
				 * impose its identity on the other. */
				const auto usageSlot = TextureResource::loaderUsageSlot(usage);

				/* NOTE: The slot is < LoaderUsageCount by construction (loaderUsageSlot()). */
				auto & usageEntry = *std::next(m_textures[textureIndex].begin(), static_cast< std::ptrdiff_t >(usageSlot));

				if ( usageEntry != nullptr )
				{
					return usageEntry;
				}

				const auto & glTFTexture = asset.textures[textureIndex];

				/* NOTE: KHR_texture_basisu and EXT_texture_webp each hang the image off their OWN
				 * index, and a texture using either has NO plain imageIndex at all. Reading only
				 * imageIndex does not degrade gracefully on such an asset : every single material
				 * comes out untextured, with no error.
				 *
				 * ⚠️ The two are ALTERNATIVES on the same texture, so all three legs must be tried.
				 * Stopping at the first one a reader happens to know is the whole trap: it leaves
				 * the other kind silently untextured. */
				auto sourceIndex = glTFTexture.imageIndex;

				if ( !sourceIndex.has_value() )
				{
					sourceIndex = glTFTexture.basisuImageIndex;
				}

				if ( !sourceIndex.has_value() )
				{
					sourceIndex = glTFTexture.webpImageIndex;
				}

				if ( !sourceIndex.has_value() )
				{
					return nullptr;
				}

				const auto imageIndex = sourceIndex.value();

				if ( imageIndex >= m_images.size() )
				{
					return nullptr;
				}

				const auto image = m_images[imageIndex];
				const auto compressedImage = m_compressedImages[imageIndex];

				if ( image == nullptr && compressedImage == nullptr )
				{
					return nullptr;
				}

				/* The sampler carries the asset's addressing intent (wrapS / wrapT). Ignoring it
				 * does not fail loudly — the texture simply TILES where the asset asked for a
				 * clamp, which silently corrupts anything authored with a border (measured on
				 * TextureTransformTest, whose quads repeat instead of showing their grey border
				 * once). glTF's own default, when a texture declares no sampler, is repeat. */
				auto wrapU = TextureResource::WrapMode::Repeat;
				auto wrapV = TextureResource::WrapMode::Repeat;

				if ( glTFTexture.samplerIndex.has_value() && *glTFTexture.samplerIndex < asset.samplers.size() )
				{
					const auto & glTFSampler = asset.samplers[*glTFTexture.samplerIndex];

					wrapU = toWrapMode(glTFSampler.wrapS);
					wrapV = toWrapMode(glTFSampler.wrapT);
				}

				/* Build texture resource name.
				 * ⚠️ The index makes the key unique per glTF texture — a name is not an identity
				 * — which also settles the addressing: each glTF texture owns exactly one
				 * sampler, so its wrap modes can no longer leak into a homonym's resource. The
				 * colour space, on the other hand, is NOT a property of the glTF texture but of
				 * the usage, so it must stay part of the key: one glTF texture legitimately
				 * yields two engine resources. Same convention as the USD loader. */
				std::string texName = buildResourceKey(m_resourcePrefix, "Texture/", {glTFTexture.name.data(), glTFTexture.name.size()}, textureIndex);
				texName += TextureResource::loaderUsageSuffix(usage);

				/* NOTE: Created SYNCHRONOUSLY (the function only links the source): the colour space is set when this returns,
				 * and the material setter checks it — an asynchronous creation would set it later, on a worker (texture identity,
				 * docs/caution-points.md). */
				auto texture = m_resources.container< TextureResource::Texture2D >()
					->getOrCreateResourceSync(texName, [image, compressedImage, usage, wrapU, wrapV] (auto & textureResource) {
						/* Set sRGB BEFORE load() so the flag is in place when
						 * onDependenciesLoaded() fires and creates the VkImage. The colour space is
						 * decided here, from the usage, for both kinds of source alike — and so is
						 * the normal map kind, which makes a decoded image BC5 (graphics § 39, D8). */
						if ( !textureResource.enableSRGB(usage.sRGB) || !textureResource.enableNormalMap(usage.normalMap) )
						{
							return false;
						}

						/* Same contract for the addressing: the sampler is built once, during
						 * creation on hardware, and never revisited. */
						textureResource.setWrapModes(wrapU, wrapV);

						if ( compressedImage != nullptr )
						{
							return textureResource.load(compressedImage);
						}

						return textureResource.load(image);
					});

				usageEntry = texture;

				return texture;
			};

			const auto & PBRData = glTFMaterial.pbrData;

			/* KHR_texture_transform: per-texture-info UV scale, rotation and offset (the tiling of
			 * tire treads, brake discs, car paint flakes...). Ignoring it does not fail — the
			 * texture renders STRETCHED over the whole UV range (measured on CarConcept).
			 * The rotation was parsed and DROPPED with a warning until 2026-08-28; it is now
			 * carried through to the material UBO as (cos, sin) — see
			 * `StandardResource::transformedTexCoords()` for the composition order and the sign
			 * convention, both of which the extension specifies and both of which are easy to
			 * invert into a plausible-looking wrong result.
			 * The texture coordinate SET (TextureInfo.texCoord and the transform's override) rides the same struct
			 * since 2026-10-03 (UVTransform, applyTextureCoordinates()). */
			const auto readUVTransform = [&glTFMaterial] (const auto & textureInfoOpt) -> UVTransform {
				UVTransform out;

				const auto set = textureCoordinateSet(textureInfoOpt);

				if ( set == 1 )
				{
					out.set = 1;
					out.present = true;
				}
				else if ( set > 1 )
				{
					TraceWarning{ClassId} << "Material '" << glTFMaterial.name << "': texture coordinate set " << set << " is not supported (sets 0 and 1), set 0 is used.";
				}

				if ( !textureInfoOpt.has_value() || textureInfoOpt->transform == nullptr )
				{
					return out;
				}

				const auto & transform = *textureInfoOpt->transform;
				out.scale = {transform.uvScale.x(), transform.uvScale.y()};
				out.offset = {transform.uvOffset.x(), transform.uvOffset.y()};
				out.rotation = static_cast< float >(transform.rotation);
				out.present = true;

				return out;
			};

			const auto albedoUVTransform = readUVTransform(PBRData.baseColorTexture);
			const auto metallicRoughnessUVTransform = readUVTransform(PBRData.metallicRoughnessTexture);
			const auto normalUVTransform = readUVTransform(glTFMaterial.normalTexture);
			const auto aoUVTransform = readUVTransform(glTFMaterial.occlusionTexture);
			const auto emissiveUVTransform = readUVTransform(glTFMaterial.emissiveTexture);

			/* Albedo (sRGB: perceptual color data). */
			auto albedoTex = PBRData.baseColorTexture.has_value()
				? resolveTexture(PBRData.baseColorTexture->textureIndex, {.sRGB = true}) : nullptr;

			const auto & bc = PBRData.baseColorFactor;
			Color< float > albedoColor{
				static_cast< float >(bc[0]),
				static_cast< float >(bc[1]),
				static_cast< float >(bc[2]),
				static_cast< float >(bc[3])
			};

			/* L2d: the data maps go through the shared texture packer (graphics § 39) BEFORE any of their textures is
			 * requested: the roughness (G) and metalness (B) of the metallic-roughness image and the occlusion (R) land in
			 * one packed Data texture, read through one fetch (D10). Only decoded images (a KTX2 image is BC blocks
			 * already), with repeat addressing (the packed texture repeats), read at the same UVs (D11). */
			const auto packableImage = [&] (const auto & textureInfoOpt) -> std::shared_ptr< ImageResource > {
				if ( !textureInfoOpt.has_value() || textureInfoOpt->textureIndex >= asset.textures.size() )
				{
					return nullptr;
				}

				const auto & glTFTexture = asset.textures[textureInfoOpt->textureIndex];

				if ( !glTFTexture.imageIndex.has_value() || *glTFTexture.imageIndex >= m_images.size() || m_compressedImages[*glTFTexture.imageIndex] != nullptr )
				{
					return nullptr;
				}

				if ( glTFTexture.samplerIndex.has_value() && *glTFTexture.samplerIndex < asset.samplers.size() )
				{
					const auto & glTFSampler = asset.samplers[*glTFTexture.samplerIndex];

					if ( toWrapMode(glTFSampler.wrapS) != TextureResource::WrapMode::Repeat || toWrapMode(glTFSampler.wrapT) != TextureResource::WrapMode::Repeat )
					{
						return nullptr;
					}
				}

				return m_images[*glTFTexture.imageIndex];
			};

			const auto samplingKey = [] (const UVTransform & transform) {
				std::stringstream key;
				key << transform.set << '/' << transform.scale[X] << ',' << transform.scale[Y] << '/' << transform.offset[X] << ',' << transform.offset[Y] << '/' << transform.rotation;

				return key.str();
			};

			const auto metallicRoughnessImage = packableImage(PBRData.metallicRoughnessTexture);
			const auto occlusionImage = packableImage(glTFMaterial.occlusionTexture);
			const std::array< PackSource, 3 > packSources{{
				{.image = metallicRoughnessImage, .samplingKey = samplingKey(metallicRoughnessUVTransform), .role = ComponentType::Roughness, .channel = Base::PixelFactory::Channel::Green},
				{.image = metallicRoughnessImage, .samplingKey = samplingKey(metallicRoughnessUVTransform), .role = ComponentType::Metalness, .channel = Base::PixelFactory::Channel::Blue},
				{.image = occlusionImage, .samplingKey = samplingKey(aoUVTransform), .role = ComponentType::AmbientOcclusion, .channel = Base::PixelFactory::Channel::Red}
			}};

			std::shared_ptr< TextureResource::Texture2D > packedData;
			auto roughnessChannel = Base::PixelFactory::Channel::Green;
			auto metalnessChannel = Base::PixelFactory::Channel::Blue;
			auto aoChannel = Base::PixelFactory::Channel::Red;
			bool metallicRoughnessPacked = false;
			bool occlusionPacked = false;

			for ( const auto & packedRole : TexturePacker{m_resources}.pack(packSources, glTFMaterial.name) )
			{
				packedData = packedRole.texture;

				switch ( packedRole.role )
				{
					case ComponentType::Roughness :
						roughnessChannel = packedRole.channel;
						metallicRoughnessPacked = true;
						break;

					case ComponentType::Metalness :
						metalnessChannel = packedRole.channel;
						metallicRoughnessPacked = true;
						break;

					case ComponentType::AmbientOcclusion :
						aoChannel = packedRole.channel;
						occlusionPacked = true;
						break;

					default :
						break;
				}
			}

			/* Metallic-Roughness. */
			auto metallicRoughnessTex = metallicRoughnessPacked ? packedData : nullptr;

			if ( !metallicRoughnessPacked && PBRData.metallicRoughnessTexture.has_value() )
			{
				metallicRoughnessTex = resolveTexture(PBRData.metallicRoughnessTexture->textureIndex);
			}

			const auto roughnessFactor = static_cast< float >(PBRData.roughnessFactor);
			const auto metallicFactor = static_cast< float >(PBRData.metallicFactor);

			/* Normal. */
			auto normalTex = glTFMaterial.normalTexture.has_value()
				? resolveTexture(glTFMaterial.normalTexture->textureIndex, {.normalMap = true}) : nullptr;

			const auto normalScale = glTFMaterial.normalTexture.has_value()
				? static_cast< float >(glTFMaterial.normalTexture->scale) : 0.0F;

			/* Ambient occlusion. */
			auto aoTex = occlusionPacked ? packedData : nullptr;

			if ( !occlusionPacked && glTFMaterial.occlusionTexture.has_value() )
			{
				aoTex = resolveTexture(glTFMaterial.occlusionTexture->textureIndex);
			}

			const auto aoStrength = glTFMaterial.occlusionTexture.has_value()
				? static_cast< float >(glTFMaterial.occlusionTexture->strength) : 0.0F;

			/* Emissive (sRGB: perceptual color data). The caller may drop a material's emission
			 * (LoaderOptions::nonEmissiveMaterialNames): its map is then not even loaded. */
			const bool isEmissionDropped = m_options.nonEmissiveMaterialNames.contains(std::string{glTFMaterial.name.data(), glTFMaterial.name.size()});
			auto emissiveTex = !isEmissionDropped && glTFMaterial.emissiveTexture.has_value()
				? resolveTexture(glTFMaterial.emissiveTexture->textureIndex, {.sRGB = true}) : nullptr;

			/* ⚠️⚠️ EXPERIMENT (Aug 2026) — PHOTOMETRIC ANCHOR FOR THE EMISSIVE.
			 *
			 * glTF emissive is UNITLESS: `emissiveFactor * emissiveTexture` in [0,1], with
			 * KHR_materials_emissive_strength as the optional HDR multiplier. The engine's colour
			 * buffer holds ABSOLUTE LUMINANCE, and the shader adds the emissive straight into it —
			 * so an asset without the extension contributes ~1 nit. Against a daylight sky metered
			 * in the thousands of nits, that is 3 to 4 stops below anything visible: the HUD of
			 * DamagedHelmet was measured invisible for exactly this reason.
			 *
			 * A reference luminance is therefore REQUIRED to bridge the unitless asset to the
			 * photometric renderer. 2000 nits matched the anchor of the WAD materializer until
			 * 2026-10-15 (8000 since: it compensates the sRGB decoding of the map's texels). ⚠️ The VALUE and the PLACE (loader vs material vs shader) are an
			 * engine-wide decision pending owner arbitration — this constant is the experiment that
			 * demonstrates the mechanism, not a settled convention. */
			constexpr auto EmissiveLuminanceAnchor{2000.0F};

			const auto emissiveStrength = static_cast< float >(glTFMaterial.emissiveStrength) * EmissiveLuminanceAnchor;

			Color< float > emissiveColor{
				static_cast< float >(glTFMaterial.emissiveFactor[0]),
				static_cast< float >(glTFMaterial.emissiveFactor[1]),
				static_cast< float >(glTFMaterial.emissiveFactor[2]),
				1.0F
			};

			const bool hasEmissiveColor = !isEmissionDropped && (glTFMaterial.emissiveFactor[0] > 0.0F
				|| glTFMaterial.emissiveFactor[1] > 0.0F
				|| glTFMaterial.emissiveFactor[2] > 0.0F);

			auto sheenColorTex = ( glTFMaterial.sheen != nullptr && glTFMaterial.sheen->sheenColorTexture.has_value() )
				? resolveTexture(glTFMaterial.sheen->sheenColorTexture->textureIndex, {.sRGB = true}) : nullptr;

			auto sheenRoughnessTex = ( glTFMaterial.sheen != nullptr && glTFMaterial.sheen->sheenRoughnessTexture.has_value() )
				? resolveTexture(glTFMaterial.sheen->sheenRoughnessTexture->textureIndex) : nullptr;

			/* ⚠️ SheenCloth tiles its 256x256 maps THIRTY times in U and V through
			 * KHR_texture_transform, and that tiling IS the weave of the fabric. Served since the
			 * transform table became indexed (2026-09-14); before it these two maps fell through to
			 * plain coordinates and the sheen read at 1/30 of the authored frequency. */
			const auto sheenColorUVTransform = glTFMaterial.sheen != nullptr ? readUVTransform(glTFMaterial.sheen->sheenColorTexture) : UVTransform{};
			const auto sheenRoughnessUVTransform = glTFMaterial.sheen != nullptr ? readUVTransform(glTFMaterial.sheen->sheenRoughnessTexture) : UVTransform{};

			/* Clear coat (KHR_materials_clearcoat), factors AND the three maps.
			 *
			 * ⚠️ All three are DATA, never sRGB: the factor map carries a coverage in RED, the
			 * roughness map a roughness in **GREEN** (not red like every other scalar map in this
			 * engine — the extension says so), and the normal map a tangent-space normal. Each map
			 * MULTIPLIES its scalar, which is why the factors are read even when a map is present:
			 * a `clearcoatFactor` of 0 means no coat at all, map or no map. */
			float clearcoatFactor = 0.0F;
			float clearcoatRoughness = 0.0F;
			float clearcoatNormalScale = 1.0F;

			if ( glTFMaterial.clearcoat != nullptr && glTFMaterial.clearcoat->clearcoatFactor > 0.0F )
			{
				clearcoatFactor = static_cast< float >(glTFMaterial.clearcoat->clearcoatFactor);
				clearcoatRoughness = static_cast< float >(glTFMaterial.clearcoat->clearcoatRoughnessFactor);
			}

			auto clearcoatTex = ( glTFMaterial.clearcoat != nullptr && glTFMaterial.clearcoat->clearcoatTexture.has_value() )
				? resolveTexture(glTFMaterial.clearcoat->clearcoatTexture->textureIndex) : nullptr;

			auto clearcoatRoughnessTex = ( glTFMaterial.clearcoat != nullptr && glTFMaterial.clearcoat->clearcoatRoughnessTexture.has_value() )
				? resolveTexture(glTFMaterial.clearcoat->clearcoatRoughnessTexture->textureIndex) : nullptr;

			std::shared_ptr< TextureResource::Texture2D > clearcoatNormalTex{};

			if ( glTFMaterial.clearcoat != nullptr && glTFMaterial.clearcoat->clearcoatNormalTexture.has_value() )
			{
				clearcoatNormalTex = resolveTexture(glTFMaterial.clearcoat->clearcoatNormalTexture->textureIndex, {.normalMap = true});
				clearcoatNormalScale = static_cast< float >(glTFMaterial.clearcoat->clearcoatNormalTexture->scale);
			}

			const auto clearcoatUVTransform = glTFMaterial.clearcoat != nullptr ? readUVTransform(glTFMaterial.clearcoat->clearcoatTexture) : UVTransform{};
			const auto clearcoatRoughnessUVTransform = glTFMaterial.clearcoat != nullptr ? readUVTransform(glTFMaterial.clearcoat->clearcoatRoughnessTexture) : UVTransform{};
			const auto clearcoatNormalUVTransform = glTFMaterial.clearcoat != nullptr ? readUVTransform(glTFMaterial.clearcoat->clearcoatNormalTexture) : UVTransform{};

			/* Sheen (KHR_materials_sheen), factors AND both maps.
			 *
			 * ⚠️ The two maps do NOT share a colour space: `sheenColorTexture` carries a perceptual
			 * colour in RGB and is **sRGB**, `sheenRoughnessTexture` a roughness in **ALPHA** and is
			 * linear DATA. An asset may point both at the SAME image — SheenCloth does — and the
			 * texture cache's two colour-space slots per index is exactly what makes that work.
			 * Each map MULTIPLIES its factor, so the factors are read either way. */
			Color< float > sheenColor{};
			float sheenRoughness = 0.0F;

			if ( glTFMaterial.sheen != nullptr )
			{
				const auto & sc = glTFMaterial.sheen->sheenColorFactor;

				if ( sc[0] > 0.0F || sc[1] > 0.0F || sc[2] > 0.0F )
				{
					sheenColor = Color< float >{
						static_cast< float >(sc[0]),
						static_cast< float >(sc[1]),
						static_cast< float >(sc[2]),
						1.0F
					};

					sheenRoughness = static_cast< float >(glTFMaterial.sheen->sheenRoughnessFactor);
				}
			}

			/* Transmission (KHR_materials_transmission). */
			float transmissionFactor = 0.0F;

			if ( glTFMaterial.transmission != nullptr && glTFMaterial.transmission->transmissionFactor > 0.0F )
			{
				transmissionFactor = static_cast< float >(glTFMaterial.transmission->transmissionFactor);
			}

			/* Volume (KHR_materials_volume) — the ABSORPTION inside a transmissive surface, which is
			 * what turns clear glass into coloured or thick glass. Beer-Lambert, and the shader
			 * already implements the extension's formula verbatim
			 * (`LightGenerator.PBR.cpp`): `exp(log(attenuationColor) / attenuationDistance * thickness)`.
			 * Only this read was missing.
			 *
			 * ⚠️⚠️ THE SPEC'S DEFAULTS ARE NOT THE ENGINE'S, and that matters more than the read
			 * itself. glTF says `thicknessFactor` **0** (0 means THIN-WALLED: no volume at all) and
			 * `attenuationDistance` **+INFINITY** (no absorption), while `StandardResource` defaults
			 * them to 1.0 and 1.0 m. Those engine defaults are harmless only because
			 * `attenuationColor` also defaults to white — `log(1)` is 0, so the product is zero and
			 * absorption is the identity whatever the distance. The moment a colour is set without a
			 * distance, the engine would invent an absorption over 1 m that the asset never asked
			 * for. So the SPEC defaults are stated here and passed unconditionally.
			 *
			 * ⚠️ Infinity travels through correctly and deliberately: `setAttenuationDistance()`
			 * clamps with `max(0.0001, value)`, which leaves it intact, and the shader's
			 * `log(colour) / inf` is 0, i.e. no absorption. `attenuationColor` is clamped away from
			 * zero in the shader so `log()` never returns -inf, so no NaN can appear. */
			float volumeThicknessFactor = 0.0F;
			auto volumeAttenuationDistance = std::numeric_limits< float >::infinity();
			Color< float > volumeAttenuationColor{1.0F, 1.0F, 1.0F, 1.0F};
			std::shared_ptr< TextureResource::Texture2D > volumeThicknessTex;

			if ( glTFMaterial.volume != nullptr )
			{
				volumeThicknessFactor = static_cast< float >(glTFMaterial.volume->thicknessFactor);
				volumeAttenuationDistance = static_cast< float >(glTFMaterial.volume->attenuationDistance);

				const auto & ac = glTFMaterial.volume->attenuationColor;

				volumeAttenuationColor = Color< float >{
					static_cast< float >(ac[0]),
					static_cast< float >(ac[1]),
					static_cast< float >(ac[2]),
					1.0F
				};

				/* The thickness MAP (G channel) MULTIPLIES the factor, giving a per-pixel volume
				 * depth: thick at a moulded base, thin at a rim.
				 * ⚠️ It is DATA, never sRGB. ⚠️ `SubsurfaceThickness` is a DIFFERENT quantity and is
				 * NOT reused here. ⚠️ It feeds BOTH consumers of the thickness — Beer's law and the
				 * refraction ray's length — through StandardResource::volumeThicknessExpression().
				 */
				if ( glTFMaterial.volume->thicknessTexture.has_value() )
				{
					volumeThicknessTex = resolveTexture(glTFMaterial.volume->thicknessTexture->textureIndex);
				}
			}

			/* Iridescence (KHR_materials_iridescence) — a thin-film interference layer.
			 *
			 * ⚠️ Only `iridescenceFactor` was read until 2026-08-28, and the other three values were
			 * left at the material's defaults — which is why both Khronos iridescence grids failed:
			 * the IOR and the film thickness are the two axes those models SWEEP. The shader has
			 * carried a complete `evalIridescence()` thin-film function all along, and
			 * `setIridescenceComponent()` already took all four values; the loader simply passed one.
			 * ⚠️ Units agree on both sides — the thicknesses are NANOMETRES in glTF and in the
			 * material UBO — so nothing is converted here. Do not "helpfully" scale them. */
			float iridescenceFactor = 0.0F;
			auto iridescenceIOR = 1.3F;
			auto iridescenceThicknessMin = 100.0F;
			auto iridescenceThicknessMax = 400.0F;
			std::shared_ptr< TextureResource::Texture2D > iridescenceTex;
			std::shared_ptr< TextureResource::Texture2D > iridescenceThicknessTex;

			if ( glTFMaterial.iridescence != nullptr )
			{
				iridescenceFactor = static_cast< float >(glTFMaterial.iridescence->iridescenceFactor);
				iridescenceIOR = static_cast< float >(glTFMaterial.iridescence->iridescenceIor);
				iridescenceThicknessMin = static_cast< float >(glTFMaterial.iridescence->iridescenceThicknessMinimum);
				iridescenceThicknessMax = static_cast< float >(glTFMaterial.iridescence->iridescenceThicknessMaximum);

				/* The per-pixel iridescence FACTOR map is supported (ComponentType::Iridescence). */
				if ( glTFMaterial.iridescence->iridescenceTexture.has_value() )
				{
					iridescenceTex = resolveTexture(glTFMaterial.iridescence->iridescenceTexture->textureIndex);
				}

				/* The per-pixel THICKNESS map is a SECOND, independent texture
				 * (ComponentType::IridescenceThickness). Its G channel places each texel between
				 * the two thicknesses: mix(min, max, texel.g).
				 * ⚠️ It is DATA, never sRGB — resolveTexture() is asked for the linear slot, as the
				 * roughness/metalness maps are. A material may declare it WITHOUT the factor map,
				 * or the factor map without it; per spec its absence means the thickness is the
				 * MAXIMUM, not the midpoint. */
				if ( glTFMaterial.iridescence->iridescenceThicknessTexture.has_value() )
				{
					iridescenceThicknessTex = resolveTexture(glTFMaterial.iridescence->iridescenceThicknessTexture->textureIndex);
				}
			}

			/* Anisotropy (KHR_materials_anisotropy) — a directional specular lobe.
			 *
			 * ⚠️⚠️ The extension was declared on the parser and NEVER READ, while the shader already
			 * carries an anisotropic GGX distribution, an anisotropic Smith-GGX visibility term and
			 * the tangent-frame construction. Only this read was missing.
			 * ⚠️⚠️ UNITS DIFFER AND THE MISMATCH IS SILENT. glTF's `anisotropyRotation` is in
			 * RADIANS; the engine's is in TURNS, because the shader computes
			 * `rotation * 2.0 * PI` — and `setAnisotropyRotation()` CLAMPS TO [0,1]. Passing radians
			 * straight through would be wrong by 2π AND clamped flat for anything above 1 rad, i.e.
			 * a plausible-looking wrong direction. Hence the division. */
			float anisotropyStrength = 0.0F;
			float anisotropyRotationTurns = 0.0F;
			std::shared_ptr< TextureResource::Texture2D > anisotropyTex;

			if ( glTFMaterial.anisotropy != nullptr )
			{
				anisotropyStrength = static_cast< float >(glTFMaterial.anisotropy->anisotropyStrength);
				anisotropyRotationTurns = static_cast< float >(glTFMaterial.anisotropy->anisotropyRotation) / (2.0F * std::numbers::pi_v< float >);

				/* RG carries the per-pixel direction and B the strength — the shader reads exactly
				 * that from the component (`m_surfaceAnisotropyDirection`). */
				if ( glTFMaterial.anisotropy->anisotropyTexture.has_value() )
				{
					anisotropyTex = resolveTexture(glTFMaterial.anisotropy->anisotropyTexture->textureIndex);
				}
			}

			/* Specular (KHR_materials_specular) — scales and TINTS the dielectric F0.
			 *
			 * The GPU side is already spec-exact and has been for a while
			 * (`LightGenerator.PBR.cpp`): `F0 = mix(min(dielectricF0 * specularColor *
			 * specularFactor, 1), albedo, metalness)`. Only this read was missing, so every
			 * asset got the identity (factor 1, white) — measured on the Khronos `SpecularTest`,
			 * whose 35 spheres declare `baseColorFactor [0,0,0,1]`, `metallicFactor 0` and
			 * `roughnessFactor 0`: with nothing but the extension able to light them, they all
			 * rendered between 3.65 and 3.83 out of 255.
			 *
			 * ⚠️ `specularColorFactor` is LINEAR in glTF, and the shader multiplies F0 with it
			 * directly — so it must NOT be gamma-converted on the way in, despite the engine's
			 * "a name ending in Color is sRGB" convention. Same treatment as `baseColorFactor`
			 * above. The textures (`specularTexture` A channel, `specularColorTexture` RGB) are
			 * NOT read yet: the material has a single `ComponentType::Specular` slot and the two
			 * glTF maps would need two. */
			/* NOTE: The defaults are the SPEC's, not the engine's — glTF declares
			 * `specularFactor` 1.0 and `specularColorFactor` [1,1,1], which are the identity of
			 * the F0 expression above, so an asset without the extension is untouched. Stating
			 * them here rather than borrowing `StandardResource`'s private constants keeps the
			 * loader answerable to the format it reads. */
			float specularFactor = 1.0F;
			/* ⚠️⚠️ A Vector, NOT a `Color`: `KHR_materials_specular` allows `specularColorFactor`
			 * **above 1.0** so that the material's IOR does not cap its specular response, and
			 * `PixelFactory::Color`'s constructor clamps every component to [0, 1] — right for a
			 * colour, wrong for a multiplier. Building a Color here silently flattened
			 * `SpecularTest`'s last row, whose final sphere must read as a mirror ball, and the
			 * clamp left no trace anywhere. Energy conservation is unaffected: the shader clamps
			 * the RESULT, `F0 = min(dielectricF0 * specularColor * specularFactor, 1)`. */
			Base::Math::Vector< 3, float > specularColor{1.0F, 1.0F, 1.0F};

			if ( glTFMaterial.specular != nullptr )
			{
				specularFactor = static_cast< float >(glTFMaterial.specular->specularFactor);

				const auto & scf = glTFMaterial.specular->specularColorFactor;

				specularColor = Base::Math::Vector< 3, float >{
					static_cast< float >(scf[0]),
					static_cast< float >(scf[1]),
					static_cast< float >(scf[2])
				};
			}

			/* The two specular maps. ⚠️ `specularTexture` carries its value in the **A channel**
			 * and is LINEAR; `specularColorTexture` carries the F0 tint in RGB and is **sRGB** —
			 * hence the `{.sRGB = true}` on the second `resolveTexture()` and not on the first. Both
			 * MULTIPLY their respective factor, which the material's component codegen folds in. */
			auto specularTex = ( glTFMaterial.specular != nullptr && glTFMaterial.specular->specularTexture.has_value() )
				? resolveTexture(glTFMaterial.specular->specularTexture->textureIndex) : nullptr;

			auto specularColorTex = ( glTFMaterial.specular != nullptr && glTFMaterial.specular->specularColorTexture.has_value() )
				? resolveTexture(glTFMaterial.specular->specularColorTexture->textureIndex, {.sRGB = true}) : nullptr;

			const auto specularUVTransform = glTFMaterial.specular != nullptr ? readUVTransform(glTFMaterial.specular->specularTexture) : UVTransform{};
			const auto specularColorUVTransform = glTFMaterial.specular != nullptr ? readUVTransform(glTFMaterial.specular->specularColorTexture) : UVTransform{};

			/* IOR (KHR_materials_ior) — drives the dielectric F0 itself:
			 * `dielectricF0 = ((ior - 1) / (ior + 1))^2`, already in the shader. glTF's default is
			 * 1.5, which is exactly the engine's `DefaultIOR`, so an asset that declares nothing
			 * keeps the behaviour it had.
			 *
			 * ⚠️ `setIOR()` clamps to [1.0, 3.0]. That covers the whole glTF range (the spec's
			 * minimum is 1.0; diamond, the highest value in the conformance assets, is 2.42) AND
			 * it happens to render the spec's `ior = 0` special case correctly: 0 clamps to 1,
			 * and ((1-1)/(1+1))^2 is 0 — which is the F0 = 0 that special case asks for. So the
			 * value goes through verbatim, including 0.
			 * fastgltf already initialises `Material::ior` to the spec default 1.5. */
			const auto materialIOR = static_cast< float >(glTFMaterial.ior);

			/* Alpha mode (OPAQUE, MASK, BLEND). The caller may declare a material a CUTOUT whatever the file says
			 * (LoaderOptions::cutoutMaterialNames): a coverage mask exported as BLEND. */
			const bool isForcedCutout = m_options.cutoutMaterialNames.contains(std::string{glTFMaterial.name.data(), glTFMaterial.name.size()});
			const bool isAlphaBlend = !isForcedCutout && glTFMaterial.alphaMode == fastgltf::AlphaMode::Blend;
			const bool isAlphaMask = isForcedCutout || glTFMaterial.alphaMode == fastgltf::AlphaMode::Mask;
			const auto alphaCutoff = static_cast< float >(glTFMaterial.alphaCutoff);

			/* Async material creation — lambda is fully self-contained, no this/reference captures.
			 * The lambda is generic so the same configuration code path applies whether the
			 * loader produces a StandardResource or a StandardResource (cross-material aliases
			 * convert PBR factors to Phong/Blinn parameters when targeting Standard). */
			auto configure = [
					albedoTexture = std::move(albedoTex), albedoColor,
					metallicRoughnessTexture = std::move(metallicRoughnessTex), roughnessFactor, metallicFactor, roughnessChannel, metalnessChannel,
					normalTexture = std::move(normalTex), normalScale,
					aoTexture = std::move(aoTex), aoStrength, aoChannel,
					albedoUVTransform, metallicRoughnessUVTransform, normalUVTransform, aoUVTransform, emissiveUVTransform,
					emissiveTexture = std::move(emissiveTex), emissiveStrength, emissiveColor, hasEmissiveColor,
					clearcoatFactor, clearcoatRoughness, clearcoatNormalScale,
					clearcoatTexture = std::move(clearcoatTex), clearcoatUVTransform, clearcoatRoughnessUVTransform, clearcoatNormalUVTransform,
					clearcoatRoughnessTexture = std::move(clearcoatRoughnessTex),
					clearcoatNormalTexture = std::move(clearcoatNormalTex),
					sheenColor, sheenRoughness,
					sheenColorTexture = std::move(sheenColorTex), sheenColorUVTransform, sheenRoughnessUVTransform,
					sheenRoughnessTexture = std::move(sheenRoughnessTex),
					transmissionFactor,
					iridescenceFactor, iridescenceIOR, iridescenceThicknessMin, iridescenceThicknessMax,
					iridescenceTexture = std::move(iridescenceTex),
					iridescenceThicknessTexture = std::move(iridescenceThicknessTex),
					anisotropyStrength, anisotropyRotationTurns, anisotropyTexture = std::move(anisotropyTex),
					specularFactor, specularColor, materialIOR,
					volumeThicknessFactor, volumeAttenuationDistance, volumeAttenuationColor,
					volumeThicknessTexture = std::move(volumeThicknessTex),
					specularTexture = std::move(specularTex), specularColorTexture = std::move(specularColorTex), specularUVTransform, specularColorUVTransform,
					environmentReflectionIntensity = m_options.environmentReflectionIntensity,
					isAlphaBlend, isAlphaMask, alphaCutoff
				] (auto & materialResource, bool primarySetOnly = false) {
					/* Albedo. */
					/* A base-colour texture and a base-colour factor MULTIPLY — that is what both
					 * glTF (baseColorFactor) and FBX (base_color) specify. Setting the component to
					 * the texture and dropping the factor tints nothing and silently loses the
					 * factor's alpha; the colour goes to the material's tint slot instead. */
					if ( albedoTexture != nullptr )
					{
						materialResource.setAlbedoComponent(albedoTexture);
						materialResource.setAlbedoColor(albedoColor);

						if ( albedoUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::Albedo, albedoUVTransform);
						}
					}
					else
					{
						materialResource.setAlbedoComponent(albedoColor);
					}

					/* Roughness / Metalness.
					 * glTF packs both in ONE texture: roughness in the GREEN channel, metalness in
					 * the BLUE channel, each multiplied by its factor (glTF 2.0 § material.pbrMetallicRoughness;
					 * reference: Khronos glTF-Sample-Renderer, material_info.glsl, getMetallicRoughnessInfo()).
					 * ⚠️ Omitting the source channel reads RED — empty in most assets (measured ~0 on
					 * DamagedHelmet) — which flattens both properties to 0 over the whole surface. */
					if ( metallicRoughnessTexture != nullptr )
					{
						materialResource.setRoughnessComponent(metallicRoughnessTexture, roughnessFactor, false, roughnessChannel);
						materialResource.setMetalnessComponent(metallicRoughnessTexture, metallicFactor, metalnessChannel);

						/* ONE glTF texture info, TWO engine components: same transform on both. */
						if ( metallicRoughnessUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::Roughness, metallicRoughnessUVTransform);
							applyTextureCoordinates(materialResource, ComponentType::Metalness, metallicRoughnessUVTransform);
						}
					}
					else
					{
						materialResource.setRoughnessComponent(roughnessFactor);
						materialResource.setMetalnessComponent(metallicFactor);
					}

					/* Environment (image-based) specular reflection.
					 *
					 * ⚠️ Without this call a metallic-roughness material has NOTHING to reflect and
					 * renders matte — the loader used to declare no reflection at all. It also
					 * decides what SSR/RTR receive: declaring it promotes the reflectivity published
					 * to the material-properties G-buffer from `metalness * (1 - roughness)` — which
					 * collapses to ~0 on a rough surface — to `max(iblIntensity * (1 - roughness),
					 * metalness)`, so rough metal stays reflective. See
					 * `LightGenerator::materialPropertiesExpression()`. */
					if ( environmentReflectionIntensity > 0.0F )
					{
						materialResource.setReflectionComponentFromEnvironmentCubemap(environmentReflectionIntensity);
					}

					/* Normal map. */
					if ( normalTexture != nullptr )
					{
						materialResource.setNormalComponent(normalTexture, normalScale);

						if ( normalUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::Normal, normalUVTransform);
						}
					}

					/* Ambient occlusion. */
					if ( aoTexture != nullptr )
					{
						materialResource.setAmbientOcclusionComponent(aoTexture, aoStrength, aoChannel);

						if ( aoUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::AmbientOcclusion, aoUVTransform);
						}
					}

					/* Emissive: the texture × emissiveFactor (glTF 2.0 § material.emissiveFactor), the factor in the
					 * material's emissive colour, which the shader multiplies with the texel — the albedo's way. */
					if ( emissiveTexture != nullptr )
					{
						materialResource.setAutoIlluminationComponent(emissiveTexture, emissiveStrength);
						materialResource.setAutoIlluminationColor(emissiveColor);

						if ( emissiveUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::AutoIllumination, emissiveUVTransform);
						}
					}
					else if ( hasEmissiveColor )
					{
						materialResource.setAutoIlluminationComponent(emissiveColor, emissiveStrength);
					}

					/* Clear coat (KHR_materials_clearcoat), factors then maps.
					 * ⚠️ Order matters: both texture setters also write the two scalars, so they
					 * are handed the SAME values the scalar path would have set. The roughness map
					 * is read from GREEN, per the extension. */
					if ( clearcoatFactor > 0.0F )
					{
						if ( clearcoatTexture != nullptr )
						{
							materialResource.setClearCoatComponent(clearcoatTexture, clearcoatRoughness, clearcoatFactor, Base::PixelFactory::Channel::Red);
						}
						else
						{
							materialResource.setClearCoatComponent(clearcoatFactor, clearcoatRoughness);
						}

						if ( clearcoatRoughnessTexture != nullptr )
						{
							materialResource.setClearCoatRoughnessComponent(clearcoatRoughnessTexture, clearcoatFactor, clearcoatRoughness, Base::PixelFactory::Channel::Green);
						}

						if ( clearcoatUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::ClearCoat, clearcoatUVTransform);
						}

						if ( clearcoatRoughnessTexture != nullptr && clearcoatRoughnessUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::ClearCoatRoughness, clearcoatRoughnessUVTransform);
						}

						if ( clearcoatNormalTexture != nullptr )
						{
							materialResource.setClearCoatNormalComponent(clearcoatNormalTexture, clearcoatNormalScale);

							if ( clearcoatNormalUVTransform.present )
							{
								applyTextureCoordinates(materialResource, ComponentType::ClearCoatNormal, clearcoatNormalUVTransform);
							}
						}
					}

					/* Sheen (KHR_materials_sheen), factors then maps.
					 * ⚠️ The two maps are independent and each falls back on its own scalar, so the
					 * colour setter is called with the texture when there is one and with the colour
					 * otherwise, and the roughness map — which also carries the colour forward, since
					 * it may be the only map — comes after it. */
					if ( sheenRoughness > 0.0F || sheenColor.red() > 0.0F || sheenColor.green() > 0.0F || sheenColor.blue() > 0.0F )
					{
						if ( sheenColorTexture != nullptr )
						{
							materialResource.setSheenComponent(sheenColorTexture, sheenRoughness);
						}
						else
						{
							materialResource.setSheenComponent(sheenColor, sheenRoughness);
						}

						if ( sheenColorUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::Sheen, sheenColorUVTransform);
						}

						if ( sheenRoughnessTexture != nullptr )
						{
							materialResource.setSheenRoughnessComponent(sheenRoughnessTexture, sheenColor, sheenRoughness, Base::PixelFactory::Channel::Alpha);

							if ( sheenRoughnessUVTransform.present )
							{
								applyTextureCoordinates(materialResource, ComponentType::SheenRoughness, sheenRoughnessUVTransform);
							}
						}
					}

					/* Transmission (KHR_materials_transmission).
					 * GRAB PASS, not the prefiltered cubemap: the extension's semantics is seeing
					 * THROUGH the surface (a car window shows the interior). The cubemap variant
					 * refracts the sky only — measured on CarConcept, the glass hid the cabin.
					 * The codegen falls back to the cubemap when the grab pass is unavailable
					 * (low quality / no bindless). */
					if ( transmissionFactor > 0.0F )
					{
						materialResource.setTransmissionComponentFromGrabPass(transmissionFactor);
					}

					/* Volume (KHR_materials_volume). Applied UNCONDITIONALLY, like specular and IOR:
					 * the spec's defaults ARE the no-volume case (thickness 0, distance infinite),
					 * so a material that declares nothing must get those rather than whatever the
					 * resource was constructed with — which is a 1 m attenuation distance.
					 * ⚠️ Note the order: `setTransmissionComponentFromGrabPass()` above also writes
					 * these three slots from ITS defaults, so the volume must be applied AFTER it or
					 * it would be overwritten. */
					materialResource.setThicknessFactor(volumeThicknessFactor);
					materialResource.setAttenuationDistance(volumeAttenuationDistance);
					materialResource.setAttenuationColor(volumeAttenuationColor);

					/* ⚠️ The thickness MAP multiplies the factor set just above, so it goes after
					 * it for the same reason the factor goes after setTransmissionComponent*(). */
					if ( volumeThicknessTexture != nullptr )
					{
						materialResource.setVolumeThicknessComponent(volumeThicknessTexture);
					}

					/* Iridescence (KHR_materials_iridescence). Gated on the factor, which is what the
					 * extension itself uses to mean "no iridescence" (default 0). */
					if ( iridescenceFactor > 0.0F )
					{
						if ( iridescenceTexture != nullptr )
						{
							materialResource.setIridescenceComponent(iridescenceTexture, iridescenceIOR, iridescenceThicknessMin, iridescenceThicknessMax);
						}
						else
						{
							materialResource.setIridescenceComponent(iridescenceFactor, iridescenceIOR, iridescenceThicknessMin, iridescenceThicknessMax);
						}

						/* ⚠️ Applied AFTER the factor: both setters write the two thickness slots
						 * from their own arguments, and this one must have the last word. The map
						 * is what turns a uniform film colour into the reference's varying one —
						 * IridescentDishWithOlives.glb sweeps a 50 nm band (500..550) entirely
						 * through this texture. */
						if ( iridescenceThicknessTexture != nullptr )
						{
							materialResource.setIridescenceThicknessComponent(iridescenceThicknessTexture, iridescenceThicknessMin, iridescenceThicknessMax);
						}
					}

					/* Anisotropy (KHR_materials_anisotropy). Gated on the strength, the extension's
					 * own "off" value (default 0) — an isotropic lobe is not a special case, it is
					 * the absence of this one. */
					if ( anisotropyStrength > 0.0F )
					{
						if ( anisotropyTexture != nullptr )
						{
							materialResource.setAnisotropyComponent(anisotropyTexture, anisotropyStrength, anisotropyRotationTurns);
						}
						else
						{
							materialResource.setAnisotropyComponent(anisotropyStrength, anisotropyRotationTurns);
						}
					}

					/* Specular (KHR_materials_specular) and IOR (KHR_materials_ior).
					 * Both are ALWAYS applied, never gated on a "is it non-default" test: they
					 * scale and tint the dielectric F0 of every material, and their defaults are
					 * the identity (factor 1, white, IOR 1.5 == glTF's own default). Gating them
					 * would leave a material that deliberately declares the default carrying
					 * whatever the resource was constructed with. */
					if ( specularTexture != nullptr )
					{
						materialResource.setSpecularComponent(specularTexture, specularFactor);
					}
					else
					{
						materialResource.setSpecularFactor(specularFactor);
					}

					if ( specularTexture != nullptr && specularUVTransform.present )
					{
						applyTextureCoordinates(materialResource, ComponentType::Specular, specularUVTransform);
					}

					if ( specularColorTexture != nullptr )
					{
						materialResource.setSpecularColorComponent(specularColorTexture, specularColor);

						if ( specularColorUVTransform.present )
						{
							applyTextureCoordinates(materialResource, ComponentType::SpecularColor, specularColorUVTransform);
						}
					}
					else
					{
						materialResource.setSpecularColor(specularColor);
					}

					materialResource.setIOR(materialIOR);

					/* Alpha blending (glTF alphaMode: BLEND). */
					if ( isAlphaBlend )
					{
						materialResource.enableBlending(BlendingMode::Normal);
					}

					/* Alpha cutout (glTF alphaMode: MASK): binary alpha test on the albedo alpha
					 * channel at the authored cutoff — the material stays opaque, casts cutout
					 * shadows and alpha-tests at RT hit time. No-op on StandardResource (parity
					 * stub — the legacy material has no albedo-alpha cutout path). */
					if ( isAlphaMask )
					{
						materialResource.enableAlphaTest(alphaCutoff);
					}

					/* The `…-uv0` variant: every map back on TEXCOORD_0, BEFORE the load success (which may create it). */
					if ( primarySetOnly )
					{
						static_cast< void >(materialResource.usePrimaryTextureCoordinatesOnly());
					}

					return materialResource.setManualLoadSuccess(true);
				};

			auto material = std::static_pointer_cast< Material::Interface >(
				m_resources.container< Material::StandardResource >()->getOrCreateResource(name, configure)
			);

			if ( material == nullptr )
			{
				TraceWarning{ClassId} << "Material " << materialIndex << " ('" << name << "') failed to create, using default.";

				m_materials[materialIndex] = m_resources.container< Material::StandardResource >()->getDefaultResource();

				allSuccess = false;
			}
			else
			{
				m_materials[materialIndex] = std::move(material);
			}

			/* The vertex-colour variant: same configuration, plus the flag. `configure` is copied
			 * rather than moved because the plain material above already consumed it, and it is
			 * self-contained by contract (no `this`, no references) so a copy is safe on the
			 * thread pool. */
			if ( materialsUsedWithVertexColor.contains(materialIndex) )
			{
				auto vertexColorMaterial = std::static_pointer_cast< Material::Interface >(
					m_resources.container< Material::StandardResource >()->getOrCreateResource(name + "-vc", [configure] (auto & materialResource) {
						materialResource.enableVertexColor();

						return configure(materialResource);
					})
				);

				if ( vertexColorMaterial == nullptr )
				{
					TraceWarning{ClassId} << "Material " << materialIndex << " ('" << name << "-vc') vertex-colour variant failed to create, falling back to the plain one — its COLOR_0 will be ignored.";
				}
				else
				{
					m_materialsVertexColor[materialIndex] = std::move(vertexColorMaterial);
				}
			}

			/* The primary-set-only variant (see m_materialsPrimarySetOnly). */
			if ( materialsNeedingPrimarySetOnly.contains(materialIndex) )
			{
				auto primarySetMaterial = std::static_pointer_cast< Material::Interface >(
					m_resources.container< Material::StandardResource >()->getOrCreateResource(name + "-uv0", [configure] (auto & materialResource) {
						return configure(materialResource, true);
					})
				);

				if ( primarySetMaterial == nullptr )
				{
					TraceWarning{ClassId} << "Material " << materialIndex << " ('" << name << "-uv0') primary-set variant failed to create; the primitives lacking TEXCOORD_1 use the default material.";
				}
				else
				{
					m_materialsPrimarySetOnly[materialIndex] = std::move(primarySetMaterial);
				}
			}
		}

		return allSuccess;
	}

	bool
	GLTFLoader::buildMeshes (const GLTF::Asset & analysed, SceneData & output) noexcept
	{
		const auto & asset = *analysed.gltf;

		m_meshes.clear();
		m_meshes.resize(asset.meshes.size());

		/* A mesh the analysis could not build already failed the asset (GLTF::Asset::meshesComplete). */
		bool allSuccess = analysed.meshesComplete;

		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			return m_resources.container< Material::StandardResource >()->getDefaultResource();
		};

		/* ⚠️ Geometry and mesh resources are cached BY NAME and their content depends on the axis
		 * flip, so the flip must be part of the key. Without it, loading the same asset twice with
		 * different flags would silently hand the first variant to the second caller. Images,
		 * textures and materials are deliberately NOT keyed this way: the flip does not touch them,
		 * and duplicating 84 images per variant would cost memory for nothing. */

		for ( size_t meshIndex = 0; meshIndex < asset.meshes.size() && meshIndex < analysed.meshes.size(); ++meshIndex )
		{
			const auto & glTFMesh = asset.meshes[meshIndex];
			const auto & description = analysed.meshes[meshIndex];

			/* No triangle, a failed read or an invalid shape: the analysis reported it. */
			if ( description.shape == nullptr )
			{
				m_meshes[meshIndex] = m_resources.container< Renderable::MeshResource >()->getDefaultResource();

				continue;
			}

			const auto & shape = description.shape;
			const auto & geoName = description.geometryName;
			const auto & meshName = description.meshName;
			const auto allPrimitivesHaveTangents = description.allPrimitivesHaveTangents;
			const auto anyPrimitiveHasVertexColors = description.hasVertexColors;
			const auto anyPrimitiveSamplesSecondarySet = description.samplesSecondarySet;

			uint32_t geometryFlags = EnableTangentSpace | EnablePrimaryTextureCoordinates;

			if ( !shape->vertices().empty() && shape->vertices()[0].influences()[0] >= 0 )
			{
				geometryFlags |= EnableInfluence | EnableWeight;
			}

			/* The attribute goes into the VBO for the whole mesh as soon as ONE of its primitives
			 * declares COLOR_0 — the shape and its buffer are per mesh, not per primitive. */
			if ( anyPrimitiveHasVertexColors )
			{
				geometryFlags |= EnableVertexColor;
			}

			if ( anyPrimitiveSamplesSecondarySet )
			{
				geometryFlags |= EnableSecondaryTextureCoordinates;
			}

			/* Phase 2: Tangent computation + GPU upload on thread pool.
			 *
			 * ⚠️ The computation is SKIPPED when the asset authored its tangents — glTF requires
			 * the authored ones to be used, and the engine's derivation loses the handedness (it
			 * only computes the tangent, leaving `biNormal()` to assume a +1 sign). Recomputing
			 * over authored data is not a harmless refresh: it discards the mirroring. */
			auto geometry = m_resources.container< IndexedVertexResource >()
				->getOrCreateResource(geoName, [shape, allPrimitivesHaveTangents] (auto & geometryResource) {
					if ( !allPrimitivesHaveTangents )
					{
						shape->computeTriangleTangent();
						shape->computeVertexTangent();
					}

					return geometryResource.load(*shape);
				}, geometryFlags);

			if ( geometry == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create geometry for mesh " << meshIndex << ", using default.";

				m_meshes[meshIndex] = m_resources.container< Renderable::MeshResource >()->getDefaultResource();

				allSuccess = false;

				continue;
			}

			/* Collect materials (and per-layer rasterization) for each primitive. */
			std::vector< std::shared_ptr< Material::Interface > > materialList;
			std::vector< RasterizationOptions > rasterizationList;
			materialList.reserve(glTFMesh.primitives.size());
			rasterizationList.reserve(glTFMesh.primitives.size());

			for ( const auto & primitive : glTFMesh.primitives )
			{
				RasterizationOptions rasterization{};

				if ( primitive.materialIndex.has_value() )
				{
					const auto materialIndex = primitive.materialIndex.value();

					/* ⚠️ The choice is PER PRIMITIVE, because COLOR_0 is: the same glTF material may
					 * serve a coloured primitive and a colourless one, and only the first may carry
					 * the `UseVertexColors` flag (whose codegen reads a vertex input attribute).
					 * The `…-vc` variant exists only for the materials a COLOR_0 primitive uses. */
					const auto primitiveHasVertexColors = primitive.findAttribute("COLOR_0") != primitive.attributes.end();
					/* A material sampling TEXCOORD_1 on a primitive without it (an invalid asset): its `…-uv0` variant. */
					const auto primitiveLacksItsSecondarySet = materialIndex < m_materialSamplesSecondarySet.size() && m_materialSamplesSecondarySet[materialIndex] != 0 &&
						primitive.findAttribute("TEXCOORD_1") == primitive.attributes.end();

					if ( primitiveLacksItsSecondarySet )
					{
						/* ⚠️ The variant carries no vertex colour: such a primitive's COLOR_0, if any, is ignored. Without the
						 * variant, the default material (never the plain one: its shader reads an absent attribute). */
						const auto hasVariant = materialIndex < m_materialsPrimarySetOnly.size() && m_materialsPrimarySetOnly[materialIndex] != nullptr;

						materialList.push_back(hasVariant ? m_materialsPrimarySetOnly[materialIndex] : defaultMaterial());
					}
					else if ( primitiveHasVertexColors && materialIndex < m_materialsVertexColor.size() && m_materialsVertexColor[materialIndex] != nullptr )
					{
						materialList.push_back(m_materialsVertexColor[materialIndex]);
					}
					else if ( materialIndex < m_materials.size() && m_materials[materialIndex] != nullptr )
					{
						materialList.push_back(m_materials[materialIndex]);
					}
					else
					{
						materialList.push_back(defaultMaterial());
					}

					/* Honour the glTF material's double-sided flag (glTF 2.0 standard): disable
					 * back-face culling so both faces rasterize (curtains, foliage, cloth, …). */
					if ( materialIndex < asset.materials.size() && asset.materials[materialIndex].doubleSided )
					{
						rasterization.setCullingMode(CullingMode::None);
					}
				}
				else
				{
					materialList.push_back(defaultMaterial());
				}

				/* Caller-forced double-sided (LoaderOptions::forceDoubleSided): OR with
				 * the asset flag above. Lets a caller render a whole model double-sided
				 * regardless of what the glTF declares (symmetric with FBXLoader). */
				if ( m_options.forceDoubleSided )
				{
					rasterization.setCullingMode(CullingMode::None);
				}

				rasterizationList.push_back(rasterization);
			}

			/* Create renderable mesh resource. */
			std::shared_ptr< Renderable::Abstract > mesh;

			if ( materialList.size() <= 1 )
			{
				auto singleMaterial = materialList.empty()
					? defaultMaterial()
					: materialList[0];

				const RasterizationOptions singleRasterization = rasterizationList.empty() ? RasterizationOptions{} : rasterizationList[0];

				mesh = m_resources.container< Renderable::MeshResource >()
					->getOrCreateResource(meshName, [geometry, material = std::move(singleMaterial), singleRasterization] (auto & meshResource) {
						return meshResource.load(geometry, material, singleRasterization);
					});
			}
			else
			{
				mesh = m_resources.container< Renderable::MultiLayerMeshResource >()
					/* NOTE: materialList is COPIED (shared pointers): the scene-data descriptor below needs it too. Moved
					 * here, it left every multi-material mesh with the DEFAULT material in its descriptor (triad 2026-09-30). */
					->getOrCreateResource(meshName, [geometry, materialList, rasterizations = std::move(rasterizationList)] (auto & meshResource) {
						return meshResource.load(geometry, materialList, rasterizations);
					});
			}

			m_meshes[meshIndex] = mesh;

			/* Store in SceneData for consumer access. */
			MeshDescriptor descriptor;
			descriptor.renderable = mesh;
			descriptor.geometry = std::static_pointer_cast< Geometry::Interface >(geometry);
			descriptor.materials = materialList.empty()
				? std::vector< std::shared_ptr< Material::Interface > >{m_resources.container< Material::StandardResource >()->getDefaultResource()}
				: std::move(materialList);

			/* Ensure output.meshes is indexed by glTF mesh index. */
			if ( output.meshes.size() <= meshIndex )
			{
				output.meshes.resize(meshIndex + 1);
			}

			output.meshes[meshIndex] = std::move(descriptor);

			if ( m_options.onMeshLoaded )
			{
				m_options.onMeshLoaded(output.meshes[meshIndex]);
			}
		}

		return allSuccess;
	}
}
