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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

/* Third-party inclusions. */
#include <ufbx/ufbx.h>

/* Local inclusions. */
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletonResource.hpp"
#include "SceneData.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Animation/AnimationChannel.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/Joint.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Skin.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Matrix.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "VertexFactory/Shape.hpp"
#include "Tracer.hpp"

namespace
{
	using namespace EmEn;
	using namespace EmEn::Base::Math;
	using namespace EmEn::Base::PixelFactory;

	/* Convert a ufbx 3x4 affine matrix (column-major) into the engine's
     * 4x4 column-major Matrix. The implicit last row is (0, 0, 0, 1). */
	Matrix< 4, float >
	convertUfbxMatrix (const ufbx_matrix & m) noexcept
	{
		Matrix< 4, float > result;

		result[0]  = static_cast< float >(m.m00);
		result[1]  = static_cast< float >(m.m10);
		result[2]  = static_cast< float >(m.m20);
		result[3]  = 0.0F;

		result[4]  = static_cast< float >(m.m01);
		result[5]  = static_cast< float >(m.m11);
		result[6]  = static_cast< float >(m.m21);
		result[7]  = 0.0F;

		result[8]  = static_cast< float >(m.m02);
		result[9]  = static_cast< float >(m.m12);
		result[10] = static_cast< float >(m.m22);
		result[11] = 0.0F;

		result[12] = static_cast< float >(m.m03);
		result[13] = static_cast< float >(m.m13);
		result[14] = static_cast< float >(m.m23);
		result[15] = 1.0F;

		return result;
	}

	/* Detect a pixmap format from a texture filename. ufbx doesn't provide a MIME
	 * type like glTF does, so we dispatch on the file extension. Supports the same
	 * formats as the engine's PixelFactory (Targa, PNG, JPEG). */
	Pixmap< uint8_t >::Format
	filenameToPixmapFormat (std::string_view filename) noexcept
	{
		if ( filename.size() < 4 )
		{
			return Pixmap< uint8_t >::Format::None;
		}

		auto endsWith = [filename] (std::string_view suffix) noexcept {
			if ( filename.size() < suffix.size() )
			{
				return false;
			}

			for ( size_t i = 0; i < suffix.size(); ++i )
			{
				const auto a = filename[filename.size() - suffix.size() + i];
				const auto b = suffix[i];
				const auto lowerA = (a >= 'A' && a <= 'Z') ? static_cast< char >(a + 32) : a;

				if ( lowerA != b )
				{
					return false;
				}
			}

			return true;
		};

		if ( endsWith(".png") )
		{
			return Pixmap< uint8_t >::Format::PNG;
		}

		if ( endsWith(".jpg") || endsWith(".jpeg") )
		{
			return Pixmap< uint8_t >::Format::Jpeg;
		}

		if ( endsWith(".tga") )
		{
			return Pixmap< uint8_t >::Format::Targa;
		}

		return Pixmap< uint8_t >::Format::None;
	}

	/* Helper: extract a CartesianFrame from a ufbx_node's local transform.
	 * ufbx already applied target_axes / target_unit_meters conversions when
	 * the scene was loaded, so the local_transform is expressed in engine
	 * conventions (right-handed Y-up, meters). */
	CartesianFrame< float >
	extractFrameFromNode (const ufbx_node & node) noexcept
	{
		const auto & trs = node.local_transform;

		/* Rotation: quaternion (x, y, z, w) → rotation matrix (column-major). */
		const auto qx = static_cast< float >(trs.rotation.x);
		const auto qy = static_cast< float >(trs.rotation.y);
		const auto qz = static_cast< float >(trs.rotation.z);
		const auto qw = static_cast< float >(trs.rotation.w);

		const auto xx = qx * qx;
		const auto yy = qy * qy;
		const auto zz = qz * qz;
		const auto xy = qx * qy;
		const auto xz = qx * qz;
		const auto yz = qy * qz;
		const auto wx = qw * qx;
		const auto wy = qw * qy;
		const auto wz = qw * qz;

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

		CartesianFrame< float > frame(rotMatrix, {
			static_cast< float >(trs.scale.x),
			static_cast< float >(trs.scale.y),
			static_cast< float >(trs.scale.z)
		});

		frame.setPosition({
			static_cast< float >(trs.translation.x),
			static_cast< float >(trs.translation.y),
			static_cast< float >(trs.translation.z)
		});

		return frame;
	}

	/* Helper: build a sanitized node name with fallback to the node's element id. */
	std::string
	buildNodeName (const std::string & prefix, const ufbx_node & node) noexcept
	{
		std::string name;
		name.reserve(prefix.size() + 6 + node.name.length);
		name = prefix;
		name += "Node/";

		if ( node.name.length == 0 )
		{
			name += std::to_string(node.element_id);
		}
		else
		{
			name.append(node.name.data, node.name.length);
		}

		return name;
	}
}

namespace EmEn::Scenes::Loaders
{
	using namespace Base::Animation;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;
	using namespace Graphics;
	using namespace Graphics::Geometry;

	bool
	FBXLoader::load (const std::filesystem::path & filepath, SceneData & output) noexcept
	{
		m_resourcePrefix = "FBX:" + filepath.stem().string() + "/";

		/* Reset the per-asset state: a loader instance is reusable across files, and every
		 * one of these collections is indexed against the CURRENT scene. Carrying them over
		 * mixes two assets' index spaces. */
		m_images.clear();
		m_textures.clear();
		m_materials.clear();
		m_meshes.clear();
		m_shapes.clear();
		m_skeletons.clear();
		m_skins.clear();
		m_meshToSkinIndex.clear();
		m_animationClips.clear();
		m_skinJointNodes.clear();
		m_skinJointNodeIndices.clear();

		/* ufbx load options: bring the scene directly into engine conventions
		 * so no axis/unit post-processing is needed downstream. */
		ufbx_load_opts opts{};
		opts.target_axes = ufbx_axes_right_handed_y_up;
		opts.target_unit_meters = 1.0F;
		/* MODIFY_GEOMETRY bakes the axis conversion into vertex positions and
		 * into `cluster->geometry_to_bone`. Static meshes and bind-pose skinning
		 * are correct. Animated skinning currently exhibits a residual
		 * coord-space mismatch between the bone local_transform (FBX-native,
		 * adjust_pre-compensated) and the evaluated animation curves — see
		 * AGENTS.md for the known limitation. */
		opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
		/* An FBX node can carry a "geometry transform" that affects its attached mesh
		 * but not its children. Left at the default PRESERVE, that transform lives only
		 * in `ufbx_node.geometry_transform` and a loader reading `local_transform` alone
		 * places the mesh WRONG, silently. 3ds Max and Maya emit them routinely.
		 * MODIFY_GEOMETRY bakes it into the vertices — the same strategy already used
		 * for the axis conversion above — and falls back to helper nodes only when the
		 * same mesh is instanced several times with different geometry transforms. */
		opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
		opts.load_external_files = true;
		opts.generate_missing_normals = true;

		ufbx_error error{};
		ufbx_scene * scene = ufbx_load_file(filepath.string().c_str(), &opts, &error);

		if ( scene == nullptr )
		{
			std::array< char, 512 > buffer{};
			ufbx_format_error(buffer.data(), sizeof(buffer), &error);

			TraceError{ClassId} << "Failed to parse '" << filepath << "' :\n" << buffer.data();

			return false;
		}

		/* RAII cleanup for the ufbx scene. */
		const std::unique_ptr< ufbx_scene, decltype(&ufbx_free_scene) > sceneGuard{scene, ufbx_free_scene};

		const auto parentPath = filepath.parent_path();

		/* Loading pipeline: Images → Materials → Meshes → Skins → Animations → Node descriptors.
		 * Step 1 scope: meshes + node descriptors only. Images, materials, skins
		 * and animations are stubbed — meshes get the default PBR material. */

		if ( !this->loadImages(*scene, parentPath) )
		{
			Tracer::warning(ClassId, "Some images failed to load, continuing with defaults.");
		}

		if ( !this->loadMaterials(*scene) )
		{
			Tracer::warning(ClassId, "Some materials failed to load, continuing with defaults.");
		}

		if ( !this->loadMeshes(*scene, output) )
		{
			Tracer::error(ClassId, "Failed to load meshes !");

			return false;
		}

		if ( !m_options.skipSkinning )
		{
			if ( scene->skin_deformers.count > 0 )
			{
				this->loadSkins(*scene, output);
			}

			if ( scene->anim_stacks.count > 0 )
			{
				this->loadAnimations(*scene, output);
			}

			/* Attach skeletal data to every renderable whose mesh was skinned
			 * (recorded during loadMeshes in m_meshToSkinIndex). The trait
			 * handles all runtime skinning — joint matrix upload, blending,
			 * clip evaluation — so all we need is a single setter. */
			for ( const auto & [meshElementId, skinIdx] : m_meshToSkinIndex )
			{
				const auto meshIt = m_meshes.find(meshElementId);

				if ( meshIt == m_meshes.end() || meshIt->second == nullptr )
				{
					continue;
				}

				if ( skinIdx >= m_skeletons.size() || m_skeletons[skinIdx] == nullptr )
				{
					continue;
				}

				if ( auto * skeletalData = dynamic_cast< Renderable::SkeletalDataTrait * >(meshIt->second.get()) )
				{
					skeletalData->setSkeletalData(m_skeletons[skinIdx], m_skins[skinIdx], m_animationClips);
				}
			}
		}

		this->buildNodeDescriptors(*scene, output);

		output.skeletons = m_skeletons;
		output.animationClips = m_animationClips;
		output.skinJointNodeIndices = m_skinJointNodeIndices;

		return true;
	}

	bool
	FBXLoader::loadImages (const ufbx_scene & scene, const std::filesystem::path & basePath) noexcept
	{
		m_images.resize(scene.textures.count);

		bool allSuccess = true;

		for ( size_t textureIndex = 0; textureIndex < scene.textures.count; ++textureIndex )
		{
			const ufbx_texture & tex = *scene.textures.data[textureIndex];

			/* Skip non-file textures (layered, procedural, shader...) — they'd need custom resolution. */
			if ( tex.type != UFBX_TEXTURE_FILE )
			{
				m_images[textureIndex] = m_resources.container< ImageResource >()->getDefaultResource();

				continue;
			}

			/* ⚠️ Keyed on the INDEX, not on the name — an FBX name is not an identity, and
			 * duplicate node/material names are the norm in that format. See buildResourceKey(). */
			const auto name = buildResourceKey(m_resourcePrefix, "Image/", {tex.name.data, tex.name.length}, textureIndex);

			std::shared_ptr< ImageResource > image;

			if ( tex.content.size > 0 )
			{
				/* Embedded content inside the FBX binary. ufbx gives us the raw
				 * file bytes (e.g. a PNG stream). Detect the format from the
				 * texture filename extension, fall back to PNG if unknown. */
				const std::string_view fname{tex.filename.data, tex.filename.length};
				auto format = filenameToPixmapFormat(fname);

				if ( format == Pixmap< uint8_t >::Format::None )
				{
					format = Pixmap< uint8_t >::Format::PNG;
				}

				const auto * dataPtr = static_cast< const std::byte * >(tex.content.data);
				auto bytes = std::make_shared< std::vector< std::byte > >(dataPtr, dataPtr + tex.content.size);

				image = m_resources.container< ImageResource >()
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
			}
			else
			{
				/* External file. ufbx exposes the path relative to the loaded FBX
				 * in `filename`; that is what the engine should resolve first.
				 * If the FBX referenced a far-away drive, ufbx may have filled
				 * `absolute_filename` instead — use it as a fallback. */
				std::filesystem::path fullPath;

				if ( tex.filename.length > 0 )
				{
					fullPath = basePath / std::filesystem::path{std::string{tex.filename.data, tex.filename.length}};
				}
				else if ( tex.absolute_filename.length > 0 )
				{
					fullPath = std::filesystem::path{std::string{tex.absolute_filename.data, tex.absolute_filename.length}};
				}
				else if ( tex.relative_filename.length > 0 )
				{
					fullPath = basePath / std::filesystem::path{std::string{tex.relative_filename.data, tex.relative_filename.length}};
				}

				if ( fullPath.empty() )
				{
					TraceWarning{ClassId} << "Texture '" << name << "' has no resolvable path, using default.";

					m_images[textureIndex] = m_resources.container< ImageResource >()->getDefaultResource();

					allSuccess = false;

					continue;
				}

				image = m_resources.container< ImageResource >()
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
			}

			if ( image == nullptr )
			{
				TraceWarning{ClassId} << "Image " << textureIndex << " ('" << name << "') failed to load, using default.";

				m_images[textureIndex] = m_resources.container< ImageResource >()->getDefaultResource();
				allSuccess = false;
			}
			else
			{
				m_images[textureIndex] = std::move(image);
			}
		}

		return allSuccess;
	}

	bool
	FBXLoader::loadMaterials (const ufbx_scene & scene) noexcept
	{
		m_materials.resize(scene.materials.count);
		m_textures.resize(scene.textures.count);

		bool allSuccess = true;

		for ( size_t materialIndex = 0; materialIndex < scene.materials.count; ++materialIndex )
		{
			const ufbx_material & fbxMaterial = *scene.materials.data[materialIndex];

			/* ⚠️ Keyed on the INDEX, not on the name. See buildResourceKey(). */
			const auto name = buildResourceKey(m_resourcePrefix, "Material/", {fbxMaterial.name.data, fbxMaterial.name.length}, materialIndex);

			/* Resolve a ufbx_texture to a Texture2D resource, creating it on demand.
			 * Cached in m_textures by the texture's typed_id so each engine texture
			 * is instantiated at most once, even when shared across many materials. */
			const auto resolveTexture = [&] (const ufbx_texture * tex, bool sRGB = false) -> std::shared_ptr< TextureResource::Texture2D > {
				if ( tex == nullptr )
				{
					return nullptr;
				}

				const auto texIdx = static_cast< size_t >(tex->typed_id);

				if ( texIdx >= m_textures.size() )
				{
					return nullptr;
				}

				/* ⚠️ TWO slots per FBX texture, selected by the colour space. The sRGB flag is
				 * baked into the resource at creation and comes from the USAGE, not from the
				 * asset: the same image legitimately serves as an sRGB albedo for one material
				 * and as a linear data map for another. Caching on the texture index alone let
				 * whichever usage resolved FIRST impose its colour space on the other. */
				const auto colourSpaceSlot = sRGB ? 1U : 0U;

				if ( m_textures[texIdx][colourSpaceSlot] != nullptr )
				{
					return m_textures[texIdx][colourSpaceSlot];
				}

				if ( texIdx >= m_images.size() || m_images[texIdx] == nullptr )
				{
					return nullptr;
				}

				/* The index makes the key unique per FBX texture; the colour space stays part of
				 * it because one texture legitimately yields two engine resources. Same
				 * convention as the glTF and USD loaders. */
				std::string texName = buildResourceKey(m_resourcePrefix, "Texture/", {tex->name.data, tex->name.length}, texIdx);
				texName += sRGB ? "-srgb" : "-data";

				auto texture = m_resources.container< TextureResource::Texture2D >()
					->getOrCreateResource(texName, [image = m_images[texIdx], sRGB] (auto & textureResource) {
						textureResource.enableSRGB(sRGB);

						return textureResource.load(image);
					});

				m_textures[texIdx][colourSpaceSlot] = texture;

				return texture;
			};

			const auto & pbr = fbxMaterial.pbr;

			/* Albedo / base color (sRGB). ufbx stores the linear factor in value_real
			 * and the tint in value_vec4; combine them into the engine's albedo. */
			auto albedoTex = resolveTexture(pbr.base_color.texture, true);

			Color< float > albedoColor{
				static_cast< float >(pbr.base_color.value_vec4.x),
				static_cast< float >(pbr.base_color.value_vec4.y),
				static_cast< float >(pbr.base_color.value_vec4.z),
				static_cast< float >(pbr.base_color.value_vec4.w > 0.0F ? pbr.base_color.value_vec4.w : 1.0F)
			};

			if ( !pbr.base_color.has_value )
			{
				albedoColor = Color< float >{1.0F, 1.0F, 1.0F, 1.0F};
			}

			/* Roughness and Metalness (linear, UNORM). */
			auto roughnessTex = resolveTexture(pbr.roughness.texture);

			const auto roughnessFactor = pbr.roughness.has_value
				? static_cast< float >(pbr.roughness.value_real) : 0.5F;

			auto metalnessTex = resolveTexture(pbr.metalness.texture);

			const auto metalnessFactor = pbr.metalness.has_value
				? static_cast< float >(pbr.metalness.value_real) : 0.0F;

			/* Normal map (linear). */
			auto normalTex = resolveTexture(pbr.normal_map.texture);

			/* Ambient occlusion (linear). */
			auto aoTex = resolveTexture(pbr.ambient_occlusion.texture);

			/* Emission (sRGB color + linear factor). */
			auto emissiveTex = resolveTexture(pbr.emission_color.texture, true);

			Color< float > emissiveColor{
				static_cast< float >(pbr.emission_color.value_vec3.x),
				static_cast< float >(pbr.emission_color.value_vec3.y),
				static_cast< float >(pbr.emission_color.value_vec3.z),
				1.0F
			};

			const auto emissiveStrength = pbr.emission_factor.has_value
				? static_cast< float >(pbr.emission_factor.value_real) : 0.0F;

			const bool hasEmissiveColor = pbr.emission_color.has_value
				&& (pbr.emission_color.value_vec3.x > 0.0F
					|| pbr.emission_color.value_vec3.y > 0.0F
					|| pbr.emission_color.value_vec3.z > 0.0F);

			/* Opacity (engine opacity contract). ufbx.pbr.opacity is 1.0 (fully opaque) by
			 * default. FBX has no cutout notion: a texture-driven opacity is a grayscale
			 * per-pixel alpha scale (rule 3), a scalar < 1 a uniform transparency (rule 1) —
			 * both blend. The map is DATA (linear, red channel). */
			auto opacityTex = resolveTexture(pbr.opacity.texture);

			const auto opacityValue = pbr.opacity.has_value
				? static_cast< float >(pbr.opacity.value_real) : 1.0F;

			auto configure = [
				albedoTex = std::move(albedoTex), albedoColor,
				roughnessTex = std::move(roughnessTex), roughnessFactor,
				metalnessTex = std::move(metalnessTex), metalnessFactor,
				normalTex = std::move(normalTex),
				aoTex = std::move(aoTex),
				emissiveTex = std::move(emissiveTex), emissiveColor, emissiveStrength, hasEmissiveColor,
				opacityTex = std::move(opacityTex), opacityValue
			] (auto & materialResource) {
				/* A base-colour texture and a base-colour factor MULTIPLY — that is what both
				 * glTF (baseColorFactor) and FBX (base_color) specify. Setting the component to
				 * the texture and dropping the factor tints nothing and silently loses the
				 * factor's alpha; the colour goes to the material's tint slot instead. */
				if ( albedoTex != nullptr )
				{
					materialResource.setAlbedoComponent(albedoTex);
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
				if ( roughnessTex != nullptr )
				{
					materialResource.setRoughnessComponent(roughnessTex);
				}
				else
				{
					materialResource.setRoughnessComponent(roughnessFactor);
				}

				if ( metalnessTex != nullptr )
				{
					materialResource.setMetalnessComponent(metalnessTex);
				}
				else
				{
					materialResource.setMetalnessComponent(metalnessFactor);
				}

				if ( normalTex != nullptr )
				{
					materialResource.setNormalComponent(normalTex);
				}

				if ( aoTex != nullptr )
				{
					materialResource.setAmbientOcclusionComponent(aoTex);
				}

				if ( emissiveTex != nullptr )
				{
					materialResource.setAutoIlluminationComponent(emissiveTex, emissiveStrength);
				}
				else if ( hasEmissiveColor )
				{
					materialResource.setAutoIlluminationComponent(emissiveColor, emissiveStrength);
				}

				/* Opacity: the component carries the alpha (map or global value); the explicit
				 * enableBlending() keeps the legacy StandardResource-mode behaviour identical
				 * (its component setter does not raise the blending flag itself). */
				if ( opacityTex != nullptr )
				{
					materialResource.setOpacityComponent(opacityTex, opacityValue);
					materialResource.enableBlending(BlendingMode::Normal);
				}
				else if ( opacityValue < 0.999F )
				{
					materialResource.setOpacityComponent(opacityValue);
					materialResource.enableBlending(BlendingMode::Normal);
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
	FBXLoader::loadMeshes (const ufbx_scene & scene, SceneData & output) noexcept
	{
		output.meshes.reserve(scene.meshes.count);

		bool allSuccess = true;

		/* ⚠️ The axis conversion is baked into the VERTEX DATA (positions and normals). A normal
		 * needs no special case: the inverse-transpose of a diagonal ±1 matrix is the matrix itself.
		 * The triangle winding is kept exactly as authored: the unconditional swap that used to be
		 * declared here only compensated the engine's ORIENTATION-REVERSING projection, and that
		 * mirror went away with the Y-up convention flip. */

		/* ⚠️ Geometry and mesh resources are cached BY NAME and their content depends on the flip,
		 * so the flip is part of the key: without it, loading the same asset twice with different
		 * flags would serve the first variant to the second caller, silently. */
		const auto flipKey = std::string{};

		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			return m_resources.container< Material::StandardResource >()->getDefaultResource();
		};

		for ( size_t meshIndex = 0; meshIndex < scene.meshes.count; ++meshIndex )
		{
			const ufbx_mesh & mesh = *scene.meshes.data[meshIndex];

			/* ⚠️⚠️ Keyed on the INDEX, not on the name. Duplicate mesh names are ordinary in FBX
			 * (every instance of the same authored object carries it), and a key built on the name
			 * alone hands the FIRST homonym's geometry AND material to every later one, silently.
			 * Measured on the glTF side of the same defect: ClearCoatTest's eighteen meshes named
			 * 'ClearCoatSampleMesh' all rendered material 0. See buildResourceKey().
			 * NOTE: mesh.element_id — unique but unrelated to the loop order — was the previous
			 * fallback for an unnamed mesh; the loop index is what identifies it here. */
			const std::string_view baseName{mesh.name.data, mesh.name.length};

			const auto geoName = buildResourceKey(m_resourcePrefix + flipKey, "Geometry/", baseName, meshIndex);
			const auto meshName = buildResourceKey(m_resourcePrefix + flipKey, "Mesh/", baseName, meshIndex);

			/* Count total triangles (fan triangulation: polygon with N vertices → N-2 triangles).
			 * Vertex count = triangleCount * 3 because we emit per-corner (attribute layers
			 * may differ per corner). A dedupe pass via ufbx_generate_indices can be added
			 * later if the overhead becomes measurable. */
			uint32_t totalTriangleCount = 0;

			for ( size_t faceIdx = 0; faceIdx < mesh.faces.count; ++faceIdx )
			{
				const auto & face = mesh.faces.data[faceIdx];

				if ( face.num_indices >= 3 )
				{
					totalTriangleCount += face.num_indices - 2;
				}
			}

			if ( totalTriangleCount == 0 )
			{
				TraceWarning{ClassId} << "Mesh '" << meshName << "' has no valid triangle faces, using default.";

				auto defaultMesh = m_resources.container< Renderable::MeshResource >()->getDefaultResource();
				m_meshes[static_cast< uint32_t >(mesh.element_id)] = defaultMesh;

				MeshDescriptor descriptor;
				descriptor.renderable = defaultMesh;
				descriptor.materials = {defaultMaterial()};
				output.meshes.push_back(std::move(descriptor));

				if ( m_options.onMeshLoaded )
				{
					m_options.onMeshLoaded(output.meshes.back());
				}

				allSuccess = false;

				continue;
			}

			const uint32_t totalVertexCount = totalTriangleCount * 3;

			/* Material parts: each part is a subset of faces sharing the same material slot.
			 * A single part (no split) is the common case; multi-material meshes produce
			 * multiple parts which map to sub-geometry groups in the engine's Shape. */
			const bool hasMaterialSplit = mesh.material_parts.count > 1;
			const bool hasNormals = mesh.vertex_normal.exists;
			const bool hasUV = mesh.vertex_uv.exists;

			/* Skinning: use the first skin_deformer on the mesh when available.
			 * cluster_index inside ufbx_skin_weight corresponds directly to the
			 * joint index in the future Skeleton (loadSkins builds joints in
			 * cluster order), so no remapping is required here. */
			const ufbx_skin_deformer * skin = (!m_options.skipSkinning && mesh.skin_deformers.count > 0)
				? mesh.skin_deformers.data[0] : nullptr;
			const bool hasSkin = skin != nullptr;

			auto shape = std::make_shared< Shape< float > >();

			/* Allocate a per-thread triangulation buffer sized for the largest face. */
			std::vector< uint32_t > triangulationBuffer(mesh.max_face_triangles * 3);

			const bool buildSuccess = shape->build([&] (auto & groups, auto & vertices, auto & triangles) {
				vertices.resize(totalVertexCount);
				triangles.reserve(totalTriangleCount);

				uint32_t globalVertexOffset = 0;

				const size_t partCount = (mesh.material_parts.count == 0) ? 1 : mesh.material_parts.count;

				for ( size_t partIdx = 0; partIdx < partCount; ++partIdx )
				{
					const auto groupTriangleStart = static_cast< uint32_t >(triangles.size());

					/* Each material part after the first starts a new sub-geometry group. */
					if ( hasMaterialSplit && partIdx > 0 )
					{
						groups.emplace_back(groupTriangleStart, 0);
					}

					/* Iterate the faces for this part (or all faces if no split). */
					const auto iterateFaces = [&] (auto && faceVisitor) {
						if ( mesh.material_parts.count == 0 )
						{
							for ( size_t faceIdx = 0; faceIdx < mesh.faces.count; ++faceIdx )
							{
								faceVisitor(mesh.faces.data[faceIdx]);
							}
						}
						else
						{
							const auto & part = mesh.material_parts.data[partIdx];

							for ( size_t i = 0; i < part.face_indices.count; ++i )
							{
								const auto fi = part.face_indices.data[i];
								faceVisitor(mesh.faces.data[fi]);
							}
						}
					};

					uint32_t groupTriangleCount = 0;

					iterateFaces([&] (const ufbx_face & face) {
						if ( face.num_indices < 3 )
						{
							return;
						}

						const uint32_t numTris = ufbx_triangulate_face(
							triangulationBuffer.data(),
							triangulationBuffer.size(),
							&mesh,
							face
						);

						for ( uint32_t triangleIndex = 0; triangleIndex < numTris; ++triangleIndex )
						{
							const uint32_t c0 = triangulationBuffer[(triangleIndex * 3) + 0];
							const uint32_t c1 = triangulationBuffer[(triangleIndex * 3) + 1];
							const uint32_t c2 = triangulationBuffer[(triangleIndex * 3) + 2];

							std::array< uint32_t, 3 > cornerIndices{c0, c1, c2};

							for ( auto k = 0U; k < 3U; ++k )
							{
								const uint32_t corner = cornerIndices[k];
								const auto pos = ufbx_get_vertex_vec3(&mesh.vertex_position, corner);

								vertices[globalVertexOffset + k].setPosition(Vector< 3, float >{static_cast< float >(pos.x) * m_options.uniformScale, static_cast< float >(pos.y) * m_options.uniformScale, static_cast< float >(pos.z) * m_options.uniformScale});

								if ( hasNormals )
								{
									const auto n = ufbx_get_vertex_vec3(&mesh.vertex_normal, corner);
									vertices[globalVertexOffset + k].setNormal(Vector< 3, float >{static_cast< float >(n.x), static_cast< float >(n.y), static_cast< float >(n.z)});
								}

								if ( hasUV )
								{
									/* FBX stores UVs with V=0 at the bottom (OpenGL convention).
									 * The engine and Vulkan use V=0 at the top, matching glTF. Flip V
									 * here so embedded textures sample the correct region. */
									const auto uv = ufbx_get_vertex_vec2(&mesh.vertex_uv, corner);

									vertices[globalVertexOffset + k].setTextureCoordinates(Vector< 3, float >{
										static_cast< float >(uv.x),
										1.0F - static_cast< float >(uv.y),
										0.0F
									});
								}

								if ( hasSkin )
								{
									/* Look up the unique-vertex index for this corner to index
									 * into the skin's per-vertex weight range. ufbx sorts weights
									 * by decreasing magnitude, so the first four entries are the
									 * four most influential bones — matching the engine's 4-bone
									 * vertex layout directly. */
									const uint32_t uniqueVertex = mesh.vertex_indices.data[corner];
									const ufbx_skin_vertex & sv = skin->vertices.data[uniqueVertex];

									std::array< int32_t, 4 > joints{-1, -1, -1, -1};
									std::array< float, 4 > weights{0.0F, 0.0F, 0.0F, 0.0F};

									const size_t take = (sv.num_weights < 4U) ? sv.num_weights : 4U;

									float weightSum = 0.0F;

									for ( size_t wi = 0; wi < take; ++wi )
									{
										const ufbx_skin_weight & sw = skin->weights.data[sv.weight_begin + wi];
										joints[wi] = static_cast< int32_t >(sw.cluster_index);
										weights[wi] = static_cast< float >(sw.weight);
										weightSum += weights[wi];
									}

									/* Normalize weights so that their sum equals 1.0 — a requirement
									 * for linear blend skinning. ufbx does not guarantee this. */
									if ( weightSum > 1e-6F )
									{
										const float invSum = 1.0F / weightSum;
										weights[0] *= invSum;
										weights[1] *= invSum;
										weights[2] *= invSum;
										weights[3] *= invSum;
									}

									vertices[globalVertexOffset + k].setInfluences(joints[0], joints[1], joints[2], joints[3]);
									vertices[globalVertexOffset + k].setWeights(weights[0], weights[1], weights[2], weights[3]);
								}
							}

							/* The winding is kept as authored — the swap of indices 1 and 2 that used to
							 * sit here compensated the engine's ORIENTATION-REVERSING projection, gone
							 * with the Y-up convention flip. */
							triangles.emplace_back(
								globalVertexOffset + 0,
								globalVertexOffset + 1,
								globalVertexOffset + 2
							);

							globalVertexOffset += 3;
							groupTriangleCount++;
						}
					});

					if ( hasMaterialSplit && !groups.empty() )
					{
						groups.back().second = groupTriangleCount;
					}
				}

				return true;
			}, hasUV);

			if ( !buildSuccess || !shape->isValid() )
			{
				TraceWarning{ClassId} << "Generated shape is invalid for mesh '" << meshName << "', using default.";

				auto defaultMesh = m_resources.container< Renderable::MeshResource >()->getDefaultResource();
				m_meshes[static_cast< uint32_t >(mesh.element_id)] = defaultMesh;

				MeshDescriptor descriptor;
				descriptor.renderable = defaultMesh;
				descriptor.materials = {defaultMaterial()};
				output.meshes.push_back(std::move(descriptor));

				if ( m_options.onMeshLoaded )
				{
					m_options.onMeshLoaded(output.meshes.back());
				}

				allSuccess = false;

				continue;
			}

			if ( hasNormals )
			{
				shape->declareNormalsAvailable();
			}
			else
			{
				/* No inversion: nothing reflects the vertex data any more (the winding swap that
				 * used to force it is gone with the orientation-reversing projection). */
				shape->computeTriangleNormal(false);
				shape->computeVertexNormal();

				TraceInfo{ClassId} << "Generated smooth normals for mesh '" << meshName << "' (not provided by FBX).";
			}

			uint32_t geometryFlags = EnableTangentSpace | EnablePrimaryTextureCoordinates;

			/* Enable bone influence/weight vertex streams whenever ufbx sees a
			 * skin deformer on the mesh — relying on the per-vertex influence
			 * array alone proved too conservative (Intel Knight FBX exposes
			 * skin_deformers without populating every mesh's vertices[0] with
			 * valid influences, so the character ended up filtered out of the
			 * skinning path even though it has a rig). */
			if ( hasSkin )
			{
				geometryFlags |= EnableInfluence | EnableWeight;

				/* Track this mesh → skin relationship for the later
				 * SkeletalDataTrait attachment pass. The actual SkeletonResource /
				 * Skin are built by loadSkins() using the same cluster ordering
				 * that was written into the vertex influences. */
				m_meshToSkinIndex[static_cast< uint32_t >(mesh.element_id)]
					= static_cast< size_t >(skin->typed_id);
			}

			auto geometry = m_resources.container< IndexedVertexResource >()
				->getOrCreateResource(geoName, [shape] (auto & geometryResource) {
					shape->computeTriangleTangent();
					shape->computeVertexTangent();

					return geometryResource.load(*shape);
				}, geometryFlags);

			if ( geometry == nullptr )
			{
				TraceWarning{ClassId} << "Failed to create geometry for mesh '" << meshName << "', using default.";

				auto defaultMesh = m_resources.container< Renderable::MeshResource >()->getDefaultResource();
				m_meshes[static_cast< uint32_t >(mesh.element_id)] = defaultMesh;

				MeshDescriptor descriptor;
				descriptor.renderable = defaultMesh;
				descriptor.materials = {defaultMaterial()};
				output.meshes.push_back(std::move(descriptor));

				if ( m_options.onMeshLoaded )
				{
					m_options.onMeshLoaded(output.meshes.back());
				}

				allSuccess = false;

				continue;
			}

			/* Collect materials: one per material_part. Parallel to mesh.materials[]
			 * — `material_parts[i]` uses `mesh.materials.data[i]` (same ordering).
			 * Fall back to the default PBR resource when the mesh has no material
			 * or the referenced material failed to load. */
			std::vector< std::shared_ptr< Material::Interface > > materialList;
			std::vector< RasterizationOptions > rasterizationList;

			const size_t partCount = (mesh.material_parts.count == 0) ? 1 : mesh.material_parts.count;
			materialList.reserve(partCount);
			rasterizationList.reserve(partCount);

			for ( size_t partIdx = 0; partIdx < partCount; ++partIdx )
			{
				std::shared_ptr< Material::Interface > material;
				RasterizationOptions rasterization{};

				if ( partIdx < mesh.materials.count )
				{
					const ufbx_material * fbxMat = mesh.materials.data[partIdx];

					if ( fbxMat != nullptr )
					{
						const auto matTypedId = static_cast< size_t >(fbxMat->typed_id);

						if ( matTypedId < m_materials.size() && m_materials[matTypedId] != nullptr )
						{
							material = m_materials[matTypedId];
						}

						/* Honour the FBX material's double-sided feature (ufbx): disable back-face
						 * culling so both faces rasterize (cloth, inner armour surfaces, …). */
						if ( fbxMat->features.double_sided.enabled )
						{
							rasterization.setCullingMode(CullingMode::None);
						}
					}
				}

				if ( material == nullptr )
				{
					material = defaultMaterial();
				}

				/* Caller-forced double-sided (LoaderOptions::forceDoubleSided): OR with
				 * the asset flag above. Covers formats/exporters (Mixamo FBX Phong) that
				 * carry no double-sided material property ufbx can surface. Applies to
				 * default-material parts too. */
				if ( m_options.forceDoubleSided )
				{
					rasterization.setCullingMode(CullingMode::None);
				}

				materialList.push_back(std::move(material));
				rasterizationList.push_back(rasterization);
			}

			/* Create the renderable. Single-material meshes use SimpleMeshResource
			 * (cheaper), multi-material use MeshResource. */
			std::shared_ptr< Renderable::Abstract > renderable;

			if ( materialList.size() <= 1 )
			{
				auto singleMaterial = materialList[0];
				const RasterizationOptions singleRasterization = rasterizationList.empty() ? RasterizationOptions{} : rasterizationList[0];

				renderable = m_resources.container< Renderable::MeshResource >()
					->getOrCreateResource(meshName, [geometry, singleMaterial = std::move(singleMaterial), singleRasterization] (auto & meshResource) {
						return meshResource.load(geometry, singleMaterial, singleRasterization);
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
				allSuccess = false;
			}

			m_meshes[static_cast< uint32_t >(mesh.element_id)] = renderable;
			m_shapes[static_cast< uint32_t >(mesh.element_id)] = shape;

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

		return allSuccess;
	}

	void
	FBXLoader::loadSkins (const ufbx_scene & scene, SceneData & /*output*/) noexcept
	{
		if ( scene.skin_deformers.count == 0 )
		{
			return;
		}

		m_skeletons.reserve(scene.skin_deformers.count);
		m_skins.reserve(scene.skin_deformers.count);

		/* ⚠️ The rig is mirrored exactly like the mesh it deforms — bind-pose translations,
		 * bind-pose ROTATIONS and inverse bind matrices — or the skin slides off the geometry. */

		/* Skin deformers are indexed by typed_id (which equals their position
		 * in scene.skin_deformers.data[]). loadMeshes wrote this typed_id into
		 * m_meshToSkinIndex, so m_skeletons / m_skins must be filled at the
		 * same positions — we resize to count and leave gaps as nullptr. */
		m_skeletons.resize(scene.skin_deformers.count);
		m_skins.resize(scene.skin_deformers.count);

		for ( size_t skinIndex = 0; skinIndex < scene.skin_deformers.count; ++skinIndex )
		{
			const ufbx_skin_deformer & skin = *scene.skin_deformers.data[skinIndex];
			const size_t jointCount = skin.clusters.count;

			if ( jointCount == 0 )
			{
				continue;
			}

			/* Build the bone-node → joint-index lookup used for parent resolution. */
			std::unordered_map< const ufbx_node *, int32_t > boneNodeToJointIndex;
			boneNodeToJointIndex.reserve(jointCount);

			for ( size_t ci = 0; ci < jointCount; ++ci )
			{
				const ufbx_skin_cluster * cluster = skin.clusters.data[ci];

				if ( cluster != nullptr && cluster->bone_node != nullptr )
				{
					boneNodeToJointIndex[cluster->bone_node] = static_cast< int32_t >(ci);
				}
			}

			std::vector< Joint< float > > joints(jointCount);
			std::vector< Matrix< 4, float > > inverseBindMatrices(jointCount);

			for ( size_t ci = 0; ci < jointCount; ++ci )
			{
				const ufbx_skin_cluster * cluster = skin.clusters.data[ci];

				if ( cluster == nullptr || cluster->bone_node == nullptr )
				{
					joints[ci].name = "missing_bone_" + std::to_string(ci);
					joints[ci].parentIndex = NoParent;
					inverseBindMatrices[ci] = Matrix< 4, float >{};

					continue;
				}

				const ufbx_node & bone = *cluster->bone_node;

				joints[ci].name = (bone.name.length == 0)
					? "bone_" + std::to_string(ci)
					: std::string{bone.name.data, bone.name.length};

				/* geometry_to_bone is ufbx's inverse bind matrix in Vulkan-friendly
				 * form (local vertex → bone space). Perfect match for the engine. */
				inverseBindMatrices[ci] = convertUfbxMatrix(cluster->geometry_to_bone);

				/* Scale the translation column of the inverse bind matrix to keep
				 * the binding math coherent with scaled vertex positions and
				 * scaled joint TRS translations. Linear part (rotation + uniform
				 * 1x1 scale) is unaffected by uniform scaling around origin. */
				inverseBindMatrices[ci][M4x4Col3Row0] *= m_options.uniformScale;
				inverseBindMatrices[ci][M4x4Col3Row1] *= m_options.uniformScale;
				inverseBindMatrices[ci][M4x4Col3Row2] *= m_options.uniformScale;

				/* ⚠️ Conjugate the WHOLE matrix (M·IBM·M), linear part included — unlike the uniform
				 * scale above, which only touches the translation column. Skinning stays coherent
				 * because the joint world matrices are conjugated identically and the two M cancel. */
				inverseBindMatrices[ci] = inverseBindMatrices[ci];
				joints[ci].inverseBindMatrix = inverseBindMatrices[ci];

				/* Walk up the bone's parent chain until we hit another bone
				 * belonging to this skin; that is this joint's parent joint. */
				joints[ci].parentIndex = NoParent;

				const ufbx_node * parent = bone.parent;

				while ( parent != nullptr && !parent->is_root )
				{
					const auto it = boneNodeToJointIndex.find(parent);

					if ( it != boneNodeToJointIndex.end() )
					{
						joints[ci].parentIndex = it->second;
						break;
					}

					parent = parent->parent;
				}

				/* Rest-pose TRS taken from the bone's static local_transform —
				 * this matches what scene-node descriptors use for non-skinned
				 * transforms and what the SkeletalAnimator expects as the bind
				 * baseline. Animations override this at runtime via the clips
				 * built in loadAnimations. */
				const auto & trs = bone.local_transform;

				joints[ci].translation = Vector< 3, float >{static_cast< float >(trs.translation.x) * m_options.uniformScale, static_cast< float >(trs.translation.y) * m_options.uniformScale, static_cast< float >(trs.translation.z) * m_options.uniformScale};

				/* ⚠️ The conjugation carries the det(M) angle inversion: without
				 * it the bind pose looks right and the animation plays backwards. */
				joints[ci].rotation = Quaternion< float >{
					static_cast< float >(trs.rotation.x),
					static_cast< float >(trs.rotation.y),
					static_cast< float >(trs.rotation.z),
					static_cast< float >(trs.rotation.w)
				};

				/* NOTE: A scale is invariant under the flip — M·diag(s)·M = diag(s). */
				joints[ci].scale = {
					static_cast< float >(trs.scale.x),
					static_cast< float >(trs.scale.y),
					static_cast< float >(trs.scale.z)
				};
			}

			Skeleton< float > skeleton{std::move(joints)};

			/* Skin joint indices are simply 0..jointCount-1 because our
			 * skeleton is built in the same order as the clusters. */
			std::vector< int32_t > skinJointIndices(jointCount);

			for ( size_t j = 0; j < jointCount; ++j )
			{
				skinJointIndices[j] = static_cast< int32_t >(j);
			}

			Skin< float > engineSkin{std::move(skinJointIndices), inverseBindMatrices};

			const std::string skeletonName = m_resourcePrefix + std::string{} + "Skeleton/"
				+ (skin.name.length == 0 ? std::to_string(skinIndex) : std::string{skin.name.data, skin.name.length});

			auto skeletonResource = m_resources.container< Animations::SkeletonResource >()
				->getOrCreateResourceSync(skeletonName, [&skeleton] (auto & resource) {
					return resource.load(std::move(skeleton));
				});

			m_skeletons[skinIndex] = std::move(skeletonResource);
			m_skins[skinIndex] = std::move(engineSkin);

			/* Collect bone nodes so the scene consumer can skip instantiating
			 * them as regular scene nodes — bone transforms are owned by the
			 * SkeletalAnimator, not by the scene graph.
			 * The NODE POINTER is what gets stored: the descriptor index it maps to is
			 * only known once buildNodeDescriptors() has compacted the node table. */
			for ( size_t ci = 0; ci < jointCount; ++ci )
			{
				const ufbx_skin_cluster * cluster = skin.clusters.data[ci];

				if ( cluster != nullptr && cluster->bone_node != nullptr )
				{
					m_skinJointNodes.insert(cluster->bone_node);
				}
			}
		}
	}

	void
	FBXLoader::loadAnimations (const ufbx_scene & scene, SceneData & /*output*/) noexcept
	{
		if ( scene.anim_stacks.count == 0 || scene.skin_deformers.count == 0 )
		{
			return;
		}

		/* All clips are sampled against the bones of the FIRST skin deformer.
		 * Mixamo and most production pipelines keep the bone ordering identical
		 * across every skin referencing the same rig, so one clip set attaches
		 * cleanly to every mesh. If a future asset breaks this assumption, we
		 * will need per-skin clip lists. */
		const ufbx_skin_deformer & refSkin = *scene.skin_deformers.data[0];
		const size_t jointCount = refSkin.clusters.count;

		std::vector< const ufbx_node * > jointToNode(jointCount, nullptr);

		for ( size_t ci = 0; ci < jointCount; ++ci )
		{
			const ufbx_skin_cluster * cluster = refSkin.clusters.data[ci];

			if ( cluster != nullptr )
			{
				jointToNode[ci] = cluster->bone_node;
			}
		}

		m_animationClips.reserve(scene.anim_stacks.count);

		for ( size_t si = 0; si < scene.anim_stacks.count; ++si )
		{
			const ufbx_anim_stack & stack = *scene.anim_stacks.data[si];

			auto channels = sampleAnimStack(stack, jointToNode, m_options.uniformScale);

			if ( channels.empty() )
			{
				continue;
			}

			const std::string clipName =
				stack.name.length == 0 ?
				"clip_" + std::to_string(si) :
				std::string{stack.name.data, stack.name.length};

			AnimationClip< float > clip{clipName, std::move(channels)};

			auto clipResource = m_resources.container< Animations::AnimationClipResource >()
				->getOrCreateResourceSync(
					m_resourcePrefix + "Animation/" + clipName,
					[&clip] (auto & resource) {
						return resource.load(std::move(clip));
					}
				);

			if ( clipResource != nullptr )
			{
				m_animationClips.push_back(std::move(clipResource));
			}
		}

		if ( !m_animationClips.empty() )
		{
			TraceDebug{ClassId} << "Loaded " << m_animationClips.size() << " animation clips.";
		}
	}

	std::vector< AnimationChannel< float > >
	FBXLoader::sampleAnimStack (const ufbx_anim_stack & stack, const std::vector< const ufbx_node * > & jointToNode, float uniformScale) noexcept
	{
		std::vector< AnimationChannel< float > > channels;

		if ( stack.anim == nullptr )
		{
			return channels;
		}

		const double t0 = stack.time_begin;
		const double t1 = stack.time_end;
		const double duration = t1 - t0;

		if ( duration <= 0.0 )
		{
			return channels;
		}

		/* 30 Hz matches Mixamo's default export and is the game-engine canonical
		 * rate. Low-frequency curves are oversampled (harmless); high-frequency
		 * ones (>30 Hz) are aliased — acceptable for skeletal anim but not for
		 * procedural VFX. */
		constexpr double sampleRate = 30.0;
		constexpr double sampleStep = 1.0 / sampleRate;

		const size_t numFrames = static_cast< size_t >(std::ceil(duration * sampleRate)) + 1;
		const size_t jointCount = jointToNode.size();

		channels.reserve(jointCount * 3);

		for ( size_t ji = 0; ji < jointCount; ++ji )
		{
			const ufbx_node * bone = jointToNode[ji];

			if ( bone == nullptr )
			{
				continue;
			}

			AnimationChannel< float > translation;
			translation.targetIndex = static_cast< int32_t >(ji);
			translation.target = ChannelTarget::Translation;
			translation.interpolation = ChannelInterpolation::Linear;
			translation.vectorKeyFrames.reserve(numFrames);

			AnimationChannel< float > rotation;
			rotation.targetIndex = static_cast< int32_t >(ji);
			rotation.target = ChannelTarget::Rotation;
			rotation.interpolation = ChannelInterpolation::Linear;
			rotation.quaternionKeyFrames.reserve(numFrames);

			AnimationChannel< float > scaleChannel;
			scaleChannel.targetIndex = static_cast< int32_t >(ji);
			scaleChannel.target = ChannelTarget::Scale;
			scaleChannel.interpolation = ChannelInterpolation::Linear;
			scaleChannel.vectorKeyFrames.reserve(numFrames);

			for ( size_t f = 0; f < numFrames; ++f )
			{
				double t = t0 + (static_cast< double >(f) * sampleStep);

				t = std::min(t, t1);

				const ufbx_transform xf = ufbx_evaluate_transform(stack.anim, bone, t);

				const auto relTime = static_cast< float >(t - t0);

				/* ⚠️ A translation is mirrored, a rotation is CONJUGATED (with its det(M) angle
				 * inversion), a scale is invariant. A clip mirrored inconsistently with the bind
				 * pose it drives makes the rig snap on the first animated frame. */
				translation.vectorKeyFrames.push_back({
					.time = relTime,
					.value = Vector< 3, float >{
						static_cast< float >(xf.translation.x) * uniformScale,
						static_cast< float >(xf.translation.y) * uniformScale,
						static_cast< float >(xf.translation.z) * uniformScale
					}
				});

				rotation.quaternionKeyFrames.push_back({
					.time = relTime,
					.value = Quaternion< float >{
						static_cast< float >(xf.rotation.x),
						static_cast< float >(xf.rotation.y),
						static_cast< float >(xf.rotation.z),
						static_cast< float >(xf.rotation.w)
					}
				});

				scaleChannel.vectorKeyFrames.push_back({
					.time = relTime,
					.value = Vector< 3, float >{
						static_cast< float >(xf.scale.x),
						static_cast< float >(xf.scale.y),
						static_cast< float >(xf.scale.z)
					}
				});
			}

			channels.push_back(std::move(translation));
			channels.push_back(std::move(rotation));
			channels.push_back(std::move(scaleChannel));
		}

		return channels;
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
		ufbx_load_opts opts{};
		opts.target_axes = ufbx_axes_right_handed_y_up;
		opts.target_unit_meters = 1.0F;
		opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
		opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
		opts.load_external_files = false;
		opts.generate_missing_normals = false;

		ufbx_error error{};
		ufbx_scene * scene = ufbx_load_file(filepath.string().c_str(), &opts, &error);

		if ( scene == nullptr )
		{
			std::array< char, 512 > buffer{};
			ufbx_format_error(buffer.data(), sizeof(buffer), &error);

			TraceError{ClassId} << "Failed to parse animation file '" << filepath << "' :\n" << buffer.data();

			return false;
		}

		const std::unique_ptr< ufbx_scene, decltype(&ufbx_free_scene) > sceneGuard{scene, ufbx_free_scene};

		if ( scene->anim_stacks.count == 0 )
		{
			TraceWarning{ClassId} << "No animation stacks in '" << filepath << "'.";

			return false;
		}

		/* Resolve each skeleton joint to its driving ufbx_node by name. Joints
		 * that have no counterpart in this file get a null entry and are
		 * silently dropped during sampling. */
		const size_t jointCount = skeleton.jointCount();
		std::vector< const ufbx_node * > jointToNode(jointCount, nullptr);
		size_t resolvedJoints = 0;

		for ( size_t ji = 0; ji < jointCount; ++ji )
		{
			const auto & jointName = skeleton.joint(ji).name;

			if ( jointName.empty() )
			{
				continue;
			}

			const ufbx_node * node = ufbx_find_node_len(scene, jointName.data(), jointName.length());

			if ( node != nullptr )
			{
				jointToNode[ji] = node;
				++resolvedJoints;
			}
		}

		if ( resolvedJoints == 0 )
		{
			TraceError{ClassId} << "Animation file '" << filepath
				<< "' shares no joint names with skeleton '" << targetSkeleton.name()
				<< "'. Aborting.";

			return false;
		}

		if ( resolvedJoints < jointCount )
		{
			TraceInfo{ClassId} << "Resolved " << resolvedJoints << " / " << jointCount
				<< " joints by name in '" << filepath << "'. Missing joints will hold their bind-pose value.";
		}

		const std::string fileStem = filepath.stem().string();
		/* ⚠️ The flip is part of the cache key: a clip mirrored one way must never be served to a
		 * rig loaded with a different flip. */
		const std::string resourcePrefix = "FBX:" + fileStem + "/Animation/";
		const size_t producedBefore = output.size();

		/* When stripRootMotion is enabled, collect the indices of every root
		 * joint once — they're the only ones whose translation track will be
		 * zeroed after sampling. Computed outside the per-stack loop so a
		 * multi-stack file pays the cost only once. */
		std::vector< size_t > rootJointIndices;

		if ( m_options.stripRootMotion )
		{
			rootJointIndices = skeleton.rootJoints();
		}

		output.reserve(output.size() + scene->anim_stacks.count);

		for ( size_t si = 0; si < scene->anim_stacks.count; ++si )
		{
			const ufbx_anim_stack & stack = *scene->anim_stacks.data[si];

			auto channels = sampleAnimStack(stack, jointToNode, m_options.uniformScale);

			if ( channels.empty() )
			{
				continue;
			}

			/* Strip the HORIZONTAL components (X, Z) of root-bone translation
			 * tracks if requested. Mixamo locomotion clips (walk_*, run_*,
			 * strafe_*) bake forward displacement into the root joint —
			 * keeping it would stack with gameplay-driven motion (addForce,
			 * navmesh, etc.) and produce a yo-yo at each clip loop.
			 *
			 * The vertical (Y) component is preserved on purpose: it carries
			 * both the bind-pose hip-height offset (~0.85 m on a Mixamo
			 * humanoid — wiping it would sink the model halfway into the
			 * ground) and the natural up/down bounce of walking, jumping or
			 * crouching, which is part of the visual we want to keep.
			 * Rotation + scale of the root and every channel of every other
			 * joint stay intact. Idiomatic "convert to in-place clip" pass at
			 * load time. */
			if ( m_options.stripRootMotion && !rootJointIndices.empty() )
			{
				for ( auto & channel : channels )
				{
					if ( channel.target != ChannelTarget::Translation )
					{
						continue;
					}

					const auto isRoot = std::ranges::find(
						rootJointIndices,
						static_cast< size_t >(channel.targetIndex)
					) != rootJointIndices.end();

					if ( !isRoot )
					{
						continue;
					}

					for ( auto & keyFrame : channel.vectorKeyFrames )
					{
						keyFrame.value[X] = 0.0F;
						keyFrame.value[Z] = 0.0F;
					}
				}
			}

			/* Mixamo names every stack `mixamo.com` (or similar) — useless. The
			 * filename is the actual semantic name in this multi-file workflow. */
			const std::string clipName = scene->anim_stacks.count == 1 ? fileStem : fileStem + "_" + std::to_string(si);

			AnimationClip< float > clip{clipName, std::move(channels)};

			auto clipResource = m_resources.container< Animations::AnimationClipResource >()
				->getOrCreateResourceSync(
					resourcePrefix + clipName,
					[&clip] (auto & resource) {
						return resource.load(std::move(clip));
					}
				);

			if ( clipResource != nullptr )
			{
				output.push_back(std::move(clipResource));
			}
		}

		if ( output.size() - producedBefore == 0 )
		{
			TraceWarning{ClassId} << "Animation file '" << filepath << "' produced no clips.";

			return false;
		}

		return true;
	}

	void
	FBXLoader::buildNodeDescriptors (const ufbx_scene & scene, SceneData & output) noexcept
	{
		/* Build a lookup from ufbx_node to its index in the ufbx scene,
		 * which becomes the corresponding index in SceneData::nodes. */
		std::unordered_map< const ufbx_node *, size_t > nodeIndexMap;
		nodeIndexMap.reserve(scene.nodes.count);

		output.nodes.clear();
		output.nodes.reserve(scene.nodes.count);

		/* Build a mapping from ufbx mesh element_id to its index in output.meshes. */
		std::unordered_map< uint32_t, size_t > meshElementToIndex;
		meshElementToIndex.reserve(scene.meshes.count);

		for ( size_t index = 0; index < scene.meshes.count; ++index )
		{
			meshElementToIndex[scene.meshes.data[index]->element_id] = index;
		}

		/* Exclusion helper: a node is filtered out if its own name or any
		 * ancestor's name appears in LoaderOptions::excludedNodeNames.
		 * Skipping a parent transparently skips the entire subtree. */
		const auto isExcluded = [this] (const ufbx_node * node) noexcept {
			if ( m_options.excludedNodeNames.empty() )
			{
				return false;
			}

			while ( node != nullptr && !node->is_root )
			{
				if ( node->name.length > 0 )
				{
					const std::string name{node->name.data, node->name.length};

					if ( m_options.excludedNodeNames.contains(name) )
					{
						return true;
					}
				}

				node = node->parent;
			}

			return false;
		};

		/* First pass: allocate a NodeDescriptor slot for every ufbx node (skipping the
		 * root node itself, whose transform is identity by ufbx convention). */
		for ( size_t index = 0; index < scene.nodes.count; ++index )
		{
			const ufbx_node * node = scene.nodes.data[index];

			if ( node == nullptr || node->is_root )
			{
				continue;
			}

			if ( isExcluded(node) )
			{
				continue;
			}

			const auto descriptorIndex = output.nodes.size();

			nodeIndexMap[node] = descriptorIndex;

			/* Resolve the bone nodes collected by loadSkins() to their COMPACTED descriptor
			 * index. Doing it here is the whole point: this is the only place where the
			 * mapping exists. A bone that is excluded never gets an index, and therefore
			 * never makes the consumer skip an unrelated node. */
			if ( m_skinJointNodes.contains(node) )
			{
				m_skinJointNodeIndices.insert(descriptorIndex);
			}

			NodeDescriptor descriptor;
			descriptor.name = buildNodeName(m_resourcePrefix, *node);
			descriptor.localFrame = extractFrameFromNode(*node);

			if ( node->mesh != nullptr )
			{
				const auto it = meshElementToIndex.find(node->mesh->element_id);

				if ( it != meshElementToIndex.end() )
				{
					descriptor.meshIndex = it->second;
				}
			}

			output.nodes.push_back(std::move(descriptor));
		}

		/* Second pass: wire children and collect roots (children of the ufbx root). */
		output.rootNodeIndices.clear();

		for ( const auto & [nodePtr, descriptorIdx] : nodeIndexMap )
		{
			const ufbx_node * parent = nodePtr->parent;

			if ( parent == nullptr || parent->is_root )
			{
				output.rootNodeIndices.push_back(descriptorIdx);
			}
			else
			{
				const auto parentIt = nodeIndexMap.find(parent);

				if ( parentIt != nodeIndexMap.end() )
				{
					output.nodes[parentIt->second].childIndices.push_back(descriptorIdx);
				}
			}
		}
	}
}
