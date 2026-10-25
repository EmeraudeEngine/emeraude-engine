/*
 * src/Graphics/Material/SkyAtmosphereResource.cpp
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

#include "SkyAtmosphereResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <initializer_list>
#include <optional>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "Graphics/Effects/Shared/AtmosphereGLSL.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/StorageTexture2D.hpp"
#include "Saphir/AbstractShader.hpp"
#include "Saphir/AbstractVertexStage.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/Declaration/Sampler.hpp"
#include "Saphir/Declaration/StageOutput.hpp"
#include "Saphir/Declaration/Types.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Generator/Abstract.hpp"
#include "Saphir/Keys.hpp"
#include "Saphir/Program.hpp"
#include "Saphir/SetIndexes.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Graphics::Material
{
	using namespace Base;
	using namespace Base::PixelFactory;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	namespace
	{
		/* The descriptor set's bindings (the material UBO first, as every material). */
		constexpr uint32_t MaterialBinding{0};
		constexpr uint32_t AtmosphereBinding{1};
		constexpr uint32_t SkyViewBinding{2};
		constexpr uint32_t TransmittanceBinding{3};
		constexpr uint32_t FrameBinding{4};

		/* GLSL names. */
		constexpr auto SkyDirection{"skyDirection"};
		constexpr auto SkyViewLut{"skyViewLut"};
		constexpr auto TransmittanceLut{"skyTransmittanceLut"};
		constexpr auto FrameData{"skyFrameData"};
		constexpr auto SkyRadiance{"skyRadiance"};

		/* The ceiling of the star's disc in the PRE-EXPOSED scene colour: under the fp16 maximum (65 504) with the sky
		 * added, so a noon sun never stores an infinity a blur would spread. The disc stays far above any display
		 * white: it saturates, it does not dim. */
		constexpr auto StarDiscCeiling{32768.0F};

		/**
		 * @brief Declares one shared atmosphere function in a Saphir shader.
		 * @param shader A reference to the shader.
		 * @param source A reference to the function source.
		 * @return bool
		 */
		bool
		declareAtmosphereFunction (AbstractShader & shader, const AtmosphereGLSL::FunctionSource & source) noexcept
		{
			Declaration::Function function{source.name, source.returnType};

			uint32_t index = 0;

			for ( const auto & parameter : source.parameters )
			{
				if ( index == source.parameterCount )
				{
					break;
				}

				function.addInParameter(parameter.type, parameter.name);

				++index;
			}

			Code{function, Location::Output} << source.body;

			return shader.declare(function);
		}
	}

	SkyAtmosphereResource::SkyAtmosphereResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags) noexcept
		: Interface{serviceProvider, name, resourceFlags}
	{
		/* A sky emits: what this material writes IS the radiance. */
		this->enableFlag(UnlitEnabled);
	}

	bool
	SkyAtmosphereResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* The defaults: the physical sky, its star drawn. */
		return this->setLoadSuccess(true);
	}

	bool
	SkyAtmosphereResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		this->setGrading(
			FastJSON::getValue< Color< float > >(data, JKTint).value_or(White),
			FastJSON::getValue< float >(data, JKIntensity).value_or(1.0F)
		);
		this->setSaturation(FastJSON::getValue< float >(data, JKSaturation).value_or(1.0F));
		this->enableStarDisc(FastJSON::getValue< bool >(data, JKStarDisc).value_or(true));
		this->setLimbDarkening(FastJSON::getValue< float >(data, JKLimbDarkening).value_or(DefaultLimbDarkening));

		return this->setLoadSuccess(true);
	}

	bool
	SkyAtmosphereResource::setAtmosphere (const std::shared_ptr< SkyAtmosphere > & atmosphere) noexcept
	{
		if ( this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is already created: its atmosphere can't change !";

			return false;
		}

		m_atmosphere = atmosphere;

		return true;
	}

	bool
	SkyAtmosphereResource::create (Renderer & renderer) noexcept
	{
		/* The renderer flushes the look changes made after creation. */
		m_renderer = &renderer;

		if ( m_atmosphere == nullptr )
		{
			m_atmosphere = std::make_shared< SkyAtmosphere >(this->name(), AtmosphereParameters::earth());
		}

		/* Idempotent: the background that owns the atmosphere may have created it already. */
		if ( !m_atmosphere->createOnHardware(renderer) )
		{
			TraceError{ClassId} << "Unable to create the atmosphere of sky material '" << this->name() << "' !";

			return false;
		}

		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !this->updateVideoMemory() )
		{
			TraceError{ClassId} << "Unable to upload the initial data of sky material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	void
	SkyAtmosphereResource::destroy () noexcept
	{
		m_sharedUBOSeat.reset();

		m_descriptorSet.reset();
		m_descriptorSetLayout.reset();
		m_sharedUniformBuffer.reset();
		m_sharedUBOIndex = 0;
		m_renderer = nullptr;
		/* NOTE: The atmosphere is kept: it is the background's, and a re-creation binds the same tables. */
	}

	bool
	SkyAtmosphereResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		m_sharedUBOSeat = m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex);

		if ( !m_sharedUBOSeat.active() )
		{
			Tracer::error(ClassId, "Unable to add the sky material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	SkyAtmosphereResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
	{
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		if ( m_descriptorSetLayout != nullptr )
		{
			return true;
		}

		auto newLayout = layoutManager.prepareNewDescriptorSetLayout(identifier);
		newLayout->setIdentifier(ClassId, identifier, "DescriptorSetLayout");

		newLayout->declareUniformBuffer(MaterialBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareUniformBuffer(AtmosphereBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareCombinedImageSampler(SkyViewBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareCombinedImageSampler(TransmittanceBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
		newLayout->declareCombinedImageSampler(FrameBinding, VK_SHADER_STAGE_FRAGMENT_BIT);

		if ( !layoutManager.createDescriptorSetLayout(newLayout) )
		{
			return false;
		}

		/* NOTE: Another thread may have registered the same layout first: hold the canonical one. */
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		return m_descriptorSetLayout != nullptr;
	}

	bool
	SkyAtmosphereResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & /*uniformBufferObject*/) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for sky material '" << this->name() << "' !";

			return false;
		}

		if ( !m_descriptorSet->writeUniformBuffer(MaterialBinding, m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		if ( !m_descriptorSet->writeUniformBufferObject(AtmosphereBinding, *m_atmosphere->parametersBuffer()) )
		{
			TraceError{ClassId} << "Unable to write the atmosphere parameters to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		/* NOTE: The tables' layouts are written explicitly: the atmosphere moves them with its own barriers, the
		 * image's tracked layout does not follow. Between the passes that write them, they are shader-read-only. */
		const auto writeTable = [this] (uint32_t binding, const StorageTexture2D & table) {
			return m_descriptorSet->writeCombinedImageSampler(binding, *table.imageView(), *table.sampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		};

		if ( !writeTable(SkyViewBinding, m_atmosphere->skyView()) ||
			 !writeTable(TransmittanceBinding, m_atmosphere->transmittance()) ||
			 !writeTable(FrameBinding, m_atmosphere->frameData()) )
		{
			TraceError{ClassId} << "Unable to write the atmosphere tables to the descriptor set of sky material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyAtmosphereResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		/* Lowered BEFORE the properties are read (see BeamResource::updateVideoMemory()). */
		m_videoMemoryUpdated.store(false, std::memory_order_release);

		return m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, 0, m_properties.data());
	}

	void
	SkyAtmosphereResource::markVideoMemoryDirty () noexcept
	{
		/* One registration per flush: the flag is cleared by updateVideoMemory(). Before creation, create() uploads. */
		if ( m_renderer == nullptr || !this->isCreated() || m_videoMemoryUpdated.exchange(true, std::memory_order_acq_rel) )
		{
			return;
		}

		m_renderer->requestMaterialVideoMemoryUpdate(std::static_pointer_cast< Interface >(this->weak_from_this().lock()));
	}

	uint32_t
	SkyAtmosphereResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	SkyAtmosphereResource::UBOOffset () const noexcept
	{
		/* LOCAL offset within the element's bank (see StandardResource::UBOOffset()). */
		return static_cast< uint32_t >(m_sharedUniformBuffer->getByteOffsetForElement(m_sharedUBOIndex, 0));
	}

	bool
	SkyAtmosphereResource::samplesTexture (const TextureInterface * texture) const noexcept
	{
		if ( texture == nullptr || m_atmosphere == nullptr )
		{
			return false;
		}

		return texture == &m_atmosphere->skyView() || texture == &m_atmosphere->transmittance() || texture == &m_atmosphere->frameData();
	}

	Declaration::UniformBlock
	SkyAtmosphereResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::SkyAtmosphereMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SkyGrading);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SkyStar);

		return block;
	}

	bool
	SkyAtmosphereResource::generateVertexShaderCode (Generator::Abstract & generator, AbstractVertexStage & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is not created ! It can't generate a vertex shader source code.";

			return false;
		}

		/* The skybox cube is centred on the camera and never rotated: its vertex position IS the world direction of
		 * the sky (see Material::checkPrimaryTextureCoordinates()). Normalized per fragment, not here: interpolating a
		 * unit vector across a face shortens it. */
		if ( !vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, SkyDirection, GLSL::Smooth}) )
		{
			return false;
		}

		Code{vertexShader, Location::Output} << SkyDirection << " = " << Attribute::Position << ";";

		return true;
	}

	bool
	SkyAtmosphereResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & /*lightGenerator*/, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The sky material '" << this->name() << "' is not created ! It can't generate a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, MaterialBinding) )
		{
			return false;
		}

		/* The pre-exposure caps the star's disc (StarDiscCeiling). */
		if ( !generator.declareViewUniformBlock(fragmentShader) )
		{
			return false;
		}

		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		/* The atmosphere: the same block and functions the tables are built with (Effects/Shared/AtmosphereGLSL.hpp). */
		{
			Declaration::UniformBlock block{materialSet, AtmosphereBinding, Declaration::MemoryLayout::Std140, AtmosphereGLSL::BlockType, AtmosphereGLSL::BlockInstance};

			for ( const auto * member : AtmosphereGLSL::BlockMembers )
			{
				block.addMember(Declaration::VariableType::FloatVector4, member);
			}

			if ( !fragmentShader.declare(block) )
			{
				return false;
			}
		}

		if ( !fragmentShader.declare(Declaration::Sampler{materialSet, SkyViewBinding, GLSL::Sampler2D, SkyViewLut}) ||
			 !fragmentShader.declare(Declaration::Sampler{materialSet, TransmittanceBinding, GLSL::Sampler2D, TransmittanceLut}) ||
			 !fragmentShader.declare(Declaration::Sampler{materialSet, FrameBinding, GLSL::Sampler2D, FrameData}) )
		{
			return false;
		}

		for ( const auto * function : {&AtmosphereGLSL::FromUnitToSubUv, &AtmosphereGLSL::RaySphereNearest, &AtmosphereGLSL::TransmittanceParametersToUv, &AtmosphereGLSL::SkyViewParametersToUv, &AtmosphereGLSL::SkyViewLookup, &AtmosphereGLSL::Grade} )
		{
			if ( !declareAtmosphereFunction(fragmentShader, *function) )
			{
				TraceError{ClassId} << "Unable to declare the atmosphere function '" << function->name << "' in the sky material '" << this->name() << "' !";

				return false;
			}
		}

		const auto grading = MaterialUB(UniformBlock::Component::SkyGrading);
		const auto star = MaterialUB(UniformBlock::Component::SkyStar);

		/* The frame texture (SkyAtmosphere::recordFrame()): texel 0 (unit vector toward the star, cos of its angular
		 * radius), texel 1 (its illuminance above the atmosphere × its colour in lux, the camera's distance to the
		 * planet's centre in km). The sky-view table holds radiance PER LUX of that illuminance. */
		Code{fragmentShader, Location::Top} <<
			"const vec3 skyDir = normalize(" << SkyDirection << ");" << Line::End <<
			"const vec4 skyFrame0 = texelFetch(" << FrameData << ", ivec2(0, 0), 0);" << Line::End <<
			"const vec4 skyFrame1 = texelFetch(" << FrameData << ", ivec2(1, 0), 0);" << Line::End <<
			"const vec3 skyStarDir = skyFrame0.xyz;" << Line::End <<
			"const float skyViewHeight = skyFrame1.w;" << Line::End <<
			/* The table's lookup, shared with the environment capture (AtmosphereGLSL::SkyViewLookup). */
			"const vec3 skyLookup = atmSkyViewLookup(skyDir, skyStarDir, skyViewHeight);" << Line::End <<
			"const bool skyHitsGround = skyLookup.z > 0.5;" << Line::End <<
			"vec3 " << SkyRadiance << " = textureLod(" << SkyViewLut << ", skyLookup.xy, 0.0).rgb * skyFrame1.rgb;" << Line::End <<
			/* The star's disc: its illuminance spread over its solid angle (the mean radiance), shaped by the linear
			 * limb darkening normalized to that mean, seen through the atmosphere. Antialiased over one pixel. */
			/* ⚠️ The derivative is taken BEFORE the branch: fwidth() in non-uniform control flow is undefined. */
			"const float skyCosRadius = skyFrame0.w;" << Line::End <<
			"const float skyCosAngle = dot(skyDir, skyStarDir);" << Line::End <<
			"const float skyEdge = max(fwidth(skyCosAngle), 1e-7);" << Line::End <<
			"if ( " << star << ".y > 0.5 && !skyHitsGround ) {" << Line::End <<
			"	const float skyCoverage = smoothstep(skyCosRadius - skyEdge, skyCosRadius + skyEdge, skyCosAngle);" << Line::End <<
			"	if ( skyCoverage > 0.0 ) {" << Line::End <<
			"		const float skySolidAngle = 2.0 * 3.14159265358979 * max(1.0 - skyCosRadius, 1e-9);" << Line::End <<
			"		const float skyRadius = acos(clamp(skyCosRadius, -1.0, 1.0));" << Line::End <<
			"		const float skyRadial = clamp(acos(clamp(skyCosAngle, -1.0, 1.0)) / max(skyRadius, 1e-6), 0.0, 1.0);" << Line::End <<
			"		const float skyMu = sqrt(max(1.0 - (skyRadial * skyRadial), 0.0));" << Line::End <<
			"		const float skyLimb = (1.0 - " << star << ".z * (1.0 - skyMu)) / (1.0 - " << star << ".z / 3.0);" << Line::End <<
			"		const vec3 skyTransmittance = textureLod(" << TransmittanceLut << ", atmTransmittanceParametersToUv(skyViewHeight, skyDir.y), 0.0).rgb;" << Line::End <<
			"		vec3 skyDisc = skyFrame1.rgb / skySolidAngle * skyLimb * skyTransmittance;" << Line::End <<
			"		const float skyPeak = max(max(skyDisc.r, skyDisc.g), skyDisc.b) * " << ViewUB(UniformBlock::Component::PreExposure, false) << ";" << Line::End <<
			"		if ( skyPeak > " << StarDiscCeiling << " ) { skyDisc *= " << StarDiscCeiling << " / skyPeak; }" << Line::End <<
			"		" << SkyRadiance << " += skyDisc * skyCoverage;" << Line::End <<
			"	}" << Line::End <<
			"}" << Line::End <<
			/* The artistic grading, shared with the environment capture (AtmosphereGLSL::Grade). */
			SkyRadiance << " = atmGrade(" << SkyRadiance << ", " << grading << ", " << star << ".x);";

		return true;
	}

	std::string
	SkyAtmosphereResource::fragmentColor () const noexcept
	{
		return std::string{"vec4("} + SkyRadiance + ", 1.0)";
	}

	void
	SkyAtmosphereResource::enableBlending (BlendingMode mode) noexcept
	{
		TraceWarning{ClassId} << "A sky is opaque: blending '" << to_cstring(mode) << "' ignored !";
	}

	void
	SkyAtmosphereResource::setGrading (const Color< float > & tint, float intensity) noexcept
	{
		m_properties[GradingOffset] = std::max(0.0F, tint.red());
		m_properties[GradingOffset + 1] = std::max(0.0F, tint.green());
		m_properties[GradingOffset + 2] = std::max(0.0F, tint.blue());
		m_properties[GradingOffset + 3] = std::max(0.0F, intensity);

		this->markVideoMemoryDirty();
	}

	void
	SkyAtmosphereResource::setSaturation (float saturation) noexcept
	{
		m_properties[StarOffset] = std::max(0.0F, saturation);

		this->markVideoMemoryDirty();
	}

	void
	SkyAtmosphereResource::enableStarDisc (bool state) noexcept
	{
		m_properties[StarOffset + 1] = state ? 1.0F : 0.0F;

		this->markVideoMemoryDirty();
	}

	void
	SkyAtmosphereResource::setLimbDarkening (float coefficient) noexcept
	{
		m_properties[StarOffset + 2] = std::clamp(coefficient, 0.0F, 1.0F);

		this->markVideoMemoryDirty();
	}
}
