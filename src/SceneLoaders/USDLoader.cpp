/*
 * src/SceneLoaders/USDLoader.cpp
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
#include <cmath>
#include <system_error>
#include <memory>
#include <utility>
#include <vector>
#include <array>

/* Third-party inclusions. */
#include "tinyusdz.hh"
#include "usdLux.hh"
#include "composition.hh"
#include "asset-resolution.hh"
#include "tydra/render-data.hh"
#include "tydra/render-data-converter.hh"

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/PBRResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Resources/Manager.hpp"
#include "SceneData.hpp"
#include "Tracer.hpp"
#include "VertexFactory/Shape.hpp"

namespace EmEn::SceneLoaders
{
	/* Above this, a prim tree is noise rather than information. */
	static constexpr size_t PrimTreeReportLimit{80};

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

			std::error_code pathError;
			const auto fullPath = std::filesystem::weakly_canonical(stageDirectory / assetIdentifier, pathError);

			if ( pathError || !std::filesystem::exists(fullPath) )
			{
				TraceWarning{ClassId} << "Texture '" << assetIdentifier << "' not found next to the stage.";

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

			const auto albedoValue = shaderSource.has_value()
				? Color< float >{shaderSource.value().diffuseColor.value[0], shaderSource.value().diffuseColor.value[1], shaderSource.value().diffuseColor.value[2], 1.0F}
				: Color< float >{0.5F, 0.5F, 0.5F, 1.0F};
			const auto roughnessValue = shaderSource.has_value() ? shaderSource.value().roughness.value : 0.5F;
			const auto metalnessValue = shaderSource.has_value() ? shaderSource.value().metallic.value : 0.0F;

			auto material = m_resources.container< Material::PBRResource >()
				->getOrCreateResource(materialName, [albedoTexture, roughnessTexture, metalnessTexture, normalTexture, albedoValue, roughnessValue, metalnessValue] (auto & materialResource) {
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

					return materialResource.setManualLoadSuccess(true);
				});

			materials.emplace_back(std::move(material));

			materialIndex++;
		}

		TraceInfo{ClassId} << materials.size() << " materials translated.";

		return materials;
	}

	size_t
	USDLoader::buildMeshes (const tinyusdz::tydra::RenderScene & renderScene, float metersPerUnit, const std::vector< std::shared_ptr< Graphics::Material::Interface > > & materials, SceneData & output) noexcept
	{
		using namespace Graphics;
		using namespace Base::Math;
		using namespace Base::VertexFactory;

		/* ⚠️ AXIS AND UNIT BAKING — the single site where USD's convention is left behind.
		 *
		 * USD here is Z-up right-handed; the engine's law is Y-DOWN (docs/coordinate-system.md).
		 * A point one unit "up" in USD is (0, 0, 1) and must become (0, -1, 0):
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
		const auto bakePosition = [metersPerUnit] (const auto & p) {
			return Vector< 3, float >{
				static_cast< float >(p[0]) * metersPerUnit,
				static_cast< float >(p[2]) * metersPerUnit,
				-static_cast< float >(p[1]) * metersPerUnit
			};
		};

		const auto bakeDirection = [] (const auto & d) {
			return Vector< 3, float >{
				static_cast< float >(d[0]),
				static_cast< float >(d[2]),
				-static_cast< float >(d[1])
			};
		};

		const auto defaultMaterial = [this] () -> std::shared_ptr< Material::Interface > {
			if ( m_options.materialMode == MaterialMode::Standard )
			{
				return m_resources.container< Material::StandardResource >()->getDefaultResource();
			}

			return m_resources.container< Material::PBRResource >()->getDefaultResource();
		};

		/* ⚠️ The node hierarchy is where a mesh gets its PLACE and its SIZE. Tydra pre-computes
		 * `global_matrix` for every node, so nothing has to be accumulated here — but ignoring
		 * it altogether leaves every mesh at the origin, at its authoring scale. Measured on
		 * Jungle Ruins, that turned the pyramid into a speck a few pixels across.
		 *
		 * A mesh referenced by several nodes is therefore built once per node: same geometry,
		 * different placement. */
		std::vector< std::pair< size_t, tinyusdz::value::matrix4d > > drawList;

		const auto collectNodes = [&drawList] (auto & self, const tinyusdz::tydra::Node & node) -> void {
			if ( node.nodeType == tinyusdz::tydra::NodeType::Mesh && node.id >= 0 )
			{
				drawList.emplace_back(static_cast< size_t >(node.id), node.global_matrix);
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

		TraceInfo{ClassId} << drawList.size() << " mesh placements collected from the node hierarchy.";

		size_t builtCount = 0;
		size_t meshIndex = 0;

		for ( const auto & [sourceMeshIndex, worldMatrix] : drawList )
		{
			if ( sourceMeshIndex >= renderScene.meshes.size() )
			{
				meshIndex++;

				continue;
			}

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

					/* ⚠️ The winding is swapped for the same reason GLTFLoader swaps it: the axis
					 * bake above sends the source's front faces to the back. Get this wrong and
					 * the scene renders inside-out — visible immediately, so it is verified on
					 * screen rather than argued about. */
					triangles.emplace_back(a, c, b);
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
				shape->computeTriangleNormal(true);
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

			/* One node per mesh for this pass: the render scene's own hierarchy is not walked
			 * yet, so every mesh sits at the stage origin with its baked coordinates. */
			NodeDescriptor node;
			node.name = resourceName;
			node.meshIndex = output.meshes.size() - 1;

			output.nodes.emplace_back(std::move(node));
			output.rootNodeIndices.push_back(output.nodes.size() - 1);

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
		tinyusdz::Layer layer;

		if ( !tinyusdz::LoadLayerFromFile(absoluteFilepath.string(), &layer, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to read layer '" << filepath.string() << "' : " << error;

			return false;
		}

		if ( !warning.empty() )
		{
			TraceWarning{ClassId} << "While reading '" << filepath.filename().string() << "' : " << warning;
			warning.clear();
		}

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

		std::vector< std::string > searchPaths{absoluteFilepath.parent_path().string()};

		{
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
		}

		resolver.set_search_paths(searchPaths);

		tinyusdz::Layer sublayered;

		if ( !tinyusdz::CompositeSublayers(resolver, layer, &sublayered, &warning, &error) )
		{
			TraceError{ClassId} << "Unable to composite sublayers of '" << filepath.filename().string() << "' : " << error;

			return false;
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

		USDLoader::reportInventory(filepath, stage);

		for ( const auto & prim : stage.root_prims() )
		{
			USDLoader::collectEnvironmentLights(prim, absoluteFilepath.parent_path(), output);
		}

		/* Tydra hands back renderer-ready data: triangulated faces, indexed vertices, resolved
		 * material bindings. Re-deriving that from raw prims would be duplicated work — the
		 * engine's own job starts at the translation into native scene logic. */
		tinyusdz::tydra::RenderSceneConverterEnv env{stage};
		env.usd_filename = absoluteFilepath.string();
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

		const auto materials = this->buildMaterials(renderScene, absoluteFilepath.parent_path());
		const auto builtCount = this->buildMeshes(renderScene, metersPerUnit, materials, output);

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

		return builtCount > 0;
	}
}
