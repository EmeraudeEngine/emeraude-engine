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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "SceneMetaData.hpp"

/* STL inclusions. */
#include <unordered_map>
#include <unordered_set>

/* Local inclusions. */
#include "GPUMeshMetaData.hpp"
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Geometry/Helpers.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Material/GPURTMaterialData.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Vulkan/VertexBufferObject.hpp"
#include "Vulkan/IndexBufferObject.hpp"
#include "Vulkan/Device.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Graphics;
	using namespace Vulkan;

	SceneMetaData::SceneMetaData (const std::shared_ptr< Device > & device, bool enableRayTracing) noexcept
		: m_device(device)
	{
		if ( !enableRayTracing || !device->rayTracingEnabled() )
		{
			return;
		}

		m_accelerationStructureBuilder = std::make_unique< AccelerationStructureBuilder >(device);

		if ( m_accelerationStructureBuilder->initialize() )
		{
			Geometry::Interface::setAccelerationStructureBuilder(m_accelerationStructureBuilder.get());
		}
		else
		{
			Tracer::error(ClassId, "Failed to initialize acceleration structure builder, RT disabled for this scene.");

			m_accelerationStructureBuilder.reset();
		}
	}

	SceneMetaData::~SceneMetaData ()
	{
		/* Reset RT static builder pointer before destroying it. */
		if ( m_accelerationStructureBuilder != nullptr )
		{
			Geometry::Interface::setAccelerationStructureBuilder(nullptr);
		}

		m_meshMetaDataSSBOs.clear();
		m_materialDataSSBOs.clear();
		m_TLAS.reset();
		m_retiredTLAS.clear();
		m_accelerationStructureBuilder.reset();
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
	SceneMetaData::rebuild (const RenderBatch::List & opaqueList, const RenderBatch::List & opaqueLightedList, Graphics::BindlessTextureManager * bindlessTextureManager, uint32_t frameIndex) noexcept
	{
		if ( m_accelerationStructureBuilder == nullptr )
		{
			return;
		}

		/* Temporary per-frame collection structures. */
		std::vector< TLASInstanceInput > instances;
		std::vector< GPUMeshMetaData > meshEntries;
		std::unordered_map< const Material::Interface *, uint32_t > materialMap;
		std::vector< Material::GPURTMaterialData > materialEntries;

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
			for ( const auto & [key, batch] : renderList )
			{
				const auto * renderable = batch.renderableInstance()->renderable();

				if ( renderable == nullptr )
				{
					continue;
				}

				const auto * geometry = renderable->geometry();

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

				/* --- Material deduplication --- */
				const auto * material = renderable->material(batch.subGeometryIndex());
				uint32_t materialIndex = 0;

				if ( material != nullptr )
				{
					auto it = materialMap.find(material);

					if ( it != materialMap.end() )
					{
						materialIndex = it->second;
					}
					else if ( materialEntries.size() < MaxRTMaterials )
					{
						materialIndex = static_cast< uint32_t >(materialEntries.size());
						materialMap.emplace(material, materialIndex);

						Material::GPURTMaterialData rtMat{};
						material->exportRTMaterialData(rtMat);
						materialEntries.emplace_back(rtMat);
					}
				}

				/* --- Mesh metadata --- */
				const auto instanceIndex = static_cast< uint32_t >(meshEntries.size());

				GPUMeshMetaData meshMeta{};

				const auto * vbo = geometry->vertexBufferObject();

				if ( vbo != nullptr )
				{
					meshMeta.vertexBufferAddress = m_accelerationStructureBuilder->getBufferDeviceAddress(vbo->handle());
					meshMeta.vertexStride = vbo->vertexElementCount() * static_cast< uint32_t >(sizeof(float));
				}

				/* Use RT-specific triangle-list IBO when available (e.g. TriangleStrip converted),
				 * otherwise fall back to the native IBO. */
				const auto * rtIBO = geometry->rtIndexBufferObject();
				const auto * ibo = (rtIBO != nullptr) ? rtIBO : geometry->indexBufferObject();

				if ( ibo != nullptr )
				{
					meshMeta.indexBufferAddress = m_accelerationStructureBuilder->getBufferDeviceAddress(ibo->handle());
				}

				meshMeta.normalByteOffset = computeNormalByteOffset(geometry);
				meshMeta.primaryUVByteOffset = computeUVByteOffset(geometry);
				meshMeta.materialIndex = materialIndex;

				meshEntries.emplace_back(meshMeta);

				/* --- TLAS instance --- */
				VkTransformMatrixKHR transform{};
				const auto * worldCoordinates = batch.worldCoordinates();

				if ( worldCoordinates != nullptr )
				{
					const auto modelMatrix = worldCoordinates->getModelMatrix();
					const auto * m = modelMatrix.data();

					/* Column-major 4x4 → Row-major 3x4 transpose. */
					transform.matrix[0][0] = m[0]; transform.matrix[0][1] = m[4]; transform.matrix[0][2] = m[8];  transform.matrix[0][3] = m[12];
					transform.matrix[1][0] = m[1]; transform.matrix[1][1] = m[5]; transform.matrix[1][2] = m[9];  transform.matrix[1][3] = m[13];
					transform.matrix[2][0] = m[2]; transform.matrix[2][1] = m[6]; transform.matrix[2][2] = m[10]; transform.matrix[2][3] = m[14];
				}
				else
				{
					/* Identity transform. */
					transform.matrix[0][0] = 1.0F;
					transform.matrix[1][1] = 1.0F;
					transform.matrix[2][2] = 1.0F;
				}

				TLASInstanceInput instance{};
				instance.blasDeviceAddress = blas->deviceAddress();
				instance.transform = transform;
				instance.instanceCustomIndex = instanceIndex;
				instance.mask = 0xFF;
				instance.shaderBindingTableRecordOffset = 0;
				instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

				instances.emplace_back(instance);
			}
		};

		collectFromList(opaqueList);
		collectFromList(opaqueLightedList);


		/* --- Resolve bindless texture indices for RT materials --- */
		if ( bindlessTextureManager != nullptr && !materialMap.empty() )
		{
			/* Track which cached textures are still in use this frame. */
			std::unordered_set< const Vulkan::TextureInterface * > activeTextures;

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

					const auto * texPtr = texture.get();
					activeTextures.insert(texPtr);

					/* Look up or register in the bindless texture manager. */
					uint32_t bindlessIndex;
					auto cacheIt = m_textureRegistrationCache.find(texPtr);

					if ( cacheIt != m_textureRegistrationCache.end() )
					{
						bindlessIndex = cacheIt->second;
					}
					else
					{
						bindlessIndex = bindlessTextureManager->registerTexture2D(*texture);

						if ( bindlessIndex == UINT32_MAX )
						{
							continue;
						}

						m_textureRegistrationCache.emplace(texPtr, bindlessIndex);
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
					}
				}
			}

			/* Unregister textures that are no longer in use. */
			for ( auto it = m_textureRegistrationCache.begin(); it != m_textureRegistrationCache.end(); )
			{
				if ( activeTextures.find(it->first) == activeTextures.end() )
				{
					bindlessTextureManager->unregisterTexture2D(it->second);
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
			m_TLAS.reset();
			m_retiredTLAS.clear();
			m_instanceCount = 0;
			m_materialCount = 0;

			return;
		}

		/* --- Upload mesh metadata SSBO for the current frame --- */
		if ( frameIndex < m_meshMetaDataSSBOs.size() && m_meshMetaDataSSBOs[frameIndex] != nullptr && !meshEntries.empty() )
		{
			auto * dst = m_meshMetaDataSSBOs[frameIndex]->mapMemoryAs< GPUMeshMetaData >();

			if ( dst != nullptr )
			{
				std::memcpy(dst, meshEntries.data(), meshEntries.size() * sizeof(GPUMeshMetaData));
				m_meshMetaDataSSBOs[frameIndex]->unmapMemory();
			}
		}

		/* --- Upload material data SSBO for the current frame --- */
		if ( frameIndex < m_materialDataSSBOs.size() && m_materialDataSSBOs[frameIndex] != nullptr && !materialEntries.empty() )
		{
			auto * dst = m_materialDataSSBOs[frameIndex]->mapMemoryAs< Material::GPURTMaterialData >();

			if ( dst != nullptr )
			{
				std::memcpy(dst, materialEntries.data(), materialEntries.size() * sizeof(Material::GPURTMaterialData));
				m_materialDataSSBOs[frameIndex]->unmapMemory();
			}
		}

		m_instanceCount = meshEntries.size();
		m_materialCount = materialEntries.size();

		/* --- Retire current TLAS and build new one --- */
		if ( m_TLAS != nullptr )
		{
			m_retiredTLAS.emplace_back(std::move(m_TLAS));
		}

		/* Keep at most 3 retired TLAS (covers typical 2-3 frames-in-flight). */
		while ( m_retiredTLAS.size() > 3 )
		{
			m_retiredTLAS.erase(m_retiredTLAS.begin());
		}

		m_TLAS = m_accelerationStructureBuilder->buildTLAS(instances);
	}
}
