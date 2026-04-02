/*
 * src/AssetLoaders/GLTFLoader.cpp
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

#include "GLTFLoader.hpp"

/* STL inclusions. */
#include <cmath>
#include <cstring>
#include <variant>

/* 3rd inclusions. */
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"

/* Local inclusions. */
#include "AssetData.hpp"
#include "Graphics/Geometry/Helpers.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/PBRResource.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SimpleMeshResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Animations/SkeletonResource.hpp"
#include "Animations/AnimationClipResource.hpp"
#include "Libs/Animation/Joint.hpp"
#include "Libs/Animation/Skeleton.hpp"
#include "Libs/Animation/Skin.hpp"
#include "Libs/Animation/AnimationClip.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Quaternion.hpp"
#include "Libs/Math/TransformUtils.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/FileIO.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Libs/PixelFactory/StreamIO.hpp"
#include "Libs/VertexFactory/Shape.hpp"
#include "Tracer.hpp"

namespace EmEn::AssetLoaders
{
	using namespace Graphics;
	using namespace Graphics::Geometry;
	using namespace Libs::Animation;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Libs::PixelFactory;

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

	/* Helper: Extract a CartesianFrame from a glTF node's TRS transform. */
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
			rotMatrix[0]  = 1.0F - 2.0F * (yy + zz);
			rotMatrix[1]  = 2.0F * (xy + wz);
			rotMatrix[2]  = 2.0F * (xz - wy);
			rotMatrix[3]  = 0.0F;
			rotMatrix[4]  = 2.0F * (xy - wz);
			rotMatrix[5]  = 1.0F - 2.0F * (xx + zz);
			rotMatrix[6]  = 2.0F * (yz + wx);
			rotMatrix[7]  = 0.0F;
			rotMatrix[8]  = 2.0F * (xz + wy);
			rotMatrix[9]  = 2.0F * (yz - wx);
			rotMatrix[10] = 1.0F - 2.0F * (xx + yy);
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

	bool
	GLTFLoader::load (const std::filesystem::path & filepath, AssetData & output) noexcept
	{
		/* Generate a resource prefix from the filename. */
		m_resourcePrefix = "glTF:" + filepath.stem().string() + "/";

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
			fastgltf::Extensions::KHR_materials_anisotropy |
			fastgltf::Extensions::KHR_materials_volume |
			fastgltf::Extensions::KHR_lights_punctual
		);

		constexpr auto options =
			fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::DecomposeNodeMatrices |
			fastgltf::Options::GenerateMeshIndices;

		auto result = (filepath.extension() == ".glb")
			? parser.loadGltfBinary(gltfFile.get(), parentPath, options)
			: parser.loadGltf(gltfFile.get(), parentPath, options);

		if ( result.error() != fastgltf::Error::None )
		{
			TraceError{ClassId} << "Failed to parse '" << filepath << "' : " << fastgltf::getErrorMessage(result.error());

			return false;
		}

		const auto & asset = result.get();

		/* Load pipeline: Images → Materials → Meshes → Skins → Animations → Node descriptors. */
		TraceInfo{ClassId} << "Phase 1: Loading " << asset.images.size() << " images...";

		if ( !this->loadImages(asset, parentPath) )
		{
			Tracer::warning(ClassId, "Some images failed to load, continuing with defaults.");
		}

		TraceInfo{ClassId} << "Phase 2: Loading " << asset.materials.size() << " materials...";

		if ( !this->loadMaterials(asset) )
		{
			Tracer::warning(ClassId, "Some materials failed to load, continuing with defaults.");
		}

		TraceInfo{ClassId} << "Phase 3: Loading " << asset.meshes.size() << " meshes...";

		if ( !this->loadMeshes(asset, output) )
		{
			Tracer::error(ClassId, "Failed to load meshes !");

			return false;
		}

		if ( !m_options.skipSkinning )
		{
			if ( !asset.skins.empty() )
			{
				TraceInfo{ClassId} << "Phase 4: Loading " << asset.skins.size() << " skins...";

				this->loadSkins(asset, output);
			}

			if ( !asset.animations.empty() )
			{
				TraceInfo{ClassId} << "Phase 5: Loading " << asset.animations.size() << " animations...";

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

					TraceInfo{ClassId} << "Attached skeletal data to mesh " << meshIdx << ".";
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

		/* Build format-agnostic node descriptors. */
		TraceInfo{ClassId} << "Phase 6: Building node descriptors...";

		this->buildNodeDescriptors(asset, output);

		/* Populate output with loaded resources. */
		output.skeletons = m_skeletons;
		output.animationClips = m_animationClips;
		output.skinJointNodeIndices = m_skinJointNodeIndices;

		TraceInfo{ClassId} << "Loading complete.";

		return true;
	}

	bool
	GLTFLoader::loadImages (const fastgltf::Asset & asset, const std::filesystem::path & basePath) noexcept
	{
		m_images.resize(asset.images.size());

		bool allSuccess = true;

		for ( size_t imageIndex = 0; imageIndex < asset.images.size(); ++imageIndex )
		{
			const auto & glTFImage = asset.images[imageIndex];

			std::string name;
			name.reserve(m_resourcePrefix.size() + 7 + glTFImage.name.size());
			name = m_resourcePrefix;
			name += "Image/";
			if ( glTFImage.name.empty() )
				name += std::to_string(imageIndex);
			else
				name.append(glTFImage.name.data(), glTFImage.name.size());

			auto image = std::visit(fastgltf::visitor{
				/* File-based image (external .gltf). */
				[&] (const fastgltf::sources::URI & uri) -> std::shared_ptr< ImageResource > {
					/* Copy path immediately to avoid string_view lifetime issues. */
					const std::filesystem::path fullPath = basePath / std::filesystem::path{std::string{uri.uri.path()}};

					return m_resources.container< ImageResource >()
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
				},

				/* Embedded image (in .glb buffer view). */
				[&] (const fastgltf::sources::BufferView & bufferView) -> std::shared_ptr< ImageResource > {
					const auto & view = asset.bufferViews[bufferView.bufferViewIndex];
					const auto & buffer = asset.buffers[view.bufferIndex];

					return std::visit(fastgltf::visitor{
						[&] (const fastgltf::sources::Array & array) -> std::shared_ptr< ImageResource > {
							const auto * dataPtr = array.bytes.data() + view.byteOffset;
							const auto dataSize = view.byteLength;

							auto bytes = std::make_shared< std::vector< std::byte > >(dataPtr, dataPtr + dataSize);

							const auto format = mimeToPixmapFormat(bufferView.mimeType);

							return m_resources.container< ImageResource >()
								->getOrCreateResource(name, [bytes, format] (auto & imageResource) {
									Pixmap< uint8_t > pixmap;

									constexpr ReadOptions options{
										.targetChannelMode = TargetChannelMode::RGBA
									};

									if ( !StreamIO::read(*bytes, format, pixmap, options) )
									{
										return false;
									}

									return imageResource.load(std::move(pixmap));
								});
						},

						[&] (const auto &) -> std::shared_ptr< ImageResource > {
							return m_resources.container< ImageResource >()->getDefaultResource();
						}
					}, buffer.data);
				},

				/* Inline base64 data (Array source). */
				[&] (const fastgltf::sources::Array & array) -> std::shared_ptr< ImageResource > {
					auto bytes = std::make_shared< std::vector< std::byte > >(array.bytes.begin(), array.bytes.end());
					const auto format = mimeToPixmapFormat(array.mimeType);

					return m_resources.container< ImageResource >()
						->getOrCreateResource(name, [bytes, format] (auto & imageResource) {
							Pixmap< uint8_t > pixmap;

							constexpr ReadOptions options{
								.targetChannelMode = TargetChannelMode::RGBA
							};

							if ( !StreamIO::read(*bytes, format, pixmap, options) )
							{
								return false;
							}

							return imageResource.load(std::move(pixmap));
						});
				},

				/* Fallback for unhandled source types. */
				[&] (const auto &) -> std::shared_ptr< ImageResource > {
					return m_resources.container< ImageResource >()->getDefaultResource();
				}
			}, glTFImage.data);

			if ( image == nullptr )
			{
				TraceWarning{ClassId} << "Image " << imageIndex << " ('" << name << "') failed to load, using default.";

				m_images[imageIndex] = m_resources.container< ImageResource >()->getDefaultResource();
				allSuccess = false;
			}
			else
			{
				m_images[imageIndex] = std::move(image);
			}
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

			std::string name;
			name.reserve(m_resourcePrefix.size() + 10 + glTFMaterial.name.size());
			name = m_resourcePrefix;
			name += "Material/";
			if ( glTFMaterial.name.empty() )
				name += std::to_string(materialIndex);
			else
				name.append(glTFMaterial.name.data(), glTFMaterial.name.size());

			/* Resolve a glTF texture index to a Texture2D resource, creating it on demand. */
			const auto resolveTexture = [&] (size_t textureIndex, bool sRGB = false) -> std::shared_ptr< TextureResource::Texture2D > {
				if ( textureIndex >= asset.textures.size() )
				{
					return nullptr;
				}

				/* Return cached texture if already created. */
				if ( m_textures[textureIndex] != nullptr )
				{
					return m_textures[textureIndex];
				}

				const auto & glTFTexture = asset.textures[textureIndex];

				if ( !glTFTexture.imageIndex.has_value() )
				{
					return nullptr;
				}

				const auto imageIndex = glTFTexture.imageIndex.value();

				if ( imageIndex >= m_images.size() || m_images[imageIndex] == nullptr )
				{
					return nullptr;
				}

				/* Build texture resource name. */
				std::string texName;
				texName.reserve(m_resourcePrefix.size() + 9 + glTFTexture.name.size());
				texName = m_resourcePrefix;
				texName += "Texture/";
				if ( glTFTexture.name.empty() )
				{
					texName += std::to_string(textureIndex);
				}
				else
				{
					texName.append(glTFTexture.name.data(), glTFTexture.name.size());
				}

				auto texture = m_resources.container< TextureResource::Texture2D >()
					->getOrCreateResource(texName, [image = m_images[imageIndex], sRGB] (auto & textureResource) {
						/* Set sRGB BEFORE load() so the flag is in place when
						 * onDependenciesLoaded() fires and creates the VkImage. */
						textureResource.enableSRGB(sRGB);

						return textureResource.load(image);
					});

				m_textures[textureIndex] = texture;

				return texture;
			};

			const auto & PBRData = glTFMaterial.pbrData;

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

			const auto emissiveStrength = static_cast< float >(glTFMaterial.emissiveStrength);

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

			/* Alpha mode (OPAQUE, MASK, BLEND). */
			const bool isAlphaBlend = glTFMaterial.alphaMode == fastgltf::AlphaMode::Blend;

			/* Async material creation — lambda is fully self-contained, no this/reference captures. */
			auto material = m_resources.container< Material::PBRResource >()
				->getOrCreateResource(name, [
					albedoTex = std::move(albedoTex), albedoColor,
					metallicRoughnessTex = std::move(metallicRoughnessTex), roughnessFactor, metallicFactor,
					normalTex = std::move(normalTex), normalScale,
					aoTex = std::move(aoTex), aoStrength,
					emissiveTex = std::move(emissiveTex), emissiveStrength, emissiveColor, hasEmissiveColor,
					clearcoatFactor, clearcoatRoughness,
					sheenColor, sheenRoughness,
					transmissionFactor,
					iridescenceFactor,
					isAlphaBlend
				] (auto & materialResource) {
					/* Albedo. */
					if ( albedoTex != nullptr )
					{
						materialResource.setAlbedoComponent(albedoTex);
					}
					else
					{
						materialResource.setAlbedoComponent(albedoColor);
					}

					/* Roughness / Metalness. */
					if ( metallicRoughnessTex != nullptr )
					{
						materialResource.setRoughnessComponent(metallicRoughnessTex, roughnessFactor);
						materialResource.setMetalnessComponent(metallicRoughnessTex, metallicFactor);
					}
					else
					{
						materialResource.setRoughnessComponent(roughnessFactor);
						materialResource.setMetalnessComponent(metallicFactor);
					}

					/* Normal map. */
					if ( normalTex != nullptr )
					{
						materialResource.setNormalComponent(normalTex, normalScale);
					}

					/* Ambient occlusion. */
					if ( aoTex != nullptr )
					{
						materialResource.setAmbientOcclusionComponent(aoTex, aoStrength);
					}

					/* Emissive. */
					if ( emissiveTex != nullptr )
					{
						materialResource.setAutoIlluminationComponent(emissiveTex, emissiveStrength);
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

					/* Transmission (KHR_materials_transmission). */
					if ( transmissionFactor > 0.0F )
					{
						materialResource.setTransmissionComponent(transmissionFactor);
					}

					/* Iridescence (KHR_materials_iridescence). */
					if ( iridescenceFactor > 0.0F )
					{
						materialResource.setIridescenceComponent(iridescenceFactor);
					}

					/* Alpha blending (glTF alphaMode: BLEND). */
					if ( isAlphaBlend )
					{
						materialResource.enableBlending(BlendingMode::Normal);
					}

					return materialResource.setManualLoadSuccess(true);
				});

			if ( material == nullptr )
			{
				TraceWarning{ClassId} << "Material " << materialIndex << " ('" << name << "') failed to create, using default.";

				m_materials[materialIndex] = m_resources.container< Material::PBRResource >()->getDefaultResource();

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
	GLTFLoader::loadMeshes (const fastgltf::Asset & asset, AssetData & output) noexcept
	{
		m_meshes.resize(asset.meshes.size());
		m_shapes.resize(asset.meshes.size());

		bool allSuccess = true;

		for ( size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex )
		{
			const auto & glTFMesh = asset.meshes[meshIndex];

			const auto suffixSize = glTFMesh.name.empty() ? size_t{4} : glTFMesh.name.size();

			std::string geoName;
			geoName.reserve(m_resourcePrefix.size() + 10 + suffixSize);
			geoName = m_resourcePrefix;
			geoName += "Geometry/";

			std::string meshName;
			meshName.reserve(m_resourcePrefix.size() + 6 + suffixSize);
			meshName = m_resourcePrefix;
			meshName += "Mesh/";

			if ( glTFMesh.name.empty() )
			{
				const auto indexString = std::to_string(meshIndex);

				geoName += indexString;
				meshName += indexString;
			}
			else
			{
				geoName.append(glTFMesh.name.data(), glTFMesh.name.size());
				meshName.append(glTFMesh.name.data(), glTFMesh.name.size());
			}

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

				const auto positionIt = primitive.findAttribute("POSITION");

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

				m_meshes[meshIndex] = m_resources.container< Renderable::SimpleMeshResource >()->getDefaultResource();

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

			/* Second pass: fill vertices and triangles directly into pre-allocated vectors. */
			const bool buildSuccess = shape->build([&] (auto & groups, auto & vertices, auto & triangles) {
				vertices.resize(totalVertexCount);
				triangles.reserve(totalTriangleCount);

				uint32_t globalVertexOffset = 0;
				bool firstPrimitive = true;

				for ( size_t primitiveIndex = 0; primitiveIndex < glTFMesh.primitives.size(); ++primitiveIndex )
				{
					const auto & glTFPrimitive = glTFMesh.primitives[primitiveIndex];

					if ( glTFPrimitive.type != fastgltf::PrimitiveType::Triangles )
					{
						continue;
					}

					const auto positionIt = glTFPrimitive.findAttribute("POSITION");

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
							vertices[globalVertexOffset + index].setPosition(Vector< 3, float >{v.x(), v.y(), v.z()});
							index++;
						});
					}

					/* Read normals directly into vertex array. */
					const auto normalIt = glTFPrimitive.findAttribute("NORMAL");

					if ( normalIt != glTFPrimitive.attributes.end() )
					{
						const auto & normAccessor = asset.accessors[normalIt->accessorIndex];
						size_t index = 0;

						fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, normAccessor, [&] (const fastgltf::math::fvec3 & v) {
							vertices[globalVertexOffset + index].setNormal(Vector< 3, float >{v.x(), v.y(), v.z()});
							index++;
						});
					}

					/* Read texture coordinates directly into vertex array. */
					const auto textureCoordinatesIt = glTFPrimitive.findAttribute("TEXCOORD_0");

					if ( textureCoordinatesIt != glTFPrimitive.attributes.end() )
					{
						const auto & uvAccessor = asset.accessors[textureCoordinatesIt->accessorIndex];
						size_t index = 0;

						fastgltf::iterateAccessor< fastgltf::math::fvec2 >(asset, uvAccessor, [&] (const fastgltf::math::fvec2 & v) {
							vertices[globalVertexOffset + index].setTextureCoordinates(Vector< 3, float >{v.x(), v.y(), 0.0F});
							index++;
						});
					}

					if ( !m_options.skipSkinning )
					{
						/* Read bone joint indices (JOINTS_0) for skeletal animation. */
						const auto jointsIt = glTFPrimitive.findAttribute("JOINTS_0");

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
							});
						}

						/* Read bone weights (WEIGHTS_0) for skeletal animation. */
						const auto weightsIt = glTFPrimitive.findAttribute("WEIGHTS_0");

						if ( weightsIt != glTFPrimitive.attributes.end() )
						{
							const auto & weightsAccessor = asset.accessors[weightsIt->accessorIndex];
							size_t index = 0;

							fastgltf::iterateAccessor< fastgltf::math::fvec4 >(asset, weightsAccessor, [&] (const fastgltf::math::fvec4 & v) {
								vertices[globalVertexOffset + index].setWeights(v.x(), v.y(), v.z(), v.w());
								index++;
							});
						}
					}

					/* Read indices and build triangles directly. */
					if ( glTFPrimitive.indicesAccessor.has_value() )
					{
						const auto & indexAccessor = asset.accessors[glTFPrimitive.indicesAccessor.value()];
						const auto primTriangleCount = static_cast< uint32_t >(indexAccessor.count / 3);

						/* Stream indices directly into triangles with winding swap.
						 * glTF is Y-up CCW; the 180° X rotation applied to
						 * the scene root (Y-up → Y-down) inverts the winding,
						 * so we swap indices 1 and 2 to keep front-faces correct. */
						uint32_t triangleBuffer[3];
						uint32_t triangleSlot = 0;

						fastgltf::iterateAccessor< uint32_t >(asset, indexAccessor, [&] (uint32_t value) {
							triangleBuffer[triangleSlot++] = globalVertexOffset + value;

							if ( triangleSlot == 3 )
							{
								triangles.emplace_back(triangleBuffer[0], triangleBuffer[2], triangleBuffer[1]);

								triangleSlot = 0;
							}
						});

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

				m_meshes[meshIndex] = m_resources.container< Renderable::SimpleMeshResource >()->getDefaultResource();

				allSuccess = false;

				continue;
			}

			/* Generate normals from geometry when the glTF mesh does not provide them. */
			if ( !hasNormals )
			{
				shape->computeTriangleNormal(true);
				shape->computeVertexNormal();

				TraceInfo{ClassId} << "Generated smooth normals for mesh '" << glTFMesh.name << "' (not provided by glTF).";
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

			/* Phase 2: Tangent computation + GPU upload on thread pool. */
			auto geometry = m_resources.container< IndexedVertexResource >()
				->getOrCreateResource(geoName, [shape] (auto & geometryResource) {
					shape->computeTriangleTangent();
					shape->computeVertexTangent();

					return geometryResource.load(*shape);
				}, geometryFlags);

			if ( geometry == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create geometry for mesh " << meshIndex << ", using default.";

				m_meshes[meshIndex] = m_resources.container< Renderable::SimpleMeshResource >()->getDefaultResource();

				allSuccess = false;

				continue;
			}

			/* Collect materials for each primitive. */
			std::vector< std::shared_ptr< Material::Interface > > materialList;
			materialList.reserve(glTFMesh.primitives.size());

			for ( const auto & primitive : glTFMesh.primitives )
			{
				if ( primitive.materialIndex.has_value() )
				{
					const auto materialIndex = primitive.materialIndex.value();

					if ( materialIndex < m_materials.size() && m_materials[materialIndex] != nullptr )
					{
						materialList.push_back(m_materials[materialIndex]);
					}
					else
					{
						materialList.push_back(m_resources.container< Material::PBRResource >()->getDefaultResource());
					}
				}
				else
				{
					materialList.push_back(m_resources.container< Material::PBRResource >()->getDefaultResource());
				}
			}

			/* Create renderable mesh resource. */
			std::shared_ptr< Renderable::Abstract > mesh;

			if ( materialList.size() <= 1 )
			{
				auto singleMaterial = materialList.empty()
					? std::static_pointer_cast< Material::Interface >(m_resources.container< Material::PBRResource >()->getDefaultResource())
					: materialList[0];

				mesh = m_resources.container< Renderable::SimpleMeshResource >()
					->getOrCreateResource(meshName, [geometry, singleMaterial = std::move(singleMaterial)] (auto & meshResource) {
						return meshResource.load(geometry, singleMaterial);
					});
			}
			else
			{
				mesh = m_resources.container< Renderable::MeshResource >()
					->getOrCreateResource(meshName, [geometry, materialList = std::move(materialList)] (auto & meshResource) {
						return meshResource.load(geometry, materialList);
					});
			}

			m_meshes[meshIndex] = mesh;
			m_shapes[meshIndex] = shape;

			/* Store in AssetData for consumer access. */
			MeshDescriptor descriptor;
			descriptor.renderable = mesh;
			descriptor.geometry = std::static_pointer_cast< Geometry::Interface >(geometry);
			descriptor.materials = materialList.empty()
				? std::vector< std::shared_ptr< Material::Interface > >{m_resources.container< Material::PBRResource >()->getDefaultResource()}
				: std::move(materialList);

			/* Ensure output.meshes is indexed by glTF mesh index. */
			if ( output.meshes.size() <= meshIndex )
			{
				output.meshes.resize(meshIndex + 1);
			}

			output.meshes[meshIndex] = std::move(descriptor);
		}

		return allSuccess;
	}

	void
	GLTFLoader::loadSkins (const fastgltf::Asset & asset, AssetData & /*output*/) noexcept
	{
		if ( asset.skins.empty() )
		{
			return;
		}

		/* Build a node-index → parent-node-index lookup from the node tree. */
		constexpr auto NoParent = static_cast< int32_t >(-1);

		std::vector< int32_t > nodeParents(asset.nodes.size(), NoParent);

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
							data[col * 4 + row] = m.col(col)[row];
						}
					}

					inverseBindMatrices[index] = Matrix< 4, float >{data};
					index++;
				});
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
					engineJoints[j].translation = {trs->translation.x(), trs->translation.y(), trs->translation.z()};
					engineJoints[j].rotation = Quaternion< float >{trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()};
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
			const auto skinName = glTFSkin.name.empty()
				? "skin_" + std::to_string(skinIndex)
				: std::string{glTFSkin.name};

			auto skeletonResource = m_resources.container< Animations::SkeletonResource >()
				->getOrCreateResourceSync(
					m_resourcePrefix + "/skeleton/" + skinName,
					[&skeleton] (auto & resource) {
						return resource.load(std::move(skeleton));
					}
				);

			m_skeletons.push_back(std::move(skeletonResource));
			m_skins.push_back(std::move(skin));

			/* Record which meshes reference this skin for later association with renderables. */
			for ( size_t nodeIdx = 0; nodeIdx < asset.nodes.size(); ++nodeIdx )
			{
				const auto & node = asset.nodes[nodeIdx];

				if ( node.skinIndex.has_value() && node.skinIndex.value() == skinIndex && node.meshIndex.has_value() )
				{
					m_meshToSkinIndex[node.meshIndex.value()] = skinIndex;
				}
			}
		}
	}

	void
	GLTFLoader::loadAnimations (const fastgltf::Asset & asset, AssetData & /*output*/) noexcept
	{
		if ( asset.animations.empty() )
		{
			return;
		}

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
				});

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

				switch ( glTFChannel.path )
				{
					case fastgltf::AnimationPath::Translation :
					{
						channel.target = ChannelTarget::Translation;
						size_t index = 0;

						channel.vectorKeyFrames.resize(timestamps.size());

						fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, outputAccessor, [&] (const fastgltf::math::fvec3 & v) {
							if ( index < timestamps.size() )
							{
								channel.vectorKeyFrames[index] = {timestamps[index], {v.x(), v.y(), v.z()}};
								index++;
							}
						});

						break;
					}

					case fastgltf::AnimationPath::Rotation :
					{
						channel.target = ChannelTarget::Rotation;
						size_t index = 0;

						channel.quaternionKeyFrames.resize(timestamps.size());

						fastgltf::iterateAccessor< fastgltf::math::fvec4 >(asset, outputAccessor, [&] (const fastgltf::math::fvec4 & v) {
							if ( index < timestamps.size() )
							{
								channel.quaternionKeyFrames[index] = {timestamps[index], Quaternion< float >{v.x(), v.y(), v.z(), v.w()}};
								index++;
							}
						});

						break;
					}

					case fastgltf::AnimationPath::Scale :
					{
						channel.target = ChannelTarget::Scale;
						size_t index = 0;

						channel.vectorKeyFrames.resize(timestamps.size());

						fastgltf::iterateAccessor< fastgltf::math::fvec3 >(asset, outputAccessor, [&] (const fastgltf::math::fvec3 & v) {
							if ( index < timestamps.size() )
							{
								channel.vectorKeyFrames[index] = {timestamps[index], {v.x(), v.y(), v.z()}};
								index++;
							}
						});

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
						m_resourcePrefix + "/animation/" + clipName,
						[&clip] (auto & resource) {
							return resource.load(std::move(clip));
						}
					);

				m_animationClips.push_back(std::move(clipResource));
			}
		}

		if ( !m_animationClips.empty() )
		{
			TraceInfo{ClassId} << "Loaded " << m_animationClips.size() << " animation clips.";
		}
	}

	void
	GLTFLoader::buildNodeDescriptors (const fastgltf::Asset & asset, AssetData & output) noexcept
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
}
