/*
 * src/Scenes/Loaders/FBXLoader.cpp
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

#include "FBXLoader.hpp"

/* STL inclusions. */
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include <ufbx/ufbx.h>

/* Local inclusions. */
#include "Animation/Skeleton.hpp"
#include "Animation/Skin.hpp"
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletonResource.hpp"
#include "BlobTrait.hpp"
#include "FBXParser.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/TexturePacker.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/Types.hpp"
#include "IO/IO.hpp"
#include "Logging/Severity.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "PixelFactory/Types.hpp"
#include "Resources/Container.hpp"
#include "Resources/Manager.hpp"
#include "SceneData.hpp"
#include "Scenes/Loaders/LoaderOptions.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "VertexFactory/Shape.hpp"

/* Forward declarations. */
namespace EmEn::Graphics::Geometry
{
	class Interface;
}

namespace EmEn::Scenes::Loaders
{
	using namespace Base::PixelFactory;
	using namespace Graphics;
	using namespace Graphics::Geometry;

	bool
	FBXLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		/* ufbx load options: bring the scene directly into engine conventions
		 * so no axis/unit post-processing is needed downstream (FBX::setLoadOptions()). */
		/* NOTE: ufbx opens the path itself: a FIFO blocked that open forever (2026-10-18). */
		if ( !Base::IO::isReadableRegularFile(filepath) )
		{
			TraceError{ClassId} << "The FBX '" << Base::IO::toU8String(filepath) << "' is not a readable file !";

			return false;
		}

		ufbx_load_opts opts{};
		FBX::setLoadOptions(opts, true, Base::IO::filesize(filepath));

		ufbx_error error{};
		ufbx_scene * scene = ufbx_load_file(Base::IO::toU8String(filepath).c_str(), &opts, &error);

		if ( scene == nullptr )
		{
			std::array< char, 512 > buffer{};
			ufbx_format_error(buffer.data(), sizeof(buffer), &error);

			TraceError{ClassId} << "Failed to parse '" << Base::IO::toU8String(filepath) << "' :\n" << buffer.data();

			return false;
		}

		/* RAII cleanup for the ufbx scene. */
		const std::unique_ptr< ufbx_scene, decltype(&ufbx_free_scene) > sceneGuard{scene, ufbx_free_scene};

		/* ---- Analysis: the whole conversion, without any resource (fuzz_fbx covers it). ---- */
		auto asset = FBX::analyse(*scene, "FBX:" + Base::IO::toU8String(filepath.stem()) + "/", filepath.parent_path(), m_options);

		/* ---- Images: one per FBX texture, by index (the default image when it cannot be made). ---- */
		std::vector< std::shared_ptr< ImageResource > > images;
		images.reserve(asset.images.size());

		bool allImages = true;

		for ( size_t textureIndex = 0; textureIndex < asset.images.size(); ++textureIndex )
		{
			const auto & description = asset.images[textureIndex];

			std::shared_ptr< ImageResource > image;

			switch ( description.kind )
			{
				case FBX::ImageKind::None :
					images.push_back(m_resources.container< ImageResource >()->getDefaultResource());

					continue;

				case FBX::ImageKind::Unresolved :
					images.push_back(m_resources.container< ImageResource >()->getDefaultResource());
					allImages = false;

					continue;

				case FBX::ImageKind::Embedded :
				{
					/* NOTE: ufbx gives no file offset for embedded content: a released CPU copy comes back by re-parsing the
					 * FBX for this texture (readEmbeddedTexture()), its size checked. */
					auto source = EncodedSource::embedded(filepath, static_cast< uint32_t >(textureIndex), description.contentSize, description.format, &FBXLoader::readEmbeddedTexture);

					image = m_resources.container< ImageResource >()
						->getOrCreateResource(description.resourceName, [bytes = description.bytes, format = description.format, encodedSource = std::move(source)] (auto & imageResource) {
							Pixmap< uint8_t > pixmap;

							constexpr ReadOptions options{
								.targetChannelMode = TargetChannelMode::RGBA
							};

							if ( !StreamIO::read(*bytes, format, pixmap, options) )
							{
								return false;
							}

							imageResource.setEncodedSource(encodedSource);

							return imageResource.load(std::move(pixmap));
						});
				}
					break;

				case FBX::ImageKind::File :
					image = m_resources.container< ImageResource >()
						->getOrCreateResource(description.resourceName, [fullPath = description.path] (auto & imageResource) {
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
					break;
			}

			if ( image == nullptr )
			{
				TraceWarning{ClassId} << "Image " << textureIndex << " ('" << description.resourceName << "') failed to load, using default.";

				image = m_resources.container< ImageResource >()->getDefaultResource();
				allImages = false;
			}

			images.push_back(std::move(image));
		}

		if ( !allImages )
		{
			Tracer::warning(ClassId, "Some images failed to load, continuing with defaults.");
		}

		/* ---- Textures: created on demand, TWO slots per FBX texture selected by the colour space. ----
		 * ⚠️ The sRGB flag is baked into the resource at creation and comes from the USAGE, not from
		 * the asset: the same image legitimately serves as an sRGB albedo for one material and as a
		 * linear data map for another. Caching on the texture index alone let whichever usage
		 * resolved FIRST impose its colour space on the other. */
		std::vector< std::array< std::shared_ptr< TextureResource::Texture2D >, 2 > > textures(asset.images.size());

		const auto resolveTexture = [&] (const std::optional< FBX::TextureSlot > & slot) -> std::shared_ptr< TextureResource::Texture2D > {
			if ( !slot.has_value() || slot->texture >= textures.size() )
			{
				return nullptr;
			}

			const auto texIdx = slot->texture;
			const auto sRGB = slot->sRGB;
			const auto colourSpaceSlot = sRGB ? 1U : 0U;

			if ( textures[texIdx][colourSpaceSlot] != nullptr )
			{
				return textures[texIdx][colourSpaceSlot];
			}

			/* NOTE: Created SYNCHRONOUSLY (the function only links the source): the colour space is set when this returns,
			 * and the material setter checks it — an asynchronous creation would set it later, on a worker (texture identity,
			 * docs/caution-points.md). */
			auto texture = m_resources.container< TextureResource::Texture2D >()
				->getOrCreateResourceSync(asset.images[texIdx].textureName + (sRGB ? "-srgb" : "-data"), [image = images[texIdx], sRGB] (auto & textureResource) {
					if ( !textureResource.enableSRGB(sRGB) )
					{
						return false;
					}

					return textureResource.load(image);
				});

			textures[texIdx][colourSpaceSlot] = texture;

			return texture;
		};

		/* ---- Materials: one per FBX material, by index (the default material when it cannot be made). ---- */
		std::vector< std::shared_ptr< Material::Interface > > materials;
		materials.reserve(asset.materials.size());

		bool allMaterials = true;

		/* NOTE: The texture packing (graphics § 39) works on the IMAGES, before any texture of a material is requested. */
		const TexturePacker packer{m_resources};

		for ( size_t materialIndex = 0; materialIndex < asset.materials.size(); ++materialIndex )
		{
			const auto & description = asset.materials[materialIndex];

			/* NOTE: FBX maps carry no UV transform: every map of a material reads the same UVs. */
			Base::StaticVector< PackSource, TexturePacker::MaxRoles > packSources;

			for ( const auto & [role, slot] : std::array< std::pair< ComponentType, const std::optional< FBX::TextureSlot > * >, 5 >{{
				{ComponentType::Albedo, &description.albedoTexture},
				{ComponentType::Opacity, &description.opacityTexture},
				{ComponentType::AmbientOcclusion, &description.ambientOcclusionTexture},
				{ComponentType::Roughness, &description.roughnessTexture},
				{ComponentType::Metalness, &description.metalnessTexture}
			}} )
			{
				/* NOTE: In capacity: five roles. */
				if ( slot->has_value() && (*slot)->texture < images.size() )
				{
					packSources.push_back({.image = images[(*slot)->texture], .samplingKey = {}, .role = role, .channel = Channel::Red});
				}
			}

			const auto packedRoles = packer.pack({packSources.data(), packSources.size()}, description.resourceName);

			/* NOTE: A packed role reads its packed texture through its channel; the others resolve their own texture. */
			const auto mapTexture = [&packedRoles, &resolveTexture] (ComponentType role, const std::optional< FBX::TextureSlot > & slot, Channel & channel) -> std::shared_ptr< TextureResource::Texture2D > {
				const auto * const packedRole = std::ranges::find_if(packedRoles, [role] (const PackedRole & entry) { return entry.role == role; });

				if ( packedRole != packedRoles.end() )
				{
					channel = packedRole->channel;

					return packedRole->texture;
				}

				channel = Channel::Red;

				return resolveTexture(slot);
			};

			/* NOTE: Resolved here, in this order: the members of a closure are initialized in an unspecified order. */
			auto albedoChannel = Channel::Red;
			auto roughnessChannel = Channel::Red;
			auto metalnessChannel = Channel::Red;
			auto aoChannel = Channel::Red;
			auto opacityChannel = Channel::Red;
			auto albedoTex = mapTexture(ComponentType::Albedo, description.albedoTexture, albedoChannel);
			auto roughnessTex = mapTexture(ComponentType::Roughness, description.roughnessTexture, roughnessChannel);
			auto metalnessTex = mapTexture(ComponentType::Metalness, description.metalnessTexture, metalnessChannel);
			auto normalTex = resolveTexture(description.normalTexture);
			auto aoTex = mapTexture(ComponentType::AmbientOcclusion, description.ambientOcclusionTexture, aoChannel);
			auto emissiveTex = resolveTexture(description.emissiveTexture);
			auto opacityTex = mapTexture(ComponentType::Opacity, description.opacityTexture, opacityChannel);

			auto configure = [
				albedoTexture = std::move(albedoTex), albedoColor = description.albedoColor,
				roughnessTexture = std::move(roughnessTex), roughnessFactor = description.roughnessFactor, roughnessChannel,
				metalnessTexture = std::move(metalnessTex), metalnessFactor = description.metalnessFactor, metalnessChannel,
				normalTexture = std::move(normalTex),
				ambientOcclusionTexture = std::move(aoTex), aoChannel,
				emissiveTexture = std::move(emissiveTex), emissiveColor = description.emissiveColor, emissiveStrength = description.emissiveStrength, hasEmissiveColor = description.hasEmissiveColor,
				opacityTexture = std::move(opacityTex), opacityValue = description.opacityValue, opacityChannel,
				specularIsDielectricF0 = description.specularIsDielectricF0, specularFactor = description.specularFactor, specularColor = description.specularColor
			] (auto & materialResource) {
				/* A base-colour texture and a base-colour factor MULTIPLY — that is what both
				 * glTF (baseColorFactor) and FBX (base_color) specify. Setting the component to
				 * the texture and dropping the factor tints nothing and silently loses the
				 * factor's alpha; the colour goes to the material's tint slot instead. */
				if ( albedoTexture != nullptr )
				{
					materialResource.setAlbedoComponent(albedoTexture);
					materialResource.setAlbedoColor(albedoColor);
				}
				else
				{
					materialResource.setAlbedoComponent(albedoColor);
				}

				/* FBX semantics: a connected texture REPLACES the scalar (unlike glTF where the
				 * factor MULTIPLIES the texel). The engine contract is multiplicative, so the
				 * translation is: texture present ⇒ neutral factor (the setter default, 1.0);
				 * passing the authored scalar here would wrongly scale the map (a metalness
				 * scalar of 0 — the FBX default — would ZERO the metalness map out). */
				if ( roughnessTexture != nullptr )
				{
					/* NOTE: 1.0, the neutral factor (the setter's default); the channel is the packed one. */
					materialResource.setRoughnessComponent(roughnessTexture, 1.0F, false, roughnessChannel);
				}
				else
				{
					materialResource.setRoughnessComponent(roughnessFactor);
				}

				if ( metalnessTexture != nullptr )
				{
					materialResource.setMetalnessComponent(metalnessTexture, 1.0F, metalnessChannel);
				}
				else
				{
					materialResource.setMetalnessComponent(metalnessFactor);
				}

				if ( normalTexture != nullptr )
				{
					materialResource.setNormalComponent(normalTexture);
				}

				if ( ambientOcclusionTexture != nullptr )
				{
					materialResource.setAmbientOcclusionComponent(ambientOcclusionTexture, 1.0F, aoChannel);
				}

				if ( emissiveTexture != nullptr )
				{
					materialResource.setAutoIlluminationComponent(emissiveTexture, emissiveStrength);
				}
				else if ( hasEmissiveColor )
				{
					materialResource.setAutoIlluminationComponent(emissiveColor, emissiveStrength);
				}

				/* Opacity: the component carries the alpha (map or global value); the explicit
				 * enableBlending() keeps the legacy StandardResource-mode behaviour identical
				 * (its component setter does not raise the blending flag itself). */
				if ( opacityTexture != nullptr )
				{
					materialResource.setOpacityComponent(opacityTexture, opacityValue, opacityChannel);
					materialResource.enableBlending(BlendingMode::Normal);
				}
				else if ( opacityValue < 0.999F )
				{
					materialResource.setOpacityComponent(opacityValue);
					materialResource.enableBlending(BlendingMode::Normal);
				}

				/* ⚠️ Applied ONLY when the shading model gives these fields the dielectric-F0
				 * meaning. A legacy Phong material keeps whatever the resource was constructed
				 * with — the engine's own defaults — rather than receiving a highlight colour in
				 * the slot that scales its F0. */
				if ( specularIsDielectricF0 )
				{
					materialResource.setSpecularFactor(specularFactor);
					materialResource.setSpecularColor(specularColor);
				}

				return materialResource.setManualLoadSuccess(true);
			};

			auto material = std::static_pointer_cast< Material::Interface >(
				m_resources.container< Material::StandardResource >()->getOrCreateResource(description.resourceName, configure)
			);

			if ( material == nullptr )
			{
				TraceWarning{ClassId} << "Material " << materialIndex << " ('" << description.resourceName << "') failed to create, using default.";

				material = m_resources.container< Material::StandardResource >()->getDefaultResource();
				allMaterials = false;
			}

			materials.push_back(std::move(material));
		}

		if ( !allMaterials )
		{
			Tracer::warning(ClassId, "Some materials failed to load, continuing with defaults.");
		}

		/* ---- Meshes: one renderable per FBX mesh, in scene order (NodeDescriptor::meshIndex). ---- */
		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			return m_resources.container< Material::StandardResource >()->getDefaultResource();
		};

		/* The renderables whose geometry carries a skin, with that skin's index, for the skeletal attachment. */
		std::vector< std::pair< std::shared_ptr< Renderable::Abstract >, size_t > > skinnedRenderables;

		output.meshes.reserve(asset.meshes.size());

		bool allMeshes = true;

		for ( const auto & description : asset.meshes )
		{
			const auto & meshName = description.meshName;

			const auto pushDefault = [&] () {
				MeshDescriptor descriptor;
				descriptor.renderable = m_resources.container< Renderable::MeshResource >()->getDefaultResource();
				descriptor.materials = {defaultMaterial()};
				output.meshes.push_back(std::move(descriptor));

				if ( m_options.onMeshLoaded )
				{
					m_options.onMeshLoaded(output.meshes.back());
				}

				allMeshes = false;
			};

			/* No valid triangle, or an invalid shape: the analysis reported it. */
			if ( description.shape == nullptr )
			{
				pushDefault();

				continue;
			}

			uint32_t geometryFlags = EnableTangentSpace | EnablePrimaryTextureCoordinates;

			/* Enable bone influence/weight vertex streams whenever ufbx sees a
			 * skin deformer on the mesh — relying on the per-vertex influence
			 * array alone proved too conservative (Intel Knight FBX exposes
			 * skin_deformers without populating every mesh's vertices[0] with
			 * valid influences, so the character ended up filtered out of the
			 * skinning path even though it has a rig). */
			if ( description.skin.has_value() )
			{
				geometryFlags |= EnableInfluence | EnableWeight;
			}

			auto geometry = m_resources.container< IndexedVertexResource >()
				->getOrCreateResource(description.geometryName, [shape = description.shape] (auto & geometryResource) {
					shape->computeTriangleTangent();
					shape->computeVertexTangent();

					return geometryResource.load(*shape);
				}, geometryFlags);

			if ( geometry == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create geometry for mesh '" << meshName << "', using default.";

				pushDefault();

				continue;
			}

			/* Collect materials: one per material part. Fall back to the default PBR resource
			 * when the part has no material or the referenced material failed to load. */
			std::vector< std::shared_ptr< Material::Interface > > materialList;
			std::vector< RasterizationOptions > rasterizationList;

			materialList.reserve(description.parts.size());
			rasterizationList.reserve(description.parts.size());

			for ( const auto & part : description.parts )
			{
				std::shared_ptr< Material::Interface > material;
				RasterizationOptions rasterization{};

				if ( part.material.has_value() && *part.material < materials.size() )
				{
					material = materials[*part.material];
				}

				if ( material == nullptr )
				{
					material = defaultMaterial();
				}

				/* Honour the FBX material's double-sided feature (ufbx), OR'd with the caller-forced
				 * double-sided (LoaderOptions::forceDoubleSided): disable back-face culling so both
				 * faces rasterize. The option covers formats/exporters (Mixamo FBX Phong) that carry
				 * no double-sided material property ufbx can surface, and applies to default-material
				 * parts too. */
				if ( part.doubleSided || m_options.forceDoubleSided )
				{
					rasterization.setCullingMode(CullingMode::None);
				}

				materialList.push_back(std::move(material));
				rasterizationList.push_back(rasterization);
			}

			/* Create the renderable. Single-material meshes use MeshResource
			 * (cheaper), multi-material use MultiLayerMeshResource. */
			std::shared_ptr< Renderable::Abstract > renderable;

			if ( materialList.size() <= 1 )
			{
				auto singleMaterial = materialList.empty() ? defaultMaterial() : materialList[0];
				const RasterizationOptions singleRasterization = rasterizationList.empty() ? RasterizationOptions{} : rasterizationList[0];

				renderable = m_resources.container< Renderable::MeshResource >()
					->getOrCreateResource(meshName, [geometry, material = std::move(singleMaterial), singleRasterization] (auto & meshResource) {
						return meshResource.load(geometry, material, singleRasterization);
					});
			}
			else
			{
				renderable = m_resources.container< Renderable::MultiLayerMeshResource >()
					->getOrCreateResource(meshName, [geometry, materialList, rasterizationList] (auto & meshResource) {
						return meshResource.load(geometry, materialList, rasterizationList);
					});
			}

			if ( renderable == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create renderable for mesh '" << meshName << "', using default.";

				renderable = m_resources.container< Renderable::MeshResource >()->getDefaultResource();
				allMeshes = false;
			}
			else if ( description.skin.has_value() )
			{
				/* NOTE: Only this mesh's own renderable: the default one is shared by every caller. */
				skinnedRenderables.emplace_back(renderable, *description.skin);
			}

			MeshDescriptor descriptor;
			descriptor.renderable = renderable;
			descriptor.geometry = std::static_pointer_cast< Geometry::Interface >(geometry);
			descriptor.materials = std::move(materialList);

			output.meshes.push_back(std::move(descriptor));

			if ( m_options.onMeshLoaded )
			{
				m_options.onMeshLoaded(output.meshes.back());
			}
		}

		if ( !allMeshes )
		{
			Tracer::error(ClassId, "Failed to load meshes !");

			return false;
		}

		/* ---- Skeletons and clips (left empty by the analysis when LoaderOptions::skipSkinning is set). ---- */
		std::vector< std::shared_ptr< Animations::SkeletonResource > > skeletons(asset.skins.size());

		for ( size_t skinIndex = 0; skinIndex < asset.skins.size(); ++skinIndex )
		{
			auto & skin = asset.skins[skinIndex];

			if ( !skin.has_value() )
			{
				continue;
			}

			skeletons[skinIndex] = m_resources.container< Animations::SkeletonResource >()
				->getOrCreateResourceSync(skin->resourceName, [&skin] (auto & resource) {
					return resource.load(std::move(skin->skeleton));
				});
		}

		std::vector< std::shared_ptr< Animations::AnimationClipResource > > clips;
		clips.reserve(asset.clips.size());

		for ( auto & clip : asset.clips )
		{
			auto clipResource = m_resources.container< Animations::AnimationClipResource >()
				->getOrCreateResourceSync(clip.resourceName, [&clip] (auto & resource) {
					return resource.load(std::move(clip.clip));
				});

			if ( clipResource != nullptr )
			{
				clips.push_back(std::move(clipResource));
			}
		}

		/* Attach skeletal data to every renderable whose mesh was skinned. The trait
		 * handles all runtime skinning — joint matrix upload, blending,
		 * clip evaluation — so all we need is a single setter. */
		for ( const auto & [renderable, skinIndex] : skinnedRenderables )
		{
			if ( skinIndex >= skeletons.size() || skeletons[skinIndex] == nullptr )
			{
				continue;
			}

			if ( auto * skeletalData = dynamic_cast< Renderable::SkeletalDataTrait * >(renderable.get()) )
			{
				skeletalData->setSkeletalData(skeletons[skinIndex], asset.skins[skinIndex]->skin, clips);
			}
		}

		/* ---- Scene graph. ---- */
		output.nodes = std::move(asset.nodes);
		output.rootNodeIndices = std::move(asset.rootNodeIndices);
		output.skeletons = std::move(skeletons);
		output.animationClips = std::move(clips);
		output.skinJointNodeIndices = std::move(asset.skinJointNodeIndices);

		return true;
	}

	bool
	FBXLoader::loadAnimationClipsOnly (const std::filesystem::path & filepath, const Animations::SkeletonResource & targetSkeleton, std::vector< std::shared_ptr< Animations::AnimationClipResource > > & output) noexcept
	{
		const auto & skeleton = targetSkeleton.skeleton();

		if ( skeleton.empty() )
		{
			TraceError{ClassId} << "Cannot load animation clips: target skeleton '" << targetSkeleton.name() << "' is empty.";

			return false;
		}

		/* Same conversion settings as load() so animation curves live in the same
		 * coord space as the bind pose baked into the target skeleton. */
		/* NOTE: ufbx opens the path itself: a FIFO blocked that open forever (2026-10-18). */
		if ( !Base::IO::isReadableRegularFile(filepath) )
		{
			TraceError{ClassId} << "The FBX '" << Base::IO::toU8String(filepath) << "' is not a readable file !";

			return false;
		}

		ufbx_load_opts opts{};
		FBX::setLoadOptions(opts, false, Base::IO::filesize(filepath));

		ufbx_error error{};
		ufbx_scene * scene = ufbx_load_file(Base::IO::toU8String(filepath).c_str(), &opts, &error);

		if ( scene == nullptr )
		{
			std::array< char, 512 > buffer{};
			ufbx_format_error(buffer.data(), sizeof(buffer), &error);

			TraceError{ClassId} << "Failed to parse animation file '" << Base::IO::toU8String(filepath) << "' :\n" << buffer.data();

			return false;
		}

		const std::unique_ptr< ufbx_scene, decltype(&ufbx_free_scene) > sceneGuard{scene, ufbx_free_scene};

		if ( scene->anim_stacks.count == 0 )
		{
			TraceWarning{ClassId} << "No animation stacks in '" << Base::IO::toU8String(filepath) << "'.";

			return false;
		}

		/* ---- Analysis: the joints matched by name, every stack sampled (fuzz_fbx covers it). ---- */
		auto clips = FBX::analyseClipsOnly(*scene, skeleton, Base::IO::toU8String(filepath.stem()), m_options);

		const size_t producedBefore = output.size();

		output.reserve(output.size() + clips.size());

		for ( auto & clip : clips )
		{
			auto clipResource = m_resources.container< Animations::AnimationClipResource >()
				->getOrCreateResourceSync(clip.resourceName, [&clip] (auto & resource) {
					return resource.load(std::move(clip.clip));
				});

			if ( clipResource != nullptr )
			{
				output.push_back(std::move(clipResource));
			}
		}

		if ( output.size() - producedBefore == 0 )
		{
			TraceWarning{ClassId} << "Animation file '" << Base::IO::toU8String(filepath) << "' produced no clips.";

			return false;
		}

		return true;
	}

	bool
	FBXLoader::readEmbeddedTexture (const std::filesystem::path & filepath, uint32_t index, std::vector< std::byte > & bytes) noexcept
	{
		/* NOTE: Only the texture list is needed: no geometry, no animation, no external file. The texture order is the
		 * FBX file's object order, unchanged by those options. */
		ufbx_load_opts opts{};
		opts.ignore_geometry = true;
		opts.ignore_animation = true;
		opts.load_external_files = false;

		/* NOTE: ufbx opens the path itself: a FIFO blocked that open forever (2026-10-18). */
		if ( !Base::IO::isReadableRegularFile(filepath) )
		{
			TraceError{ClassId} << "The FBX '" << Base::IO::toU8String(filepath) << "' is not a readable file !";

			return false;
		}

		FBX::setMemoryLimits(opts, Base::IO::filesize(filepath));

		ufbx_error error{};
		ufbx_scene * scene = ufbx_load_file(Base::IO::toU8String(filepath).c_str(), &opts, &error);

		if ( scene == nullptr )
		{
			TraceError{ClassId} << "Unable to re-read the FBX '" << Base::IO::toU8String(filepath) << "' for its embedded texture #" << index << " !";

			return false;
		}

		const std::unique_ptr< ufbx_scene, decltype(&ufbx_free_scene) > sceneGuard{scene, ufbx_free_scene};

		if ( index >= scene->textures.count )
		{
			return false;
		}

		const auto & texture = *scene->textures.data[index];

		if ( texture.content.size == 0 )
		{
			return false;
		}

		const auto * data = static_cast< const std::byte * >(texture.content.data);

		bytes.assign(data, data + texture.content.size);

		return true;
	}
}
