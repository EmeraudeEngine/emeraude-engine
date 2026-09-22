/*
 * src/Saphir/Generator/HeightfieldSurfaceHelper.cpp
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

#include "HeightfieldSurfaceHelper.hpp"

/* Local inclusions. */
#include "Graphics/Geometry/HeightfieldSurface.hpp"
#include "Saphir/AbstractShader.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/Keys.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Graphics::Geometry;

	std::shared_ptr< Vulkan::DescriptorSetLayout >
	getHeightfieldSurfaceDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept
	{
		static constexpr auto UUID{"HeightfieldSurface"};

		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(UUID);

		if ( descriptorSetLayout == nullptr )
		{
			constexpr VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(UUID);
			descriptorSetLayout->setIdentifier("HeightfieldSurface", "Clipmap", "DescriptorSetLayout");
			descriptorSetLayout->declareCombinedImageSampler(HeightfieldSurface::HeightsBinding, stages);
			descriptorSetLayout->declareCombinedImageSampler(HeightfieldSurface::NormalsBinding, stages);
			descriptorSetLayout->declareUniformBuffer(HeightfieldSurface::UniformsBinding, stages);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	bool
	declareHeightfieldSurface (AbstractShader & shader, uint32_t setIndex, bool fragmentStage) noexcept
	{
		using namespace Keys;

		if ( !shader.declare(Declaration::Sampler{setIndex, HeightfieldSurface::HeightsBinding, GLSL::Sampler2DArray, HeightfieldSurface::HeightsSamplerName}) )
		{
			return false;
		}

		if ( !shader.declare(Declaration::Sampler{setIndex, HeightfieldSurface::NormalsBinding, GLSL::Sampler2DArray, HeightfieldSurface::NormalsSamplerName}) )
		{
			return false;
		}

		/* Mirror of Graphics::Geometry::HeightfieldSurface::Uniforms (std140, vec4 members only). */
		Declaration::UniformBlock block{setIndex, HeightfieldSurface::UniformsBinding, Declaration::MemoryLayout::Std140, HeightfieldSurface::UniformBlockName, HeightfieldSurface::UniformBlockInstance};
		block.addMember(Declaration::VariableType::FloatVector4, "grid");
		block.addMember(Declaration::VariableType::FloatVector4, "height");
		block.addMember(Declaration::VariableType::FloatVector4, "textureCoordinates");
		block.addArrayMember(Declaration::VariableType::FloatVector4, "levels", HeightfieldSurface::MaxClipLevels);
		block.addArrayMember(Declaration::VariableType::FloatVector4, "levelsOfDetail", HeightfieldSurface::MaxLevelsOfDetail);

		if ( !shader.declare(block) )
		{
			return false;
		}

		/* A world lattice point i of level l lives in texel (i mod N): the REPEAT sampler addresses it
		 * as (xz / texel + 0.5) / N, and bilinear filtering interpolates between lattice points. */
		{
			Declaration::Function function{"hfHeight", GLSL::Float};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInParameter(GLSL::Integer, "level");
			function.addInstruction(
				"\t" "const vec2 uv = ((xz / hfSurface.levels[level].w) + 0.5) / hfSurface.grid.y;" "\n"
				"\t" "return (textureLod(hfHeights, vec3(uv, float(level)), 0.0).r * hfSurface.height.x) + hfSurface.height.y;" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		{
			Declaration::Function function{"hfNormalAt", GLSL::FloatVector3};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInParameter(GLSL::Integer, "level");
			function.addInstruction(
				"\t" "const vec2 uv = ((xz / hfSurface.levels[level].w) + 0.5) / hfSurface.grid.y;" "\n"
				"\t" "const vec2 horizontal = textureLod(hfNormals, vec3(uv, float(level)), 0.0).rg;" "\n\n"
				"\t" "/* A heightfield normal always points up: Y is rebuilt from X and Z. */" "\n"
				"\t" "return vec3(horizontal.x, sqrt(max(1.0 - dot(horizontal, horizontal), 0.0)), horizontal.y);" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		if ( fragmentStage )
		{
			/* The level whose texel matches the pixel's footprint on the ground — never finer than the
			 * finest level that holds the pixel. Across the outer tenth of a level's valid extent the
			 * level slides continuously to the next one, so crossing a border leaves no seam. A distant
			 * ridge is lit by its FILTERED normal (the clip levels are a tent-filtered pyramid), not by
			 * the 1 m normal of whatever vertex lands there. */
			Declaration::Function function{"hfPixelNormalAt", GLSL::FloatVector3};
			function.addInParameter(GLSL::FloatVector2, "xz");
			function.addInstruction(
				"\t" "const int levelCount = int(hfSurface.grid.z);" "\n"
				"\t" "const vec2 footprint = max(abs(dFdx(xz)), abs(dFdy(xz)));" "\n"
				"\t" "const float wanted = log2(max(max(footprint.x, footprint.y) / hfSurface.grid.x, 1.0));" "\n\n"
				"\t" "float finest = float(levelCount - 1);" "\n\n"
				"\t" "for ( int level = 0; level < levelCount; ++level )" "\n"
				"\t" "{" "\n"
				"\t\t" "const vec4 extent = hfSurface.levels[level];" "\n"
				"\t\t" "const vec2 offset = abs(xz - extent.xy);" "\n"
				"\t\t" "const float reach = max(offset.x, offset.y);" "\n\n"
				"\t\t" "if ( reach < extent.z )" "\n"
				"\t\t" "{" "\n"
				"\t\t\t" "finest = float(level) + clamp((reach - (extent.z * 0.9)) / (extent.z * 0.1), 0.0, 1.0);" "\n\n"
				"\t\t\t" "break;" "\n"
				"\t\t" "}" "\n"
				"\t" "}" "\n\n"
				"\t" "const float level = min(max(wanted, finest), float(levelCount - 1));" "\n"
				"\t" "const int lower = int(floor(level));" "\n"
				"\t" "const int upper = min(lower + 1, levelCount - 1);" "\n\n"
				"\t" "return normalize(mix(hfNormalAt(xz, lower), hfNormalAt(xz, upper), level - float(lower)));" "\n"
			);

			if ( !shader.declare(function) )
			{
				return false;
			}
		}

		return true;
	}
}
