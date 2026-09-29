/*
 * src/Saphir/PathGLSL.hpp
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
#include <cstdint>
#include <string>

#include "Saphir/AbstractShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Keys.hpp"

/*
 * The path ribbon (Scenes::Component::Path, AbstractVertexStage::enablePathRibbon()), built by VERTEX PULLING: the
 * geometry has no vertex buffer (Graphics::Geometry::PulledVertexResource); every vertex reads its points from the
 * scene's path SSBO (bindings 1-2 of the instance-transforms set, Scenes::SceneInstanceTransforms) through the
 * directory entry of its instance slot.
 *
 * NINE vertices per segment, a triangle list: the segment's quad (0-5), then the BEVEL triangle of the join at its
 * start (6-8), collapsed onto the point when unused. Built in WORLD space, facing the eye (the across direction is
 * cross(segment, point → eye), like the beam), then brought back to the object space by inverse(model).
 * - Joins: MITER when its length stays within the miter limit (the SVG rule: 1 / cos(half the turn) ≤ limit), else
 *   the segments end square at the point and the bevel triangle fills the outer gap. A segment and its neighbour
 *   decide their shared join from the same two normals: the miter edges coincide, the join is watertight.
 * - Round mode: the quad is extended by the half width at both ends and the fragment stage discards outside the
 *   capsule (PathCoordinates): round joins AND round caps for no extra geometry (Rougier, JCGT 2013 — the distance
 *   to the segment decides).
 * - Width: in entity units (× the model's scale, built in world space), or in PIXELS (built in SCREEN space: exact).
 * - A vertex past the path's last segment (the geometry's capacity exceeds the frame's points) collapses onto the first
 *   point: zero area.
 */
namespace EmEn::Saphir::PathGLSL
{
	/** @brief Vertices per segment: the quad (6) and the start join's bevel triangle (3). */
	constexpr uint32_t VerticesPerSegment{9};

	/*
	 * The function BODIES, shared by the Saphir-generated scene program (declareFunctions()) and the hand-written
	 * debug overlay (Graphics::PathDebugOverlay, rawFunctions()): one ribbon, two shaders, no second copy to drift.
	 * Both require a storage block `ubPathPoints { vec4 pathPoints[]; }` of {current, previous} pairs.
	 */

	/** @brief vec4 pathPoint (uvec4 span, int index, bool previous): a point (xyz, w the arc length), clamped to its range. */
	constexpr auto PointBody{R"GLSL(
	const uint clamped = uint(clamp(index, 0, int(span.y) - 1));
	return ubPathPoints.pathPoints[(span.x + clamped) * 2u + (previous ? 1u : 0u)];
)GLSL"};

	/** @brief vec3 pathNormal (vec3 direction, vec3 point, vec3 eye): across the direction, facing the eye. */
	constexpr auto NormalBody{R"GLSL(
	vec3 across = cross(direction, eye - point);
	if ( dot(across, across) < 1.0e-20 ) { across = cross(direction, abs(direction.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)); }
	return normalize(across);
)GLSL"};

	/** @brief vec4 pathMiter (vec3 incoming, vec3 outgoing, float limit): (direction, cos of half the turn), w = 0 for a bevel. */
	constexpr auto MiterBody{R"GLSL(
	const vec3 sum = incoming + outgoing;
	const float sumLength = length(sum);
	if ( sumLength < 1.0e-4 ) { return vec4(outgoing, 0.0); }
	const vec3 direction = sum / sumLength;
	const float cosine = dot(direction, outgoing);
	return vec4(direction, cosine >= 1.0 / max(limit, 1.0) ? cosine : 0.0);
)GLSL"};

	/**
	 * @brief vec2 pathScreen (mat4 projection, vec3 viewPoint, vec2 viewport): a view-space point in PIXELS (NDC × half
	 * the viewport); w of the clip position in the return's companion pathClipW().
	 */
	constexpr auto ScreenBody{R"GLSL(
	const vec4 clip = projection * vec4(viewPoint, 1.0);
	return (clip.xy / clip.w) * viewport * 0.5;
)GLSL"};

	/** @brief vec3 pathLift (mat4 projection, vec3 viewPoint, vec2 pixels, vec2 viewport): a pixel offset at a view-space point, as a view-space offset at its depth. */
	constexpr auto LiftBody{R"GLSL(
	const float w = (projection * vec4(viewPoint, 1.0)).w;
	const vec2 ndc = pixels * 2.0 / viewport;
	return vec3(ndc.x * w / projection[0][0], ndc.y * w / projection[1][1], 0.0);
)GLSL"};

	/**
	 * @brief vec3 pathCorner (mat4 model, uvec4 span, int vertexIndex, bool previous, vec4 style, vec3 eye, mat4 view,
	 * mat4 projection, vec2 viewport, out vec4 coordinates).
	 * @note Two constructions. A width in ENTITY UNITS: in world space, across = cross(segment, point → eye) (the beam's).
	 * A width in PIXELS: in SCREEN space — the segment projected into pixels, the normal and the joins decided in 2D,
	 * each offset lifted back to view space at its point's depth — so the line is exactly that many pixels wide
	 * everywhere on screen (Rougier, JCGT 2013). A world-space "pixel × distance" is right on the optical axis only: it
	 * measured 12 px at 4 m for a 6-px request, 6 at 28 m (Windows peer + Linux sweep, 2026-09-29).
	 */
	constexpr auto CornerBody{R"GLSL(
	coordinates = vec4(0.0);
	const int count = int(span.y);
	const int segment = vertexIndex / 9;
	const int corner = vertexIndex - segment * 9;
	/* Past the last segment (the capacity exceeds the points), or no points at all (a hidden path, a path drawn by the
	 * debug overlay): collapsed onto the origin without reading anything — pathPoint() clamps to [0, count - 1], which
	 * GLSL leaves undefined for count 0. */
	if ( count < 2 || segment >= count - 1 ) { return vec3(0.0); }
	const vec3 w0 = (model * vec4(pathPoint(span, segment, previous).xyz, 1.0)).xyz;
	const vec3 w1 = (model * vec4(pathPoint(span, segment + 1, previous).xyz, 1.0)).xyz;
	const float segmentLength = length(w1 - w0);
	if ( segmentLength < 1.0e-6 ) { return pathPoint(span, segment, previous).xyz; }
	const bool roundJoins = style.z > 0.5;
	const bool hasPrevious = segment > 0;
	const bool hasNext = segment + 2 < count;
	const vec3 wp = hasPrevious ? (model * vec4(pathPoint(span, segment - 1, previous).xyz, 1.0)).xyz : w0;
	const vec3 w2 = hasNext ? (model * vec4(pathPoint(span, segment + 2, previous).xyz, 1.0)).xyz : w1;
	vec3 world = w0;

	if ( style.y > 0.5 ) {
		/* PIXELS: screen space. */
		vec3 v0 = (view * vec4(w0, 1.0)).xyz;
		vec3 v1 = (view * vec4(w1, 1.0)).xyz;
		/* Behind the eye: the segment clipped to just in front of it (a point behind has no screen position). */
		const float nearZ = -1.0e-3;
		if ( v0.z > nearZ && v1.z > nearZ ) { return pathPoint(span, segment, previous).xyz; }
		if ( v0.z > nearZ ) { v0 = mix(v0, v1, (v0.z - nearZ) / (v0.z - v1.z)); }
		if ( v1.z > nearZ ) { v1 = mix(v1, v0, (v1.z - nearZ) / (v1.z - v0.z)); }
		const vec2 p0 = pathScreen(projection, v0, viewport);
		const vec2 p1 = pathScreen(projection, v1, viewport);
		const float screenLength = length(p1 - p0);
		const vec2 s = screenLength > 1.0e-4 ? (p1 - p0) / screenLength : vec2(1.0, 0.0);
		const vec2 ns = vec2(-s.y, s.x);
		const float hw = style.x;
		/* The joins, decided in 2D with the neighbours' screen directions. */
		vec2 startNormal = ns;
		vec2 startMiter = ns;
		float startCosine = 0.0;
		if ( !roundJoins && hasPrevious ) {
			const vec3 vp = (view * vec4(wp, 1.0)).xyz;
			if ( vp.z < nearZ && v0.z < nearZ ) {
				const vec2 pp = pathScreen(projection, vp, viewport);
				if ( length(p0 - pp) > 1.0e-4 ) {
					const vec2 sp = normalize(p0 - pp);
					startNormal = vec2(-sp.y, sp.x);
					const vec2 sum = startNormal + ns;
					if ( length(sum) > 1.0e-4 ) { startMiter = normalize(sum); const float c = dot(startMiter, ns); startCosine = c >= 1.0 / max(style.w, 1.0) ? c : 0.0; }
				}
			}
		}
		vec2 endMiter = ns;
		float endCosine = 0.0;
		if ( !roundJoins && hasNext ) {
			const vec3 vn = (view * vec4(w2, 1.0)).xyz;
			if ( vn.z < nearZ && v1.z < nearZ ) {
				const vec2 pn = pathScreen(projection, vn, viewport);
				if ( length(pn - p1) > 1.0e-4 ) {
					const vec2 sn = normalize(pn - p1);
					const vec2 sum = ns + vec2(-sn.y, sn.x);
					if ( length(sum) > 1.0e-4 ) { endMiter = normalize(sum); const float c = dot(endMiter, ns); endCosine = c >= 1.0 / max(style.w, 1.0) ? c : 0.0; }
				}
			}
		}
		vec3 viewPoint = v0;
		vec2 offset = vec2(0.0);
		if ( corner < 6 ) {
			const bool atEnd = corner == 2 || corner == 3 || corner == 5;
			const float side = (corner == 1 || corner == 4 || corner == 5) ? 1.0 : -1.0;
			const float cosine = atEnd ? endCosine : startCosine;
			offset = side * ns * hw;
			if ( roundJoins ) { offset += (atEnd ? s : -s) * hw; }
			else if ( cosine > 0.0 ) { offset = side * (atEnd ? endMiter : startMiter) * (hw / cosine); }
			viewPoint = atEnd ? v1 : v0;
			const vec2 corner2D = (atEnd ? p1 : p0) + offset;
			coordinates = vec4(dot(corner2D - p0, s), side * hw, screenLength, hw);
		} else if ( !roundJoins && hasPrevious && startCosine == 0.0 ) {
			const float outer = dot(s, startNormal) > 0.0 ? -1.0 : 1.0;
			if ( corner == 7 ) { offset = outer * startNormal * hw; }
			else if ( corner == 8 ) { offset = outer * ns * hw; }
			coordinates = vec4(0.0, 0.0, screenLength, hw);
		}
		/* The pixel offset lifted to view space at the point's depth, then to world (the view matrix is rigid). */
		const vec3 viewCorner = viewPoint + pathLift(projection, viewPoint, offset, viewport);
		world = (transpose(mat3(view)) * (viewCorner - view[3].xyz));
	} else {
		/* ENTITY UNITS: world space, facing the eye. */
		const vec3 d = (w1 - w0) / segmentLength;
		const float hw0 = style.x * length(model[1].xyz);
		const float hw1 = hw0;
		const vec3 n0 = pathNormal(d, w0, eye);
		const vec3 n1 = pathNormal(d, w1, eye);
		vec4 startMiter = vec4(n0, 0.0);
		vec3 previousNormal = n0;
		if ( !roundJoins && hasPrevious ) {
			const vec3 incoming = w0 - wp;
			if ( dot(incoming, incoming) > 1.0e-12 ) { previousNormal = pathNormal(normalize(incoming), w0, eye); startMiter = pathMiter(previousNormal, n0, style.w); }
		}
		vec4 endMiter = vec4(n1, 0.0);
		if ( !roundJoins && hasNext ) {
			const vec3 outgoing = w2 - w1;
			if ( dot(outgoing, outgoing) > 1.0e-12 ) { endMiter = pathMiter(n1, pathNormal(normalize(outgoing), w1, eye), style.w); }
		}
		if ( corner < 6 ) {
			/* The quad: 0 start-, 1 start+, 2 end-, 3 end-, 4 start+, 5 end+. */
			const bool atEnd = corner == 2 || corner == 3 || corner == 5;
			const float side = (corner == 1 || corner == 4 || corner == 5) ? 1.0 : -1.0;
			const vec3 base = atEnd ? w1 : w0;
			const vec3 across = atEnd ? n1 : n0;
			const float hw = atEnd ? hw1 : hw0;
			const vec4 join = atEnd ? endMiter : startMiter;
			vec3 offset = side * across * hw;
			if ( roundJoins ) { offset += (atEnd ? d : -d) * hw; }
			else if ( join.w > 0.0 ) { offset = side * join.xyz * (hw / join.w); }
			world = base + offset;
			coordinates = vec4(dot(world - w0, d), side * hw, segmentLength, hw);
		} else if ( !roundJoins && hasPrevious && startMiter.w == 0.0 ) {
			/* The bevel on the OUTER side of the turn: opposite to the side the path turns toward. */
			const float outer = dot(d, previousNormal) > 0.0 ? -1.0 : 1.0;
			if ( corner == 7 ) { world = w0 + outer * previousNormal * hw0; }
			else if ( corner == 8 ) { world = w0 + outer * n0 * hw0; }
			coordinates = vec4(0.0, 0.0, segmentLength, hw0);
		}
	}
	return (inverse(model) * vec4(world, 1.0)).xyz;
)GLSL"};

	/**
	 * @brief The round joins and caps: the fragment test (a GLSL statement block) discarding outside the capsule.
	 * @note Expects `coordinates` (along, across, length, half width) and `style` in scope under the given names.
	 * @param coordinates The GLSL expression of the interpolated coordinates.
	 * @param style The GLSL expression of the style vector.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	roundDiscard (const std::string & coordinates, const std::string & style) noexcept
	{
		return
			"if ( " + style + ".z > 0.5 )\n"
			"{\n"
			"	const float pathAlong = " + coordinates + ".x < 0.0 ? " + coordinates + ".x : max(" + coordinates + ".x - " + coordinates + ".z, 0.0);\n"
			"	if ( length(vec2(pathAlong, " + coordinates + ".y)) > " + coordinates + ".w ) { discard; }\n"
			"}\n";
	}

	/**
	 * @brief Returns the functions as plain GLSL, for a hand-written shader (the debug overlay).
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	rawFunctions () noexcept
	{
		return
			std::string{"vec4 pathPoint (uvec4 span, int index, bool previous)\n{"} + PointBody + "}\n\n" +
			"vec3 pathNormal (vec3 direction, vec3 point, vec3 eye)\n{" + NormalBody + "}\n\n" +
			"vec4 pathMiter (vec3 incoming, vec3 outgoing, float limit)\n{" + MiterBody + "}\n\n" +
			"vec2 pathScreen (mat4 projection, vec3 viewPoint, vec2 viewport)\n{" + ScreenBody + "}\n\n" +
			"vec3 pathLift (mat4 projection, vec3 viewPoint, vec2 pixels, vec2 viewport)\n{" + LiftBody + "}\n\n" +
			"vec3 pathCorner (mat4 model, uvec4 span, int vertexIndex, bool previous, vec4 style, vec3 eye, mat4 view, mat4 projection, vec2 viewport, out vec4 coordinates)\n{" + CornerBody + "}\n\n";
	}

	/**
	 * @brief Declares the path GLSL functions in a Saphir shader.
	 * @note pathCorner(...) returns the object-space position; `style` is the Material::PathResource style vector (Keys
	 * PathStyle); `view`, `projection` (unjittered as the scene pass uses it: the jitter is a push constant on
	 * gl_Position) and `viewport` (pixels) build the pixel mode; `coordinates` receives (along, across, segment length,
	 * half width) — world units, or pixels in the pixel mode.
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

		Declaration::Function point{"pathPoint", GLSL::FloatVector4};
		point.addInParameter(GLSL::UIntVector4, "span");
		point.addInParameter(GLSL::Integer, "index");
		point.addInParameter(GLSL::Boolean, "previous");
		Code{point, Location::Output} << PointBody;

		Declaration::Function normal{"pathNormal", GLSL::FloatVector3};
		normal.addInParameter(GLSL::FloatVector3, "direction");
		normal.addInParameter(GLSL::FloatVector3, "point");
		normal.addInParameter(GLSL::FloatVector3, "eye");
		Code{normal, Location::Output} << NormalBody;

		Declaration::Function miter{"pathMiter", GLSL::FloatVector4};
		miter.addInParameter(GLSL::FloatVector3, "incoming");
		miter.addInParameter(GLSL::FloatVector3, "outgoing");
		miter.addInParameter(GLSL::Float, "limit");
		Code{miter, Location::Output} << MiterBody;

		Declaration::Function screen{"pathScreen", GLSL::FloatVector2};
		screen.addInParameter(GLSL::Matrix4, "projection");
		screen.addInParameter(GLSL::FloatVector3, "viewPoint");
		screen.addInParameter(GLSL::FloatVector2, "viewport");
		Code{screen, Location::Output} << ScreenBody;

		Declaration::Function lift{"pathLift", GLSL::FloatVector3};
		lift.addInParameter(GLSL::Matrix4, "projection");
		lift.addInParameter(GLSL::FloatVector3, "viewPoint");
		lift.addInParameter(GLSL::FloatVector2, "pixels");
		lift.addInParameter(GLSL::FloatVector2, "viewport");
		Code{lift, Location::Output} << LiftBody;

		Declaration::Function corner{"pathCorner", GLSL::FloatVector3};
		corner.addInParameter(GLSL::Matrix4, "model");
		corner.addInParameter(GLSL::UIntVector4, "span");
		corner.addInParameter(GLSL::Integer, "vertexIndex");
		corner.addInParameter(GLSL::Boolean, "previous");
		corner.addInParameter(GLSL::FloatVector4, "style");
		corner.addInParameter(GLSL::FloatVector3, "eye");
		corner.addInParameter(GLSL::Matrix4, "view");
		corner.addInParameter(GLSL::Matrix4, "projection");
		corner.addInParameter(GLSL::FloatVector2, "viewport");
		corner.addOutParameter(GLSL::FloatVector4, "coordinates");
		Code{corner, Location::Output} << CornerBody;

		return shader.declare(point) && shader.declare(normal) && shader.declare(miter) && shader.declare(screen) && shader.declare(lift) && shader.declare(corner);
	}
}
