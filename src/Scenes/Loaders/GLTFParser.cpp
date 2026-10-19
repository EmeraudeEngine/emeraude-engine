/*
 * src/Scenes/Loaders/GLTFParser.cpp
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

#include "GLTFParser.hpp"

/* Third-party inclusions. */
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <limits>
#include <numbers>
#include <ranges>
#include <unordered_set>
#include <utility>
#include <variant>

/* Third-party inclusions. */
#include "fastgltf/tools.hpp"
#include <meshoptimizer.h>
#include <draco/compression/decode.h>
#include <draco/mesh/mesh.h>

/* Local inclusions. */
#include "Interface.hpp"
#include "Animation/Joint.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Quaternion.hpp"
#include "Math/TransformUtils.hpp"
#include "Math/Vector.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "IO/IO.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace
{
	using namespace EmEn;
	using namespace EmEn::Graphics;
	using namespace EmEn::Base::Math;
	using namespace EmEn::Base::PixelFactory;

	/* The byte span type fastgltf hands to accessor readers. */
	using ByteSpan = decltype(fastgltf::DefaultBufferDataAdapter{}(std::declval< const fastgltf::Asset & >(), std::size_t{}));

	/* Returns the whole payload of a glTF buffer. Mirrors what DefaultBufferDataAdapter does for
	 * a buffer *view*, except EXT_meshopt_compression addresses its compressed source directly in
	 * the buffer, outside of any view. */
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

	/* Helper: Extract a CartesianFrame from a glTF node's TRS transform.
	 * ⚠️ The frame is CONJUGATED by the axis flip (M·T·M), never merely mirrored: a hierarchy of
	 * conjugated transforms telescopes into a single mirror at the root, which is what lets every
	 * node keep a PROPER rotation while the mirror lives in the vertex data. */
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

		return frame;
	}

	/* Helper: Build a node name from the glTF node. */
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

	/**
	 * @brief Moves an animation's keyframes so that its earliest one sits at t = 0.
	 * @note glTF times are absolute, and an exporter may lay every clip on ONE shared timeline
	 * (Sketchfab does: the dragon's "fly2" starts at 31.6 s). Played as is, such a clip holds its
	 * first pose for all the time before it, at every loop. Every clip is therefore rebased on its
	 * own earliest keyframe (owner decision, 2026-09-28), which a deliberate initial delay would
	 * lose — a rare case, against a very common export.
	 * @param jointChannels The skeletal half of ONE glTF animation.
	 * @param nodeChannels The node half of the same animation, rebased by the same amount so
	 * that both halves stay in step.
	 * @return float The time removed, in seconds.
	 */
	float
	rebaseAnimationTime (std::vector< EmEn::Base::Animation::AnimationChannel< float > > & jointChannels, std::vector< EmEn::Base::Animation::AnimationChannel< float > > & nodeChannels) noexcept
	{
		auto earliest = std::numeric_limits< float >::max();

		const auto findEarliest = [&earliest] (const auto & channels) {
			for ( const auto & channel : channels )
			{
				if ( !channel.vectorKeyFrames.empty() )
				{
					earliest = std::min(earliest, channel.vectorKeyFrames.front().time);
				}

				if ( !channel.quaternionKeyFrames.empty() )
				{
					earliest = std::min(earliest, channel.quaternionKeyFrames.front().time);
				}
			}
		};

		findEarliest(jointChannels);
		findEarliest(nodeChannels);

		if ( earliest == std::numeric_limits< float >::max() || earliest <= 0.0F )
		{
			return 0.0F;
		}

		const auto shift = [earliest] (auto & channels) {
			for ( auto & channel : channels )
			{
				for ( auto & keyFrame : channel.vectorKeyFrames )
				{
					keyFrame.time -= earliest;
				}

				for ( auto & keyFrame : channel.quaternionKeyFrames )
				{
					keyFrame.time -= earliest;
				}
			}
		};

		shift(jointChannels);
		shift(nodeChannels);

		return earliest;
	}
}

namespace EmEn::Scenes::Loaders::GLTF
{
	using namespace Graphics;
	using namespace Graphics::Geometry;
	using namespace Base::Animation;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Base::PixelFactory;

	namespace
	{
		/* NOTE: The loader's name: the analysis reports under it, as before the split. */
		constexpr auto ClassId{"GLTFLoader"};
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
					TraceError{ClassId} << "Unable to decode the meshopt-compressed buffer view " << bufferViewIndex << " !";

					/* NOTE: fastgltf reads the accessors of this view from the span returned here, with no bounds
					 * check: an empty span was read out of bounds. Serve zeros of the declared size instead, and
					 * the load refuses the file at the end (decodeFailed()). */
					decoded.assign(bufferView.byteLength, std::byte{0});

					m_decodeFailed = true;
				}

				return ByteSpan{decoded.data(), decoded.size()};
			}

			/**
			 * @brief Returns whether a buffer view failed to decode (its accessors read zeros).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			decodeFailed () const noexcept
			{
				return m_decodeFailed;
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

				for ( const auto & buffer : m_decoded | std::views::values )
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
					Tracer::error(ClassId, "A meshopt-compressed buffer view points outside of its buffer !");

					return false;
				}

				const auto decodedSize = compression.count * compression.byteStride;

				/* NOTE: The extension mandates count * byteStride == byteLength. A mismatch means a
				 * broken exporter, and trusting the view length would overrun the decoder output. */
				if ( decodedSize != bufferView.byteLength )
				{
					TraceError{ClassId} <<
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
					TraceError{ClassId} << "The meshopt codec rejected a buffer view (error " << error << ") !";

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

					/* fastgltf 0.9.1 (KHR_meshopt_compression): RGBA colours packed as YCoCg + alpha. */
					case fastgltf::MeshoptCompressionFilter::Color :
						meshopt_decodeFilterColor(output.data(), compression.count, compression.byteStride);
						break;
				}

				return true;
			}

			std::unordered_map< size_t, std::vector< std::byte > > m_decoded;
			bool m_decodeFailed{false};
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

	/**
	 * @brief Decodes KHR_draco_mesh_compression primitives on demand and keeps the result.
	 *
	 * The extension replaces a primitive's vertex attributes AND its indices with a single Draco
	 * bitstream held in one buffer view. fastgltf parses the extension metadata — the view index
	 * and the glTF-name -> Draco-attribute-id map — but decodes nothing, so the work lands here.
	 * Same division of labour as MeshoptBufferCache, for the other compression family.
	 *
	 * ⚠️⚠️ THE MESHOPT DESIGN DOES NOT TRANSPOSE, and trying to reuse it is a dead end.
	 * EXT_meshopt_compression works at the BUFFER VIEW level, so a fastgltf::BufferDataAdapter
	 * serves decoded bytes transparently and the rest of the loader never learns it exists.
	 * Draco works at the PRIMITIVE level, and the accessors of a Draco primitive carry **no
	 * bufferView at all** — so the adapter is never even consulted, because fastgltf only calls
	 * it once a bufferView has been resolved. The interception therefore has to happen at every
	 * read site, which is what readDracoAwareAttribute() below is for.
	 *
	 * Decoding is cached per buffer view: one bitstream carries every attribute AND the indices
	 * of its primitive at once, so the eight read sites of a primitive must not decode it eight
	 * times over.
	 *
	 * @note Not thread-safe : a loader instance drives a single load, on a single thread.
	 * @see https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_draco_mesh_compression
	 */
	class DracoPrimitiveCache final
	{
		public:

			DracoPrimitiveCache () noexcept = default;

			/**
			 * @brief Returns the decoded mesh backing a Draco-compressed primitive.
			 * @param asset A reference to the parsed glTF asset.
			 * @param primitive A reference to the primitive, which MUST carry the extension.
			 * @return A pointer to the decoded mesh, nullptr on failure.
			 */
			[[nodiscard]]
			const draco::Mesh *
			mesh (const fastgltf::Asset & asset, const fastgltf::Primitive & primitive) noexcept
			{
				const auto bufferViewIndex = primitive.dracoCompression->bufferView;

				const auto decodedIt = m_decoded.find(bufferViewIndex);

				if ( decodedIt != m_decoded.cend() )
				{
					/* A present but null entry is a bitstream that already failed: the failure is
					 * paid once, not once per attribute, and it stays a failure. */
					return decodedIt->second.get();
				}

				auto & decoded = m_decoded[bufferViewIndex];

				decoded = DracoPrimitiveCache::decode(asset, bufferViewIndex);

				if ( decoded == nullptr )
				{
					TraceError{ClassId} << "Unable to decode the Draco-compressed buffer view " << bufferViewIndex << " !";
				}

				return decoded.get();
			}

			/**
			 * @brief Returns the Draco attribute backing one glTF attribute of a primitive.
			 * @param asset A reference to the parsed glTF asset.
			 * @param primitive A reference to the primitive, which MUST carry the extension.
			 * @param attributeName The glTF attribute name, e.g. "POSITION".
			 * @return A pointer to the attribute, or nullptr when this primitive does not
			 * compress it — which is LEGAL and means the plain accessor still holds the data.
			 */
			[[nodiscard]]
			const draco::PointAttribute *
			attribute (const fastgltf::Asset & asset, const fastgltf::Primitive & primitive, std::string_view attributeName) noexcept
			{
				const auto & compression = *primitive.dracoCompression;

				const auto * const attributeIt = compression.findAttribute(attributeName);

				if ( attributeIt == compression.attributes.cend() )
				{
					return nullptr;
				}

				const auto * const decoded = this->mesh(asset, primitive);

				if ( decoded == nullptr )
				{
					return nullptr;
				}

				/* ⚠️ `Attribute::accessorIndex` does NOT hold an accessor index here: fastgltf
				 * reuses its generic Attribute struct to carry the extension's map, whose values
				 * are Draco ATTRIBUTE UNIQUE IDS. The field name lies in this context, and
				 * indexing asset.accessors with it would read an unrelated accessor. */
				return decoded->GetAttributeByUniqueId(static_cast< uint32_t >(attributeIt->accessorIndex));
			}

			/**
			 * @brief Returns the number of Draco bitstreams decoded so far.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			meshCount () const noexcept
			{
				return m_decoded.size();
			}

		private:

			/**
			 * @brief Runs one buffer view through the Draco codec.
			 * @param asset A reference to the parsed glTF asset.
			 * @param bufferViewIndex The index of the buffer view holding the bitstream.
			 * @return std::unique_ptr< draco::Mesh >
			 */
			[[nodiscard]]
			static
			std::unique_ptr< draco::Mesh >
			decode (const fastgltf::Asset & asset, size_t bufferViewIndex) noexcept
			{
				/* ⚠️ Read through the DEFAULT adapter, deliberately NOT through the meshopt cache:
				 * the two compression extensions never apply to the same buffer view, and handing
				 * a Draco bitstream to the meshopt codec would only get it rejected. */
				/* ⚠️ The extension's bufferView is not covered by fastgltf::validate() nor by the extent checks: an
				 * index past the views, or a view past its buffer, was read out of bounds (fuzz_gltf, 2026-10-18).
				 * Checked here; the primitive is then unreadable, as for a rejected bitstream. */
				if ( bufferViewIndex >= asset.bufferViews.size() )
				{
					return nullptr;
				}

				const auto & view = asset.bufferViews[bufferViewIndex];
				const auto source = bufferBytes(asset, view.bufferIndex);

				if ( view.byteLength == 0 || view.byteOffset > source.size() || view.byteLength > source.size() - view.byteOffset )
				{
					return nullptr;
				}

				draco::DecoderBuffer buffer;
				buffer.Init(reinterpret_cast< const char * >(source.data() + view.byteOffset), view.byteLength);

				draco::Decoder decoder;

				/* @note DecodeMeshFromBuffer() checks the encoded geometry type itself and fails
				 * on a point cloud, so no separate GetEncodedGeometryType() call is needed. */
				auto result = decoder.DecodeMeshFromBuffer(&buffer);

				if ( !result.ok() )
				{
					TraceError{ClassId} << "The Draco codec rejected a buffer view : " << result.status().error_msg();

					return nullptr;
				}

				return std::move(result).value();
			}

			std::unordered_map< size_t, std::unique_ptr< draco::Mesh > > m_decoded;
	};

	/**
	 * @brief Describes a fastgltf vector element so a Draco value can be written into one.
	 * @tparam ElementType The fastgltf::math::vec specialisation used at the read site.
	 */
	template< typename ElementType >
	struct AttributeElement;

	template< typename component_t, size_t component_count >
	struct AttributeElement< fastgltf::math::vec< component_t, component_count > >
	{
		using Component = component_t;

		static constexpr int8_t ComponentCount{static_cast< int8_t >(component_count)};
	};

	namespace
	{
		/**
		 * @brief Reads one vertex attribute of a primitive, whichever way it is stored.
		 *
		 * Routes to the Draco decoder when the primitive compresses this attribute, and to the plain
		 * accessor otherwise. A primitive is allowed to be MIXED — KHR_draco_mesh_compression only
		 * covers the primitive's own attributes and its indices, so anything else it declares (morph
		 * targets, most notably) stays an ordinary accessor and takes the second branch.
		 *
		 * ⚠️⚠️ THE GUARD IS THE POINT OF THIS FUNCTION. An accessor with no bufferView makes
		 * fastgltf::iterateAccessor() emit `count` ZERO-INITIALISED elements without any error, as
		 * glTF § 5.1.1 mandates ("MUST be initialized with zeros; extensions MAY override"). Since
		 * every accessor of a Draco primitive is bufferView-less, a read site that forgot to route
		 * through here would produce a mesh collapsed onto the origin, silently, with no log line
		 * and no failure. So a bufferView-less accessor that Draco did not claim is a hard error.
		 *
		 * @tparam ElementType The fastgltf vector type the functor expects.
		 * @tparam Functor The per-element callback type.
		 * @param asset A reference to the parsed glTF asset.
		 * @param primitive A reference to the primitive being read.
		 * @param attributeName The glTF attribute name, e.g. "POSITION".
		 * @param accessor A reference to the accessor declared for that attribute.
		 * @param dracoCache A reference to the Draco cache.
		 * @param adapter A reference to the meshopt-backed buffer adapter.
		 * @param functor The callback invoked once per element, in vertex order.
		 * @return bool
		 */
		template< typename ElementType, typename Functor >
		[[nodiscard]]
		bool
		readDracoAwareAttribute (const fastgltf::Asset & asset, const fastgltf::Primitive & primitive, std::string_view attributeName, const fastgltf::Accessor & accessor, DracoPrimitiveCache & dracoCache, const MeshoptBufferAdapter & adapter, Functor && functor) noexcept
		{
			if ( primitive.dracoCompression != nullptr )
			{
				const auto * const dracoAttribute = dracoCache.attribute(asset, primitive, attributeName);

				if ( dracoAttribute != nullptr )
				{
					const auto * const dracoMesh = dracoCache.mesh(asset, primitive);

					if ( dracoMesh == nullptr )
					{
						return false;
					}

					/* ⚠️ The decoded point count and the accessor count MUST agree: the rest of
					 * loadMeshes() pre-allocated its vertex array from the ACCESSOR, so a shorter
					 * decode would leave uninitialised vertices and a longer one would write past
					 * the primitive's slice. */
					if ( static_cast< size_t >(dracoMesh->num_points()) != accessor.count )
					{
						TraceError{ClassId} <<
							"Draco attribute '" << attributeName << "' decodes " << dracoMesh->num_points() <<
							" points where the accessor declares " << accessor.count << " !";

						return false;
					}

					/* ⚠️ The glTF accessor is the authority on normalisation, not the bitstream.
					 * Draco carries its own `normalized` flag and ConvertValue() honours it, so a
					 * disagreement between the two would silently scale an attribute by 255 (or by
					 * 1/255). No asset of the Khronos corpus exercises this — every Draco-compressed
					 * attribute there is plain float or plain uint16 — so say so rather than guess. */
					if ( dracoAttribute->normalized() != accessor.normalized )
					{
						TraceWarning{ClassId} <<
							"Draco attribute '" << attributeName << "' declares normalized=" << dracoAttribute->normalized() <<
							" where the glTF accessor declares " << accessor.normalized << " — reading with the bitstream's own convention.";
					}

					using Component = typename AttributeElement< ElementType >::Component;

					constexpr auto componentCount = AttributeElement< ElementType >::ComponentCount;

					std::array< Component, static_cast< size_t >(componentCount) > components{};

					for ( uint32_t pointIndex = 0; pointIndex < dracoMesh->num_points(); ++pointIndex )
					{
						/* ⚠️ `mapped_index()` is not decoration: a Draco attribute is stored as a
						 * value table plus a point -> value mapping, so reading the table in point
						 * order would scramble every attribute that has fewer values than points
						 * (a hard edge splits UVs but not positions, and the two tables differ). */
						if ( !dracoAttribute->ConvertValue< Component >(dracoAttribute->mapped_index(draco::PointIndex{pointIndex}), componentCount, components.data()) )
						{
							TraceError{ClassId} << "Draco attribute '" << attributeName << "' could not be converted at point " << pointIndex << " !";

							return false;
						}

						functor(ElementType::fromPointer(components.data()));
					}

					return true;
				}
			}

			if ( !accessor.bufferViewIndex.has_value() )
			{
				TraceError{ClassId} <<
					"Attribute '" << attributeName << "' has no buffer view and no Draco source — "
					"reading it would silently yield zeros. Refusing the mesh.";

				return false;
			}

			fastgltf::iterateAccessor< ElementType >(asset, accessor, std::forward< Functor >(functor), adapter);

			return true;
		}
	}

	namespace
	{
		/**
		 * @brief Reads the triangle indices of a primitive, whichever way they are stored.
		 *
		 * ⚠️ Unlike an attribute, the indices of a Draco primitive are ALWAYS carried by the
		 * bitstream — the extension has no notion of compressing the attributes while leaving the
		 * indices behind. So a Draco primitive whose mesh fails to decode is a hard failure here,
		 * with no accessor to fall back on.
		 *
		 * @tparam Functor The per-index callback type.
		 * @param asset A reference to the parsed glTF asset.
		 * @param primitive A reference to the primitive being read.
		 * @param accessor A reference to the index accessor.
		 * @param dracoCache A reference to the Draco cache.
		 * @param adapter A reference to the meshopt-backed buffer adapter.
		 * @param functor The callback invoked once per index, three per triangle.
		 * @return bool
		 */
		template< typename Functor >
		[[nodiscard]]
		bool
		readDracoAwareIndices (const fastgltf::Asset & asset, const fastgltf::Primitive & primitive, const fastgltf::Accessor & accessor, DracoPrimitiveCache & dracoCache, const MeshoptBufferAdapter & adapter, Functor && functor) noexcept
		{
			if ( primitive.dracoCompression != nullptr )
			{
				const auto * const dracoMesh = dracoCache.mesh(asset, primitive);

				if ( dracoMesh == nullptr )
				{
					return false;
				}

				if ( static_cast< size_t >(dracoMesh->num_faces()) * 3 != accessor.count )
				{
					TraceError{ClassId} <<
						"The Draco bitstream decodes " << dracoMesh->num_faces() << " faces where the index accessor declares " <<
						accessor.count << " indices !";

					return false;
				}

				for ( uint32_t faceIndex = 0; faceIndex < dracoMesh->num_faces(); ++faceIndex )
				{
					const auto & face = dracoMesh->face(draco::FaceIndex{faceIndex});

					functor(face[0].value());
					functor(face[1].value());
					functor(face[2].value());
				}

				return true;
			}

			if ( !accessor.bufferViewIndex.has_value() )
			{
				Tracer::error(ClassId, "The index accessor has no buffer view and no Draco source — reading it would silently yield zeros. Refusing the mesh.");

				return false;
			}

			fastgltf::iterateAccessor< uint32_t >(asset, accessor, std::forward< Functor >(functor), adapter);

			return true;
		}
	}

	namespace
	{
		/**
		 * @brief Names the extensions an asset requires that this loader does not declare.
		 *
		 * Called only when a parse failed with Error::MissingExtensions. fastgltf refuses such a file
		 * whole, so its extensionsRequired array is never handed back — the list has to be recovered by
		 * re-parsing with every extension fastgltf knows, which is what this does. It costs a second
		 * parse of a file that was going to be rejected anyway, once, on an error path.
		 *
		 * @param filepath A reference to the asset path, for the .glb/.gltf discrimination and the log.
		 * @param data A reference to the already-loaded file bytes.
		 * @param parentPath A reference to the directory external resources resolve against.
		 * @param declared The extension mask this loader passes to its own Parser.
		 */
		void
		reportMissingExtensions (const std::filesystem::path & filepath, fastgltf::GltfDataBuffer & data, const std::filesystem::path & parentPath, fastgltf::Extensions declared) noexcept
		{
			/* Everything fastgltf itself knows about. A parse with this mask can still fail on
			 * MissingExtensions, but only for an extension fastgltf has never heard of. */
			auto everything = fastgltf::Extensions::None;

			for ( const auto & [name, value] : fastgltf::extensionStrings )
			{
				everything |= value;
			}

			fastgltf::Parser permissive{everything};

			/* No options: nothing is consumed from this asset, only its extension list is read. */
			auto retry = filepath.extension() == ".glb" ?
				permissive.loadGltfBinary(data, parentPath, fastgltf::Options::None) :
				permissive.loadGltf(data, parentPath, fastgltf::Options::None);

			if ( retry.error() != fastgltf::Error::None )
			{
				Tracer::error(ClassId, "The asset requires an extension that fastgltf itself does not know, so it cannot be named. Check its 'extensionsRequired' array by hand.");

				return;
			}

			const auto declaredNames = fastgltf::stringifyExtensionBits(declared);

			std::string missing;

			for ( const auto & required : retry.get().extensionsRequired )
			{
				const auto isDeclared = std::ranges::any_of(declaredNames, [&required] (const auto & name) {
					return name == required;
				});

				if ( isDeclared )
				{
					continue;
				}

				if ( !missing.empty() )
				{
					missing += ", ";
				}

				missing.append(required.data(), required.size());
			}

			if ( missing.empty() )
			{
				/* Should not happen: the first parse refused the file for a missing extension, so the
				 * diff cannot be empty. Say so rather than print a misleading empty list. */
				Tracer::error(ClassId, "The parse was refused for a missing extension, but every required extension is declared. The extension mask and the refusal disagree.");

				return;
			}

			TraceError{ClassId} << "Required extension(s) this loader does not support : " << missing << '.';
		}
	}

	namespace
	{
		/**
		 * @brief Checks that the node hierarchy is a strict FOREST, as glTF § 3.5 requires: every node has at most one
		 * parent, every child index is in range, and no cycle. fastgltf::validate() does not check it, and a recursive or
		 * unguarded walk over a cycle never ends (triad 2026-09-30).
		 * @note Iterative, O(nodes + children): with at most one parent each, a node in a cycle is unreachable from the
		 * parentless nodes, so "every node reachable from a parentless node" is the no-cycle proof.
		 * @param asset A reference to the parsed asset.
		 * @param error A reference to the reason, filled on failure.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isStrictNodeForest (const fastgltf::Asset & asset, std::string & error) noexcept
		{
			const auto nodeCount = asset.nodes.size();

			std::vector< uint8_t > parentCount(nodeCount, 0);

			for ( size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex )
			{
				for ( const auto childIndex : asset.nodes[nodeIndex].children )
				{
					if ( childIndex >= nodeCount )
					{
						error = "node #" + std::to_string(nodeIndex) + " has an out-of-range child #" + std::to_string(childIndex);

						return false;
					}

					if ( parentCount[childIndex] != 0 )
					{
						error = "node #" + std::to_string(childIndex) + " has more than one parent (or is its own ancestor)";

						return false;
					}

					parentCount[childIndex] = 1;
				}
			}

			std::vector< size_t > pending;
			pending.reserve(nodeCount);

			for ( size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex )
			{
				if ( parentCount[nodeIndex] == 0 )
				{
					pending.push_back(nodeIndex);
				}
			}

			size_t reachedCount = 0;

			while ( !pending.empty() )
			{
				const auto nodeIndex = pending.back();
				pending.pop_back();

				++reachedCount;

				for ( const auto childIndex : asset.nodes[nodeIndex].children )
				{
					pending.push_back(childIndex);
				}
			}

			if ( reachedCount != nodeCount )
			{
				error = std::to_string(nodeCount - reachedCount) + " node(s) form a cycle";

				return false;
			}

			return true;
		}

		/**
		 * @brief Returns the parent node index of every node of the asset (NoParent for a root).
		 * @pre The node hierarchy is a strict forest (isStrictNodeForest()).
		 * @param asset A reference to the parsed asset.
		 * @return std::vector< int32_t >
		 */
		[[nodiscard]]
		std::vector< int32_t >
		nodeParentTable (const fastgltf::Asset & asset) noexcept
		{
			std::vector< int32_t > nodeParents(asset.nodes.size(), Base::Animation::NoParent);

			for ( size_t nodeIdx = 0; nodeIdx < asset.nodes.size(); ++nodeIdx )
			{
				for ( const auto childIdx : asset.nodes[nodeIdx].children )
				{
					nodeParents[childIdx] = static_cast< int32_t >(nodeIdx);
				}
			}

			return nodeParents;
		}

		/**
		 * @brief Returns the skeleton index of every joint of a glTF skin, parents first.
		 * @note glTF allows the joints of a skin in ANY order, but the SkeletalAnimator computes the world matrices in
		 * one forward pass that needs every parent before its children (Skeleton::isValid()). A skin already in that
		 * order (every asset of the corpus, 2026-10-01) keeps its order: the identity. Otherwise the joints are sorted
		 * by depth in the skin (stable), which puts every parent first.
		 * @pre The node hierarchy is a strict forest and every joint node is in range (hasConsistentExtents()).
		 * @param skin A reference to the glTF skin.
		 * @param nodeParents A reference to the parent node table (nodeParentTable()).
		 * @return std::vector< int32_t > The skeleton index, indexed by the skin-local (glTF) joint index.
		 */
		[[nodiscard]]
		std::vector< int32_t >
		parentFirstJointOrder (const fastgltf::Skin & skin, const std::vector< int32_t > & nodeParents) noexcept
		{
			const auto jointCount = skin.joints.size();

			std::unordered_map< size_t, size_t > nodeToJoint;
			nodeToJoint.reserve(jointCount);

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				nodeToJoint[skin.joints[joint]] = joint;
			}

			/* The in-skin parent of each joint: the closest ancestor node that is a joint of this skin. */
			constexpr auto NoJoint = std::numeric_limits< size_t >::max();

			std::vector< size_t > parentJoint(jointCount, NoJoint);
			bool parentFirst = true;

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				auto parentNode = nodeParents[skin.joints[joint]];

				while ( parentNode != Base::Animation::NoParent )
				{
					if ( const auto it = nodeToJoint.find(static_cast< size_t >(parentNode)); it != nodeToJoint.end() )
					{
						parentJoint[joint] = it->second;
						parentFirst = parentFirst && it->second < joint;

						break;
					}

					parentNode = nodeParents[static_cast< size_t >(parentNode)];
				}
			}

			std::vector< int32_t > order(jointCount);

			if ( parentFirst )
			{
				for ( size_t joint = 0; joint < jointCount; ++joint )
				{
					order[joint] = static_cast< int32_t >(joint);
				}

				return order;
			}

			/* Depth in the skin, memoized: each chain is walked once (O(joints)), even for a hostile deep skin. */
			std::vector< size_t > depth(jointCount, NoJoint);
			std::vector< size_t > chain;

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				auto current = joint;

				while ( current != NoJoint && depth[current] == NoJoint )
				{
					chain.push_back(current);
					current = parentJoint[current];
				}

				auto known = current == NoJoint ? size_t{0} : depth[current] + 1;

				while ( !chain.empty() )
				{
					depth[chain.back()] = known++;
					chain.pop_back();
				}
			}

			std::vector< size_t > sorted(jointCount);

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				sorted[joint] = joint;
			}

			std::ranges::stable_sort(sorted, [&depth] (size_t lhs, size_t rhs) {
				return depth[lhs] < depth[rhs];
			});

			for ( size_t position = 0; position < jointCount; ++position )
			{
				order[sorted[position]] = static_cast< int32_t >(position);
			}

			return order;
		}

		/**
		 * @brief The node layout of the skeleton built for a glTF skin.
		 * @note glTF skins a vertex with the WORLD matrix of each joint, which includes every ancestor of the joint,
		 * joints or not, and it ignores the transform of the node that holds the mesh. The engine places the mesh at
		 * that node (the scene graph), so the skeleton must carry the non-joint nodes between the joints and the lowest
		 * common ancestor of the mesh node and the joints; the scene graph applies that ancestor and everything above,
		 * and Skin::rootTransform() takes the mesh node's own path back out (BrainStem, 2026-10-01).
		 */
		struct SkeletonLayout
		{
			/** @brief The glTF node of each skeleton joint, parents first: the skin's joints and the non-joint nodes on
			 * their paths up to the common ancestor (exclusive). */
			std::vector< size_t > nodes;
			/** @brief The skeleton index of each skin-local (glTF) joint. */
			std::vector< int32_t > jointOrder;
			/** @brief The skeleton index of every node of the layout. */
			std::unordered_map< size_t, int32_t > nodeToSkeletonIndex;
			/** @brief The lowest common ancestor of the skin's mesh node and every joint (NoParent: none, the scene
			 * root is above them all). */
			int32_t commonAncestor{Base::Animation::NoParent};
			/** @brief The count of non-joint nodes the layout carries. */
			size_t carriedNodeCount{0};
		};

		/**
		 * @brief Returns the first node that holds a mesh skinned by this skin.
		 * @param asset A reference to the parsed asset.
		 * @param skinIndex The skin index.
		 * @return int32_t The node index, NoParent when no node uses the skin.
		 */
		[[nodiscard]]
		int32_t
		skinnedMeshNode (const fastgltf::Asset & asset, size_t skinIndex) noexcept
		{
			for ( size_t nodeIdx = 0; nodeIdx < asset.nodes.size(); ++nodeIdx )
			{
				const auto & node = asset.nodes[nodeIdx];

				if ( node.skinIndex.has_value() && node.skinIndex.value() == skinIndex && node.meshIndex.has_value() )
				{
					return static_cast< int32_t >(nodeIdx);
				}
			}

			return Base::Animation::NoParent;
		}

		/**
		 * @brief Returns the layout of the skeleton of a glTF skin (see SkeletonLayout).
		 * @note A skin whose joints hang directly under the common ancestor (every corpus asset but BrainStem,
		 * 2026-10-01) keeps exactly the order of parentFirstJointOrder(). Every walk up the hierarchy is memoized, so
		 * a deep hostile hierarchy stays linear.
		 * @pre The node hierarchy is a strict forest and every joint node is in range (hasConsistentExtents()).
		 * @param asset A reference to the parsed asset.
		 * @param skinIndex The skin index.
		 * @param nodeParents A reference to the parent node table (nodeParentTable()).
		 * @return SkeletonLayout
		 */
		[[nodiscard]]
		SkeletonLayout
		skeletonLayout (const fastgltf::Asset & asset, size_t skinIndex, const std::vector< int32_t > & nodeParents) noexcept
		{
			using Base::Animation::NoParent;

			const auto & skin = asset.skins[skinIndex];
			const auto jointCount = skin.joints.size();

			SkeletonLayout layout;
			layout.jointOrder = parentFirstJointOrder(skin, nodeParents);

			const std::unordered_set< size_t > joints{skin.joints.begin(), skin.joints.end()};

			/* 1. The common ancestor: the highest node of the mesh node's chain that the joints reach walking up. */
			const auto meshNode = skinnedMeshNode(asset, skinIndex);
			std::vector< int32_t > meshChain;
			std::unordered_map< size_t, int32_t > meshChainRank;

			for ( auto node = meshNode; node != NoParent; node = nodeParents[static_cast< size_t >(node)] )
			{
				meshChainRank[static_cast< size_t >(node)] = static_cast< int32_t >(meshChain.size());
				meshChain.push_back(node);
			}

			/* NOTE: The rank of the first mesh-chain node strictly above a node, -1 for none; memoized. */
			std::unordered_map< size_t, int32_t > reach;
			std::vector< size_t > path;

			const auto reachFrom = [&] (int32_t start) {
				int32_t result = -1;

				path.clear();

				for ( auto node = start; node != NoParent; node = nodeParents[static_cast< size_t >(node)] )
				{
					if ( const auto rank = meshChainRank.find(static_cast< size_t >(node)); rank != meshChainRank.end() )
					{
						result = rank->second;

						break;
					}

					if ( const auto known = reach.find(static_cast< size_t >(node)); known != reach.end() )
					{
						result = known->second;

						break;
					}

					path.push_back(static_cast< size_t >(node));
				}

				for ( const auto node : path )
				{
					reach[node] = result;
				}

				return result;
			};

			bool shared = meshNode != NoParent;
			int32_t commonRank = -1;

			for ( size_t joint = 0; joint < jointCount && shared; ++joint )
			{
				const auto rank = reachFrom(nodeParents[skin.joints[joint]]);

				if ( rank < 0 )
				{
					shared = false;
				}
				else
				{
					commonRank = std::max(commonRank, rank);
				}
			}

			layout.commonAncestor = shared && commonRank >= 0 ? meshChain[static_cast< size_t >(commonRank)] : NoParent;

			/* 2. The non-joint nodes between each joint and the common ancestor. */
			std::vector< size_t > carried;
			std::unordered_set< size_t > carriedSet;

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				for ( auto node = nodeParents[skin.joints[joint]]; node != NoParent && node != layout.commonAncestor; node = nodeParents[static_cast< size_t >(node)] )
				{
					const auto index = static_cast< size_t >(node);

					/* NOTE: A joint walks its own way up, and a carried node already carries its own path. */
					if ( joints.contains(index) || !carriedSet.insert(index).second )
					{
						break;
					}

					carried.push_back(index);
				}
			}

			layout.carriedNodeCount = carried.size();

			std::vector< size_t > jointNodesInOrder(jointCount);

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				jointNodesInOrder[static_cast< size_t >(layout.jointOrder[joint])] = skin.joints[joint];
			}

			if ( carried.empty() )
			{
				layout.nodes = std::move(jointNodesInOrder);
			}
			else
			{
				/* 3. Parents first: sorted by depth below the common ancestor (stable), the carried nodes ahead. */
				std::vector< size_t > members = carried;
				members.insert(members.end(), jointNodesInOrder.begin(), jointNodesInOrder.end());

				std::unordered_map< size_t, size_t > depth;

				for ( const auto member : members )
				{
					path.clear();

					auto node = static_cast< int32_t >(member);
					size_t known = 0;

					while ( true )
					{
						if ( const auto it = depth.find(static_cast< size_t >(node)); it != depth.end() )
						{
							known = it->second + 1;

							break;
						}

						path.push_back(static_cast< size_t >(node));

						const auto parent = nodeParents[static_cast< size_t >(node)];

						if ( parent == NoParent || (!joints.contains(static_cast< size_t >(parent)) && !carriedSet.contains(static_cast< size_t >(parent))) )
						{
							known = 0;

							break;
						}

						node = parent;
					}

					while ( !path.empty() )
					{
						depth[path.back()] = known++;
						path.pop_back();
					}
				}

				std::ranges::stable_sort(members, [&depth] (size_t lhs, size_t rhs) {
					return depth[lhs] < depth[rhs];
				});

				layout.nodes = std::move(members);
			}

			for ( size_t index = 0; index < layout.nodes.size(); ++index )
			{
				layout.nodeToSkeletonIndex[layout.nodes[index]] = static_cast< int32_t >(index);
			}

			for ( size_t joint = 0; joint < jointCount; ++joint )
			{
				layout.jointOrder[joint] = layout.nodeToSkeletonIndex[skin.joints[joint]];
			}

			return layout;
		}

		/**
		 * @brief Returns the local matrix of a node (its TRS; fastgltf decomposes a node matrix on load), the
		 * translation scaled as the joints' are.
		 * @param node A reference to the glTF node.
		 * @param uniformScale The loader's uniform scale.
		 * @return Matrix< 4, float >
		 */
		[[nodiscard]]
		Matrix< 4, float >
		nodeLocalMatrix (const fastgltf::Node & node, float uniformScale) noexcept
		{
			const auto * trs = std::get_if< fastgltf::TRS >(&node.transform);

			if ( trs == nullptr )
			{
				return {};
			}

			return composeTRS(
				Vector< 3, float >{trs->translation.x() * uniformScale, trs->translation.y() * uniformScale, trs->translation.z() * uniformScale},
				Quaternion< float >{trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()},
				Vector< 3, float >{trs->scale.x(), trs->scale.y(), trs->scale.z()}
			);
		}

		/**
		 * @brief Returns the world matrix of a node at rest (NoParent: the identity).
		 * @param asset A reference to the parsed asset.
		 * @param nodeIndex The node index, or NoParent.
		 * @param nodeParents A reference to the parent node table (nodeParentTable()).
		 * @param uniformScale The loader's uniform scale.
		 * @return Matrix< 4, float >
		 */
		[[nodiscard]]
		Matrix< 4, float >
		nodeWorldMatrix (const fastgltf::Asset & asset, int32_t nodeIndex, const std::vector< int32_t > & nodeParents, float uniformScale) noexcept
		{
			Matrix< 4, float > world{};

			for ( auto node = nodeIndex; node != Base::Animation::NoParent; node = nodeParents[static_cast< size_t >(node)] )
			{
				world = nodeLocalMatrix(asset.nodes[static_cast< size_t >(node)], uniformScale) * world;
			}

			return world;
		}

		/**
		 * @brief Checks the indices fastgltf::validate() ITSELF reads without a range check (fastgltf 0.9.0): the
		 * sparse indices / values buffer views of an accessor, the sampler of an animation channel, the input and
		 * output accessors of an animation sampler. The parser does not check them either, so validate() read out of
		 * bounds on a hostile file (triad 12, 2026-10-01). Called BEFORE validate().
		 * @param asset A reference to the parsed asset.
		 * @param error A reference to the reason, filled on failure.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		hasValidatableIndices (const fastgltf::Asset & asset, std::string & error) noexcept
		{
			for ( size_t accessorIndex = 0; accessorIndex < asset.accessors.size(); ++accessorIndex )
			{
				const auto & sparse = asset.accessors[accessorIndex].sparse;

				if ( sparse && (sparse->indicesBufferView >= asset.bufferViews.size() || sparse->valuesBufferView >= asset.bufferViews.size()) )
				{
					error = "the sparse data of the accessor #" + std::to_string(accessorIndex) + " uses an out-of-range buffer view";

					return false;
				}
			}

			for ( size_t animationIndex = 0; animationIndex < asset.animations.size(); ++animationIndex )
			{
				const auto & animation = asset.animations[animationIndex];

				for ( const auto & channel : animation.channels )
				{
					if ( channel.samplerIndex >= animation.samplers.size() )
					{
						error = "a channel of the animation #" + std::to_string(animationIndex) + " uses an out-of-range sampler";

						return false;
					}
				}

				for ( const auto & sampler : animation.samplers )
				{
					if ( sampler.inputAccessor >= asset.accessors.size() || sampler.outputAccessor >= asset.accessors.size() )
					{
						error = "a sampler of the animation #" + std::to_string(animationIndex) + " uses an out-of-range accessor";

						return false;
					}
				}
			}

			return true;
		}

		/**
		 * @brief Returns whether `count` elements of `elementSize` bytes, `stride` bytes apart from `offset`, fit in
		 * `length` bytes, without any overflow.
		 * @param offset The offset of the first element.
		 * @param stride The distance between two elements.
		 * @param count The number of elements, at least 1.
		 * @param elementSize The size of one element.
		 * @param length The size of the span they must fit in.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		elementsFit (size_t offset, size_t stride, size_t count, size_t elementSize, size_t length) noexcept
		{
			if ( count == 0 )
			{
				return true;
			}

			if ( elementSize > length || offset > length - elementSize )
			{
				return false;
			}

			const auto room = length - elementSize - offset;

			return stride == 0 || count - 1 <= room / stride;
		}

		/**
		 * @brief Checks the extents and the counts fastgltf::validate() does not, which every accessor read of this
		 * loader relies on (triad 12, 2026-10-01): fastgltf's span and iterateAccessor() have no bounds check (the
		 * accessor type is only an assert), so a hostile file read — and, for the inverse bind matrices and the
		 * vertex attributes, WROTE — out of bounds.
		 * @note Checked: every buffer view inside its buffer (a meshopt view: its compressed source inside its
		 * buffer and count * byteStride == byteLength); every accessor inside its view, sparse indices and values
		 * included; the attributes of a primitive with the POSITION count; a SCALAR index accessor; the skin joints in
		 * range and a MAT4 FLOAT inverse bind matrix accessor of at least as many elements as joints; a VEC3
		 * translation / scale and a VEC4 rotation animation output. The 323 glTF-Sample-Assets and the 11 store
		 * assets pass.
		 * @param asset A reference to the parsed and validated asset.
		 * @param error A reference to the reason, filled on failure.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		hasConsistentExtents (const fastgltf::Asset & asset, std::string & error) noexcept
		{
			for ( size_t viewIndex = 0; viewIndex < asset.bufferViews.size(); ++viewIndex )
			{
				const auto & view = asset.bufferViews[viewIndex];

				if ( view.meshoptCompression != nullptr )
				{
					const auto & compression = *view.meshoptCompression;
					const auto source = bufferBytes(asset, compression.bufferIndex);

					if ( !elementsFit(compression.byteOffset, 0, 1, compression.byteLength, source.size()) ||
						 compression.byteStride == 0 || compression.count > view.byteLength / compression.byteStride ||
						 compression.count * compression.byteStride != view.byteLength )
					{
						error = "the meshopt-compressed buffer view #" + std::to_string(viewIndex) + " is out of its buffer or of its declared size";

						return false;
					}

					continue;
				}

				if ( !elementsFit(view.byteOffset, 0, 1, view.byteLength, bufferBytes(asset, view.bufferIndex).size()) )
				{
					error = "the buffer view #" + std::to_string(viewIndex) + " is out of its buffer";

					return false;
				}
			}

			for ( size_t accessorIndex = 0; accessorIndex < asset.accessors.size(); ++accessorIndex )
			{
				const auto & accessor = asset.accessors[accessorIndex];
				const auto elementSize = fastgltf::getElementByteSize(accessor.type, accessor.componentType);

				if ( accessor.bufferViewIndex.has_value() )
				{
					const auto & view = asset.bufferViews[accessor.bufferViewIndex.value()];
					const auto stride = view.byteStride.value_or(elementSize);

					if ( !elementsFit(accessor.byteOffset, stride, accessor.count, elementSize, view.byteLength) )
					{
						error = "the accessor #" + std::to_string(accessorIndex) + " is out of its buffer view";

						return false;
					}
				}

				if ( accessor.sparse && accessor.sparse->count > 0 )
				{
					const auto & sparse = *accessor.sparse;
					const auto indexSize = fastgltf::getElementByteSize(fastgltf::AccessorType::Scalar, sparse.indexComponentType);

					if ( sparse.count > accessor.count ||
						 !elementsFit(sparse.indicesByteOffset, indexSize, sparse.count, indexSize, asset.bufferViews[sparse.indicesBufferView].byteLength) ||
						 !elementsFit(sparse.valuesByteOffset, elementSize, sparse.count, elementSize, asset.bufferViews[sparse.valuesBufferView].byteLength) )
					{
						error = "the sparse data of the accessor #" + std::to_string(accessorIndex) + " is out of its buffer views";

						return false;
					}
				}
			}

			for ( size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex )
			{
				for ( const auto & primitive : asset.meshes[meshIndex].primitives )
				{
					/* NOTE: The vertex array is sized from POSITION and every other attribute writes into it. */
					const auto * position = primitive.findAttribute("POSITION");

					if ( position != primitive.attributes.cend() )
					{
						const auto vertexCount = asset.accessors[position->accessorIndex].count;

						for ( const auto & attribute : primitive.attributes )
						{
							if ( asset.accessors[attribute.accessorIndex].count != vertexCount )
							{
								error = "the attribute '" + std::string{attribute.name} + "' of the mesh #" + std::to_string(meshIndex) + " does not have the POSITION count";

								return false;
							}
						}
					}

					if ( primitive.indicesAccessor.has_value() && asset.accessors[primitive.indicesAccessor.value()].type != fastgltf::AccessorType::Scalar )
					{
						error = "the index accessor of the mesh #" + std::to_string(meshIndex) + " is not SCALAR";

						return false;
					}
				}
			}

			for ( size_t skinIndex = 0; skinIndex < asset.skins.size(); ++skinIndex )
			{
				const auto & skin = asset.skins[skinIndex];

				for ( const auto jointNode : skin.joints )
				{
					if ( jointNode >= asset.nodes.size() )
					{
						error = "the skin #" + std::to_string(skinIndex) + " has an out-of-range joint node #" + std::to_string(jointNode);

						return false;
					}
				}

				if ( skin.inverseBindMatrices.has_value() )
				{
					const auto & ibm = asset.accessors[skin.inverseBindMatrices.value()];

					if ( ibm.type != fastgltf::AccessorType::Mat4 || ibm.componentType != fastgltf::ComponentType::Float || ibm.count < skin.joints.size() )
					{
						error = "the inverse bind matrices of the skin #" + std::to_string(skinIndex) + " are not at least one MAT4 FLOAT per joint";

						return false;
					}
				}
			}

			for ( size_t animationIndex = 0; animationIndex < asset.animations.size(); ++animationIndex )
			{
				const auto & animation = asset.animations[animationIndex];

				for ( const auto & channel : animation.channels )
				{
					const auto & sampler = animation.samplers[channel.samplerIndex];
					const auto outputType = asset.accessors[sampler.outputAccessor].type;
					const bool vectorPath = channel.path == fastgltf::AnimationPath::Translation || channel.path == fastgltf::AnimationPath::Scale;

					if ( asset.accessors[sampler.inputAccessor].type != fastgltf::AccessorType::Scalar ||
						 (vectorPath && outputType != fastgltf::AccessorType::Vec3) ||
						 (channel.path == fastgltf::AnimationPath::Rotation && outputType != fastgltf::AccessorType::Vec4) )
					{
						error = "a sampler of the animation #" + std::to_string(animationIndex) + " has the wrong accessor type for its path";

						return false;
					}
				}
			}

			return true;
		}
	}

	namespace
	{
		/**
		 * @brief Joins a uri of the asset under its directory, refusing a scheme ("http:", "file:"), an absolute path or
		 * a '..' that leaves the directory (IO::confinedPath()).
		 * @param uri The uri, as fastgltf parsed it (percent-decoded).
		 * @param basePath The asset's directory.
		 * @return std::optional< std::filesystem::path >
		 */
		[[nodiscard]]
		std::optional< std::filesystem::path >
		confinedURI (const fastgltf::URI & uri, const std::filesystem::path & basePath) noexcept
		{
			if ( !uri.isLocalPath() )
			{
				return std::nullopt;
			}

			return Base::IO::confinedPath(basePath, Base::IO::u8path(std::string{uri.path()}));
		}

		/**
		 * @brief Reads every external buffer through its confined path, and checks every external image's uri.
		 * @note ⚠️ Owner decision, 2026-10-18: a buffer or image uri is CONFINED to the asset's directory. Read as
		 * written (fastgltf's LoadExternalBuffers, the loader's basePath / uri), an absolute uri or a '../../..' made the
		 * engine read any local file as geometry or texture — Ave Robustus: a path from data goes through
		 * IO::confinedPath(). None of the 150 Khronos sample models uses either.
		 * @param asset The parsed asset (its buffers' sources are replaced by their bytes).
		 * @param basePath The asset's directory.
		 * @param sourceName The name the logs use.
		 * @return bool False when a uri leaves the directory or a buffer file is missing or short (the asset is refused).
		 */
		[[nodiscard]]
		bool
		loadExternalBuffers (fastgltf::Asset & asset, const std::filesystem::path & basePath, const std::string & sourceName) noexcept
		{
			for ( size_t bufferIndex = 0; bufferIndex < asset.buffers.size(); ++bufferIndex )
			{
				auto & buffer = asset.buffers[bufferIndex];
				const auto * source = std::get_if< fastgltf::sources::URI >(&buffer.data);

				if ( source == nullptr )
				{
					continue;
				}

				const auto path = confinedURI(source->uri, basePath);

				if ( !path.has_value() )
				{
					TraceError{ClassId} << "The buffer #" << bufferIndex << " of '" << sourceName << "' has a uri outside of the asset's directory ('" << source->uri.string() << "'), refused !";

					return false;
				}

				std::vector< std::byte > bytes;

				if ( buffer.byteLength > 0 && !Base::IO::fileGetRange(*path, source->fileByteOffset, buffer.byteLength, bytes) )
				{
					TraceError{ClassId} << "The buffer #" << bufferIndex << " of '" << sourceName << "' ('" << source->uri.string() << "') is missing or shorter than its " << buffer.byteLength << " bytes, refused !";

					return false;
				}

				buffer.data = fastgltf::sources::Vector{.bytes = std::move(bytes), .mimeType = source->mimeType};
			}

			for ( size_t imageIndex = 0; imageIndex < asset.images.size(); ++imageIndex )
			{
				const auto * source = std::get_if< fastgltf::sources::URI >(&asset.images[imageIndex].data);

				if ( source != nullptr && !confinedURI(source->uri, basePath).has_value() )
				{
					TraceError{ClassId} << "The image #" << imageIndex << " of '" << sourceName << "' has a uri outside of the asset's directory ('" << source->uri.string() << "'), refused !";

					return false;
				}
			}

			return true;
		}
	}

	bool
	materialSamplesSecondarySet (const fastgltf::Material & material) noexcept
	{
		const auto & pbr = material.pbrData;
		bool secondary = textureCoordinateSet(pbr.baseColorTexture) == 1 || textureCoordinateSet(pbr.metallicRoughnessTexture) == 1 ||
			textureCoordinateSet(material.normalTexture) == 1 || textureCoordinateSet(material.occlusionTexture) == 1 ||
			textureCoordinateSet(material.emissiveTexture) == 1;

		if ( material.sheen != nullptr )
		{
			secondary = secondary || textureCoordinateSet(material.sheen->sheenColorTexture) == 1 || textureCoordinateSet(material.sheen->sheenRoughnessTexture) == 1;
		}

		if ( material.clearcoat != nullptr )
		{
			secondary = secondary || textureCoordinateSet(material.clearcoat->clearcoatTexture) == 1 || textureCoordinateSet(material.clearcoat->clearcoatRoughnessTexture) == 1 || textureCoordinateSet(material.clearcoat->clearcoatNormalTexture) == 1;
		}

		if ( material.specular != nullptr )
		{
			secondary = secondary || textureCoordinateSet(material.specular->specularTexture) == 1 || textureCoordinateSet(material.specular->specularColorTexture) == 1;
		}

		return secondary;
	}

	namespace
	{
		void
		describeMeshes (const fastgltf::Asset & asset, const std::string & resourcePrefix, const LoaderOptions & options, Asset & analysed) noexcept
		{
			analysed.meshes.resize(asset.meshes.size());

			const MeshoptBufferAdapter adapter{analysed.buffers.get()};

			DracoPrimitiveCache dracoCache;

			/* Whether each material samples TEXCOORD_1 with a map: only then is the secondary set read (and its geometry
			 * stream enabled). The loader's materials answer the same question from the same function. */
			std::vector< uint8_t > samplesSecondarySet(asset.materials.size(), 0);

			for ( size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex )
			{
				samplesSecondarySet[materialIndex] = materialSamplesSecondarySet(asset.materials[materialIndex]) ? 1 : 0;
			}

			const auto flipKey = std::string{};

			for ( size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex )
			{
				const auto & glTFMesh = asset.meshes[meshIndex];

				/* ⚠️⚠️ Keyed on the INDEX, not on the name. This is the defect that made the whole
				 * ClearCoatTest grid render material 0's red: eighteen meshes named
				 * 'ClearCoatSampleMesh', eighteen different materials, ONE renderable resource
				 * handed to all of them — geometry and material alike. See buildResourceKey(). */
				const std::string_view glTFMeshName{glTFMesh.name.data(), glTFMesh.name.size()};

				auto & description = analysed.meshes[meshIndex];
				description.geometryName = buildResourceKey(resourcePrefix + flipKey, "Geometry/", glTFMeshName, meshIndex);
				description.meshName = buildResourceKey(resourcePrefix + flipKey, "Mesh/", glTFMeshName, meshIndex);

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
					else
					{
						/* Not indexed: its vertices three by three (see the shape build). */
						totalTriangleCount += static_cast< uint32_t >(asset.accessors[positionIt->accessorIndex].count / 3);
					}
				}

				if ( totalVertexCount == 0 || totalTriangleCount == 0 )
				{
					TraceWarning{ClassId} << "Mesh '" << glTFMesh.name << "' has no valid triangle primitives, using default.";

					analysed.meshesComplete = false;

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

				/* Vertex colours (glTF `COLOR_0`), which MULTIPLY the base colour.
				 *
				 * ⚠️ Unlike the tangents above this is NOT all-or-nothing, because vertex colour HAS a
				 * neutral: white. So a mesh where only some primitives declare COLOR_0 carries the
				 * attribute and the others get white — and their PRIMITIVE picks the plain material
				 * variant, which ignores the attribute entirely. An unused vertex input attribute is
				 * legal; a shader reading an absent one is not, which is the whole reason for the
				 * `…-vc` material variant. */
				bool anyPrimitiveHasVertexColors = false;

				for ( const auto & primitive : glTFMesh.primitives )
				{
					if ( primitive.findAttribute("COLOR_0") != primitive.attributes.end() )
					{
						anyPrimitiveHasVertexColors = true;

						break;
					}
				}

				/* The secondary texture coordinates go into the mesh's VBO as soon as ONE primitive declares TEXCOORD_1 AND
				 * its material samples it; they are read for those primitives only, the others keep (0, 0) — their
				 * materials never read them (a material that does gets the `…-uv0` variant there). */
				const auto primitiveSamplesSecondarySet = [&samplesSecondarySet] (const fastgltf::Primitive & primitive) {
					return primitive.materialIndex.has_value() && primitive.materialIndex.value() < samplesSecondarySet.size() &&
						samplesSecondarySet[primitive.materialIndex.value()] != 0 && primitive.findAttribute("TEXCOORD_1") != primitive.attributes.end();
				};

				const bool anyPrimitiveSamplesSecondarySet = std::ranges::any_of(glTFMesh.primitives, primitiveSamplesSecondarySet);

				/* ⚠️⚠️ The colour vector is sized INSIDE `Shape::build()` below, never here:
				 * `build()` starts with `clear()`, which wipes `m_vertexColors`, and its callback is
				 * handed only the groups, the vertices and the triangles — the colours have to be
				 * reached through the shape itself, after the clear. Sizing them before `build()`
				 * silently produced an EMPTY vector and the first colour write segfaulted. */

				/* Second pass: fill vertices and triangles directly into pre-allocated vectors. */
				/* ⚠️ Set by any read site that could not obtain its data — a failed Draco decode, or a
				 * bufferView-less accessor nothing claimed. It must abort the mesh instead of letting
				 * the zero-filled fallback of glTF § 5.1.1 pass for geometry. */
				bool primitiveReadFailed = false;

				const bool buildSuccess = shape->build([&] (auto & groups, auto & vertices, auto & triangles) {
					vertices.resize(totalVertexCount);
					triangles.reserve(totalTriangleCount);

					if ( anyPrimitiveHasVertexColors )
					{
						/* Sized on the vertex count and pre-filled WHITE — the neutral multiplier — so
						 * a primitive that declares no COLOR_0, or an accessor shorter than the
						 * primitive, leaves the base colour untouched instead of black. */
						shape->vertexColors().assign(totalVertexCount, Vector< 4, float >{1.0F, 1.0F, 1.0F, 1.0F});
					}

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

							if ( !readDracoAwareAttribute< fastgltf::math::fvec3 >(asset, glTFPrimitive, "POSITION", posAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec3 & v) {
								vertices[globalVertexOffset + index].setPosition(Vector< 3, float >{v.x() * options.uniformScale, v.y() * options.uniformScale, v.z() * options.uniformScale});
								index++;
							}) )
							{
								primitiveReadFailed = true;
							}
						}

						/* Read normals directly into vertex array. */
						const auto * const normalIt = glTFPrimitive.findAttribute("NORMAL");

						if ( normalIt != glTFPrimitive.attributes.end() )
						{
							const auto & normAccessor = asset.accessors[normalIt->accessorIndex];
							size_t index = 0;

							if ( !readDracoAwareAttribute< fastgltf::math::fvec3 >(asset, glTFPrimitive, "NORMAL", normAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec3 & v) {
								vertices[globalVertexOffset + index].setNormal(Vector< 3, float >{v.x(), v.y(), v.z()});
								index++;
							}) )
							{
								primitiveReadFailed = true;
							}
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

								if ( !readDracoAwareAttribute< fastgltf::math::fvec4 >(asset, glTFPrimitive, "TANGENT", tangentAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec4 & v) {
									vertices[globalVertexOffset + index].setTangent(Vector< 4, float >{v.x(), v.y(), v.z(), v.w()});
									index++;
								}) )
								{
									primitiveReadFailed = true;
								}
							}
						}

						/* Read vertex colours (glTF COLOR_0).
						 *
						 * ⚠️ The accessor may be **VEC3 or VEC4**, and normalised ubyte/ushort as well
						 * as float — the reference Sponza uses VEC4 ubyte-normalised, the Khronos
						 * VertexColorTest VEC4 float. fastgltf converts a normalised integer accessor
						 * to [0,1] floats on read, so only the component COUNT needs dispatching: a
						 * VEC3 asset read through a vec4 iterator would take its alpha from whatever
						 * followed in the buffer. Absent alpha means OPAQUE, and the white pre-fill
						 * above already provides it. */
						if ( anyPrimitiveHasVertexColors )
						{
							const auto * const colorIt = glTFPrimitive.findAttribute("COLOR_0");

							if ( colorIt != glTFPrimitive.attributes.end() )
							{
								const auto & colorAccessor = asset.accessors[colorIt->accessorIndex];
								auto & colors = shape->vertexColors();
								size_t index = 0;

								if ( colorAccessor.type == fastgltf::AccessorType::Vec3 )
								{
									if ( !readDracoAwareAttribute< fastgltf::math::fvec3 >(asset, glTFPrimitive, "COLOR_0", colorAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec3 & v) {
										colors[globalVertexOffset + index] = {v.x(), v.y(), v.z(), 1.0F};
										index++;
									}) )
									{
										primitiveReadFailed = true;
									}
								}
								else
								{
									if ( !readDracoAwareAttribute< fastgltf::math::fvec4 >(asset, glTFPrimitive, "COLOR_0", colorAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec4 & v) {
										colors[globalVertexOffset + index] = {v.x(), v.y(), v.z(), v.w()};
										index++;
									}) )
									{
										primitiveReadFailed = true;
									}
								}
							}
						}

						/* Read texture coordinates directly into vertex array. */
						const auto * const textureCoordinatesIt = glTFPrimitive.findAttribute("TEXCOORD_0");

						if ( textureCoordinatesIt != glTFPrimitive.attributes.end() )
						{
							const auto & uvAccessor = asset.accessors[textureCoordinatesIt->accessorIndex];
							size_t index = 0;

							if ( !readDracoAwareAttribute< fastgltf::math::fvec2 >(asset, glTFPrimitive, "TEXCOORD_0", uvAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec2 & v) {
								vertices[globalVertexOffset + index].setTextureCoordinates(Vector< 3, float >{v.x(), v.y(), 0.0F});
								index++;
							}) )
							{
								primitiveReadFailed = true;
							}
						}

						/* The secondary texture coordinates, for a primitive whose material samples them. */
						if ( primitiveSamplesSecondarySet(glTFPrimitive) )
						{
							const auto * const secondaryIt = glTFPrimitive.findAttribute("TEXCOORD_1");
							const auto & secondaryAccessor = asset.accessors[secondaryIt->accessorIndex];
							size_t index = 0;

							if ( !readDracoAwareAttribute< fastgltf::math::fvec2 >(asset, glTFPrimitive, "TEXCOORD_1", secondaryAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec2 & v) {
								vertices[globalVertexOffset + index].setSecondaryTextureCoordinates(Vector< 2, float >{v.x(), v.y()});
								index++;
							}) )
							{
								primitiveReadFailed = true;
							}
						}

						if ( !options.skipSkinning )
						{
							/* Read bone joint indices (JOINTS_0) for skeletal animation. */
							const auto * const jointsIt = glTFPrimitive.findAttribute("JOINTS_0");

							if ( jointsIt != glTFPrimitive.attributes.end() )
							{
								const auto & jointsAccessor = asset.accessors[jointsIt->accessorIndex];
								size_t index = 0;
								/* The largest joint index of the mesh, checked against its skin once the skins are loaded. */
								auto & maxJoint = analysed.meshMaxJointIndex[meshIndex];

								if ( !readDracoAwareAttribute< fastgltf::math::u16vec4 >(asset, glTFPrimitive, "JOINTS_0", jointsAccessor, dracoCache, adapter, [&] (const fastgltf::math::u16vec4 & v) {
									maxJoint = std::max({maxJoint, static_cast< size_t >(v.x()), static_cast< size_t >(v.y()), static_cast< size_t >(v.z()), static_cast< size_t >(v.w())});

									vertices[globalVertexOffset + index].setInfluences(
										static_cast< int32_t >(v.x()),
										static_cast< int32_t >(v.y()),
										static_cast< int32_t >(v.z()),
										static_cast< int32_t >(v.w())
									);

									index++;
								}) )
								{
									primitiveReadFailed = true;
								}
							}

							/* Read bone weights (WEIGHTS_0) for skeletal animation. */
							const auto * const weightsIt = glTFPrimitive.findAttribute("WEIGHTS_0");

							if ( weightsIt != glTFPrimitive.attributes.end() )
							{
								const auto & weightsAccessor = asset.accessors[weightsIt->accessorIndex];
								size_t index = 0;

								if ( !readDracoAwareAttribute< fastgltf::math::fvec4 >(asset, glTFPrimitive, "WEIGHTS_0", weightsAccessor, dracoCache, adapter, [&] (const fastgltf::math::fvec4 & v) {
									vertices[globalVertexOffset + index].setWeights(v.x(), v.y(), v.z(), v.w());
									index++;
								}) )
								{
									primitiveReadFailed = true;
								}
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

							if ( !readDracoAwareIndices(asset, glTFPrimitive, indexAccessor, dracoCache, adapter, [&] (uint32_t value) {
								/* ⚠️ An index refers to a vertex of ITS primitive (glTF § 3.7.2.1): past them, the triangle
								 * points at another primitive's vertices or out of the shape — the normal and tangent
								 * computations then read and wrote out of bounds (fuzz_gltf, 2026-10-18). The primitive is
								 * unreadable: the mesh falls back to the default one and the asset is refused, as for a failed
								 * decode. */
								if ( value >= vertexCount )
								{
									primitiveReadFailed = true;

									return;
								}

								/* ⚠️ `operator[]`, never `at()`: the cascade builds with exceptions
								 * disabled, so a throwing std call is forbidden — `at()` would call
								 * std::terminate instead of reporting anything. Nothing is lost here:
								 * `triangleSlot` returns to 0 the moment it reaches 3, so the write
								 * index is only ever 0, 1 or 2. */
								triangleBuffer[triangleSlot++] = globalVertexOffset + value;

								if ( triangleSlot == 3 )
								{
									auto & triangle = triangles.emplace_back(triangleBuffer[0], triangleBuffer[1], triangleBuffer[2]);

									/* ⚠️ `ShapeTriangle`'s vertex-COLOUR indexes are a SEPARATE list from
									 * its vertex indexes (a face-varying attribute, as OBJ and FBX need)
									 * and they default to {0, 0, 0}. Leaving them alone would paint every
									 * triangle with colour 0. glTF's COLOR_0 shares POSITION's indexing,
									 * so the two index sets are simply equal here. */
									if ( anyPrimitiveHasVertexColors )
									{
										triangle.setVertexColorIndex(0, triangleBuffer[0]);
										triangle.setVertexColorIndex(1, triangleBuffer[1]);
										triangle.setVertexColorIndex(2, triangleBuffer[2]);
									}

									triangleSlot = 0;
								}
							}) )
							{
								primitiveReadFailed = true;
							}

							/* Track triangles per group. */
							if ( !groups.empty() )
							{
								groups.back().second += primTriangleCount;
							}
						}
						else
						{
							/* ⚠️ A primitive without indices draws its vertices in order, three by three (glTF § 3.7.2.1).
							 * Generated HERE, after the validations (owner decision, 2026-10-18): fastgltf's
							 * GenerateMeshIndices option did it during the parse, BEFORE them — an unchecked POSITION
							 * accessor index, an unbounded allocation and a count that did not match the generated data
							 * (fuzz_gltf: 50 crashes). */
							const auto primTriangleCount = vertexCount / 3;

							for ( uint32_t triangleIndex = 0; triangleIndex < primTriangleCount; ++triangleIndex )
							{
								const auto first = globalVertexOffset + (triangleIndex * 3);
								auto & triangle = triangles.emplace_back(first, first + 1, first + 2);

								if ( anyPrimitiveHasVertexColors )
								{
									triangle.setVertexColorIndex(0, first);
									triangle.setVertexColorIndex(1, first + 1);
									triangle.setVertexColorIndex(2, first + 2);
								}
							}

							if ( !groups.empty() )
							{
								groups.back().second += primTriangleCount;
							}
						}

						globalVertexOffset += vertexCount;
					}

					return true;
				}, true); /* textureCoordinatesDeclared = true */

				/* ⚠️ Kept SEPARATE from the invalid-shape branch below, and reported as an ERROR
				 * rather than a warning: a shape that fails isValid() is malformed content, whereas
				 * this one means the loader could not read data it was told exists — a defect on our
				 * side or a corrupt bitstream. Conflating the two would bury the second in the noise
				 * of the first. */
				if ( primitiveReadFailed )
				{
					TraceError{ClassId} << "Mesh '" << glTFMesh.name << "' could not be read (compressed geometry failed to decode, an accessor had no data source, or an index past its primitive's vertices), using default.";

					analysed.meshesComplete = false;

					continue;
				}

				if ( !buildSuccess || !shape->isValid() )
				{
					TraceWarning{ClassId} << "Generated shape is invalid for mesh '" << glTFMesh.name << "', using default.";

					analysed.meshesComplete = false;

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

				description.shape = std::move(shape);
				description.allPrimitivesHaveTangents = allPrimitivesHaveTangents;
				description.hasVertexColors = anyPrimitiveHasVertexColors;
				description.samplesSecondarySet = anyPrimitiveSamplesSecondarySet;
			}

			/* A POSITIVE trace on purpose: the decoded geometry is indistinguishable from plain
			 * geometry once built, so this line is the only cheap evidence that the Draco path ran
			 * at all rather than being silently skipped. */
			if ( dracoCache.meshCount() > 0 )
			{
				TraceInfo{ClassId} << "Decoded " << dracoCache.meshCount() << " Draco-compressed primitive bitstream(s).";
			}
		}

		void
		describeSkins (const fastgltf::Asset & asset, const std::string & resourcePrefix, const LoaderOptions & options, Asset & analysed) noexcept
		{
			if ( asset.skins.empty() )
			{
				return;
			}

			const MeshoptBufferAdapter adapter{analysed.buffers.get()};

			/* ⚠️ The rig follows the mesh it deforms, or the skin slides off the geometry: bind-pose
			 * translations, bind-pose rotations and inverse bind matrices all go through the same
			 * conversion. */

			/* Build a node-index → parent-node-index lookup from the node tree. */
			const auto nodeParents = nodeParentTable(asset);

			/* The nodes an animation targets (see the root transform below). */
			std::unordered_set< size_t > animatedNodes;

			for ( const auto & animation : asset.animations )
			{
				for ( const auto & channel : animation.channels )
				{
					if ( channel.nodeIndex.has_value() )
					{
						animatedNodes.insert(channel.nodeIndex.value());
					}
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

				/* Read inverse bind matrices from the accessor (if provided). */
				std::vector< Matrix< 4, float > > inverseBindMatrices(jointCount);

				if ( glTFSkin.inverseBindMatrices.has_value() )
				{
					const auto & ibmAccessor = asset.accessors[glTFSkin.inverseBindMatrices.value()];
					size_t index = 0;

					fastgltf::iterateAccessor< fastgltf::math::fmat4x4 >(asset, ibmAccessor, [&] (const fastgltf::math::fmat4x4 & m) {
						std::array< float, 16 > data{};

						/* ⚠️ `operator[]`, never `at()` — exceptions are disabled cascade-wide.
						 * Both bounds are literals, so the index stays inside [0, 16). */
						for ( size_t col = 0; col < 4; ++col )
						{
							for ( size_t row = 0; row < 4; ++row )
							{
								data[(col * 4) + row] = m.col(col)[row];
							}
						}

						auto ibm = Matrix< 4, float >{data};

						/* Scale the translation column to keep the binding math coherent with the
						 * scaled vertex positions and scaled joint TRS translations. The linear part
						 * (rotation + uniform 1x1 scale) is unaffected by uniform scaling around the
						 * origin, so only the translation column needs it. */
						ibm[M4x4Col3Row0] *= options.uniformScale;
						ibm[M4x4Col3Row1] *= options.uniformScale;
						ibm[M4x4Col3Row2] *= options.uniformScale;

						/* ⚠️ Conjugate the WHOLE matrix (M·IBM·M), linear part included — not just the
						 * translation column as the uniform scale above does. Skinning stays coherent
						 * because the joint world matrices are conjugated the same way, so the two M
						 * cancel: (M·W·M)·(M·IBM·M)·(M·v) = M·(W·IBM·v). */
						/* NOTE: glTF allows MORE matrices than joints (count >= joints): only the first ones are
						 * the skin's, and writing past jointCount overflowed the vector. */
						if ( index < jointCount )
						{
							inverseBindMatrices[index] = ibm;
						}

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

				/* Build Joint array, parents first (glTF allows any order: see skeletonLayout()). The skeleton holds the
				 * skin's joints and the non-joint nodes between them and the common ancestor of the mesh; the Skin maps the
				 * glTF joint index (vertex JOINTS_0) to its skeleton index. */
				const auto layout = skeletonLayout(asset, skinIndex, nodeParents);
				const auto & skeletonIndex = layout.jointOrder;

				std::unordered_map< size_t, size_t > nodeToSkinJoint;
				nodeToSkinJoint.reserve(jointCount);

				for ( size_t j = 0; j < jointCount; ++j )
				{
					nodeToSkinJoint[glTFSkin.joints[j]] = j;
				}

				std::vector< Joint< float > > engineJoints(layout.nodes.size());

				for ( size_t index = 0; index < layout.nodes.size(); ++index )
				{
					const auto glTFNodeIndex = layout.nodes[index];
					const auto & glTFNode = asset.nodes[glTFNodeIndex];
					auto & joint = engineJoints[index];

					joint.name = std::string{glTFNode.name};

					/* NOTE: A carried (non-joint) node skins no vertex: its inverse bind matrix stays the identity. */
					if ( const auto skinJoint = nodeToSkinJoint.find(glTFNodeIndex); skinJoint != nodeToSkinJoint.end() )
					{
						joint.inverseBindMatrix = inverseBindMatrices[skinJoint->second];
					}

					/* The parent is the glTF parent when the layout holds it; the common ancestor and above are the scene
					 * graph's. */
					joint.parentIndex = NoParent;

					if ( const auto parentNode = nodeParents[glTFNodeIndex]; parentNode != NoParent )
					{
						if ( const auto it = layout.nodeToSkeletonIndex.find(static_cast< size_t >(parentNode)); it != layout.nodeToSkeletonIndex.end() )
						{
							joint.parentIndex = it->second;
						}
					}

					/* Extract local TRS from the node. GLTF stores parent-relative transforms. */
					if ( const auto * trs = std::get_if< fastgltf::TRS >(&glTFNode.transform) )
					{
						joint.translation = Vector< 3, float >{
							trs->translation.x() * options.uniformScale,
							trs->translation.y() * options.uniformScale,
							trs->translation.z() * options.uniformScale};
						joint.rotation = Quaternion< float >{trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()};
						/* NOTE: A scale is invariant under the flip — M·diag(s)·M = diag(s). */
						joint.scale = {trs->scale.x(), trs->scale.y(), trs->scale.z()};
					}
				}

				if ( layout.carriedNodeCount > 0 )
				{
					TraceInfo{ClassId} << "Skin #" << skinIndex << ": " << layout.carriedNodeCount << " non-joint node(s) between the joints and the mesh's common ancestor carried into the skeleton.";
				}

				Skeleton< float > skeleton{std::move(engineJoints)};

				/* Build Skin: a skin-local (glTF) index maps to its parent-first skeleton index; the inverse bind matrices
				 * stay in skin-local order (the Skin indexes them by the vertex JOINTS_0 value). */
				Base::Animation::Skin< float > skin{skeletonIndex, std::move(inverseBindMatrices)};

				/* Register the skeleton as a managed resource. */
				const auto skinName = glTFSkin.name.empty() ?
					"skin_" + std::to_string(skinIndex) :
					std::string{glTFSkin.name};

				analysed.skins.push_back(Skin{.resourceName = Base::String::concatenate(resourcePrefix, "/skeleton/", skinName), .skeleton = std::move(skeleton), .skin = std::move(skin)});

				/* Record which meshes reference this skin for later association with renderables, each with the root
				 * transform that brings the skeleton into its node's space: (mesh node world)⁻¹ · (common ancestor world),
				 * at rest. */
				const auto commonAncestorWorld = nodeWorldMatrix(asset, layout.commonAncestor, nodeParents, options.uniformScale);

				for ( size_t nodeIdx = 0; nodeIdx < asset.nodes.size(); ++nodeIdx )
				{
					const auto & node = asset.nodes[nodeIdx];

					if ( !node.skinIndex.has_value() || node.skinIndex.value() != skinIndex || !node.meshIndex.has_value() )
					{
						continue;
					}

					const auto meshIndex = node.meshIndex.value();

					analysed.meshToSkinIndex[meshIndex] = skinIndex;

					const auto meshWorldInverse = nodeWorldMatrix(asset, static_cast< int32_t >(nodeIdx), nodeParents, options.uniformScale).tryInverse();

					if ( !meshWorldInverse )
					{
						TraceWarning{ClassId} << "The skinned mesh node #" << nodeIdx << " has a singular world transform: its skeleton keeps no root transform.";

						continue;
					}

					const auto rootTransform = meshWorldInverse.value() * commonAncestorWorld;

					/* NOTE: A mesh is one renderable, whatever the node using it: the first node's transform is kept. */
					if ( const auto [it, inserted] = analysed.meshSkinRootTransform.try_emplace(meshIndex, rootTransform); !inserted && it->second != rootTransform )
					{
						TraceWarning{ClassId} << "The skinned mesh #" << meshIndex << " is used by nodes with different transforms: the first one's is kept.";
					}

					/* NOTE: The root transform is taken at rest: an animated node between the mesh and the common ancestor is
					 * not followed. */
					for ( auto pathNode = static_cast< int32_t >(nodeIdx); pathNode != NoParent && pathNode != layout.commonAncestor; pathNode = nodeParents[static_cast< size_t >(pathNode)] )
					{
						if ( animatedNodes.contains(static_cast< size_t >(pathNode)) )
						{
							TraceWarning{ClassId} << "The node #" << pathNode << " above the skinned mesh node #" << nodeIdx << " is animated: the skinned mesh does not follow that animation.";

							break;
						}
					}
				}
			}
		}

		void
		describeAnimations (const fastgltf::Asset & asset, const std::string & resourcePrefix, const LoaderOptions & options, Asset & analysed) noexcept
		{
			if ( asset.animations.empty() )
			{
				return;
			}

			const MeshoptBufferAdapter adapter{analysed.buffers.get()};

			/* ⚠️ The clips are mirrored with the very same flip as the bind pose they animate. A rig
			 * whose bind pose is mirrored but whose clips are not snaps to the unmirrored pose on the
			 * first animated frame — the classic symptom of a half-applied conversion. */

			/* For mapping node indices to joint indices, we need the skin context: the SKELETON index, parents first,
			 * exactly as loadSkins() lays the skeleton out (skeletonLayout(): the joints and the non-joint nodes it
			 * carries). */
			std::unordered_map< size_t, int32_t > nodeToJointIndex;
			std::unordered_set< size_t > carriedNodes;

			if ( !asset.skins.empty() )
			{
				const auto & skin = asset.skins[0];
				auto layout = skeletonLayout(asset, 0, nodeParentTable(asset));

				nodeToJointIndex = std::move(layout.nodeToSkeletonIndex);

				const std::unordered_set< size_t > joints{skin.joints.begin(), skin.joints.end()};

				for ( const auto & [node, index] : nodeToJointIndex )
				{
					if ( !joints.contains(node) )
					{
						carriedNodes.insert(node);
					}
				}
			}

			analysed.clips.reserve(asset.animations.size());
			analysed.nodeClips.reserve(asset.animations.size());

			size_t animationIndex = 0;

			for ( const auto & glTFAnim : asset.animations )
			{
				/* ⚠️ ONE glTF animation may drive skin joints AND plain nodes through the very same
				 * construct — a character whose sword swings on a non-skinned node is the ordinary
				 * case. The two need different evaluators, so the channels are SORTED here and the
				 * animation comes out as up to two clips sharing one name. */
				std::vector< AnimationChannel< float > > channels;
				channels.reserve(glTFAnim.channels.size());

				std::vector< AnimationChannel< float > > nodeChannels;

				const auto clipName = glTFAnim.name.empty()
					? "clip_" + std::to_string(animationIndex)
					: std::string{glTFAnim.name};

				animationIndex++;

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

					/* Map the glTF node index to a joint index in the skeleton. A node that is NOT a
					 * skin joint is animated as ITSELF: its target index is the node index, which the
					 * loader keeps 1:1 with SceneData::nodes (buildNodeDescriptors resizes the array
					 * to asset.nodes.size() and writes it in place). */
					const auto nodeIdx = glTFChannel.nodeIndex.value();
					const auto it = nodeToJointIndex.find(nodeIdx);
					const bool isJointChannel = it != nodeToJointIndex.end();

					const auto targetIndex = isJointChannel
						? it->second
						: static_cast< int32_t >(nodeIdx);

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
					channel.targetIndex = targetIndex;
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
							const auto valueScale = isTranslation ? options.uniformScale : 1.0F;

							/* ⚠️ Same asymmetry for the axis flip: a translation is mirrored, a scale is
							 * invariant (M·diag(s)·M = diag(s)). Mirroring the scale track would make
							 * the rig turn inside-out on the first animated frame. Tangents live in this
							 * very array and are mirrored with the values, as they must be. */
							std::vector< Vector< 3, float > > values;
							values.reserve(timestamps.size() * valuesPerKeyFrame);

							fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, outputAccessor, [&] (const fastgltf::math::fvec3 & v) {
								values.emplace_back(v.x() * valueScale, v.y() * valueScale, v.z() * valueScale);
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
								values.emplace_back(v.x(), v.y(), v.z(), v.w());
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

					if ( isJointChannel )
					{
						/* NOTE: A node the skeleton carries (a non-joint ancestor of the joints) can still parent plain
						 * content in the scene graph: its channel drives both, as a node channel targeting the node itself. */
						if ( carriedNodes.contains(nodeIdx) )
						{
							auto nodeChannel = channel;
							nodeChannel.targetIndex = static_cast< int32_t >(nodeIdx);

							nodeChannels.push_back(std::move(nodeChannel));
						}

						channels.push_back(std::move(channel));
					}
					else
					{
						nodeChannels.push_back(std::move(channel));
					}
				}

				if ( const auto removed = rebaseAnimationTime(channels, nodeChannels); removed > 0.0F )
				{
					TraceDebug{ClassId} << "Animation '" << clipName << "' rebased to t = 0 (it started at " << removed << " s).";
				}

				/* ⚠️ The fallback name belongs to the CLIP, not only to its resource key: the animator
				 * indexes its clips on the clip's own name, so two anonymous animations used to
				 * collapse onto the empty key and only one of them stayed reachable. */
				if ( !channels.empty() )
				{
					analysed.clips.push_back(Clip{.resourceName = Base::String::concatenate(resourcePrefix, "/animation/", clipName), .clip = AnimationClip< float >{clipName, std::move(channels)}});
				}

				if ( !nodeChannels.empty() )
				{
					analysed.nodeClips.push_back(Clip{.resourceName = Base::String::concatenate(resourcePrefix, "/node-animation/", clipName), .clip = AnimationClip< float >{clipName, std::move(nodeChannels)}});
				}
			}

			if ( !analysed.clips.empty() || !analysed.nodeClips.empty() )
			{
				TraceDebug{ClassId} << "Loaded " << analysed.clips.size() << " skeletal clip(s) and "
					<< analysed.nodeClips.size() << " node clip(s).";
			}
		}

		void
		describeNodes (const fastgltf::Asset & asset, const std::string & resourcePrefix, const LoaderOptions & options, SceneData & output) noexcept
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

			/* NOTE: an ITERATIVE walk (explicit stack): the depth comes from the file, a recursion overflows the stack on a
			 * long chain. The hierarchy is a strict forest (isStrictNodeForest(), checked at load): no node twice. */
			std::vector< size_t > pending;
			pending.reserve(asset.nodes.size());

			/* Build from scene root nodes. */
			output.rootNodeIndices.reserve(glTFScene.nodeIndices.size());

			for ( const auto nodeIndex : glTFScene.nodeIndices )
			{
				output.rootNodeIndices.push_back(nodeIndex);
				pending.push_back(nodeIndex);
			}

			while ( !pending.empty() )
			{
				const auto nodeIndex = pending.back();
				pending.pop_back();

				if ( nodeIndex >= asset.nodes.size() )
				{
					continue;
				}

				const auto & glTFNode = asset.nodes[nodeIndex];

				/* Skip excluded nodes and their entire subtree. */
				if ( !glTFNode.name.empty() && options.excludedNodeNames.contains(std::string{glTFNode.name}) )
				{
					continue;
				}

				auto & descriptor = output.nodes[nodeIndex];
				descriptor.name = buildNodeName(resourcePrefix, glTFNode, nodeIndex);
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
					pending.push_back(childIndex);
				}
			}
		}

		void
		describeLights (const fastgltf::Asset & asset, SceneData & output) noexcept
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

				/* KHR_lights_punctual and the engine agree on the UNITS: illuminance in lux for a
				 * directional light, luminous intensity in candela for a point or a spot. There is
				 * deliberately no unit conversion here. ⚠️ They differ on the COLOUR: KHR's intensity is
				 * what the light would emit if it were white and its colour multiplies it, while the
				 * engine's colour is a unit-luminance chromaticity since 2026-09-25 — the descriptor keeps
				 * the KHR meaning and SceneDataConsumer::attachLight() folds the colour's luminance into the
				 * intensity. */
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

				const auto * const typeName = [&descriptor] () -> const char * {
					switch ( descriptor.type )
					{
						case LightType::Directional :
							return "directional ";

						case LightType::Point :
							return "point ";

						case LightType::Spot :
							return "spot ";

						case LightType::Environment :
							return "environment ";
					}

					return "unknown ";
				}();

				TraceDebug{ClassId} <<
					"Light '" << descriptor.name << "': " << typeName <<
					descriptor.intensity << ( descriptor.type == LightType::Directional ? " lux" : " cd" ) <<
					", range " << descriptor.range <<
					", cone " << descriptor.innerConeAngle << "/" << descriptor.outerConeAngle << " deg.";

				output.lights.emplace_back(std::move(descriptor));
			}
		}

		void
		describeCameras (const fastgltf::Asset & asset, SceneData & output) noexcept
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

	Asset::Asset () noexcept = default;

	Asset::~Asset () = default;

	Asset::Asset (Asset &&) noexcept = default;

	Asset & Asset::operator= (Asset &&) noexcept = default;

	std::optional< Asset >
	parse (fastgltf::GltfDataBuffer & data, const std::filesystem::path & basePath, bool binary, const std::string & sourceName) noexcept
	{
		constexpr auto declaredExtensions =
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
			 *    KHR_meshopt_compression, its ratified successor, is the same bitstream plus the Color
			 *    filter: fastgltf fills the same `BufferView::meshoptCompression` for both.
			 *  - KHR_mesh_quantization : attributes are normalised integers. Nothing to do — fastgltf
			 *    dequantises them on read, and the compensating scale is carried by the node
			 *    transforms extractFrameFromNode() already reads. */
			fastgltf::Extensions::KHR_texture_basisu |
			fastgltf::Extensions::EXT_meshopt_compression |
			fastgltf::Extensions::KHR_meshopt_compression |
			fastgltf::Extensions::KHR_mesh_quantization |
			/* NOTE: The second compression family, independent from the meshopt one above.
			 * fastgltf only PARSES the extension — it fills `Primitive::dracoCompression` with
			 * the buffer view holding the bitstream and the glTF-name -> Draco-attribute-id map,
			 * and decodes nothing. DracoPrimitiveCache below does the decoding.
			 *
			 * ⚠️⚠️ Declaring this line is what makes such an asset PARSE instead of being
			 * refused with Error::MissingExtensions — and a parsed Draco asset is exactly where
			 * a missed read site turns into silent garbage rather than an error, because the
			 * accessors of a Draco primitive carry NO bufferView and glTF § 5.1.1 then mandates
			 * zeros. Every attribute read in loadMeshes() therefore goes through
			 * readDracoAwareAttribute()/readDracoAwareIndices(), which fail loudly instead. */
			fastgltf::Extensions::KHR_draco_mesh_compression |
			/* NOTE: Images are WebP containers, hung off `Texture::webpImageIndex` exactly as
			 * KHR_texture_basisu hangs its own — see resolveTexture(). Decoded through
			 * PixelFactory::FileFormatWebP, in emeraude-base: a WebP is plain pixels, so it is an
			 * image format of the foundation and not a glTF special case. */
			fastgltf::Extensions::EXT_texture_webp;

		fastgltf::Parser parser(declaredExtensions);

		/* NOTE: NOT LoadExternalBuffers: fastgltf would follow a buffer uri as written (see loadExternalBuffers()). */
		constexpr auto options =
			fastgltf::Options::DecomposeNodeMatrices;

		auto result = binary ?
			parser.loadGltfBinary(data, basePath, options) :
			parser.loadGltf(data, basePath, options);

		if ( result.error() != fastgltf::Error::None )
		{
			TraceError{ClassId} << "Failed to parse '" << sourceName << "' : " << fastgltf::getErrorMessage(result.error());

			/* ⚠️ fastgltf's own message for this case is "One or more extensions are required by the
			 * glTF but not enabled in the Parser" — it never says WHICH, and the asset is refused
			 * whole, so there is nothing left to inspect either. Naming them is the difference
			 * between a one-line fix and an afternoon of bisecting an asset by hand. */
			if ( result.error() == fastgltf::Error::MissingExtensions )
			{
				reportMissingExtensions(sourceName, data, basePath, declaredExtensions);
			}

			return std::nullopt;
		}

		Asset analysed;
		analysed.gltf = std::make_unique< fastgltf::Asset >(std::move(result.get()));

		if ( !loadExternalBuffers(*analysed.gltf, basePath, sourceName) )
		{
			return std::nullopt;
		}
		const auto & asset = *analysed.gltf;

		/* NOTE: fastgltf's parse does NOT range-check the indices an asset holds (material, accessor, buffer view,
		 * image, node, skin…): only validate() does. A hostile file — a dropped one — would otherwise index out of
		 * bounds at every asset.X[i] below. Checked on the whole glTF-Sample-Assets corpus (314 assets) and the
		 * data stores (11): none refused (owner decision 2026-09-30, plan Ave Robustus). */
		/* NOTE: validate() itself indexes a few arrays without a range check: those indices first. */
		if ( std::string indexError; !hasValidatableIndices(asset, indexError) )
		{
			TraceError{ClassId} << "The glTF '" << sourceName << "' is invalid, refused : " << indexError << '.';

			return std::nullopt;
		}

		if ( const auto validation = fastgltf::validate(asset); validation != fastgltf::Error::None )
		{
			TraceError{ClassId} << "The glTF '" << sourceName << "' is invalid, refused : " << fastgltf::getErrorMessage(validation);

			return std::nullopt;
		}

		/* NOTE: validate() does not check the node hierarchy; the node walks rely on a strict forest. */
		if ( std::string hierarchyError; !isStrictNodeForest(asset, hierarchyError) )
		{
			TraceError{ClassId} << "The glTF '" << sourceName << "' has an invalid node hierarchy, refused : " << hierarchyError << '.';

			return std::nullopt;
		}

		/* NOTE: validate() does not check the extents either; every accessor read below relies on them. */
		if ( std::string extentError; !hasConsistentExtents(asset, extentError) )
		{
			TraceError{ClassId} << "The glTF '" << sourceName << "' has inconsistent data extents, refused : " << extentError << '.';

			return std::nullopt;
		}

		/* EXT_meshopt_compression working set. Every accessor read below goes through it, and it
		 * is released at the end of this function. */

		/* EXT/KHR meshopt working set: alive until the loader has read its images (releaseBuffers()). */
		analysed.buffers = std::make_unique< MeshoptBufferCache >();

		return analysed;
	}

	bool
	analyse (Asset & analysed, const std::string & resourcePrefix, const LoaderOptions & options, const std::string & sourceName) noexcept
	{
		const auto & asset = *analysed.gltf;

		describeMeshes(asset, resourcePrefix, options, analysed);

		if ( !options.skipSkinning )
		{
			if ( !asset.skins.empty() )
			{
				describeSkins(asset, resourcePrefix, options, analysed);
			}

			if ( !asset.animations.empty() )
			{
				describeAnimations(asset, resourcePrefix, options, analysed);
			}

		for ( const auto & [meshIdx, skinIdx] : analysed.meshToSkinIndex )
		{
			const auto maxJoint = analysed.meshMaxJointIndex.find(meshIdx);

			if ( skinIdx < analysed.skins.size() && maxJoint != analysed.meshMaxJointIndex.end() && maxJoint->second >= analysed.skins[skinIdx].skin.jointCount() )
			{
				TraceError{ClassId} << "The glTF '" << sourceName << "' has a mesh #" << meshIdx << " whose JOINTS_0 references joint " << maxJoint->second << " of a skin of " << analysed.skins[skinIdx].skin.jointCount() << " joints, refused !";

				return false;
			}
		}

		/* Attach skeletal data to renderables that have associated skins. */
		}

		/* Collect all joint node indices from skins so they can be
		 * skipped during scene hierarchy building. Joint transforms
		 * are handled by the SkeletalAnimator, not by scene nodes. */
		analysed.scene.skinJointNodeIndices.clear();

		for ( const auto & skin : asset.skins )
		{
			for ( const auto jointIndex : skin.joints )
			{
				analysed.scene.skinJointNodeIndices.insert(jointIndex);
			}
		}

		/* NOTE: A view that failed to decode served zeros to its accessors: the data is not the file's. */

		describeLights(asset, analysed.scene);
		describeCameras(asset, analysed.scene);
		describeNodes(asset, resourcePrefix, options, analysed.scene);

		return true;
	}

	std::span< const std::byte >
	bufferView (Asset & analysed, size_t bufferViewIndex) noexcept
	{
		if ( analysed.buffers == nullptr || analysed.gltf == nullptr )
		{
			return {};
		}

		const auto bytes = analysed.buffers->view(*analysed.gltf, bufferViewIndex);

		return {bytes.data(), bytes.size()};
	}

	bool
	buffersFailed (const Asset & analysed) noexcept
	{
		return analysed.buffers != nullptr && analysed.buffers->decodeFailed();
	}

	void
	releaseBuffers (Asset & analysed) noexcept
	{
		if ( analysed.buffers == nullptr )
		{
			return;
		}

		if ( analysed.buffers->viewCount() > 0 )
		{
			TraceInfo{ClassId} <<
				"Released " << (analysed.buffers->decodedBytes() / 1048576) << " MiB of decoded data from " <<
				analysed.buffers->viewCount() << " meshopt-compressed buffer view(s).";
		}

		analysed.buffers.reset();
	}
}
