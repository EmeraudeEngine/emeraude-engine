/*
 * src/Saphir/ImposterGLSL.hpp
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
 * @brief The GLSL side of the hemi-octahedral imposter mapping.
 * @note ⚠️⚠️ A SECOND implementation of emeraude-base `Math/OctahedralMapping.hpp` — hemiOctahedralEncode(),
 * hemiOctahedralDecode(), hemiOctahedralCellDirection() and imposterCellFrame() — transcribed LINE BY LINE. The
 * baker (Scenes::Toolkit::bakeTreeImposter()) places its views with the C++ one, the billboard and the material
 * read them back with this one: if they disagree by a cell or by a sign, the imposter shows a neighbouring view or
 * a mirrored one. The unit tests (`test_MathOctahedralMapping.cpp`) guard the C++ side only — change both at once.
 */
namespace EmEn::Saphir::ImposterGLSL
{
	/**
	 * @brief Declares the four functions in a shader: imposterHemiEncode(vec3), imposterHemiDecode(vec2),
	 * imposterCellFrame(vec3), imposterCellDirection(vec2 cell, float gridSize) and
	 * imposterAtlasCoordinates(vec2 cell, vec3 offset, float radius, float gridSize).
	 * @note Pure maths: no sampler (functions are emitted before the samplers).
	 * @param shader A reference to the shader.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	declareFunctions (AbstractShader & shader) noexcept
	{
		using namespace Keys;

		/* hemiOctahedralEncode(): the upper octahedron turned by 45°, a direction below the horizon clamped onto it. */
		Declaration::Function encode{"imposterHemiEncode", GLSL::FloatVector2};
		encode.addInParameter(GLSL::FloatVector3, "direction");
		Code{encode, Location::Output} <<
			"const float clampedY = max(direction.y, 0.0);" << Line::End <<
			"const float norm = abs(direction.x) + clampedY + abs(direction.z);" << Line::End <<
			"if ( norm <= 0.0 ) { return vec2(0.5); }" << Line::End <<
			"const float projectedX = direction.x / norm;" << Line::End <<
			"const float projectedZ = direction.z / norm;" << Line::End <<
			"return vec2(projectedX + projectedZ, projectedZ - projectedX) * 0.5 + 0.5;";

		/* hemiOctahedralDecode(). */
		Declaration::Function decode{"imposterHemiDecode", GLSL::FloatVector3};
		decode.addInParameter(GLSL::FloatVector2, "point");
		Code{decode, Location::Output} <<
			"const float u = point.x * 2.0 - 1.0;" << Line::End <<
			"const float v = point.y * 2.0 - 1.0;" << Line::End <<
			"const float resultX = (u - v) * 0.5;" << Line::End <<
			"const float resultZ = (u + v) * 0.5;" << Line::End <<
			"const float resultY = max(1.0 - abs(resultX) - abs(resultZ), 0.0);" << Line::End <<
			"return normalize(vec3(resultX, resultY, resultZ));";

		/* imposterCellFrame(): mat3(right, up, back), back toward the camera, up the object +Y made orthogonal. */
		Declaration::Function frame{"imposterCellFrame", GLSL::Matrix3};
		frame.addInParameter(GLSL::FloatVector3, "direction");
		Code{frame, Location::Output} <<
			"const vec3 back = normalize(direction);" << Line::End <<
			"const vec3 reference = abs(back.y) >= 0.9999 ? vec3(0.0, 0.0, -1.0) : vec3(0.0, 1.0, 0.0);" << Line::End <<
			"const vec3 up = normalize(reference - back * dot(reference, back));" << Line::End <<
			"return mat3(cross(up, back), up, back);";

		/* hemiOctahedralCellDirection(): the cell centres on the (gridSize - 1) lattice. */
		Declaration::Function cellDirection{"imposterCellDirection", GLSL::FloatVector3};
		cellDirection.addInParameter(GLSL::FloatVector2, "cell");
		cellDirection.addInParameter(GLSL::Float, "gridSize");
		Code{cellDirection, Location::Output} <<
			"return imposterHemiDecode(cell / (gridSize - 1.0));";

		/* The atlas coordinates of an object-space offset from the bounding-sphere centre, in one view: the point
		 * projected along THAT view's direction onto its cell (the bake is orthographic), then placed in the cell —
		 * column cell.x, row cell.y from the TOP of the atlas, the image of the view's +up. */
		Declaration::Function atlasCoordinates{"imposterAtlasCoordinates", GLSL::FloatVector2};
		atlasCoordinates.addInParameter(GLSL::FloatVector2, "cell");
		atlasCoordinates.addInParameter(GLSL::FloatVector3, "offset");
		atlasCoordinates.addInParameter(GLSL::Float, "radius");
		atlasCoordinates.addInParameter(GLSL::Float, "gridSize");
		Code{atlasCoordinates, Location::Output} <<
			"const mat3 view = imposterCellFrame(imposterCellDirection(cell, gridSize));" << Line::End <<
			"const vec2 local = vec2(dot(offset, view[0]), dot(offset, view[1])) / radius;" << Line::End <<
			"return (cell + 0.5 + vec2(0.5, -0.5) * local) / gridSize;";

		return shader.declare(encode) && shader.declare(decode) && shader.declare(frame) && shader.declare(cellDirection) && shader.declare(atlasCoordinates);
	}
}
