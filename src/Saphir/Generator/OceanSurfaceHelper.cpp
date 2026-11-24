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

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "Graphics/Geometry/OceanSurface.hpp"
#include "Graphics/OceanWaves.hpp"
#include "Saphir/AbstractShader.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/Types.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/Keys.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace
{
	using namespace EmEn;
	using namespace EmEn::Saphir;
	using namespace EmEn::Graphics::Geometry;
	using Generator::OceanSurfaceStandingBindings;

	/* THE ONE DESCRIPTION OF WHERE AN OCEAN SURFACE STANDS (owner decision 2026-11-24, engine item
	 * ocean-sheltered-water): each declaration below is built here once, for a Saphir shader (declareOceanSurface())
	 * as for a hand-written one (getOceanSurfaceStandingSource()). */

	/**
	 * @brief The displacement cascades.
	 * @param bindings Where the shader finds them.
	 * @return Declaration::Sampler
	 */
	[[nodiscard]]
	Declaration::Sampler
	displacementSampler (const OceanSurfaceStandingBindings & bindings) noexcept
	{
		return {bindings.set, bindings.displacement, Keys::GLSL::Sampler2DArray, OceanSurface::DisplacementSamplerName};
	}

	/**
	 * @brief The shelter map.
	 * @param bindings Where the shader finds it.
	 * @return Declaration::Sampler
	 */
	[[nodiscard]]
	Declaration::Sampler
	shelterSampler (const OceanSurfaceStandingBindings & bindings) noexcept
	{
		return {bindings.set, bindings.shelter, Keys::GLSL::Sampler2D, OceanSurface::ShelterSamplerName};
	}

	/**
	 * @brief The per-frame uniform block: the mirror of Graphics::Geometry::OceanSurface::Uniforms (std140, vec4 members
	 * only).
	 * @param bindings Where the shader finds it.
	 * @return Declaration::UniformBlock
	 */
	[[nodiscard]]
	Declaration::UniformBlock
	uniformBlock (const OceanSurfaceStandingBindings & bindings) noexcept
	{
		Declaration::UniformBlock block{bindings.set, bindings.uniforms, Declaration::MemoryLayout::Std140, OceanSurface::UniformBlockName, OceanSurface::UniformBlockInstance};
		block.addMember(Declaration::VariableType::FloatVector4, "grid");
		block.addMember(Declaration::VariableType::FloatVector4, "level");
		block.addMember(Declaration::VariableType::FloatVector4, "cascades");
		block.addMember(Declaration::VariableType::FloatVector4, "textureCoordinates");
		block.addMember(Declaration::VariableType::FloatVector4, "shelter");
		block.addMember(Declaration::VariableType::FloatVector4, "shelteredShares");
		block.addArrayMember(Declaration::VariableType::FloatVector4, "levelsOfDetail", OceanSurface::MaxLevelsOfDetail);

		return block;
	}

	/**
	 * @brief `float ocShelterAt(vec2 xz)`: how open the water is at an undisplaced world XZ — 1 the open sea, 0 a water
	 * sheltered by land (the shelter map; its edge holds outside its rectangle).
	 * @return Declaration::Function
	 */
	[[nodiscard]]
	Declaration::Function
	shelterFunction () noexcept
	{
		Declaration::Function function{"ocShelterAt", Keys::GLSL::Float};
		function.addInParameter(Keys::GLSL::FloatVector2, "xz");
		function.addInstruction(
			"\t" "return textureLod(ocShelter, (xz - ocSurface.shelter.xy) * ocSurface.shelter.zw, 0.0).r;" "\n"
		);

		return function;
	}

	/**
	 * @brief `float ocCascadeWeight(int cascade, float step, float open)`: what a read weighs a cascade by.
	 * @note A cascade whose texel is smaller than half the sampling step cannot be represented there: it fades out
	 * between 2 and 4 texels per step, or the far rings (and the far pixels, without mipmaps) alias its waves into
	 * shimmer. And a sheltered water (open = 0, ocShelterAt()) keeps only the cascade's sheltered share.
	 * @return Declaration::Function
	 */
	[[nodiscard]]
	Declaration::Function
	cascadeWeightFunction () noexcept
	{
		const auto texels = std::to_string(Graphics::OceanWaves::Resolution) + ".0";

		Declaration::Function function{"ocCascadeWeight", Keys::GLSL::Float};
		function.addInParameter(Keys::GLSL::Integer, "cascade");
		function.addInParameter(Keys::GLSL::Float, "step");
		function.addInParameter(Keys::GLSL::Float, "open");
		function.addInstruction(
			"\t" "const float texel = ocSurface.cascades[cascade] / " + texels + ";" "\n"
			"\t" "return (1.0 - smoothstep(2.0 * texel, 4.0 * texel, step)) * mix(ocSurface.shelteredShares[cascade], 1.0, open);" "\n"
		);

		return function;
	}

	/** @brief A sum of displacement cascades: the GLSL function's name and the cascades it reads. */
	struct DisplacementSum final
	{
		const char * function;
		const char * sampler;
	};

	/**
	 * @brief A function summing a displacement texture (λ Dx, h, λ Dz) over the cascades at an undisplaced world XZ.
	 * @note A cascade the read weighs by nothing is not fetched.
	 * @param sum The function's name and the displacement cascades it reads.
	 * @return Declaration::Function
	 */
	[[nodiscard]]
	Declaration::Function
	displacementSumFunction (const DisplacementSum & sum) noexcept
	{
		Declaration::Function function{sum.function, Keys::GLSL::FloatVector3};
		function.addInParameter(Keys::GLSL::FloatVector2, "xz");
		function.addInParameter(Keys::GLSL::Float, "step");
		function.addInstruction(
			"\t" "const float open = ocShelterAt(xz);" "\n"
			"\t" "vec3 sum = vec3(0.0);" "\n"
			"\t" "for ( int cascade = 0; cascade < int(ocSurface.cascades.w); ++cascade )" "\n"
			"\t" "{" "\n"
			"\t\t" "const float weight = ocCascadeWeight(cascade, step, open);" "\n"
			"\t\t" "if ( weight > 0.0 )" "\n"
			"\t\t" "{" "\n"
			"\t\t\t" "sum += weight * textureLod(" + std::string{sum.sampler} + ", vec3(xz / ocSurface.cascades[cascade], float(cascade)), 0.0).xyz;" "\n"
			"\t\t" "}" "\n"
			"\t" "}" "\n"
			"\t" "return sum;" "\n"
		);

		return function;
	}

	/**
	 * @brief `float ocSurfaceRiseAt(vec2 worldXZ)`: how high the DRAWN surface stands above the sea level over a world
	 * XZ, around the eye.
	 * @note The finest level's mesh (Graphics::Geometry::OceanSurface, THE MESH): the displacement at the vertices of
	 * its cells, flat in between, each cell cut along its (0,0)-(1,1) diagonal. The answer is read on the displaced
	 * triangle that covers the point, found by walking from the cell a first guess lands in. A folded cell (a crest
	 * that breaks over itself) covers nothing a point can be stood against: its first corner's height is answered.
	 * @return Declaration::Function
	 */
	[[nodiscard]]
	Declaration::Function
	surfaceRiseFunction () noexcept
	{
		Declaration::Function function{OceanSurface::SurfaceRiseFunction, Keys::GLSL::Float};
		function.addInParameter(Keys::GLSL::FloatVector2, "worldXZ");
		function.addInstruction(
			"\t" "const float cell = ocSurface.grid.x;" "\n"
			"\t" "/* The lattice point that lands on worldXZ, to a first guess: two rounds of x - D(x). */" "\n"
			"\t" "vec2 guess = worldXZ - ocDisplacementAt(worldXZ, cell).xz;" "\n"
			"\t" "guess = worldXZ - ocDisplacementAt(guess, cell).xz;" "\n"
			"\t" "vec2 base = floor(guess / cell);" "\n"
			"\t" "float rise = 0.0;" "\n"
			"\t" "for ( int walk = 0; walk < 4; ++walk )" "\n"
			"\t" "{" "\n"
			"\t\t" "const vec3 d00 = ocDisplacementAt(base * cell, cell);" "\n"
			"\t\t" "const vec3 d10 = ocDisplacementAt((base + vec2(1.0, 0.0)) * cell, cell);" "\n"
			"\t\t" "const vec3 d01 = ocDisplacementAt((base + vec2(0.0, 1.0)) * cell, cell);" "\n"
			"\t\t" "const vec3 d11 = ocDisplacementAt((base + vec2(1.0, 1.0)) * cell, cell);" "\n"
			"\t\t" "/* The cell's corners where the mesh puts them, and the point seen from the first one. */" "\n"
			"\t\t" "const vec2 p00 = (base * cell) + d00.xz;" "\n"
			"\t\t" "const vec2 p10 = ((base + vec2(1.0, 0.0)) * cell) + d10.xz;" "\n"
			"\t\t" "const vec2 p01 = ((base + vec2(0.0, 1.0)) * cell) + d01.xz;" "\n"
			"\t\t" "const vec2 p11 = ((base + vec2(1.0, 1.0)) * cell) + d11.xz;" "\n"
			"\t\t" "const vec2 point = worldXZ - p00;" "\n"
			"\t\t" "/* The triangle (0,0) (0,1) (1,1): point = v (p01 - p00) + u (p11 - p01). */" "\n"
			"\t\t" "const vec2 upperV = p01 - p00;" "\n"
			"\t\t" "const vec2 upperU = p11 - p01;" "\n"
			"\t\t" "const float upperArea = (upperV.x * upperU.y) - (upperV.y * upperU.x);" "\n"
			"\t\t" "/* The triangle (0,0) (1,0) (1,1): point = u (p10 - p00) + v (p11 - p10). */" "\n"
			"\t\t" "const vec2 lowerU = p10 - p00;" "\n"
			"\t\t" "const vec2 lowerV = p11 - p10;" "\n"
			"\t\t" "const float lowerArea = (lowerU.x * lowerV.y) - (lowerU.y * lowerV.x);" "\n"
			"\t\t" "if ( abs(upperArea) < 1.0e-9 || abs(lowerArea) < 1.0e-9 )" "\n"
			"\t\t" "{" "\n"
			"\t\t\t" "return d00.y;" "\n"
			"\t\t" "}" "\n"
			"\t\t" "/* (u, v) in each triangle's own frame. */" "\n"
			"\t\t" "const vec2 upper = vec2((upperV.x * point.y) - (upperV.y * point.x), (point.x * upperU.y) - (point.y * upperU.x)) / upperArea;" "\n"
			"\t\t" "const vec2 lower = vec2((point.x * lowerV.y) - (point.y * lowerV.x), (lowerU.x * point.y) - (lowerU.y * point.x)) / lowerArea;" "\n"
			"\t\t" "const bool inUpper = upper.y >= upper.x;" "\n"
			"\t\t" "const vec2 local = inUpper ? upper : lower;" "\n"
			"\t\t" "rise = inUpper" "\n"
			"\t\t\t" "? d00.y + ((d01.y - d00.y) * local.y) + ((d11.y - d01.y) * local.x)" "\n"
			"\t\t\t" ": d00.y + ((d10.y - d00.y) * local.x) + ((d11.y - d10.y) * local.y);" "\n"
			"\t\t" "const vec2 move = floor(local);" "\n"
			"\t\t" "if ( move == vec2(0.0) )" "\n"
			"\t\t" "{" "\n"
			"\t\t\t" "break;" "\n"
			"\t\t" "}" "\n"
			"\t\t" "base += clamp(move, vec2(-2.0), vec2(2.0));" "\n"
			"\t" "}" "\n"
			"\t" "return rise;" "\n"
		);

		return function;
	}
}

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
			descriptorSetLayout->declareCombinedImageSampler(OceanSurface::PreviousDisplacementBinding, Stages);
			descriptorSetLayout->declareCombinedImageSampler(OceanSurface::ShelterBinding, Stages);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	std::string
	getOceanSurfaceStandingSource (const OceanSurfaceStandingBindings & bindings) noexcept
	{
		return
			displacementSampler(bindings).sourceCode() +
			shelterSampler(bindings).sourceCode() +
			uniformBlock(bindings).sourceCode() +
			shelterFunction().sourceCode() +
			cascadeWeightFunction().sourceCode() +
			displacementSumFunction({.function = "ocDisplacementAt", .sampler = OceanSurface::DisplacementSamplerName}).sourceCode() +
			surfaceRiseFunction().sourceCode();
	}

	bool
	declareOceanSurface (AbstractShader & shader, uint32_t setIndex, bool fragmentStage) noexcept
	{
		using namespace Keys;

		const OceanSurfaceStandingBindings bindings{
			.set = setIndex,
			.displacement = OceanSurface::DisplacementBinding,
			.shelter = OceanSurface::ShelterBinding,
			.uniforms = OceanSurface::UniformsBinding
		};

		if ( !shader.declare(displacementSampler(bindings)) || !shader.declare(shelterSampler(bindings)) )
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

		if ( !fragmentStage && !shader.declare(Declaration::Sampler{setIndex, OceanSurface::PreviousDisplacementBinding, GLSL::Sampler2DArray, OceanSurface::PreviousDisplacementSamplerName}) )
		{
			return false;
		}

		if ( !shader.declare(uniformBlock(bindings)) || !shader.declare(shelterFunction()) || !shader.declare(cascadeWeightFunction()) )
		{
			return false;
		}

		/* The PREVIOUS frame's displacement, the same sum: the velocity's previous position (vertex stage only). */
		if ( !fragmentStage && !shader.declare(displacementSumFunction({.function = "ocPreviousDisplacementAt", .sampler = OceanSurface::PreviousDisplacementSamplerName})) )
		{
			return false;
		}

		/* The displacement (λ Dx, h, λ Dz) summed over the cascades at an undisplaced world XZ. */
		if ( !shader.declare(displacementSumFunction({.function = "ocDisplacementAt", .sampler = OceanSurface::DisplacementSamplerName})) )
		{
			return false;
		}

		/* The choppy surface's normal (Tessendorf § 4.4): its tangents along the lattice axes are
		 * (1 + λ ∂Dx/∂x, ∂h/∂x, λ ∂Dz/∂x) and (λ ∂Dx/∂z, ∂h/∂z, 1 + λ ∂Dz/∂z), and ∂Dz/∂x = ∂Dx/∂z. */
		{
			Declaration::Function function{"ocNormalAt", GLSL::FloatVector3};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInParameter(GLSL::Float, "step");
			function.addInstruction(
				"\t" "const float open = ocShelterAt(xz);" "\n"
				"\t" "vec4 slopes = vec4(0.0);" "\n"
				"\t" "float crossTerm = 0.0;" "\n"
				"\t" "for ( int cascade = 0; cascade < int(ocSurface.cascades.w); ++cascade )" "\n"
				"\t" "{" "\n"
				"\t\t" "const vec3 uvw = vec3(xz / ocSurface.cascades[cascade], float(cascade));" "\n"
				"\t\t" "const float weight = ocCascadeWeight(cascade, step, open);" "\n"
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

		/* cppcheck-suppress duplicateCondition ; one block per GLSL function, on purpose. */
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

		/* cppcheck-suppress duplicateCondition ; one block per GLSL function, on purpose. */
		if ( fragmentStage )
		{
			/* The whitecap coverage: each cascade's accumulated foam (Graphics::OceanWaves), faded like its waves where the
			 * pixel cannot hold it or the land shelters the water, the cascades united as independent coverages:
			 * 1 − Π (1 − foam). */
			Declaration::Function function{OceanSurface::WhitecapFunction, GLSL::Float};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInstruction(
				"\t" "const vec2 footprint = max(abs(dFdx(xz)), abs(dFdy(xz)));" "\n"
				"\t" "const float step = max(footprint.x, footprint.y);" "\n"
				"\t" "const float open = ocShelterAt(xz);" "\n"
				"\t" "float clear = 1.0;" "\n"
				"\t" "for ( int cascade = 0; cascade < int(ocSurface.cascades.w); ++cascade )" "\n"
				"\t" "{" "\n"
				"\t\t" "const float foam = ocFoamBSpline(vec3(xz / ocSurface.cascades[cascade], float(cascade)));" "\n"
				"\t\t" "clear *= 1.0 - clamp(ocCascadeWeight(cascade, step, open) * foam, 0.0, 1.0);" "\n"
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
