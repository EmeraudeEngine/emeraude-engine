/*
 * src/Graphics/Effects/Shared/ViewReconstructionGLSL.hpp
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

#pragma once

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <string>

/**
 * @file
 * @brief The chain's per-frame VIEW data and the ONE way the post-process effects turn a depth-buffer value into a
 * position: through the inverse of the projection the frame was rendered with, never through a scalar linearisation.
 *
 * ⚠️ Why (owner decision 2026-10-25, option B; graphics doc 13-12 § 41): the chain linearised
 * depth with clip planes frozen at 0.1 / 1000 (`PostProcessor::setClipPlanes()` had no caller), while the real near
 * plane follows the field of view (`computeNearPlane()`): the labyrinth's 13 mm lens read 2.11 m for 1 m and 680 m
 * for 1 km. (The formula itself, OpenGL's on `depth × 2 − 1`, is the Vulkan inverse in disguise.) A scalar formula is
 * also wrong for an orthographic camera, whatever its planes. The matrix reconstruction is the clouds', the GI
 * denoiser's and the aerial perspective's, validated; it is camera-RELATIVE (the projection times the view ROTATION),
 * so a float never carries a world position far from the origin.
 *
 * The block is written once per frame by the PostProcessor (FrameContext::view) and read through the helpers:
 * - `emViewPosition(uv, depth)`: the view-space position (right-handed, the camera looks down −Z);
 * - `emViewDepth(uv, depth)`: the distance along the view axis (−z), what a "linear depth" means;
 * - `emRelativePosition(uv, depth)`: the world-space offset from the camera;
 * - `emViewDistance(uv, depth)`: its length, the Euclidean distance a medium integrates;
 * - `emUVFromView(viewPosition)` and `emDepthFromView(viewPosition)`: back to the screen and to the depth buffer.
 * `uv` is the screen coordinate in [0, 1] (vUV), NDC = uv × 2 − 1 like every effect of the chain.
 */
namespace EmEn::Graphics::ViewReconstructionGLSL
{
	/**
	 * @brief The C++ twin of the GLSL block (std140: three mat4 and a vec4, column-major).
	 */
	struct Data
	{
		/** @brief The projection of the frame (view → clip). */
		std::array< float, 16 > projection{};
		/** @brief Its inverse (clip → view). */
		std::array< float, 16 > inverseProjection{};
		/** @brief The inverse of projection × view ROTATION (clip → world offset from the camera). */
		std::array< float, 16 > inverseRelativeViewProjection{};
		/** @brief xyz: the camera's world position; w: unused (0). */
		std::array< float, 4 > cameraPosition{};
	};

	static_assert(sizeof(Data) == 208, "The view block is three mat4 and a vec4 (std140).");

	/** @brief The type and the instance name of the block. */
	constexpr auto BlockType{"EmViewReconstruction"};
	constexpr auto BlockInstance{"emView"};

	/**
	 * @brief Returns the block's declaration at a set and a binding.
	 * @param set The descriptor set index.
	 * @param binding The binding index.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	uniformBlockGLSL (uint32_t set, uint32_t binding) noexcept
	{
		std::string code{"layout(set = "};
		code += std::to_string(set);
		code += ", binding = ";
		code += std::to_string(binding);
		code += ", std140) uniform ";
		code += BlockType;
		code += "\n{\n\tmat4 projection;\n\tmat4 inverseProjection;\n\tmat4 inverseRelativeViewProjection;\n\tvec4 cameraPosition;\n} ";
		code += BlockInstance;
		code += ";\n\n";

		return code;
	}

	/** @brief The helpers (after the block). */
	constexpr auto FunctionsGLSL = R"GLSL(
/* The view reconstruction (Graphics/Effects/Shared/ViewReconstructionGLSL.hpp). */
vec3 emViewPosition (vec2 uv, float depth)
{
	vec4 position = emView.inverseProjection * vec4(uv * 2.0 - 1.0, depth, 1.0);

	return position.xyz / position.w;
}

float emViewDepth (vec2 uv, float depth)
{
	return -emViewPosition(uv, depth).z;
}

vec3 emRelativePosition (vec2 uv, float depth)
{
	vec4 position = emView.inverseRelativeViewProjection * vec4(uv * 2.0 - 1.0, depth, 1.0);

	return position.xyz / position.w;
}

float emViewDistance (vec2 uv, float depth)
{
	return length(emRelativePosition(uv, depth));
}

vec3 emClipFromView (vec3 viewPosition)
{
	vec4 clip = emView.projection * vec4(viewPosition, 1.0);

	return clip.xyz / clip.w;
}

vec2 emUVFromView (vec3 viewPosition)
{
	return emClipFromView(viewPosition).xy * 0.5 + 0.5;
}

float emDepthFromView (vec3 viewPosition)
{
	return emClipFromView(viewPosition).z;
}
)GLSL";

	/**
	 * @brief Returns the block and the helpers, ready to splice into an effect's source.
	 * @param set The descriptor set index.
	 * @param binding The binding index.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	sourceGLSL (uint32_t set, uint32_t binding) noexcept
	{
		return uniformBlockGLSL(set, binding) + FunctionsGLSL;
	}
}
