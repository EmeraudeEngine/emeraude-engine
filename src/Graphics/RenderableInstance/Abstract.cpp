/*
 * src/Graphics/RenderableInstance/Abstract.cpp
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

#include "Abstract.hpp"

/* Local inclusions. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "PrimaryServices.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/Generator/SkinningLayoutHelper.hpp"
#include "Settings.hpp"
#include "Saphir/Generator/ShadowCasting.hpp"
#include "Saphir/Generator/TBNSpaceRendering.hpp"
#include "Saphir/Program.hpp"
#include "Scenes/Component/AbstractLightEmitter.hpp"
#include "Scenes/SceneInstanceTransforms.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;
	using namespace Saphir;
	using namespace Saphir::Keys;

	constexpr auto TracerTag{"RenderableInstance"};

	/* Rendered-frame cursor for the skinning per-frame upload (render thread only). */
	uint64_t Abstract::s_skinningFrameCursor{0};

	void
	Abstract::stageInstanceTransforms (Scenes::SceneInstanceTransforms & instanceTransforms, const CartesianFrame< float > * worldCoordinates, const Vector< 3, float > & cameraPosition, bool advanceHistory) noexcept
	{
		/* Prepare the model matrix (M).
		 * NOTE: Mirror of Unique::pushMatricesForRendering() — the staged matrix must be
		 * exactly what the push constant path would have computed for this instance. */
		Matrix< 4, float > modelMatrix;

		/* NOTE: If world coordinates are a nullptr, we assume to render the object at the origin. */
		if ( worldCoordinates != nullptr )
		{
			modelMatrix = m_renderable->isSprite() ?
				worldCoordinates->getSpriteModelMatrix(cameraPosition) :
				worldCoordinates->getModelMatrix();
		}

		if ( this->isFlagEnabled(ApplyTransformationMatrix) )
		{
			modelMatrix *= this->transformationMatrix();
		}

		/* Previous model matrix: the matrix staged at the previous rendered frame by the
		 * primary view. Before the first primary staging (or after a long culling gap, an
		 * accepted approximation), fall back to the current matrix — zero object velocity
		 * beats a bogus one on the first visible frame. */
		m_instanceTransformsSlot = instanceTransforms.stageEntry(modelMatrix, m_hasModelHistory ? m_lastModelMatrix : modelMatrix);

		/* NOTE: Only the primary view staging advances the history (once per rendered
		 * frame); render-to-texture stagings would otherwise zero the motion. */
		if ( advanceHistory )
		{
			m_lastModelMatrix = modelMatrix;
			m_hasModelHistory = true;
		}
	}

	bool
	Abstract::createSkinningResources (const std::shared_ptr< Device > & device, const std::shared_ptr< DescriptorSetLayout > & descriptorSetLayout, uint32_t boneCount, uint32_t sectionCount) noexcept
	{
		if ( boneCount == 0 || sectionCount == 0 || device == nullptr || descriptorSetLayout == nullptr )
		{
			return false;
		}

		/* NOTE: Interleaved {current, previous} bone matrices (stride 2) for the
		 * motion-vectors double skinning — twice the bind-pose-only size. */
		m_skinningPoseSize = static_cast< VkDeviceSize >(boneCount * 2UL * 16UL * sizeof(float));

		/* One section per frame in flight, each aligned for a descriptor buffer offset. */
		const auto minAlignment = device->physicalDevice()->propertiesVK10().limits.minStorageBufferOffsetAlignment;
		m_skinningSectionSize = minAlignment * Math::alignCount(m_skinningPoseSize, minAlignment);

		const auto bufferSize = m_skinningSectionSize * sectionCount;

		/* Create the SSBO (host-visible; the render thread writes ONE section per frame). */
		m_skinningSSBO = std::make_unique< ShaderStorageBufferObject >(device, bufferSize, true);
		m_skinningSSBO->setIdentifier(TracerTag, "SkinningMatrices", "SSBO");

		if ( !m_skinningSSBO->createOnHardware() )
		{
			Tracer::error(TracerTag, "Unable to create skinning SSBO !");

			m_skinningSSBO.reset();

			return false;
		}

		/* Create descriptor pool (one SSBO descriptor set per section, free-able). */
		m_skinningDescriptorPool = std::make_shared< DescriptorPool >(
			device,
			std::vector< VkDescriptorPoolSize >{{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sectionCount}},
			sectionCount,
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		);

		if ( !m_skinningDescriptorPool->createOnHardware() )
		{
			Tracer::error(TracerTag, "Unable to create skinning descriptor pool !");

			m_skinningSSBO.reset();
			m_skinningDescriptorPool.reset();

			return false;
		}

		/* Allocate one descriptor set per section, each on its fixed offset/range. The shader
		 * still sees bones[] from index 0 — the section split is invisible to the pipeline. */
		m_skinningDescriptorSets.reserve(sectionCount);

		for ( uint32_t section = 0; section < sectionCount; ++section )
		{
			auto descriptorSet = std::make_unique< DescriptorSet >(m_skinningDescriptorPool, descriptorSetLayout);

			const VkDescriptorBufferInfo bufferInfo{
				.buffer = m_skinningSSBO->handle(),
				.offset = m_skinningSectionSize * section,
				.range = m_skinningPoseSize
			};

			if ( !descriptorSet->create() || !descriptorSet->writeStorageBuffer(0, bufferInfo) )
			{
				Tracer::error(TracerTag, "Unable to allocate/write a skinning descriptor set !");

				m_skinningSSBO.reset();
				m_skinningDescriptorPool.reset();
				m_skinningDescriptorSets.clear();

				return false;
			}

			m_skinningDescriptorSets.emplace_back(std::move(descriptorSet));
		}

		/* Initialize EVERY section with identity matrices (current AND previous slots) so the
		 * mesh renders in bind pose with zero pose velocity until the first animation
		 * frame uploads real skinning matrices. */
		{
			std::vector< Matrix< 4, float > > identityMatrices(static_cast< size_t >(boneCount) * 2UL);

			for ( uint32_t section = 0; section < sectionCount; ++section )
			{
				m_skinningSSBO->writeData(MemoryRegion{
					identityMatrices.data(),
					identityMatrices.size() * sizeof(Matrix< 4, float >),
					static_cast< size_t >(m_skinningSectionSize * section)
				});
			}
		}

		return true;
	}

	bool
	Abstract::updateSkinningMatrices (const std::vector< Matrix< 4, float > > & matrices) noexcept
	{
		if ( m_skinningSSBO == nullptr || matrices.empty() )
		{
			return false;
		}

		const std::lock_guard< std::mutex > lock{m_skinningStagingMutex};

		/* First pose (or bone count change): previous == current, zero pose velocity. */
		if ( m_previousSkinningMatrices.size() != matrices.size() )
		{
			m_previousSkinningMatrices = matrices;
		}

		/* Interleave {current, previous} (stride 2) — matches the vertex shader layout
		 * (double skinning for the motion vectors). */
		m_skinningStaging.resize(matrices.size() * 2UL);

		for ( size_t index = 0; index < matrices.size(); ++index )
		{
			m_skinningStaging[(index * 2UL) + 0UL] = matrices[index];
			m_skinningStaging[(index * 2UL) + 1UL] = m_previousSkinningMatrices[index];
		}

		/* Archive the pose for the next update (one history step per logic update). */
		m_previousSkinningMatrices = matrices;

		/* NOTE: No GPU write here — the logic thread only STAGES. The render thread uploads
		 * once per rendered frame into the section of the frame being recorded
		 * (flushSkinningMatrices), so every pass of a frame skins with the same pose. */
		return true;
	}

	const DescriptorSet *
	Abstract::flushSkinningMatrices () const noexcept
	{
		const auto frameCursor = s_skinningFrameCursor;

		/* Already flushed for this frame: every subsequent pass (shadow, ambient, lights, TBN)
		 * binds the SAME section — that invariant is the fix for the pose desynchronization. */
		if ( m_skinningUploadedFrame == frameCursor )
		{
			return m_skinningDescriptorSets[m_skinningBoundSection].get();
		}

		const auto section = static_cast< uint32_t >(frameCursor % m_skinningDescriptorSets.size());

		{
			const std::lock_guard< std::mutex > lock{m_skinningStagingMutex};

			/* Empty staging (no animation played yet): the sections hold the identity poses
			 * written at creation, binding the section as-is is correct. */
			if ( !m_skinningStaging.empty() )
			{
				m_skinningSSBO->writeData(MemoryRegion{
					m_skinningStaging.data(),
					m_skinningStaging.size() * sizeof(Matrix< 4, float >),
					static_cast< size_t >(m_skinningSectionSize * section)
				});
			}
		}

		m_skinningBoundSection = section;
		m_skinningUploadedFrame = frameCursor;

		return m_skinningDescriptorSets[section].get();
	}

	bool
	Abstract::createRTSkinnedGeometryResources (Vulkan::AccelerationStructureBuilder & builder, const Geometry::Interface & geometry) noexcept
	{
		/* Idempotent: resources already exist. */
		if ( m_rtSkinnedBLAS != nullptr )
		{
			return true;
		}

		/* The compute pass reads the bone matrices SSBO: without skinning resources there
		 * is no pose to mirror (the bind-pose statue problem this path exists to solve). */
		if ( !this->hasSkinningResources() )
		{
			return false;
		}

		/* Skinned geometry is always a TriangleList (the TriangleStrip CPU-indices
		 * conversion path is not supported by the refit). */
		if ( geometry.topology() != Topology::TriangleList )
		{
			return false;
		}

		const auto * vbo = geometry.vertexBufferObject();

		if ( vbo == nullptr || !vbo->isCreated() )
		{
			return false;
		}

		const auto * ibo = geometry.indexBufferObject();
		const bool hasIndices = ibo != nullptr && ibo->isCreated();
		const auto device = vbo->device();

		/* Mirror buffer: full vertex layout copy (same stride and attribute offsets), so
		 * GPUMeshMetaData offsets stay valid and the trace shaders need no special case. */
		const auto floatsPerVertex = vbo->vertexElementCount();
		const auto mirrorSize = static_cast< VkDeviceSize >(vbo->vertexCount()) * floatsPerVertex * sizeof(float);

		m_rtSkinnedMirrorBuffer = std::make_unique< Vulkan::Buffer >(
			device, 0, mirrorSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			false
		);
		m_rtSkinnedMirrorBuffer->setIdentifier(TracerTag, "RTSkinnedMirror", "Buffer");

		if ( !m_rtSkinnedMirrorBuffer->createOnHardware() )
		{
			Tracer::error(TracerTag, "Unable to create the RT skinned mirror buffer !");

			m_rtSkinnedMirrorBuffer.reset();

			return false;
		}

		/* Refit inputs: same sub-geometry partition as the static BLAS path
		 * (Geometry::Interface::buildAccelerationStructure), vertex data = mirror. */
		Vulkan::BLASGeometryInput sharedHeader{};
		sharedHeader.vertexBuffer = m_rtSkinnedMirrorBuffer->handle();
		sharedHeader.vertexCount = vbo->vertexCount();
		sharedHeader.vertexStride = floatsPerVertex * static_cast< uint32_t >(sizeof(float));

		const auto totalIndexCount = hasIndices ? ibo->indexCount() : 0U;

		if ( hasIndices )
		{
			sharedHeader.indexBuffer = ibo->handle();
			sharedHeader.indexType = VK_INDEX_TYPE_UINT32;
		}

		m_rtRefitInputs.clear();

		if ( const auto subGeoCount = geometry.subGeometryCount(); subGeoCount <= 1 )
		{
			Vulkan::BLASGeometryInput single = sharedHeader;
			single.firstIndex = 0;
			single.indexCount = totalIndexCount;
			m_rtRefitInputs.emplace_back(single);
		}
		else
		{
			m_rtRefitInputs.reserve(subGeoCount);

			for ( uint32_t subGeoIndex = 0; subGeoIndex < subGeoCount; ++subGeoIndex )
			{
				const auto range = geometry.subGeometryRange(subGeoIndex); /* {firstIndex, indexCount} */
				Vulkan::BLASGeometryInput sub = sharedHeader;
				sub.firstIndex = range[0];
				sub.indexCount = range[1];
				m_rtRefitInputs.emplace_back(sub);
			}
		}

		/* Initial build from the SOURCE VBO: the bind pose is valid vertex data, while
		 * the mirror is garbage until the first compute dispatch — feeding garbage AABBs
		 * to the AS unit is exactly the class of fault behind Xid-style GPU stalls. */
		auto initialInputs = m_rtRefitInputs;

		for ( auto & input : initialInputs )
		{
			input.vertexBuffer = vbo->handle();
		}

		m_rtSkinnedBLAS = builder.buildBLAS(initialInputs, true);

		if ( m_rtSkinnedBLAS == nullptr )
		{
			Tracer::error(TracerTag, "Unable to build the refit-able BLAS for skinned geometry !");

			m_rtSkinnedMirrorBuffer.reset();
			m_rtRefitInputs.clear();

			return false;
		}

		/* Update scratch buffer (over-allocated for alignment, like the builder's own). */
		constexpr VkDeviceSize ScratchAlignment = 256;

		m_rtRefitScratchBuffer = std::make_unique< Vulkan::Buffer >(
			device, 0, m_rtSkinnedBLAS->updateScratchSize() + ScratchAlignment,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			false
		);
		m_rtRefitScratchBuffer->setIdentifier(TracerTag, "RTRefitScratch", "Buffer");

		if ( !m_rtRefitScratchBuffer->createOnHardware() )
		{
			Tracer::error(TracerTag, "Unable to create the BLAS refit scratch buffer !");

			m_rtSkinnedBLAS.reset();
			m_rtSkinnedMirrorBuffer.reset();
			m_rtRefitScratchBuffer.reset();
			m_rtRefitInputs.clear();

			return false;
		}

		m_rtRefitScratchAddress = (builder.getBufferDeviceAddress(m_rtRefitScratchBuffer->handle()) + ScratchAlignment - 1) & ~(ScratchAlignment - 1);

		/* Ready-made dispatch description. The influence vec4 and weight vec4 are always
		 * the LAST 8 floats of the vertex layout (see Geometry::getElementCountFromFlags). */
		m_rtSkinningPushConstants.srcAddress = builder.getBufferDeviceAddress(vbo->handle());
		m_rtSkinningPushConstants.dstAddress = builder.getBufferDeviceAddress(m_rtSkinnedMirrorBuffer->handle());
		m_rtSkinningPushConstants.vertexCount = vbo->vertexCount();
		m_rtSkinningPushConstants.floatsPerVertex = floatsPerVertex;
		m_rtSkinningPushConstants.tbnMode = geometry.tangentSpaceEnabled() ? 2U : (geometry.normalEnabled() ? 1U : 0U);
		m_rtSkinningPushConstants.influenceOffset = floatsPerVertex - 8U;

		TraceInfo{TracerTag} <<
			"RT skinned geometry resources created for '" << geometry.name() << "': " <<
			vbo->vertexCount() << " vertices, " << m_rtRefitInputs.size() << " sub-geometries, "
			"mirror " << mirrorSize << " bytes, update scratch " << m_rtSkinnedBLAS->updateScratchSize() << " bytes.";

		return true;
	}

	Renderable::ProgramCacheKey
	Abstract::buildProgramCacheKey (Renderable::ProgramType programType, RenderPassType renderPassType, uint64_t renderPassHandle, uint32_t layerIndex, bool isMDIEnabled) const noexcept
	{
		size_t materialLayoutHash = 0;
		bool isBindlessEnabled = false;

		if ( m_renderable != nullptr )
		{
			if ( const auto * material = m_renderable->material(layerIndex); material != nullptr )
			{
				if ( const auto layout = material->descriptorSetLayout(); layout != nullptr )
				{
					materialLayoutHash = layout->getHash();
				}

				/* Bindless is required by the automatic environment reflections AND by every
				 * lit program: the ambient pass reads the IBL reserved slots (irradiance,
				 * prefiltered environment, BRDF LUT). Keep this condition in sync with the
				 * generation site and the render-time set binding. */
				if ( programType == Renderable::ProgramType::Rendering && (material->useEnvironmentCubemap() || this->isLightingEnabled()) )
				{
					if ( material->serviceProvider().graphicsRenderer().bindlessTextureManager().usable() )
					{
						isBindlessEnabled = true;
					}
				}
			}
		}

		bool isSkeletalAnimationEnabled = false;

		if ( m_renderable != nullptr )
		{
			if ( const auto * skeletalData = dynamic_cast< const Renderable::SkeletalDataTrait * >(m_renderable.get()) )
			{
				isSkeletalAnimationEnabled = skeletalData->hasSkeletalData();
			}
		}

		return Renderable::ProgramCacheKey{
			.programType = programType,
			.renderPassType = renderPassType,
			.renderPassHandle = renderPassHandle,
			.layerIndex = layerIndex,
			.materialLayoutHash = materialLayoutHash,
			.isInstancing = this->useModelVertexBufferObject(),
			.isLightingEnabled = this->isLightingEnabled(),
			.isDepthTestDisabled = this->isDepthTestDisabled(),
			.isDepthWriteDisabled = this->isDepthWriteDisabled(),
			.isBindlessEnabled = isBindlessEnabled,
			.isMDIEnabled = isMDIEnabled,
			.isSkeletalAnimationEnabled = isSkeletalAnimationEnabled,
			.isInstanceMotionHistory = this->isFlagEnabled(EnableInstanceMotionHistory)
		};
	}

	bool
	Abstract::isReadyToCastShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		if ( m_renderable == nullptr || !m_renderable->isReadyForInstantiation() )
		{
			return false;
		}

		/* NOTE: The program cache lives on the RENDERABLE, shared by every instance of the same
		 * mesh, while the skinning descriptor sets live on the INSTANCE. Without this test, a
		 * second instance of a skeletal mesh would find the cached program, be declared ready,
		 * and never get the PerModel set the sealed pipeline layout demands. */
		if ( this->isMissingSkinningResources() )
		{
			return false;
		}

		/* Check if all shadow casting programs exist for all layers. */
		const auto layerCount = m_renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
			const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::ShadowCasting, RenderPassType::SimplePass, renderPassHandle, layerIndex);

			if ( m_renderable->findCachedProgram(renderTarget, cacheKey) == nullptr )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Abstract::isReadyToRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		if ( m_renderable == nullptr || !m_renderable->isReadyForInstantiation() )
		{
			return false;
		}

		/* NOTE: The program cache lives on the RENDERABLE, shared by every instance of the same
		 * mesh, while the skinning descriptor sets live on the INSTANCE. Without this test, a
		 * second instance of a skeletal mesh would find the cached program, be declared ready,
		 * and never get the PerModel set the sealed pipeline layout demands. */
		if ( this->isMissingSkinningResources() )
		{
			return false;
		}

		/* NOTE: Check if at least one rendering program exists for the CURRENT render pass.
		 * This is important because after a window resize, the render pass is recreated
		 * with a new handle, invalidating previously cached programs.
		 * Using the render pass handle ensures we don't falsely report readiness with stale programs. */
		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());

		/* For renderables with grab-pass layers rendered in the post-process pass,
		 * also check programs cached for the post-process render pass. */
		const auto * postProcessFB = renderTarget->postProcessFramebuffer();

		if ( postProcessFB != nullptr )
		{
			const auto postProcessRPHandle = reinterpret_cast< uint64_t >(postProcessFB->renderPass()->handle());

			return m_renderable->hasAnyCachedProgramsForRenderPass(renderTarget, renderPassHandle)
				|| m_renderable->hasAnyCachedProgramsForRenderPass(renderTarget, postProcessRPHandle);
		}

		return m_renderable->hasAnyCachedProgramsForRenderPass(renderTarget, renderPassHandle);
	}

	bool
	Abstract::isMissingSkinningResources () const noexcept
	{
		if ( this->hasSkinningResources() )
		{
			return false;
		}

		const auto * skeletalData = dynamic_cast< const Renderable::SkeletalDataTrait * >(m_renderable.get());

		if ( skeletalData == nullptr || !skeletalData->hasSkeletalData() )
		{
			return false;
		}

		return skeletalData->skin().jointCount() > 0;
	}

	bool
	Abstract::prepareSkinningResources (Renderer & renderer) noexcept
	{
		if ( !this->isMissingSkinningResources() )
		{
			return true;
		}

		const auto * skeletalData = dynamic_cast< const Renderable::SkeletalDataTrait * >(m_renderable.get());
		const auto boneCount = static_cast< uint32_t >(skeletalData->skin().jointCount());
		const auto descriptorSetLayout = Generator::getSkinningDescriptorSetLayout(renderer.layoutManager());

		if ( !this->createSkinningResources(renderer.device(), descriptorSetLayout, boneCount, renderer.framesInFlight()) )
		{
			this->setBroken("Unable to create the skinning resources of a skeletal renderable instance !");

			return false;
		}

		return true;
	}

	bool
	Abstract::getReadyForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept
	{
		if ( m_renderable == nullptr )
		{
			return false;
		}

		/* NOTE: The underlying renderable resource is not loaded yet. This is NOT an error:
		 * returning true simply defers the instance. The per-frame render-list build
		 * (Scenes::Scene::checkRenderableInstanceForRendering) re-invokes this method on
		 * subsequent frames until the renderable becomes ready. There is NO event that
		 * relaunches program generation: the resource LoadFinished event only refreshes
		 * entity-level properties (AABB, mass), and Vulkan program/pipeline creation must
		 * run on the render thread, never on the resource loader thread. */
		if ( !m_renderable->isReadyForInstantiation() )
		{
			return true;
		}

		/* Skinning GPU resources, created BEFORE the first program generation (see the method).
		 * The shadow map of a skeletal mesh is generated from the same skinned pose: this entry
		 * point can perfectly run before getReadyForRender(). */
		if ( !this->prepareSkinningResources(renderer) )
		{
			return false;
		}

		const auto layerCount = m_renderable->layerCount();

		if constexpr ( IsDebug )
		{
			if ( layerCount == 0 )
			{
				std::stringstream errorMessage;
				errorMessage <<
					"The renderable interface has no layer ! It must have at least one. "
					"Unable to setup the renderable instance '" << m_renderable->name() << "' for shadow casting.";

				return false;
			}
		}

		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::ShadowCasting, RenderPassType::SimplePass, renderPassHandle, layerIndex);

			/* Try to find a cached program from the Renderable. */
			if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
			{
				continue;
			}

			/* Generate a new program. */
			Generator::ShadowCasting generator{renderTarget, this->shared_from_this(), layerIndex};

			if ( !generator.generateShaderProgram(renderer) )
			{
				return false;
			}

			/* Cache the program on the Renderable for future instances. */
			m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
		}

		return true;
	}

	bool
	Abstract::getReadyForRender (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const StaticVector< RenderPassType, MaxPassCount > & renderPassTypes, Renderer & renderer) noexcept
	{
		if ( m_renderable == nullptr )
		{
			this->setBroken("The renderable instance has no renderable associated !");

			return false;
		}

		/* NOTE: The underlying renderable resource is not loaded yet. This is NOT an error:
		 * returning true simply defers the instance. The per-frame render-list build
		 * (Scenes::Scene::checkRenderableInstanceForRendering) re-invokes this method on
		 * subsequent frames until the renderable becomes ready. There is NO event that
		 * relaunches program generation: the resource LoadFinished event only refreshes
		 * entity-level properties (AABB, mass), and Vulkan program/pipeline creation must
		 * run on the render thread, never on the resource loader thread. */
		if ( !m_renderable->isReadyForInstantiation() )
		{
			return true;
		}

		/* Skinning GPU resources, created BEFORE the first program generation (see the method). */
		if ( !this->prepareSkinningResources(renderer) )
		{
			return false;
		}

		const auto layerCount = m_renderable->layerCount();

		/* NOTE: These tests only exist in debug mode because they are already performed beyond
		 * isReadyForInstantiation(). */
		if constexpr ( IsDebug )
		{
			if ( layerCount == 0 )
			{
				std::stringstream errorMessage;

				errorMessage <<
					"The renderable interface has no layer ! It must have at least one. "
					"Unable to setup the renderable instance '" << m_renderable->name() << "' for rendering.";

				this->setBroken(errorMessage.str());

				return false;
			}

			/* NOTE: The geometry interface is the same for every layer of the renderable interface. */
			if ( const auto * geometry = m_renderable->geometry(0); geometry == nullptr )
			{
				std::stringstream errorMessage;

				errorMessage <<
					"The renderable interface has no geometry interface ! "
					"Unable to setup the renderable instance '" << m_renderable->name() << "' for rendering.";

				this->setBroken(errorMessage.str());

				return false;
			}
		}

		const auto mainRenderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());

		/* If the render target provides a post-process framebuffer (e.g. SwapChain),
		 * grab-pass layers must create pipelines matching that single-sample render pass. */
		const auto * postProcessFB = renderTarget->postProcessFramebuffer();
		const auto postProcessRPHandle = postProcessFB != nullptr
			? reinterpret_cast< uint64_t >(postProcessFB->renderPass()->handle())
			: mainRenderPassHandle;

		for ( const auto renderPassType : renderPassTypes )
		{
			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				/* Select the correct render pass based on whether this layer uses grab pass. */
				const bool isGrabPassLayer = postProcessFB != nullptr && m_renderable->requiresGrabPass(layerIndex);
				const auto renderPassHandle = isGrabPassLayer ? postProcessRPHandle : mainRenderPassHandle;

				const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, renderPassType, renderPassHandle, layerIndex);

				/* Try to find a cached program from the Renderable. */
				if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
				{
					continue;
				}

				/* Generate a new program. */
				std::stringstream shaderProgramName;
				shaderProgramName << "RenderableInstance" << to_string(renderPassType);

				Generator::SceneRendering generator{shaderProgramName.str(), renderTarget, this->shared_from_this(), layerIndex, scene, renderPassType, renderer.primaryServices().settings()};

				/* For grab-pass layers, override the pipeline framebuffer to the post-process
				 * single-sample framebuffer so the pipeline sample count matches. */
				if ( isGrabPassLayer )
				{
					generator.setPipelineFramebuffer(postProcessFB);
				}

				/* Enable bindless textures flag if:
				 * 1. The material uses automatic reflection, OR the instance is lit (the
				 *	ambient pass reads the IBL reserved slots — irradiance, prefiltered,
				 *	BRDF LUT). Keep in sync with buildProgramCacheKey and the set binding.
				 * 2. The bindless textures manager is initialized and available */
				if ( const auto * material = m_renderable->material(layerIndex); material != nullptr && (material->useEnvironmentCubemap() || this->isLightingEnabled()) && renderer.bindlessTextureManager().usable() )
				{
					generator.enableBindlessTextures(true);
				}

				if ( !generator.generateShaderProgram(renderer) )
				{
					const auto * material = m_renderable->material(layerIndex);

					std::stringstream errorMessage;
					errorMessage <<
						"Unable to generate the shader program !\n"
						"  Renderable  : " << m_renderable->name() << "\n"
						"  Material	: " << (material != nullptr ? material->name() : "null") << "\n"
						"  RenderTarget: " << to_string(renderTarget->renderType()) << " (" << renderTarget->extent().width << "x" << renderTarget->extent().height << ")\n"
						"  RenderPass  : " << to_string(renderPassType) << "\n"
						"  Layer	   : " << layerIndex;

					this->setBroken(errorMessage.str());

					return false;
				}

				/* Cache the program on the Renderable for future instances. */
				m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
			}
		}

		if ( this->isDisplayTBNSpaceEnabled() )
		{
			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::TBNSpace, RenderPassType::SimplePass, mainRenderPassHandle, layerIndex);

				/* Try to find a cached program from the Renderable. */
				if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
				{
					continue;
				}

				/* Generate a new program. */
				Generator::TBNSpaceRendering generator{renderTarget, this->shared_from_this(), layerIndex};

				if ( !generator.generateShaderProgram(renderer) )
				{
					Tracer::error(TracerTag, "Unable to generate the TBN space program !");

					continue;
				}

				/* Cache the program on the Renderable for future instances. */
				m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
			}
		}

		return true;
	}

	bool
	Abstract::getReadyForTBNSpace (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept
	{
		if ( m_renderable == nullptr || !m_renderable->isReadyForInstantiation() )
		{
			return false;
		}

		const auto layerCount = m_renderable->layerCount();
		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::TBNSpace, RenderPassType::SimplePass, renderPassHandle, layerIndex);

			/* Try to find a cached program from the Renderable. */
			if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
			{
				continue;
			}

			/* Generate a new program. */
			Generator::TBNSpaceRendering generator{renderTarget, this->shared_from_this(), layerIndex};

			if ( !generator.generateShaderProgram(renderer) )
			{
				Tracer::error(TracerTag, "Unable to generate the TBN space program !");

				return false;
			}

			/* Cache the program on the Renderable for future instances. */
			m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
		}

		return true;
	}

	bool
	Abstract::getReadyForMDI (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept
	{
		if ( m_renderable == nullptr || !m_renderable->isReadyForInstantiation() )
		{
			return true;
		}

		const auto layerCount = m_renderable->layerCount();
		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, RenderPassType::SimplePass, renderPassHandle, layerIndex, true);

			if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
			{
				continue;
			}

			std::stringstream shaderProgramName;
			shaderProgramName << "RenderableInstanceMDI" << to_string(RenderPassType::SimplePass);

			Generator::SceneRendering generator{shaderProgramName.str(), renderTarget, this->shared_from_this(), layerIndex, scene, RenderPassType::SimplePass, renderer.primaryServices().settings()};

			generator.enableMultiDrawIndirect(true);

			if ( !generator.generateShaderProgram(renderer) )
			{
				TraceWarning{TracerTag} << "Unable to generate MDI shader program for renderable '" << m_renderable->name() << "' layer " << layerIndex;

				return false;
			}

			m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
		}

		return true;
	}

	std::shared_ptr< Saphir::Program >
	Abstract::resolveMDIProgram (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex) const noexcept
	{
		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, RenderPassType::SimplePass, renderPassHandle, layerIndex, true);

		return this->resolveProgram(renderTarget, cacheKey);
	}

	void
	Abstract::setBroken (const std::string & errorMessage, const std::source_location & location) noexcept
	{
		this->enableFlag(BrokenState);

		Tracer::error(TracerTag, errorMessage, location);
	}

	std::shared_ptr< Saphir::Program >
	Abstract::resolveProgram (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Renderable::ProgramCacheKey & cacheKey) const noexcept
	{
		const auto renderTargetId = reinterpret_cast< uint64_t >(renderTarget.get());

		/* Fast path: linear scan on the small instance-local cache. */
		for ( const auto & entry : m_resolvedPrograms )
		{
			if ( entry.renderTargetId == renderTargetId &&
				entry.renderPassHandle == cacheKey.renderPassHandle &&
				entry.programType == cacheKey.programType &&
				entry.renderPassType == cacheKey.renderPassType &&
				entry.layerIndex == cacheKey.layerIndex )
			{
				return entry.program;
			}
		}

		/* Slow path: fall back to the Renderable's hashtable cache. */
		auto program = m_renderable->findCachedProgram(renderTarget, cacheKey);

		if ( program != nullptr && !m_resolvedPrograms.full() )
		{
			m_resolvedPrograms.emplace_back(ResolvedProgram{
				.renderTargetId = renderTargetId,
				.renderPassHandle = cacheKey.renderPassHandle,
				.programType = cacheKey.programType,
				.renderPassType = cacheKey.renderPassType,
				.layerIndex = cacheKey.layerIndex,
				.program = program
			});
		}

		return program;
	}

	void
	Abstract::traceMissingDescriptorSet (const char * setName, const RenderTarget::Abstract & renderTarget) const noexcept
	{
		if ( m_missingDescriptorSetReported )
		{
			return;
		}

		m_missingDescriptorSetReported = true;

		TraceError{TracerTag} <<
			"Descriptor set contract violation: the sealed pipeline layout declares the '" << setName << "' set, "
			"but the renderable instance cannot provide it. The draw call is skipped.\n"
			"  Renderable  : " << (m_renderable != nullptr ? m_renderable->name() : "null") << "\n"
			"  RenderTarget: " << to_string(renderTarget.renderType()) << " (" << renderTarget.extent().width << "x" << renderTarget.extent().height << ")\n"
			"  NOTE        : the generation-time condition (Saphir::Generator) and the binding-time "
			"condition of that set have diverged. Fix the pair, never the binding alone.";
	}

	void
	Abstract::castShadows (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer, uint32_t LODLevel) const noexcept
	{
		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::ShadowCasting, RenderPassType::SimplePass, renderPassHandle, layerIndex);
		const auto program = this->resolveProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			TraceError{TracerTag} <<
				"There is no suitable shadow program for the renderable instance !\n"
				"  Renderable  : " << m_renderable->name() << "\n"
				"  RenderTarget: " << to_string(renderTarget->renderType()) << " (" << renderTarget->extent().width << "x" << renderTarget->extent().height << ")\n"
				"  Layer	   : " << layerIndex << "\n"
				"  CacheKey	: ProgramType=ShadowCasting, RPHandle=" << renderPassHandle << "\n"
				"  BrokenState : " << (this->isBroken() ? "YES (shader generation failed earlier)" : "no");

			return;
		}

		const auto pipelineLayout = program->pipelineLayout();

		commandBuffer.bind(*program->graphicsPipeline());

		/* NOTE: Set the dynamic viewport and scissor. */
		renderTarget->setViewport(commandBuffer);

		/* NOTE: Every set goes to the index DECLARED by the sealed pipeline layout, never to a
		 * running counter — a set left unbound can then no longer shift the ones after it.
		 * See the "Descriptor set binding" contract in src/Saphir/AGENTS.md. */
		const auto & setIndexes = program->setIndexes();

		/* NOTE: The view UBO is part of the layout when:
		 * - Renderable instance uses GPU instancing (needs view matrix from UBO)
		 * - OR render target is a cubemap (multiview needs 6 view matrices from UBO indexed by gl_ViewIndex)
		 * - OR render target is a CSM (multiview needs N cascade view matrices from UBO indexed by gl_ViewIndex) */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerView) )
		{
			commandBuffer.bind(*renderTarget->viewMatrices().descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerView));
		}

		/* Bind skinning SSBO (PerModel set) for skeletal meshes. */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerModel) )
		{
			if ( !this->hasSkinningResources() )
			{
				this->traceMissingDescriptorSet("PerModel (skinning)", *renderTarget);

				return;
			}

			/* Upload the staged pose ONCE per frame; every pass then binds the same section. */
			commandBuffer.bind(*this->flushSkinningMatrices(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerModel));
		}

		/* Bind material descriptor set for alpha-tested shadows. */
		const auto * material = m_renderable->material(layerIndex);

		if ( setIndexes.isSetEnabled(Saphir::SetType::PerModelLayer) )
		{
			if ( material == nullptr )
			{
				this->traceMissingDescriptorSet("PerModelLayer (material)", *renderTarget);

				return;
			}

			commandBuffer.bind(*material->descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerModelLayer));
		}

		this->bindInstanceModelLayer(commandBuffer, layerIndex, LODLevel);

		/* Build render pass context (created once per pass, reused for all objects). */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap(),
			.isCSM = renderTarget->isCascadedShadowMap()
		};

		/* Build push constant context (pre-computed values for this program). */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled()
		};

		this->pushMatricesForShadowCasting(passContext, pushContext, worldCoordinates);

		/* Draw with correct frame index for animated materials. */
		if ( material != nullptr && material->isAnimated() )
		{
			commandBuffer.draw(*m_renderable->geometry(LODLevel), m_frameIndex, this->instanceCount());
		}
		else if ( m_renderable->layerCount() == 1 )
		{
			commandBuffer.draw(*m_renderable->geometry(LODLevel), this->instanceCount());
		}
		else
		{
			commandBuffer.draw(*m_renderable->geometry(LODLevel), layerIndex, this->instanceCount());
		}
	}

	void
	Abstract::render (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer, uint32_t LODLevel, const BindlessTextureManager * bindlessTexturesManager, const DescriptorSet * sceneTransformsDS) const noexcept
	{
		/* For grab-pass layers on render targets with a post-process framebuffer,
		 * use the post-process render pass handle (pipelines were created for it). */
		const auto * postProcessFB = renderTarget->postProcessFramebuffer();
		const bool isGrabPassLayer = postProcessFB != nullptr && m_renderable->requiresGrabPass(layerIndex);
		const auto renderPassHandle = isGrabPassLayer
			? reinterpret_cast< uint64_t >(postProcessFB->renderPass()->handle())
			: reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, renderPassType, renderPassHandle, layerIndex);
		const auto program = this->resolveProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			const auto * material = m_renderable->material(layerIndex);

			TraceWarning{TracerTag} <<
				"There is no suitable render program for the renderable instance (Maybe in loading stage)!\n"
				"  Renderable  : " << m_renderable->name() << "\n"
				"  Material	: " << (material != nullptr ? material->name() : "null") << "\n"
				"  RenderTarget: " << to_string(renderTarget->renderType()) << " (" << renderTarget->extent().width << "x" << renderTarget->extent().height << ")\n"
				"  RenderPass  : " << to_string(renderPassType) << "\n"
				"  Layer	   : " << layerIndex << "\n"
				"  CacheKey	: ProgramType=Rendering, RPHandle=" << renderPassHandle << ", Lighting=" << (cacheKey.isLightingEnabled ? "yes" : "no") << ", Bindless=" << (cacheKey.isBindlessEnabled ? "yes" : "no") << "\n"
				"  BrokenState : " << (this->isBroken() ? "YES (shader generation failed earlier)" : "no");

			return;
		}

		const auto * geometry = m_renderable->geometry(LODLevel);
		const auto pipelineLayout = program->pipelineLayout();

		/* Bind the graphics pipeline. */
		commandBuffer.bind(*program->graphicsPipeline());

		/* NOTE: Set the dynamic viewport and scissor. */
		renderTarget->setViewport(commandBuffer);

		/* Bind a renderable instance VBO / IBO. */
		this->bindInstanceModelLayer(commandBuffer, layerIndex, LODLevel);

		/* Build render pass context (created once per pass, reused for all objects). */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap()
		};

		/* Build push constant context (pre-computed values for this program). */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled(),
			.useInstanceTransforms = program->wasInstanceTransformsEnabled()
		};

		/* Configure the push constants. */
		this->pushMatricesForRendering(passContext, pushContext, worldCoordinates);

		/* NOTE: Every set goes to the index DECLARED by the sealed pipeline layout, never to a
		 * running counter — a set left unbound can then no longer shift the ones after it.
		 * See the "Descriptor set binding" contract in src/Saphir/AGENTS.md. */
		const auto & setIndexes = program->setIndexes();

		/* Bind view UBO. */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerView) )
		{
			commandBuffer.bind(*renderTarget->viewMatrices().descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerView));
		}

		/* Bind the scene instance transforms SSBO set.
		 * NOTE: Driven by the SEALED pipeline layout (setIndexes), not by the shader flag —
		 * the set may be part of the layout yet unreferenced by the shader (advanced
		 * fallback). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerSceneTransforms) )
		{
			if ( sceneTransformsDS == nullptr )
			{
				this->traceMissingDescriptorSet("PerSceneTransforms", *renderTarget);

				return;
			}

			commandBuffer.bind(*sceneTransformsDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerSceneTransforms));
		}

		/* Bind light UBO (and shadow map sampler if applicable). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerLight) )
		{
			if ( lightEmitter == nullptr || !lightEmitter->isCreated() )
			{
				this->traceMissingDescriptorSet("PerLight", *renderTarget);

				return;
			}

			const bool useShadowMap = renderPassUsesShadowMap(renderPassType);

			commandBuffer.bind(*lightEmitter->descriptorSet(useShadowMap), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerLight), lightEmitter->UBOOffset());
		}

		/* Bind skinning SSBO (PerModel set) for skeletal meshes. */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerModel) )
		{
			if ( !this->hasSkinningResources() )
			{
				this->traceMissingDescriptorSet("PerModel (skinning)", *renderTarget);

				return;
			}

			/* Upload the staged pose ONCE per frame; every pass then binds the same section. */
			commandBuffer.bind(*this->flushSkinningMatrices(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerModel));
		}

		/* Bind material UBO and samplers. */
		const auto * material = m_renderable->material(layerIndex);

		if ( setIndexes.isSetEnabled(Saphir::SetType::PerModelLayer) )
		{
			if ( material == nullptr )
			{
				this->traceMissingDescriptorSet("PerModelLayer (material)", *renderTarget);

				return;
			}

			commandBuffer.bind(*material->descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerModelLayer));
		}

		/* Bind the bindless textures descriptor set. The layout declares it when the material
		 * uses automatic reflection, the instance is lit (ambient-pass IBL reserved slots), or
		 * the light uses bindless color projection — see the program-generation gating
		 * (buildProgramCacheKey / getReadyForRender). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerBindless) )
		{
			if ( bindlessTexturesManager == nullptr || bindlessTexturesManager->descriptorSet() == nullptr )
			{
				this->traceMissingDescriptorSet("PerBindless", *renderTarget);

				return;
			}

			commandBuffer.bind(*bindlessTexturesManager->descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerBindless));
		}

		/* NOTE: The InstanceTransforms slot travels through the firstInstance draw
		 * parameter (read as gl_InstanceIndex in the vertex shader, instanceCount == 1). */
		const uint32_t firstInstance = pushContext.useInstanceTransforms ? this->instanceTransformsSlot() : 0;

		/* Check for adaptive LOD rendering. */
		if ( geometry->isAdaptiveLOD() )
		{
			const auto & viewPosition = renderTarget->viewMatrices().position();

			/* Prepare LODs and stitching for this frame. */
			geometry->prepareAdaptiveRendering(viewPosition);

			/* Draw all sectors at their computed LOD level. */
			const auto drawCallCount = geometry->getAdaptiveDrawCallCount(viewPosition);

			for ( uint32_t drawCallIndex = 0; drawCallIndex < drawCallCount; ++drawCallIndex )
			{
				const auto range = geometry->getAdaptiveDrawCallRange(drawCallIndex, viewPosition);

				commandBuffer.drawIndexed(range[0], range[1], this->instanceCount(), firstInstance);
			}

			/* Draw stitching geometry between LOD zones. */
			const auto stitchingCount = geometry->getStitchingDrawCallCount();

			for ( uint32_t stitchIndex = 0; stitchIndex < stitchingCount; ++stitchIndex )
			{
				const auto range = geometry->getStitchingDrawCallRange(stitchIndex);

				commandBuffer.drawIndexed(range[0], range[1], this->instanceCount(), firstInstance);
			}
		}
		else if ( material->isAnimated() )
		{
			commandBuffer.drawWithFirstInstance(*geometry, firstInstance, m_frameIndex, this->instanceCount());
		}
		else if ( m_renderable->layerCount() == 1 )
		{
			commandBuffer.drawWithFirstInstance(*geometry, firstInstance, this->instanceCount());
		}
		else
		{
			commandBuffer.drawWithFirstInstance(*geometry, firstInstance, layerIndex, this->instanceCount());
		}
	}

	void
	Abstract::render (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer, RenderStateTracker & tracker, uint32_t LODLevel, const BindlessTextureManager * bindlessTexturesManager, const DescriptorSet * sceneTransformsDS) const noexcept
	{
		/* For grab-pass layers on render targets with a post-process framebuffer,
		 * use the post-process render pass handle (pipelines were created for it). */
		const auto * postProcessFB = renderTarget->postProcessFramebuffer();
		const bool isGrabPassLayer = postProcessFB != nullptr && m_renderable->requiresGrabPass(layerIndex);
		const auto renderPassHandle = isGrabPassLayer
			? reinterpret_cast< uint64_t >(postProcessFB->renderPass()->handle())
			: reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, renderPassType, renderPassHandle, layerIndex);
		const auto program = this->resolveProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			const auto * material = m_renderable->material(layerIndex);

			TraceWarning{TracerTag} <<
				"There is no suitable render program for the renderable instance (Maybe in loading stage)!\n"
				"  Renderable  : " << m_renderable->name() << "\n"
				"  Material	: " << (material != nullptr ? material->name() : "null") << "\n"
				"  RenderTarget: " << to_string(renderTarget->renderType()) << " (" << renderTarget->extent().width << "x" << renderTarget->extent().height << ")\n"
				"  RenderPass  : " << to_string(renderPassType) << "\n"
				"  Layer	   : " << layerIndex << "\n"
				"  CacheKey	: ProgramType=Rendering, RPHandle=" << renderPassHandle << ", Lighting=" << (cacheKey.isLightingEnabled ? "yes" : "no") << ", Bindless=" << (cacheKey.isBindlessEnabled ? "yes" : "no") << "\n"
				"  BrokenState : " << (this->isBroken() ? "YES (shader generation failed earlier)" : "no");

			return;
		}

		const auto * geometry = m_renderable->geometry(LODLevel);
		const auto pipelineLayout = program->pipelineLayout();
		const auto pipelineHandle = program->graphicsPipeline()->handle();

		/* Bind the graphics pipeline only if it changed. */
		if ( tracker.lastPipeline != pipelineHandle )
		{
			commandBuffer.bind(*program->graphicsPipeline());
			tracker.lastPipeline = pipelineHandle;

			/* Pipeline change invalidates descriptor set compatibility. */
			tracker.invalidateDescriptorSets();
		}
#ifdef DEBUG
		else
		{
			tracker.savedPipelineBinds++;
		}
#endif

		/* Set the dynamic viewport and scissor only once per pass. */
		if ( !tracker.viewportSet )
		{
			renderTarget->setViewport(commandBuffer);
			tracker.viewportSet = true;
		}
#ifdef DEBUG
		else
		{
			tracker.savedViewportSets++;
		}
#endif

		/* Bind geometry (VBO/IBO) only if it changed. */
		if ( tracker.lastGeometry != static_cast< const void * >(geometry) || tracker.lastLayerIndex != layerIndex )
		{
			this->bindInstanceModelLayer(commandBuffer, layerIndex, LODLevel);
			tracker.lastGeometry = geometry;
			tracker.lastLayerIndex = layerIndex;
		}
#ifdef DEBUG
		else
		{
			tracker.savedGeometryBinds++;
		}
#endif

		/* Build render pass context. */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap()
		};

		/* Build push constant context. */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled(),
			.useInstanceTransforms = program->wasInstanceTransformsEnabled()
		};

		/* Push constants are always required (unique model matrix per object). */
		this->pushMatricesForRendering(passContext, pushContext, worldCoordinates);

		/* NOTE: Every set goes to the index DECLARED by the sealed pipeline layout, never to a
		 * running counter — a set left unbound can then no longer shift the ones after it.
		 * See the "Descriptor set binding" contract in src/Saphir/AGENTS.md. */
		const auto & setIndexes = program->setIndexes();

		/* Bind view UBO (skip if already bound for this pipeline). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerView) )
		{
			const auto * viewDS = renderTarget->viewMatrices().descriptorSet();

			if ( tracker.lastViewDS != viewDS->handle() )
			{
				commandBuffer.bind(*viewDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerView));
				tracker.lastViewDS = viewDS->handle();
			}
		}

		/* Bind the scene instance transforms SSBO set (skip if already bound).
		 * NOTE: Driven by the SEALED pipeline layout (setIndexes), not by the shader flag —
		 * the set may be part of the layout yet unreferenced by the shader (advanced
		 * fallback). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerSceneTransforms) )
		{
			if ( sceneTransformsDS == nullptr )
			{
				this->traceMissingDescriptorSet("PerSceneTransforms", *renderTarget);

				return;
			}

			if ( tracker.lastSceneTransformsDS != sceneTransformsDS->handle() )
			{
				commandBuffer.bind(*sceneTransformsDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerSceneTransforms));
				tracker.lastSceneTransformsDS = sceneTransformsDS->handle();
			}
		}

		/* Bind light UBO (and shadow map sampler if applicable). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerLight) )
		{
			if ( lightEmitter == nullptr || !lightEmitter->isCreated() )
			{
				this->traceMissingDescriptorSet("PerLight", *renderTarget);

				return;
			}

			const bool useShadowMap = renderPassUsesShadowMap(renderPassType);
			const auto * lightDS = lightEmitter->descriptorSet(useShadowMap);
			const auto lightUBOOffset = lightEmitter->UBOOffset();

			/* NOTE: Every light of a scene shares ONE descriptor set (shared UBO) — only
			 * the dynamic offset tells them apart. The redundancy check MUST include the
			 * offset: deduplicating on the handle alone made every light pass reuse the
			 * first light's data (a single light lit the whole scene). */
			if ( tracker.lastLightDS != lightDS->handle() || tracker.lastLightUBOOffset != lightUBOOffset )
			{
				commandBuffer.bind(*lightDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerLight), lightUBOOffset);
				tracker.lastLightDS = lightDS->handle();
				tracker.lastLightUBOOffset = lightUBOOffset;
			}
		}

		/* Bind skinning SSBO (PerModel set) for skeletal meshes. */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerModel) )
		{
			if ( !this->hasSkinningResources() )
			{
				this->traceMissingDescriptorSet("PerModel (skinning)", *renderTarget);

				return;
			}

			/* Upload the staged pose ONCE per frame; every pass then binds the same section. */
			commandBuffer.bind(*this->flushSkinningMatrices(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerModel));
		}

		/* Bind material UBO and samplers (skip if already bound). */
		const auto * material = m_renderable->material(layerIndex);

		if ( setIndexes.isSetEnabled(Saphir::SetType::PerModelLayer) )
		{
			if ( material == nullptr )
			{
				this->traceMissingDescriptorSet("PerModelLayer (material)", *renderTarget);

				return;
			}

			const auto * materialDS = material->descriptorSet();

			if ( tracker.lastMaterialDS != materialDS->handle() )
			{
				commandBuffer.bind(*materialDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerModelLayer));
				tracker.lastMaterialDS = materialDS->handle();
			}
#ifdef DEBUG
			else
			{
				tracker.savedMaterialBinds++;
			}
#endif
		}

		/* Bind the bindless textures descriptor set. The layout declares it when the material
		 * uses automatic reflection, the instance is lit (ambient-pass IBL reserved slots), or
		 * the light uses bindless color projection — see the program-generation gating
		 * (buildProgramCacheKey / getReadyForRender). */
		if ( setIndexes.isSetEnabled(Saphir::SetType::PerBindless) )
		{
			if ( bindlessTexturesManager == nullptr || bindlessTexturesManager->descriptorSet() == nullptr )
			{
				this->traceMissingDescriptorSet("PerBindless", *renderTarget);

				return;
			}

			const auto * bindlessDS = bindlessTexturesManager->descriptorSet();

			if ( tracker.lastBindlessDS != bindlessDS->handle() )
			{
				commandBuffer.bind(*bindlessDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setIndexes.set(Saphir::SetType::PerBindless));
				tracker.lastBindlessDS = bindlessDS->handle();
			}
		}

#ifdef DEBUG
		tracker.totalDrawCalls++;
#endif

		/* NOTE: The InstanceTransforms slot travels through the firstInstance draw
		 * parameter (read as gl_InstanceIndex in the vertex shader, instanceCount == 1). */
		const uint32_t firstInstance = pushContext.useInstanceTransforms ? this->instanceTransformsSlot() : 0;

		/* Issue the draw command (same logic as non-tracked render). */
		if ( geometry->isAdaptiveLOD() )
		{
			const auto & viewPosition = renderTarget->viewMatrices().position();

			geometry->prepareAdaptiveRendering(viewPosition);

			const auto drawCallCount = geometry->getAdaptiveDrawCallCount(viewPosition);

			for ( uint32_t drawCallIndex = 0; drawCallIndex < drawCallCount; ++drawCallIndex )
			{
				const auto range = geometry->getAdaptiveDrawCallRange(drawCallIndex, viewPosition);

				commandBuffer.drawIndexed(range[0], range[1], this->instanceCount(), firstInstance);
			}

			const auto stitchingCount = geometry->getStitchingDrawCallCount();

			for ( uint32_t stitchIndex = 0; stitchIndex < stitchingCount; ++stitchIndex )
			{
				const auto range = geometry->getStitchingDrawCallRange(stitchIndex);

				commandBuffer.drawIndexed(range[0], range[1], this->instanceCount(), firstInstance);
			}
		}
		else if ( material->isAnimated() )
		{
			commandBuffer.drawWithFirstInstance(*geometry, firstInstance, m_frameIndex, this->instanceCount());
		}
		else if ( m_renderable->layerCount() == 1 )
		{
			commandBuffer.drawWithFirstInstance(*geometry, firstInstance, this->instanceCount());
		}
		else
		{
			commandBuffer.drawWithFirstInstance(*geometry, firstInstance, layerIndex, this->instanceCount());
		}
	}

	void
	Abstract::renderTBNSpace (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer) const noexcept
	{
		const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::TBNSpace, RenderPassType::SimplePass, renderPassHandle, layerIndex);
		const auto program = this->resolveProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable TBN space program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto pipelineLayout = program->pipelineLayout();

		commandBuffer.bind(*program->graphicsPipeline());

		/* NOTE: Set the dynamic viewport and scissor. */
		renderTarget->setViewport(commandBuffer);

		/* NOTE: Bind the view UBO if renderable instance uses GPU instancing. */
		if ( this->useModelVertexBufferObject() )
		{
			commandBuffer.bind(*renderTarget->viewMatrices().descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
		}

		this->bindInstanceModelLayer(commandBuffer, layerIndex, 0);

		/* Build render pass context (created once per pass, reused for all objects). */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap()
		};

		/* Build push constant context (pre-computed values for this program). */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled()
		};

		this->pushMatricesForRendering(passContext, pushContext, worldCoordinates);

		/* TBN debug always uses LOD 0 (full detail). */
		if ( m_renderable->layerCount() == 1 )
		{
			commandBuffer.draw(*m_renderable->geometry(0), this->instanceCount());
		}
		else
		{
			commandBuffer.draw(*m_renderable->geometry(0), layerIndex, this->instanceCount());
		}
	}
}
