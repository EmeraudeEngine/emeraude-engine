/*
 * src/Graphics/Effects/Framebuffer/MotionBlur.cpp
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

#include "MotionBlur.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

static constexpr auto TracerTag{"MotionBlurEffect"};

namespace
{
	/* ---- GLSL Shader Sources ----
	 *
	 * Technique credits:
	 * - Tile velocity reduction (TileMax/NeighborMax) and the reconstruction filter with its
	 *   cone/cylinder weights and soft depth classification: M. McGuire, P. Hennessy,
	 *   M. Bukowski, B. Osman, "A Reconstruction Filter for Plausible Motion Blur", I3D 2012.
	 * - Interleaved gradient noise for the sample jitter, and the silhouette weighting rationale:
	 *   J. Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare",
	 *   SIGGRAPH 2014.
	 *
	 * CONVENTIONS. The velocity G-buffer holds an NDC delta over ONE RENDERED FRAME. These
	 * shaders work in PIXELS: pixels = ndc * 0.5 * resolution, i.e. ndc * (0.5 / texelSize).
	 * The shutter angle (shutterSpeed / frameTime) scales it to the fraction of the frame the
	 * shutter stays open, and it is applied in the TILE pass as well as in the gather so the
	 * dominant velocity of a tile is exactly the one the gather walks. */

	constexpr auto TileMaxHorizontalFragmentShader = R"GLSL(
#version 450

layout(location = 0) out vec2 outVelocity;

layout(set = 0, binding = 0) uniform sampler2D velocityTex;

layout(push_constant) uniform PushConstants
{
	float velocityTexelSizeX;
	float velocityTexelSizeY;
	float tileSize;
	float shutterAngle;
	float maxBlurRadiusPixels;
	float deadZonePixels;
};

void main()
{
	/* Horizontal half of the separable reduction. Target is (tilesX, height): x is a TILE
	 * index, y is still a full-resolution row. */
	const ivec2 coord = ivec2(gl_FragCoord.xy);
	const int K = int(tileSize);
	const int baseX = coord.x * K;
	const ivec2 limit = textureSize(velocityTex, 0) - ivec2(1);

	/* NDC -> pixels. Ranking velocities by length is only meaningful in pixels: the NDC scale
	 * differs per axis whenever the frame is not square. The shutter angle belongs here too, so
	 * the tile carries exactly the velocity the gather will walk. */
	const vec2 toPixels = vec2(0.5) / vec2(velocityTexelSizeX, velocityTexelSizeY);

	vec2 dominant = vec2(0.0);
	float dominantLength = 0.0;

	for ( int x = 0; x < K; ++x )
	{
		/* texelFetch: exact texel centres, no sampler filtering — a bilinear blend of two
		 * velocities belongs to neither surface. */
		const vec2 rawPixels = texelFetch(velocityTex, clamp(ivec2(baseX + x, coord.y), ivec2(0), limit), 0).rg * toPixels;

		/* DEAD ZONE on the RAW per-frame velocity: this is where the buffer's floating-point
		 * noise floor lives, and the shutter scale below would amplify it. */
		if ( length(rawPixels) < deadZonePixels )
		{
			continue;
		}

		vec2 velocity = rawPixels * shutterAngle;

		/* Clamp to the ceiling here: clamping preserves direction and is monotone in length, so
		 * reducing clamped values gives the same result as clamping the reduction. */
		const float velocityLength = length(velocity);

		if ( velocityLength > maxBlurRadiusPixels )
		{
			velocity *= maxBlurRadiusPixels / velocityLength;
		}

		const float clampedLength = min(velocityLength, maxBlurRadiusPixels);

		if ( clampedLength > dominantLength )
		{
			dominantLength = clampedLength;
			dominant = velocity;
		}
	}

	outVelocity = dominant;
}
)GLSL";

	constexpr auto TileMaxVerticalFragmentShader = R"GLSL(
#version 450

layout(location = 0) out vec2 outVelocity;

layout(set = 0, binding = 0) uniform sampler2D tileMaxHTex;

layout(push_constant) uniform PushConstants
{
	float velocityTexelSizeX;
	float velocityTexelSizeY;
	float tileSize;
	float shutterAngle;
	float maxBlurRadiusPixels;
	float deadZonePixels;
};

void main()
{
	/* Vertical half: target is (tilesX, tilesY), input rows are full resolution. Values are
	 * already in pixels, shutter-scaled and clamped by the horizontal pass. */
	const ivec2 tile = ivec2(gl_FragCoord.xy);
	const int K = int(tileSize);
	const int baseY = tile.y * K;
	const ivec2 limit = textureSize(tileMaxHTex, 0) - ivec2(1);

	vec2 dominant = vec2(0.0);
	float dominantLength = 0.0;

	for ( int y = 0; y < K; ++y )
	{
		const vec2 velocity = texelFetch(tileMaxHTex, clamp(ivec2(tile.x, baseY + y), ivec2(0), limit), 0).rg;
		const float velocityLength = length(velocity);

		if ( velocityLength > dominantLength )
		{
			dominantLength = velocityLength;
			dominant = velocity;
		}
	}

	outVelocity = dominant;
}
)GLSL";

	constexpr auto NeighborMaxFragmentShader = R"GLSL(
#version 450

layout(location = 0) out vec2 outVelocity;

layout(set = 0, binding = 0) uniform sampler2D tileMaxTex;

layout(push_constant) uniform PushConstants
{
	float velocityTexelSizeX;
	float velocityTexelSizeY;
	float tileSize;
	float shutterAngle;
	float maxBlurRadiusPixels;
	float deadZonePixels;
};

void main()
{
	/* Spread each tile's dominant velocity over its 3x3 tile neighbourhood: a fast object must
	 * be able to smear BEYOND the tile it occupies, otherwise its blur stops on a tile border
	 * and the seam is visible. */
	const ivec2 tile = ivec2(gl_FragCoord.xy);
	const ivec2 limit = textureSize(tileMaxTex, 0) - ivec2(1);

	vec2 dominant = vec2(0.0);
	float dominantLength = 0.0;

	for ( int y = -1; y <= 1; ++y )
	{
		for ( int x = -1; x <= 1; ++x )
		{
			const vec2 velocity = texelFetch(tileMaxTex, clamp(tile + ivec2(x, y), ivec2(0), limit), 0).rg;
			const float velocityLength = length(velocity);

			if ( velocityLength > dominantLength )
			{
				dominantLength = velocityLength;
				dominant = velocity;
			}
		}
	}

	outVelocity = dominant;
}
)GLSL";

	constexpr auto GatherFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D velocityTex;
layout(set = 0, binding = 3) uniform sampler2D neighborMaxTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float frameWidth;
	float frameHeight;
	float nearPlane;
	float farPlane;
	float shutterAngle;
	float maxBlurRadiusPixels;
	float softDepthExtent;
	float minVelocityPixels;
	float time;
	uint sampleCount;
};

float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;

	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

/* Foreground/background classification (McGuire): 1 when 'a' is closer to the camera than 'b',
 * softened over softDepthExtent meters so a silhouette does not switch weights on one texel. */
float softDepthCompare (float a, float b)
{
	return clamp(1.0 - (a - b) / max(softDepthExtent, 1e-5), 0.0, 1.0);
}

/* Cone: the sample's own smear covers the distance d. */
float cone (float d, float velocityLength)
{
	return clamp(1.0 - d / max(velocityLength, 1e-5), 0.0, 1.0);
}

/* Cylinder: 1 while d stays inside the smear, falling off right at its end. */
float cylinder (float d, float velocityLength)
{
	return 1.0 - smoothstep(0.95 * velocityLength, 1.05 * velocityLength, d);
}

/* Interleaved gradient noise (Jimenez): decorrelates the sample offsets between neighbouring
 * pixels, so the finite sample count reads as film grain instead of ghost copies. */
float interleavedGradientNoise (vec2 position)
{
	return fract(52.9829189 * fract(dot(position, vec2(0.06711056, 0.00583715))));
}

void main()
{
	const vec2 texel = vec2(texelSizeX, texelSizeY);
	const vec2 toPixels = vec2(0.5) * vec2(frameWidth, frameHeight);

	const vec3 centerColor = texture(colorTex, vUV).rgb;

	/* Dominant velocity of the neighbourhood, already in pixels, shutter-scaled and clamped. */
	const vec2 dominant = texture(neighborMaxTex, vUV).rg;
	const float dominantLength = length(dominant);

	/* Nothing moves here: passthrough. This is the common case on a static camera and it keeps
	 * the effect free when the shutter is fast. */
	if ( dominantLength < minVelocityPixels )
	{
		outColor = vec4(centerColor, 1.0);

		return;
	}

	const vec2 centerVelocity = texture(velocityTex, vUV).rg * toPixels * shutterAngle;
	/* Half a pixel floor: a still surface must still WEIGH in its own reconstruction, and it
	 * divides the centre term below. */
	const float centerVelocityLength = max(length(centerVelocity), 0.5);
	const float centerDepth = linearizeDepth(texture(depthTex, vUV).r);

	const float jitter = interleavedGradientNoise(gl_FragCoord.xy + vec2(time * 1000.0)) - 0.5;

	/* McGuire's centre term: weighted by the inverse of its own smear length, so a still pixel
	 * dominates its reconstruction while a fast one lets the neighbourhood take over. */
	float weightSum = 1.0 / (float(sampleCount) * centerVelocityLength);
	vec3 colorSum = centerColor * weightSum;

	for ( uint i = 0u; i < sampleCount; ++i )
	{
		/* Walk the dominant velocity symmetrically around the pixel: t in (-1, 1), jittered. */
		const float t = mix(-1.0, 1.0, (float(i) + jitter + 1.0) / (float(sampleCount) + 1.0));
		const vec2 offsetPixels = dominant * t * 0.5;
		const vec2 sampleUV = vUV + offsetPixels * texel;

		const float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);
		const vec2 sampleVelocity = texture(velocityTex, sampleUV).rg * toPixels * shutterAngle;
		const float sampleVelocityLength = max(length(sampleVelocity), 0.5);
		const float d = length(offsetPixels);

		/* Three ways a sample can legitimately contribute to this pixel:
		 *  - it is IN FRONT and its own smear reaches us (a moving foreground streaking over);
		 *  - it is BEHIND and OUR smear reaches it (we moved and uncovered the background);
		 *  - both are blurred and overlap (the cylinder product, weighted twice per the paper). */
		const float foreground = softDepthCompare(sampleDepth, centerDepth);
		const float background = softDepthCompare(centerDepth, sampleDepth);

		const float weight =
			foreground * cone(d, sampleVelocityLength) +
			background * cone(d, centerVelocityLength) +
			2.0 * cylinder(d, sampleVelocityLength) * cylinder(d, centerVelocityLength);

		colorSum += texture(colorTex, sampleUV).rgb * weight;
		weightSum += weight;
	}

	outColor = vec4(colorSum / max(weightSum, 1e-5), 1.0);
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	MotionBlur::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* Effect-quality knobs, engine-wide and persisted in the settings file. The PHOTOGRAPHIC
		 * parameter — how long the shutter stays open, hence how long the smear is — is NOT a
		 * setting: it belongs to the active camera and is read per frame in execute(). */
		auto & settings = renderer.primaryServices().settings();

		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsMotionBlurSampleCountKey, DefaultGraphicsMotionBlurSampleCount);
		m_parameters.softDepthExtent = settings.getOrSetDefault< float >(GraphicsMotionBlurSoftDepthExtentKey, DefaultGraphicsMotionBlurSoftDepthExtent);

		/* The tile targets hold a 2D velocity in pixels: RG16F is enough (a 16-bit float
		 * resolves any pixel count we can render). The output is the colour chain format. */
		constexpr auto velocityFormat = VK_FORMAT_R16G16_SFLOAT;
		constexpr auto colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

		const auto tileWidth = std::max(1U, (width + TileSize - 1) / TileSize);
		const auto tileHeight = std::max(1U, (height + TileSize - 1) / TileSize);

		/* Separable reduction: (tilesX, height) then (tilesX, tilesY). */
		if ( !m_tileMaxHTarget.create(renderer, tileWidth, height, velocityFormat, "MB_TileMaxH") )
		{
			TraceError{TracerTag} << "Failed to create the horizontal tile max target !";

			return false;
		}

		if ( !m_tileMaxTarget.create(renderer, tileWidth, tileHeight, velocityFormat, "MB_TileMax") )
		{
			TraceError{TracerTag} << "Failed to create the tile max target !";

			return false;
		}

		if ( !m_neighborMaxTarget.create(renderer, tileWidth, tileHeight, velocityFormat, "MB_NeighborMax") )
		{
			TraceError{TracerTag} << "Failed to create the neighbor max target !";

			return false;
		}

		if ( !m_outputTarget.create(renderer, width, height, colorFormat, "MB_Output") )
		{
			TraceError{TracerTag} << "Failed to create the output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		auto singleInputLayout = this->getInputLayout(1);
		auto quadInputLayout = this->getInputLayout(4);

		if ( singleInputLayout == nullptr || quadInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleInputLayout);

			m_tileMaxHLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TilePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleInputLayout);

			m_tileMaxLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TilePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleInputLayout);

			m_neighborMaxLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TilePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(quadInputLayout);

			m_gatherLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GatherPushConstants)}
			});
		}

		if ( m_tileMaxHLayout == nullptr || m_tileMaxLayout == nullptr || m_neighborMaxLayout == nullptr || m_gatherLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile the vertex shader !";

			return false;
		}

		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto tileMaxHFragment = shaderManager.getShaderModuleFromSourceCode(device, "MB_TileMaxH_FS", ShaderType::FragmentShader, TileMaxHorizontalFragmentShader);
		const auto tileMaxFragment = shaderManager.getShaderModuleFromSourceCode(device, "MB_TileMaxV_FS", ShaderType::FragmentShader, TileMaxVerticalFragmentShader);
		const auto neighborMaxFragment = shaderManager.getShaderModuleFromSourceCode(device, "MB_NeighborMax_FS", ShaderType::FragmentShader, NeighborMaxFragmentShader);
		const auto gatherFragment = shaderManager.getShaderModuleFromSourceCode(device, "MB_Gather_FS", ShaderType::FragmentShader, GatherFragmentShader);

		if ( tileMaxHFragment == nullptr || tileMaxFragment == nullptr || neighborMaxFragment == nullptr || gatherFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile a fragment shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tileMaxHPipeline = this->createFullscreenPipeline(ClassId, "MB_TileMaxH", vertexModule, tileMaxHFragment, m_tileMaxHLayout, m_tileMaxHTarget);
		m_tileMaxPipeline = this->createFullscreenPipeline(ClassId, "MB_TileMaxV", vertexModule, tileMaxFragment, m_tileMaxLayout, m_tileMaxTarget);
		m_neighborMaxPipeline = this->createFullscreenPipeline(ClassId, "MB_NeighborMax", vertexModule, neighborMaxFragment, m_neighborMaxLayout, m_neighborMaxTarget);
		m_gatherPipeline = this->createFullscreenPipeline(ClassId, "MB_Gather", vertexModule, gatherFragment, m_gatherLayout, m_outputTarget);

		if ( m_tileMaxHPipeline == nullptr || m_tileMaxPipeline == nullptr || m_neighborMaxPipeline == nullptr || m_gatherPipeline == nullptr )
		{
			return false;
		}

		/* ---- Descriptor sets ---- */

		/* The horizontal reduction reads the velocity G-buffer, which comes from the frame
		 * context: per-frame. Everything after it reads our own targets: fixed sets. */
		m_tileMaxHPerFrame = this->createPerFrameDescriptorSets(singleInputLayout, ClassId, "MB_TileMaxH_DescSet");

		if ( m_tileMaxHPerFrame.empty() )
		{
			return false;
		}

		const auto & pool = renderer.descriptorPool();

		m_tileMaxDescSet = std::make_unique< DescriptorSet >(pool, singleInputLayout);
		m_tileMaxDescSet->setIdentifier(ClassId, "MB_TileMaxV_DescSet", "DescriptorSet");

		if ( !m_tileMaxDescSet->create() || !m_tileMaxDescSet->writeCombinedImageSampler(0, m_tileMaxHTarget) )
		{
			return false;
		}

		m_neighborMaxDescSet = std::make_unique< DescriptorSet >(pool, singleInputLayout);
		m_neighborMaxDescSet->setIdentifier(ClassId, "MB_NeighborMax_DescSet", "DescriptorSet");

		if ( !m_neighborMaxDescSet->create() || !m_neighborMaxDescSet->writeCombinedImageSampler(0, m_tileMaxTarget) )
		{
			return false;
		}

		/* Gather reads colour + depth + velocity (per-frame) and our neighbour target (fixed). */
		m_gatherPerFrame = this->createPerFrameDescriptorSets(quadInputLayout, ClassId, "MB_Gather_DescSet");

		if ( m_gatherPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_gatherPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(3, m_neighborMaxTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	MotionBlur::destroy () noexcept
	{
		m_gatherPerFrame.clear();
		m_neighborMaxDescSet.reset();
		m_tileMaxDescSet.reset();
		m_tileMaxHPerFrame.clear();

		m_gatherPipeline.reset();
		m_neighborMaxPipeline.reset();
		m_tileMaxPipeline.reset();
		m_tileMaxHPipeline.reset();

		m_gatherLayout.reset();
		m_neighborMaxLayout.reset();
		m_tileMaxLayout.reset();
		m_tileMaxHLayout.reset();

		m_outputTarget.destroy();
		m_neighborMaxTarget.destroy();
		m_tileMaxTarget.destroy();
		m_tileMaxHTarget.destroy();
	}

	const TextureInterface &
	MotionBlur::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();
		const auto & constants = context.constants;

		/* No velocity buffer means no motion blur: pass the colour through untouched rather
		 * than blur by garbage. requiresVelocity() makes this a defensive branch only. */
		if ( context.velocity == nullptr || context.depth == nullptr )
		{
			return inputColor;
		}

		/* SHUTTER ANGLE — the whole photographic contract in one line: how many FRAMES of motion
		 * the exposure covers. Greater than 1 is the normal case and it must NOT be clamped: a
		 * 1/60 s exposure at 500 fps covers 8.3 frames, and clamping it to 1 was what made the
		 * blur invisible (the smear could never exceed one frame of movement, so it shrank as the
		 * framerate rose — exactly what a shutter speed is supposed to prevent). The velocity is
		 * extrapolated linearly over that span, the standard approximation: a curved trajectory
		 * becomes a straight streak. The upper bound is numerical hygiene only — the real ceiling
		 * is maxBlurRadiusPixels, applied in the reduction. A missing camera means no
		 * photographic authority, hence no blur. */
		const auto shutterSpeed = context.camera != nullptr ? context.camera->shutterSpeed() : 0.0F;
		const auto shutterAngle = constants.deltaTime > 0.0F ?
			std::clamp(shutterSpeed / constants.deltaTime, 0.0F, 128.0F) :
			0.0F;

		if ( shutterAngle <= 0.0F )
		{
			return inputColor;
		}

		static_cast< void >(m_tileMaxHPerFrame[frameIndex]->writeCombinedImageSampler(0, *context.velocity));
		static_cast< void >(m_gatherPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));
		static_cast< void >(m_gatherPerFrame[frameIndex]->writeCombinedImageSampler(1, *context.depth));
		static_cast< void >(m_gatherPerFrame[frameIndex]->writeCombinedImageSampler(2, *context.velocity));

		const auto maxBlurRadiusPixels = static_cast< float >(TileSize);

		const TilePushConstants tilePC{
			.velocityTexelSizeX = 1.0F / constants.frameWidth,
			.velocityTexelSizeY = 1.0F / constants.frameHeight,
			.tileSize = static_cast< float >(TileSize),
			.shutterAngle = shutterAngle,
			.maxBlurRadiusPixels = maxBlurRadiusPixels,
			.deadZonePixels = m_parameters.deadZonePixels
		};

		/* Pass 1a: horizontal half of the reduction (NDC -> pixels, shutter scale, radius clamp). */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_tileMaxHTarget,
			*m_tileMaxHPipeline,
			*m_tileMaxHLayout,
			*m_tileMaxHPerFrame[frameIndex],
			&tilePC,
			sizeof(TilePushConstants)
		);

		/* Pass 1b: vertical half — dominant velocity per KxK tile. */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_tileMaxTarget,
			*m_tileMaxPipeline,
			*m_tileMaxLayout,
			*m_tileMaxDescSet,
			&tilePC,
			sizeof(TilePushConstants)
		);

		/* Pass 2: spread it over the 3x3 tile neighbourhood. */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_neighborMaxTarget,
			*m_neighborMaxPipeline,
			*m_neighborMaxLayout,
			*m_neighborMaxDescSet,
			&tilePC,
			sizeof(TilePushConstants)
		);

		/* Pass 3: full-resolution reconstruction. */
		const GatherPushConstants gatherPC{
			.texelSizeX = 1.0F / constants.frameWidth,
			.texelSizeY = 1.0F / constants.frameHeight,
			.frameWidth = constants.frameWidth,
			.frameHeight = constants.frameHeight,
			.nearPlane = constants.nearPlane,
			.farPlane = constants.farPlane,
			.shutterAngle = shutterAngle,
			.maxBlurRadiusPixels = maxBlurRadiusPixels,
			.softDepthExtent = m_parameters.softDepthExtent,
			.minVelocityPixels = m_parameters.minVelocityPixels,
			.time = constants.time,
			.sampleCount = std::max(1U, m_parameters.sampleCount)
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_gatherPipeline,
			*m_gatherLayout,
			*m_gatherPerFrame[frameIndex],
			&gatherPC,
			sizeof(GatherPushConstants)
		);

		return m_outputTarget;
	}
}
