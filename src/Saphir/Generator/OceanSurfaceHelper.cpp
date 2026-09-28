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
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Graphics::Geometry;

	std::shared_ptr< Vulkan::DescriptorSetLayout >
	getOceanSurfaceDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept
	{
		static constexpr auto UUID{"OceanSurface"};

		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(UUID);

		if ( descriptorSetLayout == nullptr )
		{
			constexpr VkShaderStageFlags Stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(UUID);
			descriptorSetLayout->setIdentifier("OceanSurface", "Cascades", "DescriptorSetLayout");
			descriptorSetLayout->declareCombinedImageSampler(OceanSurface::DisplacementBinding, Stages);
			descriptorSetLayout->declareCombinedImageSampler(OceanSurface::SlopesBinding, Stages);
			descriptorSetLayout->declareUniformBuffer(OceanSurface::UniformsBinding, Stages);
			descriptorSetLayout->declareCombinedImageSampler(OceanSurface::FoamBinding, Stages);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

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

		if ( fragmentStage && !shader.declare(Declaration::Sampler{setIndex, OceanSurface::FoamBinding, GLSL::Sampler2DArray, OceanSurface::FoamSamplerName}) )
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

		if ( fragmentStage )
		{
			/* ⚠️ The foam is read through a cubic B-spline, not the sampler's bilinear filter: the 512 m cascade stores it at
			 * 2 m per texel, and a bilinear field compared to a threshold draws POLYGONS — the close whitecaps were
			 * straight-edged scraps. Third-order filtering in four bilinear taps (C. Sigg, M. Hadwiger, "Fast Third-Order
			 * Texture Filtering", GPU Gems 2, ch. 20, 2005); the REPEAT sampler wraps the taps. */
			Declaration::Function function{"ocFoamBSpline", GLSL::Float};
			function.addInParameter(GLSL::FloatVector3, "uvw");
			function.addInstruction(
				"\t" "const vec2 size = vec2(textureSize(ocFoam, 0).xy);" "\n"
				"\t" "const vec2 st = uvw.xy * size - 0.5;" "\n"
				"\t" "const vec2 i = floor(st);" "\n"
				"\t" "const vec2 f = st - i;" "\n"
				"\t" "const vec2 w0 = (1.0 - f) * (1.0 - f) * (1.0 - f) / 6.0;" "\n"
				"\t" "const vec2 w1 = (3.0 * f * f * f - 6.0 * f * f + 4.0) / 6.0;" "\n"
				"\t" "const vec2 w3 = f * f * f / 6.0;" "\n"
				"\t" "const vec2 w2 = 1.0 - w0 - w1 - w3;" "\n"
				"\t" "const vec2 g0 = w0 + w1;" "\n"
				"\t" "const vec2 g1 = w2 + w3;" "\n"
				"\t" "const vec2 p0 = (i - 0.5 + w1 / g0) / size;" "\n"
				"\t" "const vec2 p1 = (i + 1.5 + w3 / g1) / size;" "\n"
				"\t" "return g0.y * (g0.x * textureLod(ocFoam, vec3(p0.x, p0.y, uvw.z), 0.0).r + g1.x * textureLod(ocFoam, vec3(p1.x, p0.y, uvw.z), 0.0).r)" "\n"
				"\t" "     + g1.y * (g0.x * textureLod(ocFoam, vec3(p0.x, p1.y, uvw.z), 0.0).r + g1.x * textureLod(ocFoam, vec3(p1.x, p1.y, uvw.z), 0.0).r);" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		if ( fragmentStage )
		{
			/* The whitecap coverage: each cascade's accumulated foam (Graphics::OceanWaves), faded like its waves where the
			 * pixel cannot hold it, the cascades united as independent coverages: 1 − Π (1 − foam). */
			Declaration::Function function{OceanSurface::WhitecapFunction, GLSL::Float};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInstruction(
				"\t" "const vec2 footprint = max(abs(dFdx(xz)), abs(dFdy(xz)));" "\n"
				"\t" "const float step = max(footprint.x, footprint.y);" "\n"
				"\t" "float clear = 1.0;" "\n"
				"\t" "for ( int cascade = 0; cascade < int(ocSurface.cascades.w); ++cascade )" "\n"
				"\t" "{" "\n"
				"\t\t" "const float foam = ocFoamBSpline(vec3(xz / ocSurface.cascades[cascade], float(cascade)));" "\n"
				"\t\t" "clear *= 1.0 - clamp(ocCascadeWeight(cascade, step) * foam, 0.0, 1.0);" "\n"
				"\t" "}" "\n"
				"\t" "return 1.0 - clear;" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		return true;
	}
}
