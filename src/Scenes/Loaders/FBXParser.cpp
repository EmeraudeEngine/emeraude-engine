/*
 * src/Scenes/Loaders/FBXParser.cpp
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

#include "FBXParser.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/* Third-party inclusions. */
#include <ufbx/ufbx.h>

/* Local inclusions. */
#include "Interface.hpp"
#include "Animation/AnimationChannel.hpp"
#include "Animation/Joint.hpp"
#include "IO/IO.hpp"
#include "Math/CartesianFrame.hpp"
#include "Math/Matrix.hpp"
#include "Math/Quaternion.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace
{
	using namespace EmEn;
	using namespace EmEn::Base::Math;
	using namespace EmEn::Base::PixelFactory;

	/* NOTE: The loader's name: the analysis reports under it, as before the split. */
	constexpr auto ClassId{"FBXLoader"};

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

namespace EmEn::Scenes::Loaders::FBX
{
	using namespace Base::Animation;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Base::VertexFactory;

	namespace
	{
		/**
		 * @brief Describes the image of every FBX texture, by texture index.
		 * @param scene The ufbx scene.
		 * @param resourcePrefix The resource name prefix.
		 * @param basePath The directory external textures are relative to.
		 * @param images The images, one per texture.
		 */
		void
		describeImages (const ufbx_scene & scene, const std::string & resourcePrefix, const std::filesystem::path & basePath, std::vector< Image > & images) noexcept
		{
			images.resize(scene.textures.count);

			for ( size_t textureIndex = 0; textureIndex < scene.textures.count; ++textureIndex )
			{
				const ufbx_texture & tex = *scene.textures.data[textureIndex];
				auto & image = images[textureIndex];

				/* The index makes the key unique per FBX texture; the loader appends the colour space,
				 * because one texture legitimately yields two engine resources. Same convention as the
				 * glTF and USD loaders. */
				image.textureName = buildResourceKey(resourcePrefix, "Texture/", {tex.name.data, tex.name.length}, textureIndex);

				/* Skip non-file textures (layered, procedural, shader...) — they'd need custom resolution. */
				if ( tex.type != UFBX_TEXTURE_FILE )
				{
					continue;
				}

				/* ⚠️ Keyed on the INDEX, not on the name — an FBX name is not an identity, and
				 * duplicate node/material names are the norm in that format. See buildResourceKey(). */
				image.resourceName = buildResourceKey(resourcePrefix, "Image/", {tex.name.data, tex.name.length}, textureIndex);

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

					image.bytes = std::make_shared< std::vector< std::byte > >(dataPtr, dataPtr + tex.content.size);
					image.contentSize = tex.content.size;
					image.format = format;
					image.kind = ImageKind::Embedded;

					continue;
				}

				/* External file. ufbx exposes the path relative to the loaded FBX
				 * in `filename`; that is what the engine should resolve first.
				 * If the FBX referenced a far-away drive, ufbx may have filled
				 * `absolute_filename` instead — use it as a fallback.
				 * NOTE: ufbx strings are UTF-8: IO::u8path(), never the narrow path constructor (the ANSI
				 * code page on Windows). */
				std::filesystem::path fullPath;

				if ( tex.filename.length > 0 )
				{
					fullPath = basePath / Base::IO::u8path(std::string{tex.filename.data, tex.filename.length});
				}
				else if ( tex.absolute_filename.length > 0 )
				{
					fullPath = Base::IO::u8path(std::string{tex.absolute_filename.data, tex.absolute_filename.length});
				}
				else if ( tex.relative_filename.length > 0 )
				{
					fullPath = basePath / Base::IO::u8path(std::string{tex.relative_filename.data, tex.relative_filename.length});
				}

				if ( fullPath.empty() )
				{
					TraceWarning{ClassId} << "Texture '" << image.resourceName << "' has no resolvable path, using default.";

					image.kind = ImageKind::Unresolved;

					continue;
				}

				image.path = std::move(fullPath);
				image.kind = ImageKind::File;
			}
		}

		/**
		 * @brief Describes every FBX material, by material index.
		 * @param scene The ufbx scene.
		 * @param resourcePrefix The resource name prefix.
		 * @param materials The materials.
		 */
		void
		describeMaterials (const ufbx_scene & scene, const std::string & resourcePrefix, std::vector< Material > & materials) noexcept
		{
			materials.resize(scene.materials.count);

			for ( size_t materialIndex = 0; materialIndex < scene.materials.count; ++materialIndex )
			{
				const ufbx_material & fbxMaterial = *scene.materials.data[materialIndex];
				auto & material = materials[materialIndex];

				/* ⚠️ Keyed on the INDEX, not on the name. See buildResourceKey(). */
				material.resourceName = buildResourceKey(resourcePrefix, "Material/", {fbxMaterial.name.data, fbxMaterial.name.length}, materialIndex);

				/* A texture use: the texture's typed_id (its index in scene.textures) and the colour
				 * space the USAGE needs — the loader keeps one engine texture per (index, colour space). */
				const auto slot = [&scene] (const ufbx_texture * tex, bool sRGB = false) -> std::optional< TextureSlot > {
					if ( tex == nullptr )
					{
						return std::nullopt;
					}

					const auto texIdx = static_cast< size_t >(tex->typed_id);

					if ( texIdx >= scene.textures.count )
					{
						return std::nullopt;
					}

					return TextureSlot{.texture = texIdx, .sRGB = sRGB};
				};

				const auto & pbr = fbxMaterial.pbr;

				/* Albedo / base color (sRGB). ufbx stores the linear factor in value_real
				 * and the tint in value_vec4; combine them into the engine's albedo. */
				material.albedoTexture = slot(pbr.base_color.texture, true);

				material.albedoColor = Color< float >{
					static_cast< float >(pbr.base_color.value_vec4.x),
					static_cast< float >(pbr.base_color.value_vec4.y),
					static_cast< float >(pbr.base_color.value_vec4.z),
					static_cast< float >(pbr.base_color.value_vec4.w > 0.0 ? pbr.base_color.value_vec4.w : 1.0)
				};

				if ( !pbr.base_color.has_value )
				{
					material.albedoColor = Color< float >{1.0F, 1.0F, 1.0F, 1.0F};
				}

				/* Roughness and Metalness (linear, UNORM). */
				material.roughnessTexture = slot(pbr.roughness.texture);
				material.roughnessFactor = pbr.roughness.has_value ? static_cast< float >(pbr.roughness.value_real) : 0.5F;

				material.metalnessTexture = slot(pbr.metalness.texture);
				material.metalnessFactor = pbr.metalness.has_value ? static_cast< float >(pbr.metalness.value_real) : 0.0F;

				/* Normal map (linear). */
				material.normalTexture = slot(pbr.normal_map.texture);

				/* Ambient occlusion (linear). */
				material.ambientOcclusionTexture = slot(pbr.ambient_occlusion.texture);

				/* Emission (sRGB color + linear factor). */
				material.emissiveTexture = slot(pbr.emission_color.texture, true);

				material.emissiveColor = Color< float >{
					static_cast< float >(pbr.emission_color.value_vec3.x),
					static_cast< float >(pbr.emission_color.value_vec3.y),
					static_cast< float >(pbr.emission_color.value_vec3.z),
					1.0F
				};

				material.emissiveStrength = pbr.emission_factor.has_value ? static_cast< float >(pbr.emission_factor.value_real) : 0.0F;

				material.hasEmissiveColor = pbr.emission_color.has_value
					&& (pbr.emission_color.value_vec3.x > 0.0
						|| pbr.emission_color.value_vec3.y > 0.0
						|| pbr.emission_color.value_vec3.z > 0.0);

				/* Specular (KHR_materials_specular semantics) — read ONLY on a PBR shading model.
				 *
				 * ⚠️⚠️ `pbr.specular_factor` / `pbr.specular_color` are a VIEW ufbx builds over whatever
				 * the file declares, and the SAME two fields carry two incompatible meanings:
				 *   - on a Standard-Surface / OpenPBR / physical material they are the **dielectric
				 *     specular weight and tint**, i.e. exactly what KHR_materials_specular scales and
				 *     tints in the engine's F0;
				 *   - on a legacy `FbxSurfacePhong` they are the **Phong specular highlight**, whose
				 *     engine counterpart is the glossiness path, not F0.
				 * Reading them blind would push an old asset's Phong highlight colour into the
				 * dielectric F0 of every surface it touches, and nothing would report it.
				 *
				 * `ufbx_material::shader_type` is always defined, so the file tells us which meaning
				 * applies. Anything ufbx cannot name — and `3DS_MAX_PBR_SPEC_GLOSS`, which is a
				 * specular/glossiness workflow rather than an F0 scale — is left alone on purpose: the
				 * identity is what a material that declares nothing must keep. */
				switch ( fbxMaterial.shader_type )
				{
					case UFBX_SHADER_OSL_STANDARD_SURFACE :
					case UFBX_SHADER_ARNOLD_STANDARD_SURFACE :
					case UFBX_SHADER_3DS_MAX_PHYSICAL_MATERIAL :
					case UFBX_SHADER_3DS_MAX_PBR_METAL_ROUGH :
					case UFBX_SHADER_GLTF_MATERIAL :
					case UFBX_SHADER_OPENPBR_MATERIAL :
						material.specularIsDielectricF0 = true;
						break;

					default :
						break;
				}

				/* ⚠️ The colour is a Vector, not a Color: the factor is a MULTIPLIER and may legitimately
				 * exceed 1, which `PixelFactory::Color` would clamp away. Same reasoning as the glTF loader. */
				if ( material.specularIsDielectricF0 )
				{
					if ( pbr.specular_factor.has_value )
					{
						material.specularFactor = static_cast< float >(pbr.specular_factor.value_real);
					}

					if ( pbr.specular_color.has_value )
					{
						material.specularColor = Vector< 3, float >{
							static_cast< float >(pbr.specular_color.value_vec3.x),
							static_cast< float >(pbr.specular_color.value_vec3.y),
							static_cast< float >(pbr.specular_color.value_vec3.z)
						};
					}
				}

				/* Opacity (engine opacity contract). ufbx.pbr.opacity is 1.0 (fully opaque) by
				 * default. FBX has no cutout notion: a texture-driven opacity is a grayscale
				 * per-pixel alpha scale (rule 3), a scalar < 1 a uniform transparency (rule 1) —
				 * both blend. The map is DATA (linear, red channel). */
				material.opacityTexture = slot(pbr.opacity.texture);
				material.opacityValue = pbr.opacity.has_value ? static_cast< float >(pbr.opacity.value_real) : 1.0F;
			}
		}

		/**
		 * @brief Describes every FBX mesh: its shape, its material parts and its skin.
		 * @param scene The ufbx scene.
		 * @param resourcePrefix The resource name prefix.
		 * @param options The loader options.
		 * @param meshes The meshes, one per scene mesh (a null shape for a mesh without a valid triangle).
		 */
		void
		describeMeshes (const ufbx_scene & scene, const std::string & resourcePrefix, const LoaderOptions & options, std::vector< Mesh > & meshes) noexcept
		{
			meshes.resize(scene.meshes.count);

			/* ⚠️ The axis conversion is baked into the VERTEX DATA (positions and normals). A normal
			 * needs no special case: the inverse-transpose of a diagonal ±1 matrix is the matrix itself.
			 * The triangle winding is kept exactly as authored: the unconditional swap that used to be
			 * declared here only compensated the engine's ORIENTATION-REVERSING projection, and that
			 * mirror went away with the Y-up convention flip. */

			for ( size_t meshIndex = 0; meshIndex < scene.meshes.count; ++meshIndex )
			{
				const ufbx_mesh & mesh = *scene.meshes.data[meshIndex];
				auto & description = meshes[meshIndex];

				/* ⚠️⚠️ Keyed on the INDEX, not on the name. Duplicate mesh names are ordinary in FBX
				 * (every instance of the same authored object carries it), and a key built on the name
				 * alone hands the FIRST homonym's geometry AND material to every later one, silently.
				 * Measured on the glTF side of the same defect: ClearCoatTest's eighteen meshes named
				 * 'ClearCoatSampleMesh' all rendered material 0. See buildResourceKey().
				 * NOTE: mesh.element_id — unique but unrelated to the loop order — was the previous
				 * fallback for an unnamed mesh; the loop index is what identifies it here. */
				const std::string_view baseName{mesh.name.data, mesh.name.length};

				description.geometryName = buildResourceKey(resourcePrefix, "Geometry/", baseName, meshIndex);
				description.meshName = buildResourceKey(resourcePrefix, "Mesh/", baseName, meshIndex);

				const auto & meshName = description.meshName;

				/* Count total triangles (fan triangulation: polygon with N vertices → N-2 triangles).
				 * Vertex count = triangleCount * 3 because we emit per-corner (attribute layers
				 * may differ per corner). A dedupe pass via ufbx_generate_indices can be added
				 * later if the overhead becomes measurable.
				 * NOTE: Counted in 64 bits and bounded: the vertex count is a uint32_t, and a 32-bit
				 * sum wrapped into a short vertex array the build then overran. */
				uint64_t triangleSum = 0;

				for ( size_t faceIdx = 0; faceIdx < mesh.faces.count; ++faceIdx )
				{
					const auto & face = mesh.faces.data[faceIdx];

					if ( face.num_indices >= 3 )
					{
						triangleSum += face.num_indices - 2;
					}
				}

				if ( triangleSum == 0 )
				{
					TraceWarning{ClassId} << "Mesh '" << meshName << "' has no valid triangle faces, using default.";

					continue;
				}

				if ( triangleSum > std::numeric_limits< uint32_t >::max() / 3 )
				{
					TraceWarning{ClassId} << "Mesh '" << meshName << "' has " << triangleSum << " triangles, more than a geometry holds, using default.";

					continue;
				}

				const auto totalTriangleCount = static_cast< uint32_t >(triangleSum);
				const uint32_t totalVertexCount = totalTriangleCount * 3;

				/* Material parts: each part is a subset of faces sharing the same material slot.
				 * A single part (no split) is the common case; multi-material meshes produce
				 * multiple parts which map to sub-geometry groups in the engine's Shape. */
				const bool hasMaterialSplit = mesh.material_parts.count > 1;
				const bool hasNormals = mesh.vertex_normal.exists;
				const bool hasUV = mesh.vertex_uv.exists;

				/* Skinning: use the first skin_deformer on the mesh when available.
				 * cluster_index inside ufbx_skin_weight corresponds directly to the
				 * joint index in the future Skeleton (describeSkins builds joints in
				 * cluster order), so no remapping is required here. */
				const ufbx_skin_deformer * skin = (!options.skipSkinning && mesh.skin_deformers.count > 0)
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

									vertices[globalVertexOffset + k].setPosition(Vector< 3, float >{static_cast< float >(pos.x) * options.uniformScale, static_cast< float >(pos.y) * options.uniformScale, static_cast< float >(pos.z) * options.uniformScale});

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

				/* The skin deformer's typed_id: the loader enables the influence and weight streams and
				 * attaches the skeletal data built by describeSkins() at the same index. */
				if ( hasSkin )
				{
					description.skin = static_cast< size_t >(skin->typed_id);
				}

				/* Material parts: one per material_part. Parallel to mesh.materials[]
				 * — `material_parts[i]` uses `mesh.materials.data[i]` (same ordering). */
				const size_t partCount = (mesh.material_parts.count == 0) ? 1 : mesh.material_parts.count;

				description.parts.resize(partCount);

				for ( size_t partIdx = 0; partIdx < partCount && partIdx < mesh.materials.count; ++partIdx )
				{
					const ufbx_material * fbxMat = mesh.materials.data[partIdx];

					if ( fbxMat == nullptr )
					{
						continue;
					}

					description.parts[partIdx].material = static_cast< size_t >(fbxMat->typed_id);
					/* The FBX material's double-sided feature (ufbx): cloth, inner armour surfaces, … */
					description.parts[partIdx].doubleSided = fbxMat->features.double_sided.enabled;
				}

				description.shape = std::move(shape);
			}
		}

		/**
		 * @brief Builds the skeleton and the skin of every skin deformer.
		 * @param scene The ufbx scene.
		 * @param resourcePrefix The resource name prefix.
		 * @param options The loader options.
		 * @param skins The skins, by skin deformer index.
		 * @param skinJointNodes The bone nodes of every skin.
		 */
		void
		describeSkins (const ufbx_scene & scene, const std::string & resourcePrefix, const LoaderOptions & options, std::vector< std::optional< Skin > > & skins, std::unordered_set< const ufbx_node * > & skinJointNodes) noexcept
		{
			if ( scene.skin_deformers.count == 0 )
			{
				return;
			}

			/* ⚠️ The rig is mirrored exactly like the mesh it deforms — bind-pose translations,
			 * bind-pose ROTATIONS and inverse bind matrices — or the skin slides off the geometry. */

			/* Skin deformers are indexed by typed_id (which equals their position
			 * in scene.skin_deformers.data[]). describeMeshes() wrote this typed_id into
			 * the meshes' skin, so the skins must be filled at the same positions —
			 * we resize to count and leave gaps empty. */
			skins.resize(scene.skin_deformers.count);

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
					inverseBindMatrices[ci][M4x4Col3Row0] *= options.uniformScale;
					inverseBindMatrices[ci][M4x4Col3Row1] *= options.uniformScale;
					inverseBindMatrices[ci][M4x4Col3Row2] *= options.uniformScale;

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
					 * built in describeAnimations(). */
					const auto & trs = bone.local_transform;

					joints[ci].translation = Vector< 3, float >{static_cast< float >(trs.translation.x) * options.uniformScale, static_cast< float >(trs.translation.y) * options.uniformScale, static_cast< float >(trs.translation.z) * options.uniformScale};

					joints[ci].rotation = Quaternion< float >{
						static_cast< float >(trs.rotation.x),
						static_cast< float >(trs.rotation.y),
						static_cast< float >(trs.rotation.z),
						static_cast< float >(trs.rotation.w)
					};

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

				Base::Animation::Skin< float > engineSkin{std::move(skinJointIndices), inverseBindMatrices};

				const std::string skeletonName = resourcePrefix + std::string{} + "Skeleton/"
					+ (skin.name.length == 0 ? std::to_string(skinIndex) : std::string{skin.name.data, skin.name.length});

				skins[skinIndex] = Skin{.resourceName = skeletonName, .skeleton = std::move(skeleton), .skin = std::move(engineSkin)};

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
						skinJointNodes.insert(cluster->bone_node);
					}
				}
			}
		}

		/**
		 * @brief Resamples one anim_stack into per-joint T/R/S channels.
		 * @note jointToNode[i] is the ufbx_node driving joint i, or nullptr to skip
		 * the joint. Sample rate is 30 Hz (Mixamo canonical).
		 * @param stack The FBX animation stack to evaluate.
		 * @param jointToNode Per-joint ufbx_node mapping. Entries can be null.
		 * @param uniformScale The uniform scale applied to translation keyframes.
		 * @return std::vector< AnimationChannel< float > >
		 */
		[[nodiscard]]
		std::vector< AnimationChannel< float > >
		sampleAnimStack (const ufbx_anim_stack & stack, const std::vector< const ufbx_node * > & jointToNode, float uniformScale) noexcept
		{
			std::vector< AnimationChannel< float > > channels;

			if ( stack.anim == nullptr )
			{
				return channels;
			}

			const double t0 = stack.time_begin;
			const double t1 = stack.time_end;
			const double duration = t1 - t0;

			/* NOTE: A non-finite bound reached the size_t conversion below (undefined behaviour). */
			if ( !std::isfinite(t0) || !std::isfinite(t1) || !std::isfinite(duration) )
			{
				TraceWarning{ClassId} << "Animation stack '" << std::string_view{stack.name.data, stack.name.length} << "' has a non-finite time range, skipped.";

				return channels;
			}

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

			const size_t jointCount = jointToNode.size();
			const auto animatedJoints = static_cast< double >(std::ranges::count_if(jointToNode, [] (const ufbx_node * node) {
				return node != nullptr;
			}));

			/* NOTE: No joint to animate, no channel (and no frame count to convert). */
			if ( animatedJoints == 0.0 )
			{
				return channels;
			}

			/* NOTE: Bounded BEFORE any allocation, in double: the duration is the file's (fuzz_fbx, 2026-10-18: up to
			 * 10^12 samples asked). Owner decision: a stack beyond MaxClipKeyFrames is skipped. */
			const double frames = std::ceil(duration * sampleRate) + 1.0;

			if ( frames * animatedJoints * 3.0 > static_cast< double >(MaxClipKeyFrames) )
			{
				TraceWarning{ClassId} << "Animation stack '" << std::string_view{stack.name.data, stack.name.length} << "' (" << duration << " s, " << animatedJoints << " joints) exceeds " << MaxClipKeyFrames << " keyframes, skipped.";

				return channels;
			}

			const auto numFrames = static_cast< size_t >(frames);

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

					/* ⚠️ The keys are taken as ufbx returns them, exactly like the bind pose (no axis
					 * conversion since the Y-up migration). A clip converted inconsistently with the bind
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

		/**
		 * @brief Samples every animation stack against the joints of the first skin deformer.
		 * @param scene The ufbx scene.
		 * @param resourcePrefix The resource name prefix.
		 * @param options The loader options.
		 * @param clips The clips.
		 */
		void
		describeAnimations (const ufbx_scene & scene, const std::string & resourcePrefix, const LoaderOptions & options, std::vector< Clip > & clips) noexcept
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

			clips.reserve(scene.anim_stacks.count);

			for ( size_t si = 0; si < scene.anim_stacks.count; ++si )
			{
				const ufbx_anim_stack & stack = *scene.anim_stacks.data[si];

				auto channels = sampleAnimStack(stack, jointToNode, options.uniformScale);

				if ( channels.empty() )
				{
					continue;
				}

				const std::string clipName =
					stack.name.length == 0 ?
					"clip_" + std::to_string(si) :
					std::string{stack.name.data, stack.name.length};

				AnimationClip< float > clip{clipName, std::move(channels)};

				clips.push_back(Clip{.resourceName = Base::String::concatenate(resourcePrefix, "Animation/", clipName), .clip = std::move(clip)});
			}

			if ( !clips.empty() )
			{
				TraceDebug{ClassId} << "Loaded " << clips.size() << " animation clips.";
			}
		}

		/**
		 * @brief Builds the node descriptors, the roots and the descriptor indices of the bone nodes.
		 * @param scene The ufbx scene.
		 * @param resourcePrefix The resource name prefix.
		 * @param options The loader options.
		 * @param skinJointNodes The bone nodes collected by describeSkins().
		 * @param asset The asset receiving the nodes.
		 */
		void
		describeNodes (const ufbx_scene & scene, const std::string & resourcePrefix, const LoaderOptions & options, const std::unordered_set< const ufbx_node * > & skinJointNodes, Asset & asset) noexcept
		{
			/* Build a lookup from ufbx_node to its index in the ufbx scene,
			 * which becomes the corresponding index in SceneData::nodes. */
			std::unordered_map< const ufbx_node *, size_t > nodeIndexMap;
			nodeIndexMap.reserve(scene.nodes.count);

			asset.nodes.clear();
			asset.nodes.reserve(scene.nodes.count);

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
			const auto isExcluded = [&options] (const ufbx_node * node) noexcept {
				if ( options.excludedNodeNames.empty() )
				{
					return false;
				}

				while ( node != nullptr && !node->is_root )
				{
					if ( node->name.length > 0 )
					{
						const std::string name{node->name.data, node->name.length};

						if ( options.excludedNodeNames.contains(name) )
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

				const auto descriptorIndex = asset.nodes.size();

				nodeIndexMap[node] = descriptorIndex;

				/* Resolve the bone nodes collected by describeSkins() to their COMPACTED descriptor
				 * index. Doing it here is the whole point: this is the only place where the
				 * mapping exists. A bone that is excluded never gets an index, and therefore
				 * never makes the consumer skip an unrelated node. */
				if ( skinJointNodes.contains(node) )
				{
					asset.skinJointNodeIndices.insert(descriptorIndex);
				}

				NodeDescriptor descriptor;
				descriptor.name = buildNodeName(resourcePrefix, *node);
				descriptor.localFrame = extractFrameFromNode(*node);

				if ( node->mesh != nullptr )
				{
					const auto it = meshElementToIndex.find(node->mesh->element_id);

					if ( it != meshElementToIndex.end() )
					{
						descriptor.meshIndex = it->second;
					}
				}

				asset.nodes.push_back(std::move(descriptor));
			}

			/* Second pass: wire children and collect roots (children of the ufbx root).
			 * NOTE: in the FILE order (scene.nodes), never by iterating nodeIndexMap: an unordered_map keyed by pointer
			 * yields a different order on every run — the order of the roots and children, and therefore which duplicate
			 * name the scene keeps verbatim (SceneDataConsumer renames the later ones), changed from run to run. */
			asset.rootNodeIndices.clear();

			for ( size_t index = 0; index < scene.nodes.count; ++index )
			{
				const ufbx_node * nodePtr = scene.nodes.data[index];

				if ( nodePtr == nullptr )
				{
					continue;
				}

				const auto nodeIt = nodeIndexMap.find(nodePtr);

				if ( nodeIt == nodeIndexMap.end() )
				{
					/* The ufbx root or an excluded node: no descriptor. */
					continue;
				}

				const auto descriptorIdx = nodeIt->second;
				const ufbx_node * parent = nodePtr->parent;

				if ( parent == nullptr || parent->is_root )
				{
					asset.rootNodeIndices.push_back(descriptorIdx);
				}
				else
				{
					const auto parentIt = nodeIndexMap.find(parent);

					if ( parentIt != nodeIndexMap.end() )
					{
						asset.nodes[parentIt->second].childIndices.push_back(descriptorIdx);
					}
				}
			}
		}
	}

	void
	setMemoryLimits (ufbx_load_opts & options, uint64_t sourceBytes) noexcept
	{
		const auto limit = sourceBytes > std::numeric_limits< size_t >::max() / MemoryLimitPerFileByte
			? std::numeric_limits< size_t >::max()
			: std::max(MinimumMemoryLimit, static_cast< size_t >(sourceBytes) * MemoryLimitPerFileByte);

		options.temp_allocator.memory_limit = limit;
		options.result_allocator.memory_limit = limit;
	}

	void
	setLoadOptions (ufbx_load_opts & options, bool wholeScene, uint64_t sourceBytes) noexcept
	{
		setMemoryLimits(options, sourceBytes);

		options.target_axes = ufbx_axes_right_handed_y_up;
		options.target_unit_meters = 1.0;
		/* MODIFY_GEOMETRY bakes the axis conversion into vertex positions and
		 * into `cluster->geometry_to_bone`. Static meshes and bind-pose skinning
		 * are correct. Animated skinning currently exhibits a residual
		 * coord-space mismatch between the bone local_transform (FBX-native,
		 * adjust_pre-compensated) and the evaluated animation curves — see
		 * AGENTS.md for the known limitation. */
		options.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
		/* An FBX node can carry a "geometry transform" that affects its attached mesh
		 * but not its children. Left at the default PRESERVE, that transform lives only
		 * in `ufbx_node.geometry_transform` and a loader reading `local_transform` alone
		 * places the mesh WRONG, silently. 3ds Max and Maya emit them routinely.
		 * MODIFY_GEOMETRY bakes it into the vertices — the same strategy already used
		 * for the axis conversion above — and falls back to helper nodes only when the
		 * same mesh is instanced several times with different geometry transforms. */
		options.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY;
		/* The animation clips only need the curves, in the same coord space as the bind pose
		 * baked into their target skeleton: same conversions, nothing else. */
		options.load_external_files = wholeScene;
		options.generate_missing_normals = wholeScene;
	}

	Asset
	analyse (const ufbx_scene & scene, const std::string & resourcePrefix, const std::filesystem::path & basePath, const LoaderOptions & options) noexcept
	{
		Asset asset;

		/* Bone nodes collected by describeSkins(). They are kept as ufbx POINTERS, never as
		 * element ids: SceneData::skinJointNodeIndices is consumed as an index into
		 * SceneData::nodes, which describeNodes() COMPACTS (the root and every
		 * excluded subtree are skipped). The two numbering schemes do not coincide, and
		 * feeding an element id to the consumer makes it drop an unrelated node together
		 * with its whole subtree. describeNodes() resolves these pointers to real
		 * node indices once the compaction is known. */
		std::unordered_set< const ufbx_node * > skinJointNodes;

		/* Pipeline: Images → Materials → Meshes → Skins → Animations → Node descriptors. */
		describeImages(scene, resourcePrefix, basePath, asset.images);
		describeMaterials(scene, resourcePrefix, asset.materials);
		describeMeshes(scene, resourcePrefix, options, asset.meshes);

		if ( !options.skipSkinning )
		{
			describeSkins(scene, resourcePrefix, options, asset.skins, skinJointNodes);
			describeAnimations(scene, resourcePrefix, options, asset.clips);
		}

		describeNodes(scene, resourcePrefix, options, skinJointNodes, asset);

		return asset;
	}

	std::vector< Clip >
	analyseClipsOnly (const ufbx_scene & scene, const Skeleton< float > & skeleton, const std::string & fileStem, const LoaderOptions & options) noexcept
	{
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

			const ufbx_node * node = ufbx_find_node_len(&scene, jointName.data(), jointName.length());

			if ( node != nullptr )
			{
				jointToNode[ji] = node;
				++resolvedJoints;
			}
		}

		if ( resolvedJoints == 0 )
		{
			TraceError{ClassId} << "Animation file '" << fileStem << "' shares no joint names with its target skeleton. Aborting.";

			return {};
		}

		if ( resolvedJoints < jointCount )
		{
			TraceInfo{ClassId} << "Resolved " << resolvedJoints << " / " << jointCount
				<< " joints by name in '" << fileStem << "'. Missing joints will hold their bind-pose value.";
		}

		const std::string resourcePrefix = "FBX:" + fileStem + "/Animation/";

		/* When stripRootMotion is enabled, collect the indices of every root
		 * joint once — they're the only ones whose translation track will be
		 * zeroed after sampling. Computed outside the per-stack loop so a
		 * multi-stack file pays the cost only once. */
		std::vector< size_t > rootJointIndices;

		if ( options.stripRootMotion )
		{
			rootJointIndices = skeleton.rootJoints();
		}

		std::vector< Clip > clips;
		clips.reserve(scene.anim_stacks.count);

		for ( size_t si = 0; si < scene.anim_stacks.count; ++si )
		{
			const ufbx_anim_stack & stack = *scene.anim_stacks.data[si];

			auto channels = sampleAnimStack(stack, jointToNode, options.uniformScale);

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
			if ( options.stripRootMotion && !rootJointIndices.empty() )
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
			const std::string clipName = scene.anim_stacks.count == 1 ? fileStem : fileStem + "_" + std::to_string(si);

			AnimationClip< float > clip{clipName, std::move(channels)};

			clips.push_back(Clip{.resourceName = resourcePrefix + clipName, .clip = std::move(clip)});
		}

		return clips;
	}
}
