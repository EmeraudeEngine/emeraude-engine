/*
 * src/Graphics/SkinnedGeometryProcessor.cpp
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

#include "SkinnedGeometryProcessor.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/Generator/SkinningLayoutHelper.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

namespace
{
	using namespace EmEn;

	/* Skins one vertex per invocation into the mirror buffer (same layout as the source
	 * VBO). The skinning math MUST stay in sync with Saphir::VertexShader (interleaved
	 * {current, previous} bone matrices, stride 2 — the compute pass uses the CURRENT
	 * slots only: the BLAS refit needs this frame's pose, velocity is a raster concern). */
	constexpr auto SkinningMirrorComputeShader = R"GLSL(
#version 450
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require

layout(local_size_x = 64) in;

layout(buffer_reference, scalar, buffer_reference_align = 4) readonly buffer SourceVertices { float v[]; };
layout(buffer_reference, scalar, buffer_reference_align = 4) writeonly buffer MirrorVertices { float v[]; };

/* Same set as the raster skinning (per-frame section bound by the caller). */
layout(set = 0, binding = 0, std430) readonly buffer SkinningMatrices { mat4 bones[]; };

layout(push_constant, scalar) uniform PushConstants
{
	uvec2 srcAddress;
	uvec2 dstAddress;
	uint vertexCount;
	uint floatsPerVertex;
	uint tbnMode; /* 0 = none, 1 = normal at floats 3-5, 2 = TBN at floats 3-11 (T, B, N). */
	uint influenceOffset; /* Float offset of the influence vec4; weights follow at +4. */
} pc;

void main ()
{
	const uint vertexIndex = gl_GlobalInvocationID.x;

	if ( vertexIndex >= pc.vertexCount )
	{
		return;
	}

	SourceVertices src = SourceVertices(pc.srcAddress);
	MirrorVertices dst = MirrorVertices(pc.dstAddress);

	const uint base = vertexIndex * pc.floatsPerVertex;

	/* Copy the whole vertex first (UVs, colors, influences, weights stay verbatim). */
	for ( uint i = 0u; i < pc.floatsPerVertex; ++i )
	{
		dst.v[base + i] = src.v[base + i];
	}

	/* Interleaved {current, previous}: even slots are the current pose. */
	const ivec4 boneIdx = ivec4(
		int(src.v[base + pc.influenceOffset + 0u]),
		int(src.v[base + pc.influenceOffset + 1u]),
		int(src.v[base + pc.influenceOffset + 2u]),
		int(src.v[base + pc.influenceOffset + 3u])) * 2;

	const vec4 weights = vec4(
		src.v[base + pc.influenceOffset + 4u],
		src.v[base + pc.influenceOffset + 5u],
		src.v[base + pc.influenceOffset + 6u],
		src.v[base + pc.influenceOffset + 7u]);

	const mat4 skinMatrix =
		weights.x * bones[boneIdx.x] +
		weights.y * bones[boneIdx.y] +
		weights.z * bones[boneIdx.z] +
		weights.w * bones[boneIdx.w];

	const vec3 position = vec3(src.v[base + 0u], src.v[base + 1u], src.v[base + 2u]);
	const vec3 skinnedPosition = (skinMatrix * vec4(position, 1.0)).xyz;

	dst.v[base + 0u] = skinnedPosition.x;
	dst.v[base + 1u] = skinnedPosition.y;
	dst.v[base + 2u] = skinnedPosition.z;

	if ( pc.tbnMode == 0u )
	{
		return;
	}

	const mat3 skinMatrix3 = mat3(skinMatrix);

	if ( pc.tbnMode == 1u )
	{
		const vec3 skinnedNormal = normalize(skinMatrix3 * vec3(src.v[base + 3u], src.v[base + 4u], src.v[base + 5u]));

		dst.v[base + 3u] = skinnedNormal.x;
		dst.v[base + 4u] = skinnedNormal.y;
		dst.v[base + 5u] = skinnedNormal.z;
	}
	else
	{
		for ( uint vector = 0u; vector < 3u; ++vector )
		{
			const uint offset = base + 3u + (vector * 3u);
			const vec3 skinned = normalize(skinMatrix3 * vec3(src.v[offset + 0u], src.v[offset + 1u], src.v[offset + 2u]));

			dst.v[offset + 0u] = skinned.x;
			dst.v[offset + 1u] = skinned.y;
			dst.v[offset + 2u] = skinned.z;
		}
	}
}
)GLSL";
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	SkinnedGeometryProcessor::~SkinnedGeometryProcessor () noexcept = default;

	bool
	SkinnedGeometryProcessor::initialize (Renderer & renderer) noexcept
	{
		const auto & device = renderer.device();

		const auto skinningSetLayout = Generator::getSkinningDescriptorSetLayout(renderer.layoutManager());

		if ( skinningSetLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the skinning descriptor set layout !");

			return false;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants);

		m_pipelineLayout = std::make_shared< PipelineLayout >(
			device, "SkinnedGeometryProcessorPipelineLayout",
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{skinningSetLayout},
			StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
		);

		if ( !m_pipelineLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the pipeline layout !");

			m_pipelineLayout.reset();

			return false;
		}

		const auto shaderModule = renderer.shaderManager().getShaderModuleFromSourceCode(device, "SkinningMirror_CS", ShaderType::ComputeShader, SkinningMirrorComputeShader);

		if ( shaderModule == nullptr )
		{
			Tracer::error(ClassId, "Failed to compile the skinning mirror compute shader !");

			m_pipelineLayout.reset();

			return false;
		}

		m_pipeline = std::make_unique< ComputePipeline >(m_pipelineLayout);
		m_pipeline->setShaderModule(shaderModule->handle());

		if ( !m_pipeline->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the compute pipeline !");

			m_pipeline.reset();
			m_pipelineLayout.reset();

			return false;
		}

		return true;
	}

	void
	SkinnedGeometryProcessor::recordDispatch (VkCommandBuffer cmdBuf, const PushConstants & pushConstants, VkDescriptorSet bonesDescriptorSet) const noexcept
	{
		if ( m_pipeline == nullptr || pushConstants.vertexCount == 0 || bonesDescriptorSet == VK_NULL_HANDLE )
		{
			return;
		}

		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->handle());
		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout->handle(), 0, 1, &bonesDescriptorSet, 0, nullptr);
		vkCmdPushConstants(cmdBuf, m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);
		vkCmdDispatch(cmdBuf, (pushConstants.vertexCount + 63U) / 64U, 1, 1);
	}
}
