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
#include "Saphir/PathGLSL.hpp"

/**
 * @brief The GLSL side of a beam ribbon (a laser, an electric arc): the arc displacement and the camera-facing
 * ribbon corner (AbstractVertexStage::enableBeamRibbon(), Material::BeamResource).
 * @note The beam follows a curve (Scenes::Component::Beam) drawn by VERTEX PULLING: no vertex buffer, every vertex reads
 * the beam's STATIONS from the scene's path SSBO (the instance-transforms set, bindings 1-2, through the directory entry
 * of its instance slot) — two records per station, (position, t) then (normal, 0), in the entity's space; t is the
 * normalized arc length. SIX vertices per segment, a triangle list (the quad between two stations); a vertex past the
 * last segment collapses onto the origin.
 * @note The ribbon is built in WORLD space, where the eye and the pixel size live, and brought back to the entity's
 * space through the inverse model matrix, so the standard matrices and the velocity downstream apply unchanged — the
 * imposter billboard's scheme. Its across direction faces the eye, around the local tangent (the neighbour stations).
 * @note The arc is two independent fBm of 1D gradient noise along the beam, one along each axis of the station's
 * rotation minimizing frame (normal, cross(tangent, normal)), under a sin(πt) envelope that pins both ENDS
 * (owner decision 2026-09-30). Deterministic per (seed, time): the previous position is the same code at the previous
 * time with the previous stations and model matrix, which is what gives a moving or re-striking arc a real velocity.
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
	 * @brief The gain of the arc's saturation (beamOffset()): the ENERGY-normalized fBm (its spread is the same for 1 to
	 * 8 octaves, σ ≈ 0.28) times this, through tanh, has a mean magnitude of 0.50 and never reaches 1 — the statistics of
	 * a uniform draw in [-1, 1], which is what the AMPLITUDE of Unreal's Cascade beam noise means (UParticleModuleBeamNoise
	 * NoiseRange: each noise point displaced within ± the range). Measured offline on 40 000 samples per octave count,
	 * 2026-09-30: mean |offset| 0.495-0.506, 99th percentile 0.988-0.990.
	 */
	constexpr auto ArcGain{3.4F};

	/** @brief Vertices per segment: the quad between two stations (a triangle list). */
	constexpr uint32_t VerticesPerSegment{6};

	/**
	 * @brief Declares the beam functions in a vertex stage: beamHash(uint), beamNoise(float, uint),
	 * beamFbm(float, uint, int), beamOffset(float t, float time, vec4 shape, vec4 motion),
	 * beamStation(mat4 model, uvec4 span, int station, float time, vec4 shape, vec4 motion, bool previous) and
	 * beamCorner(mat4 model, uvec4 span, int vertexIndex, float time, vec4 shape, vec4 motion, vec3 eye, vec2 pixel,
	 * bool previous, out vec2 coordinates) — and pathPoint() (PathGLSL), their reader of the path SSBO.
	 * @note beamCorner() returns (object-space position, coverage) and writes (t, side in [-1, 1]) to `coordinates`.
	 * `pixel` is (world size of one pixel at a distance of 1 — or at any distance under an orthographic projection —, 1
	 * for a perspective projection else 0); a zero pixel size disables the clamp. `shape` and `motion` are the
	 * Material::BeamResource UBO vectors (Keys BeamShape, BeamMotion); `previous` reads the stations of the previous
	 * frame (the velocity pass).
	 * @note Requires the path blocks (Generator::Abstract::declarePathBlocks()).
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

		/* fBm, amplitude halved per octave (a 1/f spectrum: the jagged look of a discharge), normalized by its ENERGY (the
		 * root of the summed squared amplitudes): the same spread whatever the octave count. */
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
			"	norm += amplitude * amplitude;" << Line::End <<
			"	amplitude *= 0.5;" << Line::End <<
			"	frequency *= 2.0;" << Line::End <<
			"}" << Line::End <<
			"return norm > 0.0 ? sum / sqrt(norm) : 0.0;";

		/* The arc: its offset along the station's normal and binormal, in entity units. The AMPLITUDE is the largest offset
		 * (Cascade's NoiseRange): the saturated fBm stays within ±1, its mean magnitude is 1/2 (ArcGain). A re-strike rate
		 * > 0 draws a new arc every 1 / rate seconds (the seed changes); the drift scrolls the noise along the beam. */
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
			"return shape.y * envelope * tanh(" << ArcGain << " * vec2(beamFbm(x, seed, octaves), beamFbm(x, seed ^ 0x5BD1E995u, octaves)));";

		/* A station of the beam's centre line, displaced by the arc along its frame, in world space. A station index out
		 * of range is clamped (the neighbours of the end stations). */
		Declaration::Function station{"beamStation", GLSL::FloatVector3};
		station.addInParameter(GLSL::Matrix4, "model");
		station.addInParameter(GLSL::UIntVector4, "span");
		station.addInParameter(GLSL::Integer, "station");
		station.addInParameter(GLSL::Float, "time");
		station.addInParameter(GLSL::FloatVector4, "shape");
		station.addInParameter(GLSL::FloatVector4, "motion");
		station.addInParameter(GLSL::Boolean, "previous");
		Code{station, Location::Output} <<
			"const int count = int(span.y) / 2;" << Line::End <<
			"const int index = clamp(station, 0, count - 1);" << Line::End <<
			"const vec4 point = pathPoint(span, index * 2, previous);" << Line::End <<
			"const vec3 normal = pathPoint(span, index * 2 + 1, previous).xyz;" << Line::End <<
			"const vec3 tangent = pathPoint(span, min(index + 1, count - 1) * 2, previous).xyz - pathPoint(span, max(index - 1, 0) * 2, previous).xyz;" << Line::End <<
			"const vec3 binormal = cross(tangent, normal);" << Line::End <<
			"const float binormalLength = length(binormal);" << Line::End <<
			"const vec2 across = beamOffset(point.w, time, shape, motion);" << Line::End <<
			"return (model * vec4(point.xyz + normal * across.x + (binormalLength > 1.0e-12 ? binormal / binormalLength : vec3(0.0)) * across.y, 1.0)).xyz;";

		/* A corner of the ribbon: the station moved across the local tangent, facing the eye, by the half width — clamped
		 * to one pixel, the coverage returned in w. */
		Declaration::Function corner{"beamCorner", GLSL::FloatVector4};
		corner.addInParameter(GLSL::Matrix4, "model");
		corner.addInParameter(GLSL::UIntVector4, "span");
		corner.addInParameter(GLSL::Integer, "vertexIndex");
		corner.addInParameter(GLSL::Float, "time");
		corner.addInParameter(GLSL::FloatVector4, "shape");
		corner.addInParameter(GLSL::FloatVector4, "motion");
		corner.addInParameter(GLSL::FloatVector3, "eye");
		corner.addInParameter(GLSL::FloatVector2, "pixel");
		corner.addInParameter(GLSL::Boolean, "previous");
		corner.addOutParameter(GLSL::FloatVector2, "coordinates");
		Code{corner, Location::Output} <<
			"coordinates = vec2(0.0);" << Line::End <<
			"const int count = int(span.y) / 2;" << Line::End <<
			"const int segment = vertexIndex / " << VerticesPerSegment << ";" << Line::End <<
			"const int corner = vertexIndex - segment * " << VerticesPerSegment << ";" << Line::End <<
			"/* Past the last segment (the capacity exceeds the stations), or no station at all (a hidden beam): collapsed" << Line::End <<
			" * onto the origin without reading anything — pathPoint() clamps to [0, count - 1], undefined for count 0. */" << Line::End <<
			"if ( count < 2 || segment >= count - 1 ) { return vec4(0.0); }" << Line::End <<
			"/* Two triangles: (start, -1) (end, -1) (end, +1), (start, -1) (end, +1) (start, +1). */" << Line::End <<
			"const int index = segment + ((corner == 1 || corner == 2 || corner == 4) ? 1 : 0);" << Line::End <<
			"const float side = (corner == 2 || corner == 4 || corner == 5) ? 1.0 : -1.0;" << Line::End <<
			"const vec3 point = beamStation(model, span, index, time, shape, motion, previous);" << Line::End <<
			"const vec3 tangent = beamStation(model, span, index + 1, time, shape, motion, previous) - beamStation(model, span, index - 1, time, shape, motion, previous);" << Line::End <<
			"const vec3 toEye = eye - point;" << Line::End <<
			"vec3 across = cross(tangent, toEye);" << Line::End <<
			"/* Looking straight down the beam: the station's normal will do. */" << Line::End <<
			"if ( dot(across, across) < 1.0e-20 ) { across = (model * vec4(pathPoint(span, index * 2 + 1, previous).xyz, 0.0)).xyz; }" << Line::End <<
			"across = normalize(across);" << Line::End <<
			"const float halfWidth = shape.x * length(model[1].xyz);" << Line::End <<
			"const float pixelSize = pixel.x * (pixel.y > 0.0 ? length(toEye) : 1.0);" << Line::End <<
			"const float drawnHalfWidth = max(halfWidth, pixelSize);" << Line::End <<
			"const vec3 world = point + across * (side * drawnHalfWidth);" << Line::End <<
			"coordinates = vec2(pathPoint(span, index * 2, previous).w, side);" << Line::End <<
			"return vec4((inverse(model) * vec4(world, 1.0)).xyz, drawnHalfWidth > 0.0 ? halfWidth / drawnHalfWidth : 0.0);";

		if ( !PathGLSL::declarePointFunction(shader) )
		{
			return false;
		}

		return shader.declare(hash) && shader.declare(noise) && shader.declare(fbm) && shader.declare(offset) && shader.declare(station) && shader.declare(corner);
	}
}
