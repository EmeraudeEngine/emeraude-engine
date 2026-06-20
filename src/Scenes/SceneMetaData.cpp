/*
 * src/Scenes/SceneMetaData.cpp
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

#include "SceneMetaData.hpp"

/* STL inclusions. */
#include <unordered_map>

/* Local inclusions. */
#include "GPUMeshMetaData.hpp"
#include "BindlessTextureSet.hpp"
#include "Graphics/Geometry/Helpers.hpp"
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Material/GPURTMaterialData.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Tracer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/IndexBufferObject.hpp"
#include "Vulkan/VertexBufferObject.hpp"

namespace EmEn::Scenes
{
	using namespace Graphics;
	using namespace Vulkan;

	SceneMetaData::SceneMetaData (const std::shared_ptr< Device > & device, AccelerationStructureBuilder * accelerationStructureBuilder) noexcept
		: m_device(device),
		m_accelerationStructureBuilder(accelerationStructureBuilder)
	{
		/* NOTE: The acceleration structure builder is owned by the Renderer (single shared
		 * instance) and is null when RT is unavailable/disabled. SceneMetaData only borrows it for
		 * TLAS building and buffer device addresses; it does NOT own it and must not destroy it. */
	}

	SceneMetaData::~SceneMetaData ()
	{
		m_meshMetaDataSSBOs.clear();
		m_materialDataSSBOs.clear();
		m_TLAS.reset();
		m_retiredRequests.clear();
	}

	bool
	SceneMetaData::initializePerFrameBuffers (uint32_t frameCount) noexcept
	{
		if ( m_accelerationStructureBuilder == nullptr )
		{
			return false;
		}

		const auto & device = m_device;

		m_meshMetaDataSSBOs.resize(frameCount);
		m_materialDataSSBOs.resize(frameCount);

		for ( uint32_t i = 0; i < frameCount; ++i )
		{
			m_meshMetaDataSSBOs[i] = std::make_unique< ShaderStorageBufferObject >(device, MaxRTInstances * sizeof(GPUMeshMetaData));
			m_materialDataSSBOs[i] = std::make_unique< ShaderStorageBufferObject >(device, MaxRTMaterials * sizeof(Material::GPURTMaterialData));

			if ( !m_meshMetaDataSSBOs[i]->createOnHardware() || !m_materialDataSSBOs[i]->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create RT SSBOs for frame-in-flight, mesh metadata and material data will be unavailable.");

				m_meshMetaDataSSBOs.clear();
				m_materialDataSSBOs.clear();

				return false;
			}
		}

		return true;
	}

	void
	SceneMetaData::rebuild (const RenderBatch::List & opaqueList, const RenderBatch::List & opaqueLightedList, BindlessTextureSet * bindlessTextureSet, uint32_t frameIndex, const Base::Math::Vector< 3, float > & cameraPosition) noexcept
	{
		if ( m_accelerationStructureBuilder == nullptr )
		{
			return;
		}

		/* Reuse persistent collection structures (clear but keep allocated memory). */
		m_rebuildInstances.clear();
		m_rebuildMeshEntries.clear();
		m_rebuildMaterialMap.clear();
		m_rebuildMaterialEntries.clear();

		auto & instances = m_rebuildInstances;
		auto & meshEntries = m_rebuildMeshEntries;
		auto & materialMap = m_rebuildMaterialMap;
		auto & materialEntries = m_rebuildMaterialEntries;

		/* Compute vertex attribute byte offsets from geometry flags.
		 * Layout order: Position(3) → TangentSpace(9) or Normal(3) → PrimaryUV(2/3) → ... */
		const auto computeNormalByteOffset = [] (const Geometry::Interface * geometry) -> uint32_t {
			/* Position = 3 floats. */
			uint32_t offset = 3;

			/* When tangent space is enabled, layout is Position(3) → Tangent(3) → Bitangent(3) → Normal(3).
			 * Skip Tangent + Bitangent to reach Normal. */
			if ( geometry->tangentSpaceEnabled() )
			{
				offset += 6;
			}

			return offset * static_cast< uint32_t >(sizeof(float));
		};

		const auto computeUVByteOffset = [] (const Geometry::Interface * geometry) -> uint32_t {
			/* Position = 3 floats. */
			uint32_t offset = 3;

			/* TangentSpace (9 floats) overrides Normal (3 floats). */
			if ( geometry->tangentSpaceEnabled() )
			{
				offset += 9;
			}
			else if ( geometry->normalEnabled() )
			{
				offset += 3;
			}

			return offset * static_cast< uint32_t >(sizeof(float));
		};

		/* Collect instances from a render list. */
		const auto collectFromList = [&] (const RenderBatch::List & renderList) {
			for ( const auto & batch : renderList | std::views::values )
			{
				const auto * renderable = batch.renderableInstance()->renderable();

				if ( renderable == nullptr )
				{
					continue;
				}

				const auto * geometry = renderable->geometry(0);

				if ( geometry == nullptr )
				{
					continue;
				}

				auto * blas = geometry->accelerationStructure();

				/* Build BLAS on-demand for geometries loaded before the RT builder was set. */
				if ( blas == nullptr )
				{
					const_cast< Geometry::Interface * >(geometry)->buildAccelerationStructure();
					blas = geometry->accelerationStructure();

					if ( blas == nullptr )
					{
						continue;
					}
				}

				/* Enforce instance limit. */
				if ( meshEntries.size() >= MaxRTInstances )
				{
					break;
				}

				/* --- Per-sub-geometry material lookup ---
				 * For each of the renderable's layers (capped at MaxSubGeometriesPerMesh),
				 * resolve its material index in the RT material SSBO. The RT trace shader
				 * uses materialIndices[rayQueryGetIntersectionGeometryIndexEXT(...)] to
				 * pick the right material at hit time. Without this, multi-layer meshes
				 * (e.g. palm trunk + alpha-test leaves) would alias their materials.
				 *
				 * Also tracks whether ANY sub-geometry is alpha-test, which triggers the
				 * FORCE_NO_OPAQUE TLAS instance flag (so the ray query receives candidate
				 * hits and the shader can sample the opacity texture). */
				const auto subGeoCount = std::min(renderable->subGeometryCount(), MaxSubGeometriesPerMesh);
				bool anyAlphaTest = false;

				/* --- Mesh metadata --- */
				const auto instanceIndex = static_cast< uint32_t >(meshEntries.size());

				GPUMeshMetaData meshMeta{};

				if ( const auto * VBO = geometry->vertexBufferObject(); VBO != nullptr )
				{
					meshMeta.vertexBufferAddress = m_accelerationStructureBuilder->getBufferDeviceAddress(VBO->handle());
					meshMeta.vertexStride = VBO->vertexElementCount() * static_cast< uint32_t >(sizeof(float));
				}

				/* Use RT-specific triangle-list IBO when available (e.g. TriangleStrip converted),
				 * otherwise fall back to the native IBO. */
				const auto * RayTracingIBO = geometry->rtIndexBufferObject();

				if ( const auto * IBO = RayTracingIBO != nullptr ? RayTracingIBO : geometry->indexBufferObject(); IBO != nullptr )
				{
					meshMeta.indexBufferAddress = m_accelerationStructureBuilder->getBufferDeviceAddress(IBO->handle());
				}

				meshMeta.normalByteOffset = computeNormalByteOffset(geometry);
				meshMeta.primaryUVByteOffset = computeUVByteOffset(geometry);
				meshMeta.subGeometryCount = subGeoCount;

				for ( uint32_t subGeo = 0; subGeo < subGeoCount; ++subGeo )
				{
					const auto * subMaterial = renderable->material(subGeo);
					uint32_t subMaterialIndex = 0;

					if ( subMaterial != nullptr )
					{
						if ( auto it = materialMap.find(subMaterial); it != materialMap.end() )
						{
							subMaterialIndex = it->second;
						}
						else if ( materialEntries.size() < MaxRTMaterials )
						{
							subMaterialIndex = static_cast< uint32_t >(materialEntries.size());
							materialMap.emplace(subMaterial, subMaterialIndex);

							Material::GPURTMaterialData rtMat{};
							subMaterial->exportRTMaterialData(rtMat);
							materialEntries.emplace_back(rtMat);
						}

						if ( subMaterial->isAlphaTest() )
						{
							anyAlphaTest = true;
						}
					}

					meshMeta.materialIndices[subGeo] = subMaterialIndex;
				}

				meshEntries.emplace_back(meshMeta);

				/* --- TLAS instance --- */
				VkTransformMatrixKHR transform{};

				{
					/* Build the TLAS instance transform by combining the entity's world
					 * coordinates (position + rotation) with the renderable instance's
					 * transformation matrix (which contains the uniform scale from
					 * setUniformScale / setTransformationMatrix).
					 *
					 * Without this, the BLAS geometry remains at its raw object-space size
					 * in the acceleration structure, causing RT effects to trace against
					 * un-scaled geometry while the rasterizer renders the scaled version. */
					auto finalMatrix = batch.renderableInstance()->transformationMatrix();

					if ( const auto * worldCoordinates = batch.worldCoordinates(); worldCoordinates != nullptr )
					{
						finalMatrix = worldCoordinates->getModelMatrix() * finalMatrix;
					}

					const auto * m = finalMatrix.data();

					/* Column-major 4x4 → Row-major 3x4 transpose. */
					transform.matrix[0][0] = m[0]; transform.matrix[0][1] = m[4]; transform.matrix[0][2] = m[8];  transform.matrix[0][3] = m[12];
					transform.matrix[1][0] = m[1]; transform.matrix[1][1] = m[5]; transform.matrix[1][2] = m[9];  transform.matrix[1][3] = m[13];
					transform.matrix[2][0] = m[2]; transform.matrix[2][1] = m[6]; transform.matrix[2][2] = m[10]; transform.matrix[2][3] = m[14];
				}

				TLASInstanceInput instance{};
				instance.blasDeviceAddress = blas->deviceAddress();
				instance.transform = transform;
				instance.instanceCustomIndex = instanceIndex;
				instance.mask = 0xFF;
				instance.shaderBindingTableRecordOffset = 0;
				instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

				/* Force this instance non-opaque when ANY of its sub-geometry materials
				 * uses alpha-test, so the ray query receives candidate intersections
				 * (instead of auto-confirmed). The shader then samples the opacity
				 * texture per hit, and the alpha-test per-material flag determines
				 * whether to actually run the test or auto-accept. Materials without
				 * alpha-test on any sub-geometry keep the BLAS default opacity. */
				if ( anyAlphaTest )
				{
					instance.flags |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
				}

				/* Sprite billboard rotation (CPU-side, per frame): the sprite's BLAS quad
				 * is a flat XY plane in object space — the rasterizer billboards it via
				 * vertex shader, but the RT pipeline only sees the BLAS geometry, so we
				 * must bake the face-camera rotation into the TLAS instance transform.
				 * Cylindrical billboard: only rotates around the vertical (Y) axis so the
				 * sprite stays upright regardless of camera elevation. Uniform scale and
				 * translation are preserved from the original transform. */
				if ( renderable->isSprite() )
				{
					auto & rm = instance.transform.matrix;

					/* Recover uniform scale from the first column's magnitude — sprites
					 * are always uniformly scaled via SpriteResource's UniformScale. */
					const float scale = std::sqrt(rm[0][0] * rm[0][0] + rm[1][0] * rm[1][0] + rm[2][0] * rm[2][0]);

					const float dx = cameraPosition[0] - rm[0][3];
					const float dz = cameraPosition[2] - rm[2][3];
					const float horizLen = std::sqrt(dx * dx + dz * dz);

					float fx;
					float fz;

					if ( horizLen > 0.0001F )
					{
						fx = dx / horizLen;
						fz = dz / horizLen;
					}
					else
					{
						/* Camera directly above/below the sprite — fall back to a default
						 * facing so the rotation matrix stays well-defined. */
						fx = 0.0F;
						fz = 1.0F;
					}

					/* RHS basis: col 1 (local +Y) preserved as world up axis,
					 * col 2 (local +Z, the sprite's normal) = forward_horiz,
					 * col 0 (local +X) = col 1 × col 2 = (fz, 0, -fx). */
					rm[0][0] = scale * fz;   rm[0][1] = 0.0F;	rm[0][2] = scale * fx;
					rm[1][0] = 0.0F;		  rm[1][1] = scale;	rm[1][2] = 0.0F;
					rm[2][0] = -scale * fx;  rm[2][1] = 0.0F;	rm[2][2] = scale * fz;
					/* Translation (column 3) intentionally preserved. */
				}

				instances.emplace_back(instance);
			}
		};

		collectFromList(opaqueList);
		collectFromList(opaqueLightedList);


		/* --- Resolve bindless texture indices for RT materials --- */
		if ( bindlessTextureSet != nullptr && !materialMap.empty() )
		{
			/* Reuse persistent set (clear but keep allocated memory). */
			m_rebuildActiveTextures.clear();
			auto & activeTextures = m_rebuildActiveTextures;

			for ( auto & [material, matIndex] : materialMap )
			{
				std::vector< Material::RTTextureSlot > slots;
				material->collectRTTextures(slots);

				auto & rtMat = materialEntries[matIndex];

				for ( const auto & [role, texture] : slots )
				{
					if ( texture == nullptr )
					{
						continue;
					}

					const auto * texturePtr = texture.get();
					activeTextures.insert(texturePtr);

					/* Look up or register in the bindless texture manager. */
					uint32_t bindlessIndex;

					if ( auto cacheIt = m_textureRegistrationCache.find(texturePtr); cacheIt != m_textureRegistrationCache.end() )
					{
						bindlessIndex = cacheIt->second;
					}
					else
					{
						bindlessIndex = bindlessTextureSet->registerTexture2D(texture);

						if ( bindlessIndex == UINT32_MAX )
						{
							continue;
						}

						m_textureRegistrationCache.emplace(texturePtr, bindlessIndex);
					}

					/* Fill the appropriate index and flag in the RT material. */
					switch ( role )
					{
						case Material::RTTextureRole::Albedo :
							rtMat.albedoTextureIndex = static_cast< int32_t >(bindlessIndex);
							rtMat.flags |= Material::GPURTMaterialData::HasAlbedoTexture;
							break;

						case Material::RTTextureRole::Normal :
							rtMat.normalTextureIndex = static_cast< int32_t >(bindlessIndex);
							rtMat.flags |= Material::GPURTMaterialData::HasNormalTexture;
							break;

						case Material::RTTextureRole::Roughness :
							rtMat.roughnessTextureIndex = static_cast< int32_t >(bindlessIndex);
							rtMat.flags |= Material::GPURTMaterialData::HasRoughnessTexture;
							break;

						case Material::RTTextureRole::Metalness :
							rtMat.metalnessTextureIndex = static_cast< int32_t >(bindlessIndex);
							rtMat.flags |= Material::GPURTMaterialData::HasMetalnessTexture;
							break;

						case Material::RTTextureRole::Emission :
							rtMat.emissionTextureIndex = static_cast< int32_t >(bindlessIndex);
							rtMat.flags |= Material::GPURTMaterialData::HasEmissionTexture;
							break;

						case Material::RTTextureRole::Opacity :
							rtMat.opacityTextureIndex = static_cast< int32_t >(bindlessIndex);
							rtMat.flags |= Material::GPURTMaterialData::HasOpacityTexture;
							break;
					}
				}
			}

			/* Unregister textures that are no longer in use. */
			for ( auto it = m_textureRegistrationCache.begin(); it != m_textureRegistrationCache.end(); )
			{
				if ( !activeTextures.contains(it->first) )
				{
					bindlessTextureSet->unregisterTexture2D(it->first);
					it = m_textureRegistrationCache.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		if ( instances.empty() )
		{
			m_pendingTLASBuild.reset();
			m_TLAS.reset();
			m_retiredRequests.clear();
			m_instanceCount = 0;
			m_materialCount = 0;

			return;
		}

		/* --- Upload mesh metadata SSBO for the current frame --- */
		if ( frameIndex < m_meshMetaDataSSBOs.size() && m_meshMetaDataSSBOs[frameIndex] != nullptr && !meshEntries.empty() )
		{
			if ( auto * dst = m_meshMetaDataSSBOs[frameIndex]->mapMemoryAs< GPUMeshMetaData >(); dst != nullptr )
			{
				std::memcpy(dst, meshEntries.data(), meshEntries.size() * sizeof(GPUMeshMetaData));
				m_meshMetaDataSSBOs[frameIndex]->unmapMemory();
			}
		}

		/* --- Upload material data SSBO for the current frame --- */
		if ( frameIndex < m_materialDataSSBOs.size() && m_materialDataSSBOs[frameIndex] != nullptr && !materialEntries.empty() )
		{
			if ( auto * dst = m_materialDataSSBOs[frameIndex]->mapMemoryAs< Material::GPURTMaterialData >(); dst != nullptr )
			{
				std::memcpy(dst, materialEntries.data(), materialEntries.size() * sizeof(Material::GPURTMaterialData));
				m_materialDataSSBOs[frameIndex]->unmapMemory();
			}
		}

		m_instanceCount = meshEntries.size();
		m_materialCount = materialEntries.size();

		/* Keep at most 3 retired requests (covers typical 2-3 frames-in-flight).
		 * Each request owns instance/scratch buffers still referenced by in-flight command buffers. */
		while ( m_retiredRequests.size() > 3 )
		{
			m_retiredRequests.pop_front();
		}

		/* Prepare the TLAS build (CPU-side only). The GPU build commands will be
		 * recorded into the main render command buffer by recordTLASBuild(). */
		m_pendingTLASBuild = m_accelerationStructureBuilder->prepareTLAS(instances);
	}

	void
	SceneMetaData::recordTLASBuild (VkCommandBuffer cmdBuf) noexcept
	{
		if ( m_pendingTLASBuild == nullptr || m_accelerationStructureBuilder == nullptr )
		{
			return;
		}

		m_accelerationStructureBuilder->recordTLASBuild(cmdBuf, *m_pendingTLASBuild);

		/* Swap the old TLAS into the retiring request so it stays alive alongside
		 * the instance/scratch buffers while in-flight command buffers reference them. */
		auto oldTLAS = std::move(m_TLAS);
		m_TLAS = std::move(m_pendingTLASBuild->tlas);
		m_pendingTLASBuild->tlas = std::move(oldTLAS);
		m_retiredRequests.emplace_back(std::move(m_pendingTLASBuild));
	}
}
