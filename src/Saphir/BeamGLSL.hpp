/*
 * src/Saphir/BeamGLSL.hpp
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

/* Local inclusions. */
#include "Saphir/AbstractShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Keys.hpp"

/**
 * @brief The GLSL side of a beam ribbon (a laser, an electric arc): the arc displacement and the camera-facing
 * ribbon corner (AbstractVertexStage::enableBeamRibbon(), Material::BeamResource).
 * @note The beam lives in a UNIT SEGMENT space: the model matrix maps x ∈ [0, 1] from the start to the end of the
 * beam, and its y and z columns are two unit directions across it (Scenes::Component::Beam publishes that matrix per
 * logic tick, RenderableInstance::Abstract::publishTransformationMatrix()). The ribbon is built in WORLD space, where
 * the eye and the pixel size live, and brought back to that space through the inverse model matrix, so the standard
 * matrices and the velocity downstream apply unchanged — the imposter billboard's scheme.
 * @note The arc is two independent fBm of 1D gradient noise along the beam (one per axis across it), under a
 * sin(πt) envelope that pins both ends. Deterministic per (seed, time): the previous position is the same code at
 * the previous time with the previous model matrix, which is what gives a moving or re-striking arc a real velocity.
 * @note Hash: "lowbias32", Chris Wellons, "Prospecting for Hash Functions" (2018, public domain),
 * https://nullprogram.com/blog/2018/07/31/ . Gradient noise and its quintic fade: Ken Perlin, "Improving Noise",
 * SIGGRAPH 2002. Sub-pixel width: Emil Persson, "Phone-wire AA", GPU Pro 5 (2014) — the drawn width is clamped to one
 * pixel and the light scaled by the true width over it, so a far beam fades instead of breaking into sub-pixel
 * triangles.
 */
namespace EmEn::Saphir::BeamGLSL
{
	/** @brief The octave count the fBm loop is unrolled against (the UBO value is clamped to it). */
	constexpr auto MaxOctaves{8};

	/**
	 * @brief Declares the beam functions in a vertex stage: beamHash(uint), beamNoise(float, uint),
	 * beamFbm(float, uint, int), beamOffset(float t, float time, vec4 shape, vec4 motion),
	 * beamCentre(mat4 model, float t, float time, vec4 shape, vec4 motion) and
	 * beamCorner(mat4 model, vec2 coordinates, float time, vec4 shape, vec4 motion, vec3 eye, vec2 pixel).
	 * @note beamCorner() returns (object-space position, coverage). `pixel` is (world size of one pixel at a distance
	 * of 1 — or at any distance under an orthographic projection —, 1 for a perspective projection else 0); a zero
	 * pixel size disables the clamp. `shape` and `motion` are the Material::BeamResource UBO vectors (Keys
	 * BeamShape, BeamMotion).
	 * @param shader A reference to the shader.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	declareFunctions (AbstractShader & shader) noexcept
	{
		using namespace Keys;

		/* lowbias32, mapped to [-1, 1]. */
		Declaration::Function hash{"beamHash", GLSL::Float};
		hash.addInParameter(GLSL::UnsignedInteger, "key");
		Code{hash, Location::Output} <<
			"uint value = key;" << Line::End <<
			"value ^= value >> 16u;" << Line::End <<
			"value *= 0x7FEB352Du;" << Line::End <<
			"value ^= value >> 15u;" << Line::End <<
			"value *= 0x846CA68Bu;" << Line::End <<
			"value ^= value >> 16u;" << Line::End <<
			"return float(value) * (2.0 / 4294967295.0) - 1.0;";

		/* 1D gradient noise: a random slope at each integer, blended by the quintic fade. Zero at every integer. */
		Declaration::Function noise{"beamNoise", GLSL::Float};
		noise.addInParameter(GLSL::Float, "x");
		noise.addInParameter(GLSL::UnsignedInteger, "seed");
		Code{noise, Location::Output} <<
			"const float cell = floor(x);" << Line::End <<
			"const float f = x - cell;" << Line::End <<
			"const uint index = uint(int(cell));" << Line::End <<
			"const float g0 = beamHash(seed + index * 0x9E3779B1u);" << Line::End <<
			"const float g1 = beamHash(seed + (index + 1u) * 0x9E3779B1u);" << Line::End <<
			"const float fade = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);" << Line::End <<
			"return mix(g0 * f, g1 * (f - 1.0), fade) * 2.0;";

		/* fBm, amplitude halved per octave (a 1/f spectrum: the jagged look of a discharge), normalized. */
		Declaration::Function fbm{"beamFbm", GLSL::Float};
		fbm.addInParameter(GLSL::Float, "x");
		fbm.addInParameter(GLSL::UnsignedInteger, "seed");
		fbm.addInParameter(GLSL::Integer, "octaves");
		Code{fbm, Location::Output} <<
			"float sum = 0.0;" << Line::End <<
			"float amplitude = 1.0;" << Line::End <<
			"float norm = 0.0;" << Line::End <<
			"float frequency = x;" << Line::End <<
			"for ( int octave = 0; octave < " << MaxOctaves << "; ++octave ) {" << Line::End <<
			"	if ( octave >= octaves ) { break; }" << Line::End <<
			"	sum += amplitude * beamNoise(frequency, seed + uint(octave) * 0x632BE5ABu);" << Line::End <<
			"	norm += amplitude;" << Line::End <<
			"	amplitude *= 0.5;" << Line::End <<
			"	frequency *= 2.0;" << Line::End <<
			"}" << Line::End <<
			"return norm > 0.0 ? sum / norm : 0.0;";

		/* The arc: its offset across the beam, in unit-segment space (y, z). A re-strike rate > 0 draws a new arc
		 * every 1 / rate seconds (the seed changes); the drift scrolls the noise along the beam in between. */
		Declaration::Function offset{"beamOffset", GLSL::FloatVector2};
		offset.addInParameter(GLSL::Float, "t");
		offset.addInParameter(GLSL::Float, "time");
		offset.addInParameter(GLSL::FloatVector4, "shape");
		offset.addInParameter(GLSL::FloatVector4, "motion");
		Code{offset, Location::Output} <<
			"if ( shape.y <= 0.0 ) { return vec2(0.0); }" << Line::End <<
			"const float epoch = motion.y > 0.0 ? floor(time * motion.y) : 0.0;" << Line::End <<
			"const uint seed = uint(motion.x) + uint(epoch) * 0x27D4EB2Du;" << Line::End <<
			"const int octaves = clamp(int(shape.w), 1, " << MaxOctaves << ");" << Line::End <<
			"const float x = t * shape.z + time * motion.z;" << Line::End <<
			"const float envelope = sin(3.14159265 * clamp(t, 0.0, 1.0));" << Line::End <<
			"return shape.y * envelope * vec2(beamFbm(x, seed, octaves), beamFbm(x, seed ^ 0x5BD1E995u, octaves));";

		/* A point of the beam's centre line, in world space. */
		Declaration::Function centre{"beamCentre", GLSL::FloatVector3};
		centre.addInParameter(GLSL::Matrix4, "model");
		centre.addInParameter(GLSL::Float, "t");
		centre.addInParameter(GLSL::Float, "time");
		centre.addInParameter(GLSL::FloatVector4, "shape");
		centre.addInParameter(GLSL::FloatVector4, "motion");
		Code{centre, Location::Output} <<
			"const vec2 across = beamOffset(t, time, shape, motion);" << Line::End <<
			"return (model * vec4(t, across.x, across.y, 1.0)).xyz;";

		/* A corner of the ribbon: the centre moved across the local tangent, facing the eye, by the half width —
		 * clamped to one pixel, the coverage returned in w. */
		Declaration::Function corner{"beamCorner", GLSL::FloatVector4};
		corner.addInParameter(GLSL::Matrix4, "model");
		corner.addInParameter(GLSL::FloatVector2, "coordinates");
		corner.addInParameter(GLSL::Float, "time");
		corner.addInParameter(GLSL::FloatVector4, "shape");
		corner.addInParameter(GLSL::FloatVector4, "motion");
		corner.addInParameter(GLSL::FloatVector3, "eye");
		corner.addInParameter(GLSL::FloatVector2, "pixel");
		Code{corner, Location::Output} <<
			"/* A beam of zero length has no inverse model matrix: collapse it. */" << Line::End <<
			"if ( dot(model[0].xyz, model[0].xyz) < 1.0e-12 ) { return vec4(0.0); }" << Line::End <<
			"const float t = coordinates.x;" << Line::End <<
			"const vec3 point = beamCentre(model, t, time, shape, motion);" << Line::End <<
			"const vec3 tangent = beamCentre(model, t + 0.00390625, time, shape, motion) - beamCentre(model, t - 0.00390625, time, shape, motion);" << Line::End <<
			"const vec3 toEye = eye - point;" << Line::End <<
			"vec3 across = cross(tangent, toEye);" << Line::End <<
			"/* Looking straight down the beam: any direction across it will do. */" << Line::End <<
			"if ( dot(across, across) < 1.0e-20 ) { across = model[1].xyz; }" << Line::End <<
			"across = normalize(across);" << Line::End <<
			"const float halfWidth = shape.x * length(model[1].xyz);" << Line::End <<
			"const float pixelSize = pixel.x * (pixel.y > 0.0 ? length(toEye) : 1.0);" << Line::End <<
			"const float drawnHalfWidth = max(halfWidth, pixelSize);" << Line::End <<
			"const vec3 world = point + across * (coordinates.y * drawnHalfWidth);" << Line::End <<
			"return vec4((inverse(model) * vec4(world, 1.0)).xyz, drawnHalfWidth > 0.0 ? halfWidth / drawnHalfWidth : 0.0);";

		return shader.declare(hash) && shader.declare(noise) && shader.declare(fbm) && shader.declare(offset) && shader.declare(centre) && shader.declare(corner);
	}
}
