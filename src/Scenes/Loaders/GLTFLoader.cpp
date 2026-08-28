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
#include <span>
#include <unordered_map>
#include <variant>
#include <vector>

/* Third-party inclusions. */
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include <meshoptimizer.h>

/* Local inclusions. */
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletonResource.hpp"
#include "SceneData.hpp"
#include "Graphics/CompressedImageResource.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/KTX2Decoder.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/Joint.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Skin.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Quaternion.hpp"
#include "Math/TransformUtils.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "VertexFactory/Shape.hpp"
#include "IO/IO.hpp"
#include "PrimaryServices.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/PhysicalDevice.hpp"

namespace EmEn::Scenes::Loaders
{
	using namespace Graphics;
	using namespace Graphics::Geometry;
	using namespace Base::Animation;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Base::PixelFactory;

	/* The byte span type fastgltf hands to accessor readers. */
	using ByteSpan = decltype(fastgltf::DefaultBufferDataAdapter{}(std::declval< const fastgltf::Asset & >(), std::size_t{}));

	/* Translates a glTF sampler wrap mode. The three values are the whole of what glTF can
	 * express (GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE), so this conversion is total. */
	static
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

	/* Returns the whole payload of a glTF buffer. Mirrors what DefaultBufferDataAdapter does for
	 * a buffer *view*, except EXT_meshopt_compression addresses its compressed source directly in
	 * the buffer, outside of any view. */
	static
	ByteSpan
	bufferBytes (const fastgltf::Asset & asset, size_t bufferIndex) noexcept
	{
		if ( bufferIndex >= asset.buffers.size() )
		{
			return {};
		}

		return std::visit(fastgltf::visitor{
			[] (const auto &) -> ByteSpan {
				return {};
			},
			[] (const fastgltf::sources::Array & array) -> ByteSpan {
				return ByteSpan{array.bytes.data(), array.bytes.size_bytes()};
			},
			[] (const fastgltf::sources::Vector & vector) -> ByteSpan {
				return ByteSpan{vector.bytes.data(), vector.bytes.size()};
			},
			[] (const fastgltf::sources::ByteView & byteView) -> ByteSpan {
				return byteView.bytes;
			}
		}, asset.buffers[bufferIndex].data);
	}

	/**
	 * @brief Decodes EXT_meshopt_compression buffer views on demand and keeps the result.
	 *
	 * The extension replaces a buffer view's payload with a meshopt-encoded block that has to be
	 * run through the meshoptimizer codec before any accessor can be read. fastgltf parses the
	 * extension metadata but deliberately does not decode it, so the work lands here.
	 *
	 * Decoding is **lazy and cached**: a view is decoded the first time an accessor reaches into
	 * it, and the result is kept until the load ends. Caching is not an optimisation detail — a
	 * compressed glTF interleaves several attributes in a single view, so a dozen accessors read
	 * the same block, and decoding per accessor would repeat the same work over and over.
	 *
	 * The cache is the load's peak memory cost (~300 MiB on a compressed Sponza, against ~99 MiB
	 * of encoded source). GLTFLoader::load() releases it as soon as the geometry is built.
	 *
	 * @note Not thread-safe : a loader instance drives a single load, on a single thread.
	 * @see https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Vendor/EXT_meshopt_compression
	 */
	class MeshoptBufferCache final
	{
		public:

			MeshoptBufferCache () noexcept = default;

			/**
			 * @brief Returns the bytes of a buffer view, decoding it first when it is compressed.
			 * @param asset A reference to the parsed glTF asset.
			 * @param bufferViewIndex The index of the buffer view to read.
			 * @return A span over the view bytes, empty on failure.
			 */
			[[nodiscard]]
			ByteSpan
			view (const fastgltf::Asset & asset, size_t bufferViewIndex) noexcept
			{
				if ( bufferViewIndex >= asset.bufferViews.size() )
				{
					return {};
				}

				const auto & bufferView = asset.bufferViews[bufferViewIndex];

				if ( bufferView.meshoptCompression == nullptr )
				{
					return fastgltf::DefaultBufferDataAdapter{}(asset, bufferViewIndex);
				}

				const auto decodedIt = m_decoded.find(bufferViewIndex);

				if ( decodedIt != m_decoded.cend() )
				{
					return ByteSpan{decodedIt->second.data(), decodedIt->second.size()};
				}

				auto & decoded = m_decoded[bufferViewIndex];

				if ( !MeshoptBufferCache::decode(asset, bufferView, decoded) )
				{
					TraceError{GLTFLoader::ClassId} << "Unable to decode the meshopt-compressed buffer view " << bufferViewIndex << " !";

					decoded.clear();
				}

				return ByteSpan{decoded.data(), decoded.size()};
			}

			/**
			 * @brief Returns the total number of bytes currently held decoded.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			decodedBytes () const noexcept
			{
				size_t bytes = 0;

				for ( const auto & [index, buffer] : m_decoded )
				{
					bytes += buffer.size();
				}

				return bytes;
			}

			/**
			 * @brief Returns the number of buffer views decoded so far.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			viewCount () const noexcept
			{
				return m_decoded.size();
			}

		private:

			/**
			 * @brief Runs one buffer view through the meshoptimizer codec.
			 * @param asset A reference to the parsed glTF asset.
			 * @param bufferView A reference to the compressed buffer view.
			 * @param output A reference to the vector receiving the decoded bytes.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			decode (const fastgltf::Asset & asset, const fastgltf::BufferView & bufferView, std::vector< std::byte > & output) noexcept
			{
				const auto & compression = *bufferView.meshoptCompression;

				const auto source = bufferBytes(asset, compression.bufferIndex);

				if ( source.empty() || compression.byteOffset + compression.byteLength > source.size() )
				{
					Tracer::error(GLTFLoader::ClassId, "A meshopt-compressed buffer view points outside of its buffer !");

					return false;
				}

				const auto decodedSize = compression.count * compression.byteStride;

				/* NOTE: The extension mandates count * byteStride == byteLength. A mismatch means a
				 * broken exporter, and trusting the view length would overrun the decoder output. */
				if ( decodedSize != bufferView.byteLength )
				{
					TraceError{GLTFLoader::ClassId} <<
						"A meshopt-compressed buffer view declares " << bufferView.byteLength <<
						" bytes but its element count implies " << decodedSize << " !";

					return false;
				}

				output.resize(decodedSize);

				const auto * encoded = reinterpret_cast< const unsigned char * >(source.data() + compression.byteOffset);

				int error = 0;

				switch ( compression.mode )
				{
					case fastgltf::MeshoptCompressionMode::Attributes :
						error = meshopt_decodeVertexBuffer(output.data(), compression.count, compression.byteStride, encoded, compression.byteLength);
						break;

					case fastgltf::MeshoptCompressionMode::Triangles :
						error = meshopt_decodeIndexBuffer(output.data(), compression.count, compression.byteStride, encoded, compression.byteLength);
						break;

					case fastgltf::MeshoptCompressionMode::Indices :
						error = meshopt_decodeIndexSequence(output.data(), compression.count, compression.byteStride, encoded, compression.byteLength);
						break;
				}

				if ( error != 0 )
				{
					TraceError{GLTFLoader::ClassId} << "The meshopt codec rejected a buffer view (error " << error << ") !";

					return false;
				}

				/* Filters run in place on the decoded buffer : they undo the lossy packing the
				 * encoder applied to normals, tangents (octahedral) and quantised floats. */
				switch ( compression.filter )
				{
					case fastgltf::MeshoptCompressionFilter::None :
						break;

					case fastgltf::MeshoptCompressionFilter::Octahedral :
						meshopt_decodeFilterOct(output.data(), compression.count, compression.byteStride);
						break;

					case fastgltf::MeshoptCompressionFilter::Quaternion :
						meshopt_decodeFilterQuat(output.data(), compression.count, compression.byteStride);
						break;

					case fastgltf::MeshoptCompressionFilter::Exponential :
						meshopt_decodeFilterExp(output.data(), compression.count, compression.byteStride);
						break;
				}

				return true;
			}

			std::unordered_map< size_t, std::vector< std::byte > > m_decoded;
	};

	/**
	 * @brief The fastgltf buffer data adapter backed by the meshopt cache.
	 * @note fastgltf takes the adapter by const reference and calls it as a const functor, hence
	 * the pointer : the cache it points to is mutated (it fills on demand), the adapter is not.
	 */
	struct MeshoptBufferAdapter final
	{
		MeshoptBufferCache * cache;

		ByteSpan
		operator() (const fastgltf::Asset & asset, size_t bufferViewIndex) const noexcept
		{
			return cache->view(asset, bufferViewIndex);
		}
	};

	/* Detect image format from MIME type. */
	static
	Pixmap< uint8_t >::Format
	mimeToPixmapFormat (fastgltf::MimeType mime) noexcept
	{
		switch ( mime )
		{
			case fastgltf::MimeType::JPEG :
				return Pixmap< uint8_t >::Format::Jpeg;

			case fastgltf::MimeType::PNG :
				return Pixmap< uint8_t >::Format::PNG;

			default :
				return Pixmap< uint8_t >::Format::None;
		}
	}

	/* Helper: Extract a CartesianFrame from a glTF node's TRS transform.
	 * ⚠️ The frame is CONJUGATED by the axis flip (M·T·M), never merely mirrored: a hierarchy of
	 * conjugated transforms telescopes into a single mirror at the root, which is what lets every
	 * node keep a PROPER rotation while the mirror lives in the vertex data. */
	static
	CartesianFrame< float >
	extractFrameFromNode (const fastgltf::Node & glTFNode) noexcept
	{
		CartesianFrame< float > frame;

		if ( const auto * trs = std::get_if< fastgltf::TRS >(&glTFNode.transform) )
		{
			/* Rotation: quaternion (x, y, z, w) → rotation matrix. */
			const auto qx = trs->rotation.x();
			const auto qy = trs->rotation.y();
			const auto qz = trs->rotation.z();
			const auto qw = trs->rotation.w();

			const auto xx = qx * qx;
			const auto yy = qy * qy;
			const auto zz = qz * qz;
			const auto xy = qx * qy;
			const auto xz = qx * qz;
			const auto yz = qy * qz;
			const auto wx = qw * qx;
			const auto wy = qw * qy;
			const auto wz = qw * qz;

			/* 4x4 rotation matrix (column-major). */
			Matrix< 4, float > rotMatrix;
			rotMatrix[0]  = 1.0F - (2.0F * (yy + zz));
			rotMatrix[1]  = 2.0F * (xy + wz);
			rotMatrix[2]  = 2.0F * (xz - wy);
			rotMatrix[3]  = 0.0F;
			rotMatrix[4]  = 2.0F * (xy - wz);
			rotMatrix[5]  = 1.0F - (2.0F * (xx + zz));
			rotMatrix[6]  = 2.0F * (yz + wx);
			rotMatrix[7]  = 0.0F;
			rotMatrix[8]  = 2.0F * (xz + wy);
			rotMatrix[9]  = 2.0F * (yz - wx);
			rotMatrix[10] = 1.0F - (2.0F * (xx + yy));
			rotMatrix[11] = 0.0F;
			rotMatrix[12] = 0.0F;
			rotMatrix[13] = 0.0F;
			rotMatrix[14] = 0.0F;
			rotMatrix[15] = 1.0F;

			/* Build frame from rotation matrix + scale, then set position. */
			frame = CartesianFrame< float >(rotMatrix, {trs->scale.x(), trs->scale.y(), trs->scale.z()});
			frame.setPosition({trs->translation.x(), trs->translation.y(), trs->translation.z()});
		}

		if ( true )
		{
			return frame;
		}

		return frame;
	}

	/* Helper: Build a node name from the glTF node. */
	static
	std::string
	buildNodeName (const std::string & prefix, const fastgltf::Node & glTFNode, size_t nodeIndex) noexcept
	{
		std::string name;
		name.reserve(prefix.size() + 6 + glTFNode.name.size());
		name = prefix;
		name += "Node/";

		if ( glTFNode.name.empty() )
		{
			name += std::to_string(nodeIndex);
		}
		else
		{
			name.append(glTFNode.name.data(), glTFNode.name.size());
		}

		return name;
	}

	GLTFLoader::GLTFLoader (Resources::Manager & resources) noexcept
		: m_resources{resources}
	{

	}

	GLTFLoader::~GLTFLoader () = default;

	bool
	GLTFLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		/* Generate a resource prefix from the filename. */
		m_resourcePrefix = "glTF:" + filepath.stem().string() + "/";

		/* stripRootMotion only ever acted on loadAnimationClipsOnly(), which this loader
		 * does not implement (glTF carries its clips inside the asset, so the split-animation
		 * workflow has no glTF equivalent). Say so instead of ignoring the option: a caller
		 * porting FBX code over would otherwise believe the root motion had been stripped. */
		if ( m_options.stripRootMotion )
		{
			Tracer::warning(ClassId, "LoaderOptions::stripRootMotion has no effect on glTF: the option only applies to loadAnimationClipsOnly(), which this loader does not implement.");
		}

		/* Parse the glTF/glb asset. */
		auto gltfFile = fastgltf::GltfDataBuffer::FromPath(filepath);

		if ( gltfFile.error() != fastgltf::Error::None )
		{
			TraceError{ClassId} << "Failed to open '" << filepath << "' !";

			return false;
		}

		const auto parentPath = filepath.parent_path();

		fastgltf::Parser parser(
			fastgltf::Extensions::KHR_materials_clearcoat |
			fastgltf::Extensions::KHR_materials_emissive_strength |
			fastgltf::Extensions::KHR_materials_ior |
			fastgltf::Extensions::KHR_materials_iridescence |
			fastgltf::Extensions::KHR_materials_sheen |
			fastgltf::Extensions::KHR_materials_specular |
			fastgltf::Extensions::KHR_materials_transmission |
			fastgltf::Extensions::KHR_texture_transform |
			fastgltf::Extensions::KHR_materials_anisotropy |
			fastgltf::Extensions::KHR_materials_volume |
			fastgltf::Extensions::KHR_lights_punctual |
			/* NOTE: The three extensions a "compressed glTF" (glTF-Transform, gltfpack) leans on.
			 * They come as a package : such an asset lists all three in `extensionsRequired`, and
			 * fastgltf refuses the whole file with Error::MissingExtensions if a single one is not
			 * declared here — there is no partial support to fall back on.
			 *  - KHR_texture_basisu    : images are KTX2 containers, see loadImages().
			 *  - EXT_meshopt_compression : buffer views are meshopt-encoded, see MeshoptBufferCache.
			 *  - KHR_mesh_quantization : attributes are normalised integers. Nothing to do — fastgltf
			 *    dequantises them on read, and the compensating scale is carried by the node
			 *    transforms extractFrameFromNode() already reads. */
			fastgltf::Extensions::KHR_texture_basisu |
			fastgltf::Extensions::EXT_meshopt_compression |
			fastgltf::Extensions::KHR_mesh_quantization
		);

		constexpr auto options =
			fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::DecomposeNodeMatrices |
			fastgltf::Options::GenerateMeshIndices;

		auto result = filepath.extension() == ".glb" ?
			parser.loadGltfBinary(gltfFile.get(), parentPath, options) :
			parser.loadGltf(gltfFile.get(), parentPath, options);

		if ( result.error() != fastgltf::Error::None )
		{
			TraceError{ClassId} << "Failed to parse '" << filepath << "' : " << fastgltf::getErrorMessage(result.error());

			return false;
		}

		const auto & asset = result.get();

		/* EXT_meshopt_compression working set. Every accessor read below goes through it, and it
		 * is released at the end of this function. */
		m_bufferCache = std::make_unique< MeshoptBufferCache >();

		/* Load pipeline: Images → Materials → Meshes → Skins → Animations → Node descriptors. */
		if ( !this->loadImages(asset, parentPath) )
		{
			Tracer::warning(ClassId, "Some images failed to load, continuing with defaults.");
		}

		if ( !this->loadMaterials(asset) )
		{
			Tracer::warning(ClassId, "Some materials failed to load, continuing with defaults.");
		}

		if ( !this->loadMeshes(asset, output) )
		{
			Tracer::error(ClassId, "Failed to load meshes !");

			m_bufferCache.reset();

			return false;
		}

		if ( !m_options.skipSkinning )
		{
			if ( !asset.skins.empty() )
			{
				this->loadSkins(asset, output);
			}

			if ( !asset.animations.empty() )
			{
				this->loadAnimations(asset, output);
			}

			/* Attach skeletal data to renderables that have associated skins. */
			for ( const auto & [meshIdx, skinIdx] : m_meshToSkinIndex )
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
					skeletalData->setSkeletalData(m_skeletons[skinIdx], m_skins[skinIdx], m_animationClips);
				}
			}
		}

		/* Collect all joint node indices from skins so they can be
		 * skipped during scene hierarchy building. Joint transforms
		 * are handled by the SkeletalAnimator, not by scene nodes. */
		m_skinJointNodeIndices.clear();

		for ( const auto & skin : asset.skins )
		{
			for ( const auto jointIndex : skin.joints )
			{
				m_skinJointNodeIndices.insert(jointIndex);
			}
		}

		/* Everything that reads an accessor is done : drop the decoded meshopt views, which are
		 * the biggest thing this loader ever holds. */
		if ( m_bufferCache->viewCount() > 0 )
		{
			TraceInfo{ClassId} <<
				"Released " << (m_bufferCache->decodedBytes() / 1048576) << " MiB of decoded data from " <<
				m_bufferCache->viewCount() << " meshopt-compressed buffer view(s).";
		}

		m_bufferCache.reset();

		/* Lights and cameras MUST be collected before the node descriptors, which index into
		 * these tables and bound-check against their size. */
		GLTFLoader::loadLights(asset, output);
		GLTFLoader::loadCameras(asset, output);

		/* Build format-agnostic node descriptors. */
		this->buildNodeDescriptors(asset, output);

		/* Populate output with loaded resources. */
		output.skeletons = m_skeletons;
		output.animationClips = m_animationClips;
		output.skinJointNodeIndices = m_skinJointNodeIndices;

		/* Inventory of what the asset actually declared. Without it, "the scene is not lit" is
		 * indistinguishable from "the asset declares no light", and every diagnosis starts blind
		 * — the Sponza glTF, for one, declares 24 lights whose intensity is all zero. */
		TraceInfo{ClassId} <<
			filepath.filename().string() << " loaded: " <<
			output.nodes.size() << " nodes, " <<
			output.meshes.size() << " meshes, " <<
			output.skeletons.size() << " skeletons, " <<
			output.animationClips.size() << " clips, " <<
			output.lights.size() << " lights, " <<
			output.cameras.size() << " cameras.";

		return true;
	}

	bool
	GLTFLoader::loadImages (const fastgltf::Asset & asset, const std::filesystem::path & basePath) noexcept
	{
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
			const auto buildFromBytes = [&] (const std::byte * data, size_t size, fastgltf::MimeType mime) -> bool {
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
					->getOrCreateResource(name, [blob, format = mimeToPixmapFormat(mime)] (auto & resource) {
						Pixmap< uint8_t > pixmap;

						constexpr ReadOptions options{
							.targetChannelMode = TargetChannelMode::RGBA
						};

						if ( !StreamIO::read(*blob, format, pixmap, options) )
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
			};

			const auto loaded = std::visit(fastgltf::visitor{
				/* File-based image (external .gltf). */
				[&] (const fastgltf::sources::URI & uri) -> bool {
					/* Copy path immediately to avoid string_view lifetime issues. */
					const std::filesystem::path fullPath = basePath / std::filesystem::path{std::string{uri.uri.path()}};

					/* A sidecar .ktx2 is read whole and handed to the KTX2 path ; every other
					 * format keeps going through PixelFactory, which picks its decoder from the
					 * file extension. */
					if ( fullPath.extension() == ".ktx2" )
					{
						std::vector< std::byte > content;

						if ( !Base::IO::fileGetContents(fullPath, content) )
						{
							TraceError{ClassId} << "Unable to read the KTX2 file '" << fullPath << "' !";

							return false;
						}

						return buildFromBytes(content.data(), content.size(), fastgltf::MimeType::KTX2);
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
					const auto bytes = m_bufferCache->view(asset, bufferView.bufferViewIndex);

					if ( bytes.empty() )
					{
						return false;
					}

					return buildFromBytes(bytes.data(), bytes.size(), bufferView.mimeType);
				},

				/* Inline base64 data (Array source). */
				[&] (const fastgltf::sources::Array & array) -> bool {
					return buildFromBytes(array.bytes.data(), array.bytes.size_bytes(), array.mimeType);
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
		m_textures.resize(asset.textures.size());

		bool allSuccess = true;

		for ( size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex )
		{
			const auto & glTFMaterial = asset.materials[materialIndex];

			/* ⚠️ Keyed on the INDEX, not on the name: TransmissionTest carries three genuinely
			 * different materials all named 'BlueTransWithMask'. See buildResourceKey(). */
			const auto name = buildResourceKey(m_resourcePrefix, "Material/", {glTFMaterial.name.data(), glTFMaterial.name.size()}, materialIndex);

			/* Resolve a glTF texture index to a Texture2D resource, creating it on demand. */
			const auto resolveTexture = [&] (size_t textureIndex, bool sRGB = false) -> std::shared_ptr< TextureResource::Texture2D > {
				if ( textureIndex >= asset.textures.size() )
				{
					return nullptr;
				}

				/* Return cached texture if already created.
				 * ⚠️ TWO slots per glTF texture, selected by the colour space. The sRGB flag is
				 * baked into the resource at creation and comes from the USAGE, not from the
				 * asset: the same image legitimately serves as an sRGB albedo for one material
				 * and as a linear data map for another. Caching on the texture index alone let
				 * whichever usage resolved FIRST impose its colour space on the other. */
				const auto colourSpaceSlot = sRGB ? 1U : 0U;

				if ( m_textures[textureIndex][colourSpaceSlot] != nullptr )
				{
					return m_textures[textureIndex][colourSpaceSlot];
				}

				const auto & glTFTexture = asset.textures[textureIndex];

				/* NOTE: KHR_texture_basisu hangs the image off its own index, and a texture that
				 * uses it has NO plain imageIndex at all. Reading only imageIndex does not degrade
				 * gracefully on such an asset : every single material comes out untextured. */
				const auto sourceIndex = glTFTexture.imageIndex.has_value() ? glTFTexture.imageIndex : glTFTexture.basisuImageIndex;

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
				texName += sRGB ? "-srgb" : "-data";

				auto texture = m_resources.container< TextureResource::Texture2D >()
					->getOrCreateResource(texName, [image, compressedImage, sRGB, wrapU, wrapV] (auto & textureResource) {
						/* Set sRGB BEFORE load() so the flag is in place when
						 * onDependenciesLoaded() fires and creates the VkImage. The colour space is
						 * decided here, from the usage, for both kinds of source alike. */
						textureResource.enableSRGB(sRGB);

						/* Same contract for the addressing: the sampler is built once, during
						 * creation on hardware, and never revisited. */
						textureResource.setWrapModes(wrapU, wrapV);

						if ( compressedImage != nullptr )
						{
							return textureResource.load(compressedImage);
						}

						return textureResource.load(image);
					});

				m_textures[textureIndex][colourSpaceSlot] = texture;

				return texture;
			};

			const auto & PBRData = glTFMaterial.pbrData;

			/* KHR_texture_transform: per-texture-info UV scale/offset (the tiling of tire
			 * treads, brake discs, car paint flakes...). Ignoring it does not fail — the
			 * texture renders STRETCHED over the whole UV range (measured on CarConcept).
			 * The rotation part and a texCoordIndex override are NOT supported: logged. */
			struct UVTransform
			{
				Vector< 2, float > scale{1.0F, 1.0F};
				Vector< 2, float > offset{0.0F, 0.0F};
				bool present{false};
			};

			const auto readUVTransform = [&glTFMaterial] (const auto & textureInfoOpt) -> UVTransform {
				UVTransform out;

				if ( !textureInfoOpt.has_value() || textureInfoOpt->transform == nullptr )
				{
					return out;
				}

				const auto & transform = *textureInfoOpt->transform;
				out.scale = {transform.uvScale.x(), transform.uvScale.y()};
				out.offset = {transform.uvOffset.x(), transform.uvOffset.y()};
				out.present = true;

				if ( transform.rotation != 0.0F )
				{
					TraceWarning{ClassId} << "Material '" << glTFMaterial.name << "': KHR_texture_transform rotation is not supported, ignored.";
				}

				if ( transform.texCoordIndex.has_value() && *transform.texCoordIndex != 0 )
				{
					TraceWarning{ClassId} << "Material '" << glTFMaterial.name << "': KHR_texture_transform texCoord override is not supported (multi-UV gap), ignored.";
				}

				return out;
			};

			const auto albedoUVTransform = readUVTransform(PBRData.baseColorTexture);
			const auto metallicRoughnessUVTransform = readUVTransform(PBRData.metallicRoughnessTexture);
			const auto normalUVTransform = readUVTransform(glTFMaterial.normalTexture);
			const auto aoUVTransform = readUVTransform(glTFMaterial.occlusionTexture);
			const auto emissiveUVTransform = readUVTransform(glTFMaterial.emissiveTexture);

			/* Albedo (sRGB: perceptual color data). */
			auto albedoTex = PBRData.baseColorTexture.has_value()
				? resolveTexture(PBRData.baseColorTexture->textureIndex, true) : nullptr;

			const auto & bc = PBRData.baseColorFactor;
			Color< float > albedoColor{
				static_cast< float >(bc[0]),
				static_cast< float >(bc[1]),
				static_cast< float >(bc[2]),
				static_cast< float >(bc[3])
			};

			/* Metallic-Roughness. */
			auto metallicRoughnessTex = PBRData.metallicRoughnessTexture.has_value()
				? resolveTexture(PBRData.metallicRoughnessTexture->textureIndex) : nullptr;

			const auto roughnessFactor = static_cast< float >(PBRData.roughnessFactor);
			const auto metallicFactor = static_cast< float >(PBRData.metallicFactor);

			/* Normal. */
			auto normalTex = glTFMaterial.normalTexture.has_value()
				? resolveTexture(glTFMaterial.normalTexture->textureIndex) : nullptr;

			const auto normalScale = glTFMaterial.normalTexture.has_value()
				? static_cast< float >(glTFMaterial.normalTexture->scale) : 0.0F;

			/* Ambient occlusion. */
			auto aoTex = glTFMaterial.occlusionTexture.has_value()
				? resolveTexture(glTFMaterial.occlusionTexture->textureIndex) : nullptr;

			const auto aoStrength = glTFMaterial.occlusionTexture.has_value()
				? static_cast< float >(glTFMaterial.occlusionTexture->strength) : 0.0F;

			/* Emissive (sRGB: perceptual color data). */
			auto emissiveTex = glTFMaterial.emissiveTexture.has_value()
				? resolveTexture(glTFMaterial.emissiveTexture->textureIndex, true) : nullptr;

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
			 * photometric renderer. 2000 nits matches the anchor already used by the WAD
			 * materializer. ⚠️ The VALUE and the PLACE (loader vs material vs shader) are an
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

			const bool hasEmissiveColor = glTFMaterial.emissiveFactor[0] > 0.0F
				|| glTFMaterial.emissiveFactor[1] > 0.0F
				|| glTFMaterial.emissiveFactor[2] > 0.0F;

			/* Clear coat (KHR_materials_clearcoat). */
			float clearcoatFactor = 0.0F;
			float clearcoatRoughness = 0.0F;

			if ( glTFMaterial.clearcoat != nullptr && glTFMaterial.clearcoat->clearcoatFactor > 0.0F )
			{
				clearcoatFactor = static_cast< float >(glTFMaterial.clearcoat->clearcoatFactor);
				clearcoatRoughness = static_cast< float >(glTFMaterial.clearcoat->clearcoatRoughnessFactor);
			}

			/* Sheen (KHR_materials_sheen). */
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

			/* Iridescence (KHR_materials_iridescence). */
			float iridescenceFactor = 0.0F;

			if ( glTFMaterial.iridescence != nullptr && glTFMaterial.iridescence->iridescenceFactor > 0.0F )
			{
				iridescenceFactor = static_cast< float >(glTFMaterial.iridescence->iridescenceFactor);
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
			Color< float > specularColor{1.0F, 1.0F, 1.0F, 1.0F};

			if ( glTFMaterial.specular != nullptr )
			{
				specularFactor = static_cast< float >(glTFMaterial.specular->specularFactor);

				const auto & scf = glTFMaterial.specular->specularColorFactor;

				specularColor = Color< float >{
					static_cast< float >(scf[0]),
					static_cast< float >(scf[1]),
					static_cast< float >(scf[2]),
					1.0F
				};

			}

			/* The two specular maps. ⚠️ `specularTexture` carries its value in the **A channel**
			 * and is LINEAR; `specularColorTexture` carries the F0 tint in RGB and is **sRGB** —
			 * hence the `true` on the second `resolveTexture()` and not on the first. Both
			 * MULTIPLY their respective factor, which the material's component codegen folds in. */
			auto specularTex = ( glTFMaterial.specular != nullptr && glTFMaterial.specular->specularTexture.has_value() )
				? resolveTexture(glTFMaterial.specular->specularTexture->textureIndex) : nullptr;

			auto specularColorTex = ( glTFMaterial.specular != nullptr && glTFMaterial.specular->specularColorTexture.has_value() )
				? resolveTexture(glTFMaterial.specular->specularColorTexture->textureIndex, true) : nullptr;

			/* ⚠️ Neither map gets a KHR_texture_transform slot: the material UBO carries six UV
			 * transforms (albedo, roughness, metalness, normal, AO, emissive) and the specular
			 * component types are not among them, so `transformedTexCoords()` falls back to the
			 * plain coordinates. An asset that transforms its specular UVs is silently untransformed
			 * — none in the conformance bench does. Read before assuming it is supported. */
			if ( ( specularTex != nullptr && readUVTransform(glTFMaterial.specular->specularTexture).present )
			  || ( specularColorTex != nullptr && readUVTransform(glTFMaterial.specular->specularColorTexture).present ) )
			{
				TraceWarning{ClassId} << "Material '" << glTFMaterial.name << "': KHR_texture_transform on a KHR_materials_specular texture is not supported (no UV transform slot), ignored.";
			}

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

			/* Alpha mode (OPAQUE, MASK, BLEND). */
			const bool isAlphaBlend = glTFMaterial.alphaMode == fastgltf::AlphaMode::Blend;
			const bool isAlphaMask = glTFMaterial.alphaMode == fastgltf::AlphaMode::Mask;
			const auto alphaCutoff = static_cast< float >(glTFMaterial.alphaCutoff);

			/* Async material creation — lambda is fully self-contained, no this/reference captures.
			 * The lambda is generic so the same configuration code path applies whether the
			 * loader produces a StandardResource or a StandardResource (cross-material aliases
			 * convert PBR factors to Phong/Blinn parameters when targeting Standard). */
			auto configure = [
					albedoTex = std::move(albedoTex), albedoColor,
					metallicRoughnessTex = std::move(metallicRoughnessTex), roughnessFactor, metallicFactor,
					normalTex = std::move(normalTex), normalScale,
					aoTex = std::move(aoTex), aoStrength,
					albedoUVTransform, metallicRoughnessUVTransform, normalUVTransform, aoUVTransform, emissiveUVTransform,
					emissiveTex = std::move(emissiveTex), emissiveStrength, emissiveColor, hasEmissiveColor,
					clearcoatFactor, clearcoatRoughness,
					sheenColor, sheenRoughness,
					transmissionFactor,
					iridescenceFactor,
					specularFactor, specularColor, materialIOR,
					specularTex = std::move(specularTex), specularColorTex = std::move(specularColorTex),
					environmentReflectionIntensity = m_options.environmentReflectionIntensity,
					isAlphaBlend, isAlphaMask, alphaCutoff
				] (auto & materialResource) {
					/* Albedo. */
					/* A base-colour texture and a base-colour factor MULTIPLY — that is what both
					 * glTF (baseColorFactor) and FBX (base_color) specify. Setting the component to
					 * the texture and dropping the factor tints nothing and silently loses the
					 * factor's alpha; the colour goes to the material's tint slot instead. */
					if ( albedoTex != nullptr )
					{
						materialResource.setAlbedoComponent(albedoTex);
						materialResource.setAlbedoColor(albedoColor);

						if ( albedoUVTransform.present )
						{
							materialResource.setComponentUVWTransform(ComponentType::Albedo, albedoUVTransform.scale, albedoUVTransform.offset);
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
					if ( metallicRoughnessTex != nullptr )
					{
						materialResource.setRoughnessComponent(metallicRoughnessTex, roughnessFactor, false, Base::PixelFactory::Channel::Green);
						materialResource.setMetalnessComponent(metallicRoughnessTex, metallicFactor, Base::PixelFactory::Channel::Blue);

						/* ONE glTF texture info, TWO engine components: same transform on both. */
						if ( metallicRoughnessUVTransform.present )
						{
							materialResource.setComponentUVWTransform(ComponentType::Roughness, metallicRoughnessUVTransform.scale, metallicRoughnessUVTransform.offset);
							materialResource.setComponentUVWTransform(ComponentType::Metalness, metallicRoughnessUVTransform.scale, metallicRoughnessUVTransform.offset);
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
					if ( normalTex != nullptr )
					{
						materialResource.setNormalComponent(normalTex, normalScale);

						if ( normalUVTransform.present )
						{
							materialResource.setComponentUVWTransform(ComponentType::Normal, normalUVTransform.scale, normalUVTransform.offset);
						}
					}

					/* Ambient occlusion. */
					if ( aoTex != nullptr )
					{
						materialResource.setAmbientOcclusionComponent(aoTex, aoStrength);

						if ( aoUVTransform.present )
						{
							materialResource.setComponentUVWTransform(ComponentType::AmbientOcclusion, aoUVTransform.scale, aoUVTransform.offset);
						}
					}

					/* Emissive. */
					if ( emissiveTex != nullptr )
					{
						materialResource.setAutoIlluminationComponent(emissiveTex, emissiveStrength);

						if ( emissiveUVTransform.present )
						{
							materialResource.setComponentUVWTransform(ComponentType::AutoIllumination, emissiveUVTransform.scale, emissiveUVTransform.offset);
						}
					}
					else if ( hasEmissiveColor )
					{
						materialResource.setAutoIlluminationComponent(emissiveColor, emissiveStrength);
					}

					/* Clear coat (KHR_materials_clearcoat). */
					if ( clearcoatFactor > 0.0F )
					{
						materialResource.setClearCoatComponent(clearcoatFactor, clearcoatRoughness);
					}

					/* Sheen (KHR_materials_sheen). */
					if ( sheenRoughness > 0.0F || sheenColor.red() > 0.0F || sheenColor.green() > 0.0F || sheenColor.blue() > 0.0F )
					{
						materialResource.setSheenComponent(sheenColor, sheenRoughness);
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

					/* Iridescence (KHR_materials_iridescence). */
					if ( iridescenceFactor > 0.0F )
					{
						materialResource.setIridescenceComponent(iridescenceFactor);
					}

					/* Specular (KHR_materials_specular) and IOR (KHR_materials_ior).
					 * Both are ALWAYS applied, never gated on a "is it non-default" test: they
					 * scale and tint the dielectric F0 of every material, and their defaults are
					 * the identity (factor 1, white, IOR 1.5 == glTF's own default). Gating them
					 * would leave a material that deliberately declares the default carrying
					 * whatever the resource was constructed with. */
					if ( specularTex != nullptr )
					{
						materialResource.setSpecularComponent(specularTex, specularFactor);
					}
					else
					{
						materialResource.setSpecularFactor(specularFactor);
					}

					if ( specularColorTex != nullptr )
					{
						materialResource.setSpecularColorComponent(specularColorTex, specularColor);
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
		}

		return allSuccess;
	}

	bool
	GLTFLoader::loadMeshes (const fastgltf::Asset & asset, SceneData & output) noexcept
	{
		m_meshes.resize(asset.meshes.size());
		m_shapes.resize(asset.meshes.size());

		/* Every accessor read below goes through the meshopt cache : with EXT_meshopt_compression
		 * the buffer views hold encoded blocks, and reading them raw yields silent garbage
		 * geometry rather than an error. */
		const MeshoptBufferAdapter adapter{m_bufferCache.get()};

		bool allSuccess = true;

		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			return m_resources.container< Material::StandardResource >()->getDefaultResource();
		};

		/* ⚠️ Geometry and mesh resources are cached BY NAME and their content depends on the axis
		 * flip, so the flip must be part of the key. Without it, loading the same asset twice with
		 * different flags would silently hand the first variant to the second caller. Images,
		 * textures and materials are deliberately NOT keyed this way: the flip does not touch them,
		 * and duplicating 84 images per variant would cost memory for nothing. */
		const auto flipKey = std::string{};

		for ( size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex )
		{
			const auto & glTFMesh = asset.meshes[meshIndex];

			/* ⚠️⚠️ Keyed on the INDEX, not on the name. This is the defect that made the whole
			 * ClearCoatTest grid render material 0's red: eighteen meshes named
			 * 'ClearCoatSampleMesh', eighteen different materials, ONE renderable resource
			 * handed to all of them — geometry and material alike. See buildResourceKey(). */
			const std::string_view glTFMeshName{glTFMesh.name.data(), glTFMesh.name.size()};

			const auto geoName = buildResourceKey(m_resourcePrefix + flipKey, "Geometry/", glTFMeshName, meshIndex);
			const auto meshName = buildResourceKey(m_resourcePrefix + flipKey, "Mesh/", glTFMeshName, meshIndex);

			/* Phase 1: Build shape using direct vector access (OBJ-style, no per-element reallocation).
			 * First pass: count total vertices and triangles to pre-allocate. */
			auto shape = std::make_shared< Shape< float > >();

			uint32_t totalVertexCount = 0;
			uint32_t totalTriangleCount = 0;

			for ( const auto & primitive : glTFMesh.primitives )
			{
				if ( primitive.type != fastgltf::PrimitiveType::Triangles )
				{
					continue;
				}

				const auto * const positionIt = primitive.findAttribute("POSITION");

				if ( positionIt == primitive.attributes.end() )
				{
					continue;
				}

				totalVertexCount += static_cast< uint32_t >(asset.accessors[positionIt->accessorIndex].count);

				if ( primitive.indicesAccessor.has_value() )
				{
					totalTriangleCount += static_cast< uint32_t >(asset.accessors[primitive.indicesAccessor.value()].count / 3);
				}
			}

			if ( totalVertexCount == 0 || totalTriangleCount == 0 )
			{
				TraceWarning{ClassId} << "Mesh '" << glTFMesh.name << "' has no valid triangle primitives, using default.";

				m_meshes[meshIndex] = m_resources.container< Renderable::MeshResource >()->getDefaultResource();

				allSuccess = false;

				continue;
			}

			/* Check if any primitive provides normals. */
			bool hasNormals = false;

			for ( const auto & primitive : glTFMesh.primitives )
			{
				if ( primitive.findAttribute("NORMAL") != primitive.attributes.end() )
				{
					hasNormals = true;

					break;
				}
			}

			/* Authored tangents (glTF `TANGENT`, a vec4 whose W is the bitangent handedness).
			 *
			 * ⚠️ This is ALL-OR-NOTHING per mesh, and deliberately so: the tangent computation
			 * below runs over the whole shape and would OVERWRITE the authored tangents of the
			 * primitives that did supply them. A mesh where only some primitives carry TANGENT
			 * therefore falls back to computing every one, and says so — silently keeping half
			 * the authored tangents and recomputing the other half is the worse outcome, because
			 * the two halves would disagree at the seam. */
			bool allPrimitivesHaveTangents = !glTFMesh.primitives.empty();
			bool somePrimitivesHaveTangents = false;

			for ( const auto & primitive : glTFMesh.primitives )
			{
				if ( primitive.findAttribute("TANGENT") != primitive.attributes.end() )
				{
					somePrimitivesHaveTangents = true;
				}
				else
				{
					allPrimitivesHaveTangents = false;
				}
			}

			if ( somePrimitivesHaveTangents && !allPrimitivesHaveTangents )
			{
				TraceWarning{ClassId} << "Mesh '" << glTFMesh.name << "': only some primitives declare TANGENT — recomputing tangents for the whole mesh, the authored ones are discarded.";
			}

			/* Second pass: fill vertices and triangles directly into pre-allocated vectors. */
			const bool buildSuccess = shape->build([&] (auto & groups, auto & vertices, auto & triangles) {
				vertices.resize(totalVertexCount);
				triangles.reserve(totalTriangleCount);

				uint32_t globalVertexOffset = 0;
				bool firstPrimitive = true;

				for ( const auto & glTFPrimitive : glTFMesh.primitives )
				{
					if ( glTFPrimitive.type != fastgltf::PrimitiveType::Triangles )
					{
						continue;
					}

					const auto * const positionIt = glTFPrimitive.findAttribute("POSITION");

					if ( positionIt == glTFPrimitive.attributes.end() )
					{
						continue;
					}

					/* Each primitive after the first starts a new sub-geometry group. */
					if ( !firstPrimitive )
					{
						groups.emplace_back(static_cast< uint32_t >(triangles.size()), 0);
					}

					firstPrimitive = false;

					const auto & posAccessor = asset.accessors[positionIt->accessorIndex];
					const auto vertexCount = static_cast< uint32_t >(posAccessor.count);

					/* Read positions directly into vertex array. */
					{
						size_t index = 0;

						fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, posAccessor, [&] (const fastgltf::math::fvec3 & v) {
							vertices[globalVertexOffset + index].setPosition(Vector< 3, float >{v.x() * m_options.uniformScale, v.y() * m_options.uniformScale, v.z() * m_options.uniformScale});
							index++;
						}, adapter);
					}

					/* Read normals directly into vertex array. */
					const auto * const normalIt = glTFPrimitive.findAttribute("NORMAL");

					if ( normalIt != glTFPrimitive.attributes.end() )
					{
						const auto & normAccessor = asset.accessors[normalIt->accessorIndex];
						size_t index = 0;

						fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, normAccessor, [&] (const fastgltf::math::fvec3 & v) {
							vertices[globalVertexOffset + index].setNormal(Vector< 3, float >{v.x(), v.y(), v.z()});
							index++;
						}, adapter);
					}

					/* Read authored tangents directly into vertex array.
					 *
					 * ⚠️⚠️ `TANGENT` is a **vec4**, and its W is the BITANGENT HANDEDNESS (±1),
					 * not a homogeneous coordinate. It is the only thing that distinguishes a
					 * mirrored UV island from a plain one: the bitangent is
					 * `cross(normal, tangent) * w`. Dropping W — which is what happened until
					 * 2026-08-28, when the accessor was not read at all — lights the normal map
					 * BACKWARDS on every mirrored island. `ShapeVertex::setTangent(Vector<4>)`
					 * stores the handedness and `biNormal()` applies it. */
					if ( allPrimitivesHaveTangents )
					{
						const auto * const tangentIt = glTFPrimitive.findAttribute("TANGENT");

						if ( tangentIt != glTFPrimitive.attributes.end() )
						{
							const auto & tangentAccessor = asset.accessors[tangentIt->accessorIndex];
							size_t index = 0;

							fastgltf::iterateAccessor< fastgltf::math::fvec4 >(asset, tangentAccessor, [&] (const fastgltf::math::fvec4 & v) {
								vertices[globalVertexOffset + index].setTangent(Vector< 4, float >{v.x(), v.y(), v.z(), v.w()});
								index++;
							}, adapter);
						}
					}

					/* Read texture coordinates directly into vertex array. */
					const auto * const textureCoordinatesIt = glTFPrimitive.findAttribute("TEXCOORD_0");

					if ( textureCoordinatesIt != glTFPrimitive.attributes.end() )
					{
						const auto & uvAccessor = asset.accessors[textureCoordinatesIt->accessorIndex];
						size_t index = 0;

						fastgltf::iterateAccessor< fastgltf::math::fvec2 >(asset, uvAccessor, [&] (const fastgltf::math::fvec2 & v) {
							vertices[globalVertexOffset + index].setTextureCoordinates(Vector< 3, float >{v.x(), v.y(), 0.0F});
							index++;
						}, adapter);
					}

					if ( !m_options.skipSkinning )
					{
						/* Read bone joint indices (JOINTS_0) for skeletal animation. */
						const auto * const jointsIt = glTFPrimitive.findAttribute("JOINTS_0");

						if ( jointsIt != glTFPrimitive.attributes.end() )
						{
							const auto & jointsAccessor = asset.accessors[jointsIt->accessorIndex];
							size_t index = 0;

							fastgltf::iterateAccessor< fastgltf::math::u16vec4 >(asset, jointsAccessor, [&] (const fastgltf::math::u16vec4 & v) {
								vertices[globalVertexOffset + index].setInfluences(
									static_cast< int32_t >(v.x()),
									static_cast< int32_t >(v.y()),
									static_cast< int32_t >(v.z()),
									static_cast< int32_t >(v.w())
								);

								index++;
							}, adapter);
						}

						/* Read bone weights (WEIGHTS_0) for skeletal animation. */
						const auto * const weightsIt = glTFPrimitive.findAttribute("WEIGHTS_0");

						if ( weightsIt != glTFPrimitive.attributes.end() )
						{
							const auto & weightsAccessor = asset.accessors[weightsIt->accessorIndex];
							size_t index = 0;

							fastgltf::iterateAccessor< fastgltf::math::fvec4 >(asset, weightsAccessor, [&] (const fastgltf::math::fvec4 & v) {
								vertices[globalVertexOffset + index].setWeights(v.x(), v.y(), v.z(), v.w());
								index++;
							}, adapter);
						}
					}

					/* Read indices and build triangles directly. */
					if ( glTFPrimitive.indicesAccessor.has_value() )
					{
						const auto & indexAccessor = asset.accessors[glTFPrimitive.indicesAccessor.value()];
						const auto primTriangleCount = static_cast< uint32_t >(indexAccessor.count / 3);

						/* Stream indices directly into triangles, in the order the asset authored them.
						 *
						 * A swap of indices 1 and 2 used to sit here to compensate the engine's
						 * ORIENTATION-REVERSING projection. The Y-up convention flip removed that mirror,
						 * so the authored winding is now the correct one and is kept untouched. */

						std::array< uint32_t, 3 > triangleBuffer{};
						uint32_t triangleSlot = 0;

						fastgltf::iterateAccessor< uint32_t >(asset, indexAccessor, [&] (uint32_t value) {
							triangleBuffer.at(triangleSlot++) = globalVertexOffset + value;

							if ( triangleSlot == 3 )
							{
								triangles.emplace_back(triangleBuffer[0], triangleBuffer[1], triangleBuffer[2]);

								triangleSlot = 0;
							}
						}, adapter);

						/* Track triangles per group. */
						if ( !groups.empty() )
						{
							groups.back().second += primTriangleCount;
						}
					}

					globalVertexOffset += vertexCount;
				}

				return true;
			}, true); /* textureCoordinatesDeclared = true */

			if ( !buildSuccess || !shape->isValid() )
			{
				TraceWarning{ClassId} << "Generated shape is invalid for mesh '" << glTFMesh.name << "', using default.";

				m_meshes[meshIndex] = m_resources.container< Renderable::MeshResource >()->getDefaultResource();

				allSuccess = false;

				continue;
			}

			/* Generate normals from geometry when the glTF mesh does not provide them. */
			if ( !hasNormals )
			{
				/* No inversion: nothing reflects the vertex data any more (the winding swap that
				 * used to force it is gone with the orientation-reversing projection). */
				shape->computeTriangleNormal(false);
				shape->computeVertexNormal();

				TraceDebug{ClassId} << "Generated smooth normals for mesh '" << glTFMesh.name << "' (not provided by glTF).";
			}
			else
			{
				shape->declareNormalsAvailable();
			}

			/* Detect if the shape has skeletal bone influences. */
			uint32_t geometryFlags = EnableTangentSpace | EnablePrimaryTextureCoordinates;

			if ( !shape->vertices().empty() && shape->vertices()[0].influences()[0] >= 0 )
			{
				geometryFlags |= EnableInfluence | EnableWeight;
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

					if ( materialIndex < m_materials.size() && m_materials[materialIndex] != nullptr )
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
					->getOrCreateResource(meshName, [geometry, singleMaterial = std::move(singleMaterial), singleRasterization] (auto & meshResource) {
						return meshResource.load(geometry, singleMaterial, singleRasterization);
					});
			}
			else
			{
				mesh = m_resources.container< Renderable::MultiLayerMeshResource >()
					->getOrCreateResource(meshName, [geometry, materialList = std::move(materialList), rasterizationList = std::move(rasterizationList)] (auto & meshResource) {
						return meshResource.load(geometry, materialList, rasterizationList);
					});
			}

			m_meshes[meshIndex] = mesh;
			m_shapes[meshIndex] = shape;

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

	void
	GLTFLoader::loadSkins (const fastgltf::Asset & asset, SceneData & /*output*/) noexcept
	{
		if ( asset.skins.empty() )
		{
			return;
		}

		const MeshoptBufferAdapter adapter{m_bufferCache.get()};

		/* ⚠️ The rig follows the mesh it deforms, or the skin slides off the geometry: bind-pose
		 * translations, bind-pose rotations and inverse bind matrices all go through the same
		 * conversion. */

		/* Build a node-index → parent-node-index lookup from the node tree. */
		std::vector< int32_t > nodeParents(asset.nodes.size(), Base::Animation::NoParent);

		for ( size_t nodeIdx = 0; nodeIdx < asset.nodes.size(); ++nodeIdx )
		{
			for ( const auto childIdx : asset.nodes[nodeIdx].children )
			{
				nodeParents[childIdx] = static_cast< int32_t >(nodeIdx);
			}
		}

		for ( size_t skinIndex = 0; skinIndex < asset.skins.size(); ++skinIndex )
		{
			const auto & glTFSkin = asset.skins[skinIndex];
			const auto jointCount = glTFSkin.joints.size();

			if ( jointCount == 0 )
			{
				continue;
			}

			/* Build a fast lookup: GLTF node index → skin-local joint index. */
			std::unordered_map< size_t, int32_t > nodeToJointIndex;
			nodeToJointIndex.reserve(jointCount);

			for ( size_t j = 0; j < jointCount; ++j )
			{
				nodeToJointIndex[glTFSkin.joints[j]] = static_cast< int32_t >(j);
			}

			/* Read inverse bind matrices from the accessor (if provided). */
			std::vector< Matrix< 4, float > > inverseBindMatrices(jointCount);

			if ( glTFSkin.inverseBindMatrices.has_value() )
			{
				const auto & ibmAccessor = asset.accessors[glTFSkin.inverseBindMatrices.value()];
				size_t index = 0;

				fastgltf::iterateAccessor< fastgltf::math::fmat4x4 >(asset, ibmAccessor, [&] (const fastgltf::math::fmat4x4 & m) {
					std::array< float, 16 > data{};

					for ( size_t col = 0; col < 4; ++col )
					{
						for ( size_t row = 0; row < 4; ++row )
						{
							data.at((col * 4) + row) = m.col(col)[row];
						}
					}

					auto ibm = Matrix< 4, float >{data};

					/* Scale the translation column to keep the binding math coherent with the
					 * scaled vertex positions and scaled joint TRS translations. The linear part
					 * (rotation + uniform 1x1 scale) is unaffected by uniform scaling around the
					 * origin, so only the translation column needs it. */
					ibm[M4x4Col3Row0] *= m_options.uniformScale;
					ibm[M4x4Col3Row1] *= m_options.uniformScale;
					ibm[M4x4Col3Row2] *= m_options.uniformScale;

					/* ⚠️ Conjugate the WHOLE matrix (M·IBM·M), linear part included — not just the
					 * translation column as the uniform scale above does. Skinning stays coherent
					 * because the joint world matrices are conjugated the same way, so the two M
					 * cancel: (M·W·M)·(M·IBM·M)·(M·v) = M·(W·IBM·v). */
					inverseBindMatrices[index] = ibm;
					index++;
				}, adapter);
			}
			else
			{
				for ( auto & ibm : inverseBindMatrices )
				{
					ibm = Matrix< 4, float >{};
				}
			}

			/* Build Joint array. For each skin joint, find its parent within the skin. */
			std::vector< Joint< float > > engineJoints(jointCount);

			for ( size_t j = 0; j < jointCount; ++j )
			{
				const auto glTFNodeIndex = glTFSkin.joints[j];
				const auto & glTFNode = asset.nodes[glTFNodeIndex];

				engineJoints[j].name = std::string{glTFNode.name};
				engineJoints[j].inverseBindMatrix = inverseBindMatrices[j];

				/* Find parent joint: walk up the node tree until we find a node that is in this skin. */
				engineJoints[j].parentIndex = NoParent;
				auto parentNodeIdx = nodeParents[glTFNodeIndex];

				while ( parentNodeIdx != NoParent )
				{
					const auto it = nodeToJointIndex.find(static_cast< size_t >(parentNodeIdx));

					if ( it != nodeToJointIndex.end() )
					{
						engineJoints[j].parentIndex = it->second;
						break;
					}

					parentNodeIdx = nodeParents[static_cast< size_t >(parentNodeIdx)];
				}

				/* Extract local TRS from the node. GLTF stores parent-relative transforms. */
				if ( const auto * trs = std::get_if< fastgltf::TRS >(&glTFNode.transform) )
				{
					engineJoints[j].translation = Vector< 3, float >{
						trs->translation.x() * m_options.uniformScale,
						trs->translation.y() * m_options.uniformScale,
						trs->translation.z() * m_options.uniformScale};
					engineJoints[j].rotation = Quaternion< float >{trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()};
					/* NOTE: A scale is invariant under the flip — M·diag(s)·M = diag(s). */
					engineJoints[j].scale = {trs->scale.x(), trs->scale.y(), trs->scale.z()};
				}
			}

			Skeleton< float > skeleton{std::move(engineJoints)};

			/* Build Skin: skin-local indices map 1:1. */
			std::vector< int32_t > skinJointIndices(jointCount);

			for ( size_t j = 0; j < jointCount; ++j )
			{
				skinJointIndices[j] = static_cast< int32_t >(j);
			}

			Skin< float > skin{std::move(skinJointIndices), std::move(inverseBindMatrices)};

			/* Register the skeleton as a managed resource. */
			const auto skinName = glTFSkin.name.empty() ?
				"skin_" + std::to_string(skinIndex) :
				std::string{glTFSkin.name};

			auto skeletonResource = m_resources.container< Animations::SkeletonResource >()
				->getOrCreateResourceSync(m_resourcePrefix + std::string{} + "/skeleton/" + skinName, [&skeleton] (auto & resource) {
					return resource.load(std::move(skeleton));
				});

			m_skeletons.push_back(std::move(skeletonResource));
			m_skins.push_back(std::move(skin));

			/* Record which meshes reference this skin for later association with renderables. */
			for ( const auto & node : asset.nodes )
			{
				if ( node.skinIndex.has_value() && node.skinIndex.value() == skinIndex && node.meshIndex.has_value() )
				{
					m_meshToSkinIndex[node.meshIndex.value()] = skinIndex;
				}
			}
		}
	}

	void
	GLTFLoader::loadAnimations (const fastgltf::Asset & asset, SceneData & /*output*/) noexcept
	{
		if ( asset.animations.empty() )
		{
			return;
		}

		const MeshoptBufferAdapter adapter{m_bufferCache.get()};

		/* ⚠️ The clips are mirrored with the very same flip as the bind pose they animate. A rig
		 * whose bind pose is mirrored but whose clips are not snaps to the unmirrored pose on the
		 * first animated frame — the classic symptom of a half-applied conversion. */

		/* For mapping node indices to joint indices, we need the skin context. */
		std::unordered_map< size_t, int32_t > nodeToJointIndex;

		if ( !asset.skins.empty() )
		{
			const auto & skin = asset.skins[0];

			for ( size_t j = 0; j < skin.joints.size(); ++j )
			{
				nodeToJointIndex[skin.joints[j]] = static_cast< int32_t >(j);
			}
		}

		m_animationClips.reserve(asset.animations.size());

		for ( const auto & glTFAnim : asset.animations )
		{
			std::vector< AnimationChannel< float > > channels;
			channels.reserve(glTFAnim.channels.size());

			for ( const auto & glTFChannel : glTFAnim.channels )
			{
				if ( !glTFChannel.nodeIndex.has_value() )
				{
					continue;
				}

				/* Skip morph target weights — not supported yet. */
				if ( glTFChannel.path == fastgltf::AnimationPath::Weights )
				{
					continue;
				}

				/* Map GLTF node index to joint index in the skeleton. */
				const auto nodeIdx = glTFChannel.nodeIndex.value();
				const auto it = nodeToJointIndex.find(nodeIdx);

				if ( it == nodeToJointIndex.end() )
				{
					continue;
				}

				const auto jointIndex = it->second;
				const auto & glTFSampler = glTFAnim.samplers[glTFChannel.samplerIndex];

				/* Read keyframe timestamps. */
				const auto & inputAccessor = asset.accessors[glTFSampler.inputAccessor];
				std::vector< float > timestamps;
				timestamps.reserve(inputAccessor.count);

				fastgltf::iterateAccessor< float >(asset, inputAccessor, [&] (float t) {
					timestamps.push_back(t);
				}, adapter);

				/* Map interpolation type. */
				ChannelInterpolation interp = ChannelInterpolation::Linear;

				switch ( glTFSampler.interpolation )
				{
					case fastgltf::AnimationInterpolation::Step :
						interp = ChannelInterpolation::Step;
						break;

					case fastgltf::AnimationInterpolation::CubicSpline :
						interp = ChannelInterpolation::CubicSpline;
						break;

					default :
						break;
				}

				AnimationChannel< float > channel;
				channel.jointIndex = jointIndex;
				channel.interpolation = interp;

				const auto & outputAccessor = asset.accessors[glTFSampler.outputAccessor];

				/* GLTF CUBICSPLINE packs THREE values per keyframe in the output accessor —
				 * in-tangent, value, out-tangent — where STEP and LINEAR pack one. Reading that
				 * accessor flat and indexing it against the timestamps turns the leading tangents
				 * into keyframe values: the clip then plays wrong, with no diagnostic whatsoever.
				 * This stride is the whole difference. */
				const size_t valuesPerKeyFrame = interp == ChannelInterpolation::CubicSpline ? 3 : 1;

				switch ( glTFChannel.path )
				{
					case fastgltf::AnimationPath::Translation :
					case fastgltf::AnimationPath::Scale :
					{
						const bool isTranslation = glTFChannel.path == fastgltf::AnimationPath::Translation;

						channel.target = isTranslation ? ChannelTarget::Translation : ChannelTarget::Scale;

						/* A translation is a LENGTH, so it follows uniformScale, and so do its tangents,
						 * which are lengths per second. A scale is a RATIO and never does. */
						const auto valueScale = isTranslation ? m_options.uniformScale : 1.0F;

						/* ⚠️ Same asymmetry for the axis flip: a translation is mirrored, a scale is
						 * invariant (M·diag(s)·M = diag(s)). Mirroring the scale track would make
						 * the rig turn inside-out on the first animated frame. Tangents live in this
						 * very array and are mirrored with the values, as they must be. */
						std::vector< Vector< 3, float > > values;
						values.reserve(timestamps.size() * valuesPerKeyFrame);

						fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, outputAccessor, [&] (const fastgltf::math::fvec3 & v) {
							const Vector< 3, float > scaled{v.x() * valueScale, v.y() * valueScale, v.z() * valueScale};

							values.push_back(isTranslation ? scaled : scaled);
						}, adapter);

						const auto keyFrameCount = std::min(timestamps.size(), values.size() / valuesPerKeyFrame);

						channel.vectorKeyFrames.resize(keyFrameCount);

						for ( size_t index = 0; index < keyFrameCount; ++index )
						{
							auto & keyFrame = channel.vectorKeyFrames[index];

							keyFrame.time = timestamps[index];

							if ( valuesPerKeyFrame == 3 )
							{
								keyFrame.inTangent = values[index * 3];
								keyFrame.value = values[(index * 3) + 1];
								keyFrame.outTangent = values[(index * 3) + 2];
							}
							else
							{
								keyFrame.value = values[index];
							}
						}

						break;
					}

					case fastgltf::AnimationPath::Rotation :
					{
						channel.target = ChannelTarget::Rotation;

						std::vector< Quaternion< float > > values;
						values.reserve(timestamps.size() * valuesPerKeyFrame);

						/* ⚠️ Every rotation keyframe is CONJUGATED, tangents included. The conjugation
						 * carries the det(M) angle inversion, without which the bind pose would look
						 * right while the animation played backwards. */
						fastgltf::iterateAccessor< fastgltf::math::fvec4 >(asset, outputAccessor, [&] (const fastgltf::math::fvec4 & v) {
							values.push_back(Quaternion< float >{v.x(), v.y(), v.z(), v.w()});
						}, adapter);

						const auto keyFrameCount = std::min(timestamps.size(), values.size() / valuesPerKeyFrame);

						channel.quaternionKeyFrames.resize(keyFrameCount);

						for ( size_t index = 0; index < keyFrameCount; ++index )
						{
							auto & keyFrame = channel.quaternionKeyFrames[index];

							keyFrame.time = timestamps[index];

							if ( valuesPerKeyFrame == 3 )
							{
								keyFrame.inTangent = values[index * 3];
								keyFrame.value = values[(index * 3) + 1];
								keyFrame.outTangent = values[(index * 3) + 2];
							}
							else
							{
								keyFrame.value = values[index];
							}
						}

						break;
					}

					default :
						continue;
				}
				channels.push_back(std::move(channel));
			}

			if ( !channels.empty() )
			{
				AnimationClip< float > clip{std::string{glTFAnim.name}, std::move(channels)};

				const auto clipName = glTFAnim.name.empty()
					? "clip_" + std::to_string(m_animationClips.size())
					: std::string{glTFAnim.name};

				auto clipResource = m_resources.container< Animations::AnimationClipResource >()
					->getOrCreateResourceSync(
						m_resourcePrefix + std::string{} + "/animation/" + clipName,
						[&clip] (auto & resource) {
							return resource.load(std::move(clip));
						}
					);

				m_animationClips.push_back(std::move(clipResource));
			}
		}

		if ( !m_animationClips.empty() )
		{
			TraceDebug{ClassId} << "Loaded " << m_animationClips.size() << " animation clips.";
		}
	}

	void
	GLTFLoader::buildNodeDescriptors (const fastgltf::Asset & asset, SceneData & output) const noexcept
	{
		/* Determine the default scene. */
		size_t sceneIndex = 0;

		if ( asset.defaultScene.has_value() )
		{
			sceneIndex = asset.defaultScene.value();
		}

		if ( sceneIndex >= asset.scenes.size() )
		{
			Tracer::error(ClassId, "Default scene index is out of range !");

			return;
		}

		const auto & glTFScene = asset.scenes[sceneIndex];

		/* Pre-allocate nodes vector (one per glTF node). */
		output.nodes.resize(asset.nodes.size());

		/* Recursive lambda to build node descriptors. */
		const auto buildNode = [&] (auto & self, size_t nodeIndex) -> void {
			if ( nodeIndex >= asset.nodes.size() )
			{
				return;
			}

			const auto & glTFNode = asset.nodes[nodeIndex];

			/* Skip excluded nodes and their entire subtree. */
			if ( !glTFNode.name.empty() && m_options.excludedNodeNames.contains(std::string{glTFNode.name}) )
			{
				return;
			}

			auto & descriptor = output.nodes[nodeIndex];
			descriptor.name = buildNodeName(m_resourcePrefix, glTFNode, nodeIndex);
			descriptor.localFrame = extractFrameFromNode(glTFNode);

			if ( glTFNode.meshIndex.has_value() )
			{
				descriptor.meshIndex = glTFNode.meshIndex.value();
			}

			/* Lights and cameras are referenced by index, exactly like meshes. The node carries
			 * their pose; the descriptor tables carry their parameters. */
			if ( glTFNode.lightIndex.has_value() && glTFNode.lightIndex.value() < output.lights.size() )
			{
				descriptor.lightIndex = glTFNode.lightIndex.value();
			}

			if ( glTFNode.cameraIndex.has_value() && glTFNode.cameraIndex.value() < output.cameras.size() )
			{
				descriptor.cameraIndex = glTFNode.cameraIndex.value();
			}

			/* Process children. */
			descriptor.childIndices.reserve(glTFNode.children.size());

			for ( const auto childIndex : glTFNode.children )
			{
				descriptor.childIndices.push_back(childIndex);

				self(self, childIndex);
			}
		};

		/* Build from scene root nodes. */
		output.rootNodeIndices.reserve(glTFScene.nodeIndices.size());

		for ( const auto nodeIndex : glTFScene.nodeIndices )
		{
			output.rootNodeIndices.push_back(nodeIndex);

			buildNode(buildNode, nodeIndex);
		}
	}

	void
	GLTFLoader::loadLights (const fastgltf::Asset & asset, SceneData & output) noexcept
	{
		output.lights.reserve(asset.lights.size());

		for ( const auto & glTFLight : asset.lights )
		{
			LightDescriptor descriptor;
			descriptor.name = std::string{glTFLight.name};
			descriptor.color = Base::PixelFactory::Color< float >{
				static_cast< float >(glTFLight.color[0]),
				static_cast< float >(glTFLight.color[1]),
				static_cast< float >(glTFLight.color[2]),
				1.0F
			};

			/* KHR_lights_punctual and the engine's photometric contract agree TERM FOR TERM:
			 * illuminance in lux for a directional light, luminous intensity in candela for a
			 * point or a spot. There is deliberately no conversion factor here — if an exporter
			 * ever writes something else, it will show up as a wrong magnitude, not as a wrong
			 * model. */
			descriptor.intensity = static_cast< float >(glTFLight.intensity);

			if ( glTFLight.range.has_value() )
			{
				descriptor.range = static_cast< float >(glTFLight.range.value());
			}

			switch ( glTFLight.type )
			{
				case fastgltf::LightType::Directional :
					descriptor.type = LightType::Directional;
					break;

				case fastgltf::LightType::Point :
					descriptor.type = LightType::Point;
					break;

				case fastgltf::LightType::Spot :
					descriptor.type = LightType::Spot;

					/* glTF authors cone angles in RADIANS, the engine takes DEGREES. */
					if ( glTFLight.innerConeAngle.has_value() )
					{
						descriptor.innerConeAngle = Base::Math::Degree(static_cast< float >(glTFLight.innerConeAngle.value()));
					}

					if ( glTFLight.outerConeAngle.has_value() )
					{
						descriptor.outerConeAngle = Base::Math::Degree(static_cast< float >(glTFLight.outerConeAngle.value()));
					}
					break;
			}

			TraceDebug{ClassId} <<
				"Light '" << descriptor.name << "': " <<
				( descriptor.type == LightType::Directional ? "directional " : ( descriptor.type == LightType::Point ? "point " : "spot " ) ) <<
				descriptor.intensity << ( descriptor.type == LightType::Directional ? " lux" : " cd" ) <<
				", range " << descriptor.range <<
				", cone " << descriptor.innerConeAngle << "/" << descriptor.outerConeAngle << " deg.";

			output.lights.emplace_back(std::move(descriptor));
		}
	}

	void
	GLTFLoader::loadCameras (const fastgltf::Asset & asset, SceneData & output) noexcept
	{
		output.cameras.reserve(asset.cameras.size());

		for ( const auto & glTFCamera : asset.cameras )
		{
			CameraDescriptor descriptor;
			descriptor.name = std::string{glTFCamera.name};

			if ( const auto * perspective = std::get_if< fastgltf::Camera::Perspective >(&glTFCamera.camera) )
			{
				/* glTF authors a VERTICAL field of view in radians. The engine takes degrees and
				 * derives the focal length through its own sensor height — the framing therefore
				 * stays a lens, which is the engine's rule. */
				descriptor.yFieldOfView = Base::Math::Degree(static_cast< float >(perspective->yfov));
				descriptor.distanceNear = static_cast< float >(perspective->znear);

				if ( perspective->zfar.has_value() )
				{
					descriptor.distanceFar = static_cast< float >(perspective->zfar.value());
				}

				if ( perspective->aspectRatio.has_value() )
				{
					descriptor.aspectRatio = static_cast< float >(perspective->aspectRatio.value());
				}
			}
			else if ( const auto * orthographic = std::get_if< fastgltf::Camera::Orthographic >(&glTFCamera.camera) )
			{
				descriptor.orthographic = true;
				descriptor.xMagnification = static_cast< float >(orthographic->xmag);
				descriptor.yMagnification = static_cast< float >(orthographic->ymag);
				descriptor.distanceNear = static_cast< float >(orthographic->znear);
				descriptor.distanceFar = static_cast< float >(orthographic->zfar);
			}

			TraceDebug{ClassId} <<
				"Camera '" << descriptor.name << "': " <<
				( descriptor.orthographic ? "orthographic" : "perspective" ) <<
				", yFOV " << descriptor.yFieldOfView << " deg" <<
				", near " << descriptor.distanceNear << ", far " << descriptor.distanceFar << ".";

			output.cameras.emplace_back(std::move(descriptor));
		}
	}
}
