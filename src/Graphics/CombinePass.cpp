/*
 * src/Graphics/CombinePass.cpp
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

#include "CombinePass.hpp"

/* STL inclusions. */
#include <sstream>
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Hash/FNV1a.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/UniformBufferObject.hpp"

static constexpr auto TracerTag{"CombinePass"};

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	namespace
	{
		/* Context sampler names, declared in a fixed order after the chain color. */
		constexpr std::array< const char *, 4 > ContextSamplerNames{"emDepth", "emNormals", "emMaterialProps", "emAlbedo"};

		/* Maximum dynamics vec4 slots per contribution (mirrors CombineContribution::dynamics). */
		constexpr uint32_t DynamicsSlots{2};
	}

	/* ---- Lifecycle ---- */

	bool
	CombinePass::create (uint32_t width, uint32_t height) noexcept
	{
		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		auto & renderer = this->renderer();

		for ( size_t index = 0; index < m_targets.size(); ++index )
		{
			if ( !m_targets[index].create(renderer, width, height, format, "CombineGroup" + std::to_string(index)) )
			{
				TraceError{TracerTag} << "Failed to create the combine target #" << index << " !";

				return false;
			}
		}

		return true;
	}

	void
	CombinePass::destroy () noexcept
	{
		for ( auto & [signature, variant] : m_variants )
		{
			variant.descriptorSetsPerFrame.clear();
			variant.uniformBuffersPerFrame.clear();
			variant.pipeline.reset();
			variant.pipelineLayout.reset();
		}

		m_variants.clear();

		for ( auto & target : m_targets )
		{
			target.destroy();
		}
	}

	/* ---- Shader generation ---- */

	std::string
	CombinePass::buildFragmentShaderSource (const std::vector< GroupEntry > & contributions) noexcept
	{
		bool needsDepth = false;
		bool needsNormals = false;
		bool needsMaterialProperties = false;
		bool needsAlbedo = false;

		for ( const auto & contribution : contributions )
		{
			needsDepth |= contribution.needsDepth;
			needsNormals |= contribution.needsNormals;
			needsMaterialProperties |= contribution.needsMaterialProperties;
			needsAlbedo |= contribution.needsAlbedo;
		}

		std::stringstream source;

		source <<
			"#version 450\n\n"
			"layout(location = 0) in vec2 vUV;\n"
			"layout(location = 0) out vec4 outColor;\n\n"
			"layout(set = 0, binding = 0) uniform sampler2D emChainColor;\n";

		uint32_t binding = 1;

		const std::array< bool, 4 > contextNeeds{needsDepth, needsNormals, needsMaterialProperties, needsAlbedo};

		for ( size_t index = 0; index < ContextSamplerNames.size(); ++index )
		{
			if ( contextNeeds[index] )
			{
				source << "layout(set = 0, binding = " << binding++ << ") uniform sampler2D " << ContextSamplerNames[index] << ";\n";
			}
		}

		for ( const auto & contribution : contributions )
		{
			for ( const auto & sampler : contribution.samplers )
			{
				source << "layout(set = 0, binding = " << binding++ << ") uniform sampler2D " << contribution.prefix << sampler.nameSuffix << ";\n";
			}
		}

		/* Per-frame scalars: a fixed two-vec4 block per contribution (std140-safe). */
		source << "\nlayout(set = 0, binding = " << binding << ") uniform CombineDynamics\n{\n";

		for ( const auto & contribution : contributions )
		{
			for ( uint32_t slot = 0; slot < DynamicsSlots; ++slot )
			{
				source << "\tvec4 " << contribution.prefix << "Dynamics" << slot << ";\n";
			}
		}

		source << "} emDyn;\n";

		/* Matches PostProcessor::PushConstants. */
		source <<
			"\nlayout(push_constant) uniform PushConstants\n"
			"{\n"
			"\tfloat frameWidth;\n"
			"\tfloat frameHeight;\n"
			"\tfloat time;\n"
			"\tfloat nearPlane;\n"
			"\tfloat farPlane;\n"
			"\tfloat tanHalfFovY;\n"
			"\tfloat deltaTime;\n"
			"};\n\n"
			"void main()\n"
			"{\n"
			"\tvec4 em_Color = texture(emChainColor, vUV);\n\n";

		for ( const auto & contribution : contributions )
		{
			source << "\t/* --- " << contribution.prefix << " --- */\n" << contribution.code << '\n';
		}

		source <<
			"\toutColor = em_Color;\n"
			"}\n";

		return source.str();
	}

	size_t
	CombinePass::computeSignature (const std::vector< GroupEntry > & contributions) noexcept
	{
		const auto hashCombine = [] (size_t & seed, size_t value) noexcept {
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

		size_t hash = Hash::FNV1a(ClassId);

		for ( const auto & contribution : contributions )
		{
			hashCombine(hash, Hash::FNV1a(contribution.prefix));
			hashCombine(hash, contribution.samplers.size());
			hashCombine(hash, static_cast< size_t >(contribution.needsDepth) | (static_cast< size_t >(contribution.needsNormals) << 1U) | (static_cast< size_t >(contribution.needsMaterialProperties) << 2U) | (static_cast< size_t >(contribution.needsAlbedo) << 3U));
		}

		return hash;
	}

	/* ---- Variant management ---- */

	CombinePass::Variant *
	CombinePass::getOrCreateVariant (size_t signature, const std::vector< GroupEntry > & contributions) noexcept
	{
		const auto existing = m_variants.find(signature);

		if ( existing != m_variants.end() )
		{
			return existing->second.pipeline != nullptr ? &existing->second : nullptr;
		}

		auto & renderer = this->renderer();
		auto & variant = m_variants[signature];

		/* Sampler count: chain color + required context samplers + per-effect samplers. */
		uint32_t samplerCount = 1;

		{
			bool needsDepth = false;
			bool needsNormals = false;
			bool needsMaterialProperties = false;
			bool needsAlbedo = false;

			for ( const auto & contribution : contributions )
			{
				needsDepth |= contribution.needsDepth;
				needsNormals |= contribution.needsNormals;
				needsMaterialProperties |= contribution.needsMaterialProperties;
				needsAlbedo |= contribution.needsAlbedo;

				samplerCount += static_cast< uint32_t >(contribution.samplers.size());
			}

			samplerCount += static_cast< uint32_t >(needsDepth) + static_cast< uint32_t >(needsNormals) + static_cast< uint32_t >(needsMaterialProperties) + static_cast< uint32_t >(needsAlbedo);
		}

		variant.samplerCount = samplerCount;

		/* Shader module: name derived from the signature (ShaderManager caches by name). */
		const auto source = buildFragmentShaderSource(contributions);

		std::stringstream shaderName;
		shaderName << "CombineFS";

		for ( const auto & contribution : contributions )
		{
			shaderName << '_' << contribution.prefix;
		}

		auto vertexModule = this->getFullscreenVertexShader();
		auto fragmentModule = renderer.shaderManager().getShaderModuleFromSourceCode(renderer.device(), shaderName.str(), ShaderType::FragmentShader, source);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile the combine shader '" << shaderName.str() << "' !";

			return nullptr;
		}

		/* Descriptor set layout: samplers + the dynamics UBO. */
		auto descriptorSetLayout = this->getInputLayout(samplerCount, 1);

		if ( descriptorSetLayout == nullptr )
		{
			return nullptr;
		}

		/* Pipeline layout: the shared post-processing push constants. */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(descriptorSetLayout);

			variant.pipelineLayout = renderer.layoutManager().getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(PostProcessor::PushConstants)
				}
			});
		}

		if ( variant.pipelineLayout == nullptr )
		{
			return nullptr;
		}

		variant.pipeline = this->createFullscreenPipeline(ClassId, shaderName.str(), vertexModule, fragmentModule, variant.pipelineLayout, m_targets[0]);

		if ( variant.pipeline == nullptr )
		{
			return nullptr;
		}

		/* Per-frame descriptor sets + dynamics uniform buffers. */
		variant.descriptorSetsPerFrame = this->createPerFrameDescriptorSets(descriptorSetLayout, ClassId, shaderName.str() + "DescSet");

		if ( variant.descriptorSetsPerFrame.empty() )
		{
			return nullptr;
		}

		const VkDeviceSize uboSize = static_cast< VkDeviceSize >(contributions.size()) * DynamicsSlots * 4 * sizeof(float);

		variant.uniformBuffersPerFrame = this->createPerFrameUniformBuffers(uboSize, ClassId, shaderName.str() + "Dynamics");

		if ( variant.uniformBuffersPerFrame.empty() )
		{
			return nullptr;
		}

		/* Bind each per-frame UBO to its descriptor set once (binding = samplerCount). */
		for ( size_t frameIndex = 0; frameIndex < variant.descriptorSetsPerFrame.size(); ++frameIndex )
		{
			if ( !variant.descriptorSetsPerFrame[frameIndex]->writeUniformBufferObject(samplerCount, *variant.uniformBuffersPerFrame[frameIndex]) )
			{
				TraceError{TracerTag} << "Failed to bind the combine dynamics UBO !";

				return nullptr;
			}
		}

		return &variant;
	}

	/* ---- Recording ---- */

	const TextureInterface &
	CombinePass::record (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const std::vector< GroupEntry > & contributions, const FrameContext & context, uint32_t groupIndex) noexcept
	{
		if ( contributions.empty() || !this->isCreated() )
		{
			return inputColor;
		}

		auto * variant = this->getOrCreateVariant(computeSignature(contributions), contributions);

		if ( variant == nullptr )
		{
			/* Creation failed: the chain continues on the unmodified color (traced above). */
			return inputColor;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();
		auto & descriptorSet = *variant->descriptorSetsPerFrame[frameIndex];

		/* Rebind every sampler for this frame: chain color, context, then effect textures. */
		uint32_t binding = 0;

		static_cast< void >(descriptorSet.writeCombinedImageSampler(binding++, inputColor));

		{
			bool needsDepth = false;
			bool needsNormals = false;
			bool needsMaterialProperties = false;
			bool needsAlbedo = false;

			for ( const auto & contribution : contributions )
			{
				needsDepth |= contribution.needsDepth;
				needsNormals |= contribution.needsNormals;
				needsMaterialProperties |= contribution.needsMaterialProperties;
				needsAlbedo |= contribution.needsAlbedo;
			}

			const std::array< const TextureInterface *, 4 > contextTextures{context.depth, context.normals, context.materialProperties, context.albedo};
			const std::array< bool, 4 > contextNeeds{needsDepth, needsNormals, needsMaterialProperties, needsAlbedo};

			for ( size_t index = 0; index < contextTextures.size(); ++index )
			{
				if ( !contextNeeds[index] )
				{
					continue;
				}

				if ( contextTextures[index] == nullptr )
				{
					TraceError{TracerTag} << "A combine contribution requires the missing context sampler '" << ContextSamplerNames[index] << "' !";

					return inputColor;
				}

				static_cast< void >(descriptorSet.writeCombinedImageSampler(binding++, *contextTextures[index]));
			}
		}

		for ( const auto & contribution : contributions )
		{
			for ( const auto & sampler : contribution.samplers )
			{
				if ( sampler.texture == nullptr )
				{
					TraceError{TracerTag} << "A combine contribution ('" << contribution.prefix << "') provided a null texture !";

					return inputColor;
				}

				static_cast< void >(descriptorSet.writeCombinedImageSampler(binding++, *sampler.texture));
			}
		}

		/* Upload this frame's dynamics. */
		{
			std::vector< float > dynamicsData(contributions.size() * DynamicsSlots * 4, 0.0F);

			size_t offset = 0;

			for ( const auto & contribution : contributions )
			{
				for ( uint32_t slot = 0; slot < DynamicsSlots; ++slot )
				{
					if ( slot < contribution.dynamics.size() )
					{
						const auto & vector = contribution.dynamics[slot];

						dynamicsData[offset + 0] = vector.x();
						dynamicsData[offset + 1] = vector.y();
						dynamicsData[offset + 2] = vector.z();
						dynamicsData[offset + 3] = vector.w();
					}

					offset += 4;
				}
			}

			if ( !updateUniformBufferData(*variant->uniformBuffersPerFrame[frameIndex], dynamicsData.data(), dynamicsData.size() * sizeof(float)) )
			{
				TraceError{TracerTag} << "Failed to upload the combine dynamics !";

				return inputColor;
			}
		}

		auto & target = m_targets[groupIndex % m_targets.size()];

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			target,
			*variant->pipeline,
			*variant->pipelineLayout,
			descriptorSet,
			&context.constants,
			sizeof(PostProcessor::PushConstants)
		);

		return target;
	}
}
