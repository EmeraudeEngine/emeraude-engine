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
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
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
 * - `emUVFromView(viewPosition)` and `emDepthFromView(viewPosition)`: back to the screen and to the depth buffer;
 * - the FAST twins for a marching loop (owner decision 2026-11-02): `emViewDepthFast(depth)`,
 *   `emViewPositionFast(uv, depth)`, `emUVFromViewFast(viewPosition)`. They are a COMPILE-TIME variant
 *   (`ProjectionForm`): the `Sparse` form reads a few coefficients the CPU takes once per frame from the SAME projection,
 *   exact to float rounding for a matrix with the sparse layout of a perspective or an orthographic projection, centred or
 *   not (`isSparseProjection()`); the `Matrix` form forwards to the matrix helpers. A marching effect builds both pipelines
 *   and records the one `FrameContext::viewProjectionSparse` names. Measured on an Intel iGPU, reflexion-debug at
 *   1280×720: the full matrix product at every SSGI march step cost +0.73 ms (13.35 → 14.08, engine e64e67da; +0.02 on an
 *   RTX 3070 Ti); the Sparse form gives back 0.60 of it; a RUNTIME uniform branch choosing between the two inside the
 *   helpers cost +0.85 instead (the Intel compiler kept both paths live in the loop).
 * `uv` is the screen coordinate in [0, 1] (vUV), NDC = uv × 2 − 1 like every effect of the chain.
 */
namespace EmEn::Graphics::ViewReconstructionGLSL
{
	/**
	 * @brief The C++ twin of the GLSL block (std140: three mat4 and four vec4, column-major).
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
		/** @brief The projection's depth terms P22, P32, P23, P33 (GLSL `projection[col][row]`: [2][2], [3][2], [2][3], [3][3]):
		 * clip z = P22 z + P32, clip w = P23 z + P33. */
		std::array< float, 4 > depthCoefficients{};
		/** @brief P00, P11 and their reciprocals. */
		std::array< float, 4 > perspectiveScale{};
		/** @brief P20, P21 (the off-centre terms) and P30, P31 (the orthographic offsets). */
		std::array< float, 4 > perspectiveOffset{};
	};

	static_assert(sizeof(Data) == 256, "The view block is three mat4 and four vec4 (std140).");

	/** @brief The form of the fast helpers a shader is compiled with. */
	enum class ProjectionForm : uint8_t
	{
		/** @brief The fast helpers forward to the matrix helpers (exact for any projection). */
		Matrix,
		/** @brief The fast helpers use the precomputed coefficients (exact for a sparse projection only). */
		Sparse
	};

	/**
	 * @brief Returns whether a projection has the sparse layout the `Sparse` form is exact for: no x↔y shear, no x or y
	 * term in the depth and w rows, a finite non-zero scale on both axes. A perspective or an orthographic projection,
	 * centred or not (`Matrix::perspectiveProjection()`, `orthographicProjection()`), has it.
	 * @param projection The projection, column-major (element (column, row) at column × 4 + row).
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	isSparseProjection (const std::array< float, 16 > & projection) noexcept
	{
		const auto element = [&projection] (size_t column, size_t row) noexcept {
			return *std::next(projection.cbegin(), static_cast< std::ptrdiff_t >((column * 4U) + row));
		};
		const auto scale = std::max(std::abs(element(0, 0)), std::abs(element(1, 1)));
		const auto negligible = [scale] (float value) noexcept {
			return std::abs(value) <= scale * 1.0e-6F;
		};

		return std::isfinite(scale) && std::abs(element(0, 0)) > 0.0F && std::abs(element(1, 1)) > 0.0F &&
			negligible(element(1, 0)) && negligible(element(0, 1)) && negligible(element(0, 2)) && negligible(element(1, 2)) &&
			negligible(element(0, 3)) && negligible(element(1, 3));
	}

	/**
	 * @brief Fills the block's fast coefficients from its projection.
	 * @pre isSparseProjection(data.projection): the coefficients are meaningless otherwise (and the Matrix form is used).
	 * @param data A reference to the block, its projection written.
	 */
	inline
	void
	fillFastCoefficients (Data & data) noexcept
	{
		const auto element = [&data] (size_t column, size_t row) noexcept {
			return *std::next(data.projection.cbegin(), static_cast< std::ptrdiff_t >((column * 4U) + row));
		};
		const auto p00 = element(0, 0);
		const auto p11 = element(1, 1);

		data.depthCoefficients = {element(2, 2), element(3, 2), element(2, 3), element(3, 3)};
		data.perspectiveScale = {p00, p11, 1.0F / p00, 1.0F / p11};
		data.perspectiveOffset = {element(2, 0), element(2, 1), element(3, 0), element(3, 1)};
	}

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
		code += "\n{\n\tmat4 projection;\n\tmat4 inverseProjection;\n\tmat4 inverseRelativeViewProjection;\n\tvec4 cameraPosition;\n\tvec4 depthCoefficients;\n\tvec4 perspectiveScale;\n\tvec4 perspectiveOffset;\n} ";
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

	/** @brief The fast helpers, Sparse form (after FunctionsGLSL). */
	constexpr auto FastSparseGLSL = R"GLSL(
/* The FAST twins, SPARSE form (a marching loop): the projection's few non-zero terms, taken once per frame from the
 * same matrix. clip = (P00 x + P20 z + P30, P11 y + P21 z + P31, P22 z + P32, P23 z + P33); the inverse follows. Exact
 * to float rounding for a sparse projection (ViewReconstructionGLSL::isSparseProjection()). */
float emViewDepthFast (float depth)
{
	vec4 k = emView.depthCoefficients;

	return -(k.y - depth * k.w) / (depth * k.z - k.x);
}

vec3 emViewPositionFast (vec2 uv, float depth)
{
	vec4 k = emView.depthCoefficients;
	float z = (k.y - depth * k.w) / (depth * k.z - k.x);
	float w = k.z * z + k.w;
	vec2 xy = ((uv * 2.0 - 1.0) * w - emView.perspectiveOffset.xy * z - emView.perspectiveOffset.zw) * emView.perspectiveScale.zw;

	return vec3(xy, z);
}

vec2 emUVFromViewFast (vec3 viewPosition)
{
	float w = emView.depthCoefficients.z * viewPosition.z + emView.depthCoefficients.w;
	vec2 clip = emView.perspectiveScale.xy * viewPosition.xy + emView.perspectiveOffset.xy * viewPosition.z + emView.perspectiveOffset.zw;

	return clip / w * 0.5 + 0.5;
}
)GLSL";

	/** @brief The fast helpers, Matrix form (after FunctionsGLSL): the matrix helpers under the fast names. */
	constexpr auto FastMatrixGLSL = R"GLSL(
/* The FAST twins, MATRIX form: the projection is not sparse (or the effect has no Sparse variant). */
float emViewDepthFast (float depth)
{
	return emViewDepth(vec2(0.5), depth);
}

vec3 emViewPositionFast (vec2 uv, float depth)
{
	return emViewPosition(uv, depth);
}

vec2 emUVFromViewFast (vec3 viewPosition)
{
	return emUVFromView(viewPosition);
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
		return uniformBlockGLSL(set, binding) + FunctionsGLSL + FastMatrixGLSL;
	}

	/**
	 * @brief Returns the block and the helpers with the fast helpers in a given form.
	 * @param set The descriptor set index.
	 * @param binding The binding index.
	 * @param form The fast helpers' form.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	sourceGLSL (uint32_t set, uint32_t binding, ProjectionForm form) noexcept
	{
		return uniformBlockGLSL(set, binding) + FunctionsGLSL + (form == ProjectionForm::Sparse ? FastSparseGLSL : FastMatrixGLSL);
	}
}
