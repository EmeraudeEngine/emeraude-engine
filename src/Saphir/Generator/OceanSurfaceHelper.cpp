/*
 * src/Saphir/Generator/OceanSurfaceHelper.cpp
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

#include "OceanSurfaceHelper.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions. */
#include "Graphics/Geometry/OceanSurface.hpp"
#include "Graphics/OceanWaves.hpp"
#include "Saphir/AbstractShader.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Graphics::Geometry;

	bool
	declareOceanSurface (AbstractShader & shader, uint32_t setIndex, bool fragmentStage) noexcept
	{
		using namespace Keys;

		if ( !shader.declare(Declaration::Sampler{setIndex, OceanSurface::DisplacementBinding, GLSL::Sampler2DArray, OceanSurface::DisplacementSamplerName}) )
		{
			return false;
		}

		if ( !shader.declare(Declaration::Sampler{setIndex, OceanSurface::SlopesBinding, GLSL::Sampler2DArray, OceanSurface::SlopesSamplerName}) )
		{
			return false;
		}

		/* Mirror of Graphics::Geometry::OceanSurface::Uniforms (std140, vec4 members only). */
		Declaration::UniformBlock block{setIndex, OceanSurface::UniformsBinding, Declaration::MemoryLayout::Std140, OceanSurface::UniformBlockName, OceanSurface::UniformBlockInstance};
		block.addMember(Declaration::VariableType::FloatVector4, "grid");
		block.addMember(Declaration::VariableType::FloatVector4, "level");
		block.addMember(Declaration::VariableType::FloatVector4, "cascades");
		block.addMember(Declaration::VariableType::FloatVector4, "textureCoordinates");
		block.addArrayMember(Declaration::VariableType::FloatVector4, "levelsOfDetail", OceanSurface::MaxLevelsOfDetail);

		if ( !shader.declare(block) )
		{
			return false;
		}

		const auto texels = std::to_string(Graphics::OceanWaves::Resolution) + ".0";

		/* A cascade whose texel is smaller than half the sampling step cannot be represented there: it fades out between
		 * 2 and 4 texels per step, or the far rings (and the far pixels, without mipmaps) alias its waves into shimmer. */
		{
			Declaration::Function function{"ocCascadeWeight", GLSL::Float};
			function.addInParameter(GLSL::Integer, "cascade");
			function.addInParameter(GLSL::Float, "step");
			function.addInstruction(
				"\t" "const float texel = ocSurface.cascades[cascade] / " + texels + ";" "\n"
				"\t" "return 1.0 - smoothstep(2.0 * texel, 4.0 * texel, step);" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		/* The displacement (λ Dx, h, λ Dz) summed over the cascades at an undisplaced world XZ. */
		{
			Declaration::Function function{"ocDisplacementAt", GLSL::FloatVector3};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInParameter(GLSL::Float, "step");
			function.addInstruction(
				"\t" "vec3 sum = vec3(0.0);" "\n"
				"\t" "for ( int cascade = 0; cascade < int(ocSurface.cascades.w); ++cascade )" "\n"
				"\t" "{" "\n"
				"\t\t" "sum += ocCascadeWeight(cascade, step) * textureLod(ocDisplacement, vec3(xz / ocSurface.cascades[cascade], float(cascade)), 0.0).xyz;" "\n"
				"\t" "}" "\n"
				"\t" "return sum;" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		/* The choppy surface's normal (Tessendorf § 4.4): its tangents along the lattice axes are
		 * (1 + λ ∂Dx/∂x, ∂h/∂x, λ ∂Dz/∂x) and (λ ∂Dx/∂z, ∂h/∂z, 1 + λ ∂Dz/∂z), and ∂Dz/∂x = ∂Dx/∂z. */
		{
			Declaration::Function function{"ocNormalAt", GLSL::FloatVector3};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInParameter(GLSL::Float, "step");
			function.addInstruction(
				"\t" "vec4 slopes = vec4(0.0);" "\n"
				"\t" "float crossTerm = 0.0;" "\n"
				"\t" "for ( int cascade = 0; cascade < int(ocSurface.cascades.w); ++cascade )" "\n"
				"\t" "{" "\n"
				"\t\t" "const vec3 uvw = vec3(xz / ocSurface.cascades[cascade], float(cascade));" "\n"
				"\t\t" "const float weight = ocCascadeWeight(cascade, step);" "\n"
				"\t\t" "slopes += weight * textureLod(ocSlopes, uvw, 0.0);" "\n"
				"\t\t" "crossTerm += weight * textureLod(ocDisplacement, uvw, 0.0).w;" "\n"
				"\t" "}" "\n"
				"\t" "const vec3 tangentX = vec3(1.0 + slopes.z, slopes.x, crossTerm);" "\n"
				"\t" "const vec3 tangentZ = vec3(crossTerm, slopes.y, 1.0 + slopes.w);" "\n"
				"\t" "return normalize(cross(tangentZ, tangentX));" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		if ( fragmentStage )
		{
			/* The per-pixel normal the heightfield pixel frame asks for: the step is the pixel's footprint on the sea. */
			Declaration::Function function{"hfPixelNormalAt", GLSL::FloatVector3};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInstruction(
				"\t" "const vec2 footprint = max(abs(dFdx(xz)), abs(dFdy(xz)));" "\n"
				"\t" "return ocNormalAt(xz, max(footprint.x, footprint.y));" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		return true;
	}
}
