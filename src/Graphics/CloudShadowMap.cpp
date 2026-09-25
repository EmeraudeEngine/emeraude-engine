/*
 * src/Graphics/CloudShadowMap.cpp
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

#include "CloudShadowMap.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>
#include <string>

/* Local inclusions. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/UniformBufferObject.hpp"

namespace
{
	/* ---- GLSL Shader Source ----
	 * Set 0: binding 0 the shadow block. Set 1: the bindless table (the cloud SHAPES in its 3D array).
	 * One fragment per texel of the map: the texel's ray runs along the light, from the sun side of the
	 * range to the far side, and the parameter IS the depth the lit shaders compare against. */
	constexpr auto ShadowFragmentShader = R"GLSL(
#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outBeer;

layout(set = 1, binding = 2) uniform sampler3D textures3D[];
)GLSL" EMEN_CLOUD_VOLUME_GLSL R"GLSL(

const int MaxHits = 8;

layout(set = 0, binding = 0, std140) uniform EmCloudShadow
{
	mat4 mapToWorld;		/* (u, v, depth, 1) -> world. */
	vec4 lightDirection;	/* xyz = direction of PROPAGATION, w = cloud count. */
	vec4 parameters;		/* x = depth range either side of the camera plane (m), y = step count. */
	EmCloud clouds[MaxClouds];
} emShadow;

void main()
{
	vec3 lightDirection = emShadow.lightDirection.xyz;
	float range = emShadow.parameters.x;
	int stepCount = max(int(emShadow.parameters.y), 1);
	int cloudCount = min(int(emShadow.lightDirection.w), MaxClouds);

	/* The texel's point on the camera plane (depth 0); the ray is origin + lightDirection * depth. */
	vec3 origin = (emShadow.mapToWorld * vec4(vUV, 0.0, 1.0)).xyz;

	/* ---- The boxes this ray crosses, within the range. ---- */
	float hitEnter[MaxHits];
	float hitExit[MaxHits];
	int hitCloud[MaxHits];
	int hitCount = 0;
	float firstEnter = range;
	float lastExit = -range;

	for ( int cloudIndex = 0; cloudIndex < cloudCount && hitCount < MaxHits; ++cloudIndex )
	{
		vec2 interval = intersectBox(toBox(emShadow.clouds[cloudIndex], origin), toBoxDirection(emShadow.clouds[cloudIndex], lightDirection));

		interval.x = max(interval.x, -range);
		interval.y = min(interval.y, range);

		if ( interval.y <= interval.x )
		{
			continue;
		}

		hitEnter[hitCount] = interval.x;
		hitExit[hitCount] = interval.y;
		hitCloud[hitCount] = cloudIndex;
		++hitCount;

		firstEnter = min(firstEnter, interval.x);
		lastExit = max(lastExit, interval.y);
	}

	/* No cloud over this texel: nothing starts before the far end, nothing is extinguished. */
	if ( hitCount == 0 )
	{
		outBeer = vec4(range, 0.0, 0.0, 1.0);

		return;
	}

	/* ---- One march over the union, all the crossed clouds together (the view march's rule). The
	 * density is the shape at mip 1, without the detail erosion: the shadow is soft, and the wisps a
	 * receiver could tell apart are not worth a noise fetch per step. ---- */
	float stepLength = (lastExit - firstEnter) / float(stepCount);
	float opticalDepth = 0.0;
	float front = range;
	float back = -range;

	for ( int stepIndex = 0; stepIndex < stepCount; ++stepIndex )
	{
		float depth = firstEnter + (float(stepIndex) + 0.5) * stepLength;
		vec3 samplePosition = origin + lightDirection * depth;
		float sigmaT = 0.0;

		for ( int hit = 0; hit < hitCount; ++hit )
		{
			if ( depth < hitEnter[hit] || depth >= hitExit[hit] )
			{
				continue;
			}

			EmCloud cloud = emShadow.clouds[hitCloud[hit]];
			uint shapeIndex = uint(cloud.shape.w + 0.5);
			vec3 box = toBox(cloud, samplePosition);

			sigmaT += textureLod(textures3D[nonuniformEXT(shapeIndex)], box * 0.5 + 0.5, 1.0).r * cloud.centerAndExtinction.w;
		}

		if ( sigmaT > 0.0 )
		{
			front = min(front, depth - 0.5 * stepLength);
			back = max(back, depth + 0.5 * stepLength);
			opticalDepth += sigmaT * stepLength;
		}
	}

	if ( opticalDepth <= 1.0e-4 )
	{
		outBeer = vec4(range, 0.0, 0.0, 1.0);

		return;
	}

	/* R = where the clouds start, G = their mean extinction over their thickness, B = the total. */
	outBeer = vec4(front, opticalDepth / max(back - front, stepLength), opticalDepth, 1.0);
}
)GLSL";
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	bool
	CloudShadowMap::create (uint32_t /*width*/, uint32_t /*height*/) noexcept
	{
		auto & renderer = this->renderer();

		if ( renderer.bindlessTextureManager().descriptorSetLayout() == nullptr )
		{
			TraceError{ClassId} << "The bindless texture table is not available: the cloud shapes cannot be read !";

			return false;
		}

		/* ⚠️ A half float per channel: the depths are taken from the camera's plane (see the class note),
		 * so the precision is where the receivers are. */
		if ( !m_target->create(renderer, m_resolution, m_resolution, VK_FORMAT_R16G16B16A16_SFLOAT, "CloudShadowMap") )
		{
			TraceError{ClassId} << "Failed to create the cloud shadow map target !";

			return false;
		}

		const auto inputLayout = this->getInputLayout(0, 1);

		if ( inputLayout == nullptr )
		{
			return false;
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(inputLayout);
			/* Set 1 = the bindless table, where recordFullscreenPass() binds it. */
			sets.emplace_back(renderer.bindlessTextureManager().descriptorSetLayout());

			m_layout = renderer.layoutManager().getPipelineLayout(sets, {});
		}

		if ( m_layout == nullptr )
		{
			return false;
		}

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto fragmentModule = renderer.shaderManager().getShaderModuleFromSourceCode(renderer.device(), "CloudShadowMap_FS", Saphir::ShaderType::FragmentShader, ShadowFragmentShader);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the cloud shadow map shaders !";

			return false;
		}

		m_pipeline = this->createFullscreenPipeline(ClassId, "CloudShadowMap", vertexModule, fragmentModule, m_layout, *m_target);

		if ( m_pipeline == nullptr )
		{
			return false;
		}

		m_perFrame = this->createPerFrameDescriptorSets(inputLayout, ClassId, "CloudShadowMap_DescSet");
		m_frameUBOs = this->createPerFrameUniformBuffers(sizeof(ShadowBlock), ClassId, "CloudShadowMap_UBO");

		if ( m_perFrame.empty() || m_frameUBOs.empty() )
		{
			return false;
		}

		for ( size_t frame = 0; frame < m_perFrame.size(); ++frame )
		{
			if ( !m_perFrame[frame]->writeUniformBufferObject(0, *m_frameUBOs[frame]) )
			{
				return false;
			}
		}

		TraceInfo{ClassId} << "Cloud shadow map created: " << m_resolution << "² texels over " << m_coverage << " m (" << (m_coverage / static_cast< float >(m_resolution)) << " m per texel).";

		return true;
	}

	void
	CloudShadowMap::destroy () noexcept
	{
		m_frameUBOs.clear();
		m_perFrame.clear();
		m_pipeline.reset();
		m_layout.reset();
		m_target->destroy();
	}

	bool
	CloudShadowMap::record (const CommandBuffer & commandBuffer, const std::array< CloudBlock, MaxCloudVolumes > & clouds, uint32_t cloudCount, const Matrix< 4, float > & worldToMap, float depthRange) noexcept
	{
		if ( m_pipeline == nullptr )
		{
			return false;
		}

		auto & renderer = this->renderer();
		const auto frameIndex = renderer.currentFrameIndex();

		ShadowBlock block{};

		const auto mapToWorld = worldToMap.inverse();

		std::memcpy(block.mapToWorld.data(), mapToWorld.data(), block.mapToWorld.size() * sizeof(float));

		/* The light direction is the third ROW of the matrix: depth = light . P + offset. Column-major
		 * storage puts row 2 at elements 2, 6, 10. */
		const auto * matrix = worldToMap.data();

		block.lightDirection = {matrix[2], matrix[6], matrix[10], static_cast< float >(std::min(cloudCount, MaxCloudVolumes))};
		block.parameters = {std::max(depthRange, MinimumDepthRange), static_cast< float >(StepCount), 0.0F, 0.0F};
		block.clouds = clouds;

		if ( !updateUniformBufferData(*m_frameUBOs[frameIndex], &block, sizeof(block)) )
		{
			return false;
		}

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			*m_target,
			*m_pipeline,
			*m_layout,
			*m_perFrame[frameIndex],
			nullptr,
			0,
			renderer.bindlessTextureManager().descriptorSet()
		);

		return true;
	}
}
