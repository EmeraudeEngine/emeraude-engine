/*
 * src/Graphics/Material/BeamResource.cpp
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

#include "BeamResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <sstream>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Saphir/AbstractVertexStage.hpp"
#include "Saphir/BeamGLSL.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Generator/Abstract.hpp"
#include "Saphir/Keys.hpp"
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

	/** @brief The cross-section of the beam at this fragment, before the sub-pixel coverage. */
	constexpr auto BeamProfile{"beamProfile"};
	/** @brief The light of the beam at this fragment: the profile times the sub-pixel coverage. */
	constexpr auto BeamLight{"beamLight"};

	BeamResource::BeamResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags) noexcept
		: Interface{serviceProvider, name, resourceFlags}
	{
		/* A beam is emitted light added over the scene, whatever it is set up with. */
		this->enableFlag(UnlitEnabled);
		this->enableFlag(BlendingEnabled);

		this->updateRadiance();
	}

	bool
	BeamResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* The defaults: a thin straight red laser. */
		return this->setLoadSuccess(true);
	}

	bool
	BeamResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( const auto color = FastJSON::getValue< Color< float > >(data, JKColor) )
		{
			this->setColor(color.value());
		}

		this->setLuminance(FastJSON::getValue< float >(data, JKLuminance).value_or(DefaultLuminance));
		this->setHalfWidth(FastJSON::getValue< float >(data, JKHalfWidth).value_or(DefaultHalfWidth));
		this->setCoreExponent(FastJSON::getValue< float >(data, JKCoreExponent).value_or(DefaultCoreExponent));

		this->setArc(
			FastJSON::getValue< float >(data, JKArcAmplitude).value_or(0.0F),
			FastJSON::getValue< float >(data, JKArcFrequency).value_or(DefaultArcFrequency),
			FastJSON::getValue< uint32_t >(data, JKArcOctaves).value_or(static_cast< uint32_t >(DefaultArcOctaves))
		);

		this->setArcMotion(
			FastJSON::getValue< uint32_t >(data, JKSeed).value_or(0),
			FastJSON::getValue< float >(data, JKRestrikeRate).value_or(0.0F),
			FastJSON::getValue< float >(data, JKDrift).value_or(0.0F)
		);

		return this->setLoadSuccess(true);
	}

	bool
	BeamResource::create (Renderer & renderer) noexcept
	{
		/* The renderer flushes the look changes made after creation. */
		m_renderer = &renderer;

		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for beam material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for beam material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for beam material '" << this->name() << "' !";

			return false;
		}

		if ( !this->updateVideoMemory() )
		{
			TraceError{ClassId} << "Unable to upload the initial data of beam material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	void
	BeamResource::destroy () noexcept
	{
		if ( m_sharedUniformBuffer != nullptr )
		{
			m_sharedUniformBuffer->removeElement(this);
		}

		m_descriptorSet.reset();
		m_descriptorSetLayout.reset();
		m_sharedUniformBuffer.reset();
		m_sharedUBOIndex = 0;
		m_renderer = nullptr;
	}

	bool
	BeamResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		if ( !m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex) )
		{
			Tracer::error(ClassId, "Unable to add the beam material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	BeamResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
	{
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		if ( m_descriptorSetLayout != nullptr )
		{
			return true;
		}

		auto newLayout = layoutManager.prepareNewDescriptorSetLayout(identifier);
		newLayout->setIdentifier(ClassId, identifier, "DescriptorSetLayout");

		/* The vertex stage reads the shape and the motion (the ribbon), the fragment stage the radiance. */
		newLayout->declareUniformBuffer(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		if ( !layoutManager.createDescriptorSetLayout(newLayout) )
		{
			return false;
		}

		/* NOTE: Another thread may have registered the same layout first: hold the canonical one. */
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		return m_descriptorSetLayout != nullptr;
	}

	bool
	BeamResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & /*uniformBufferObject*/) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for beam material '" << this->name() << "' !";

			return false;
		}

		if ( !m_descriptorSet->writeUniformBuffer(0, m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of beam material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	BeamResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		/* NOTE: Frame region 0, like every material UBO: only the LOOK lives here, and a look change reaching a frame
		 * in flight one frame early is harmless. The endpoints, which move every tick, never come here. */
		if ( !m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, 0, m_properties.data()) )
		{
			return false;
		}

		m_videoMemoryUpdated = false;

		return true;
	}

	void
	BeamResource::markVideoMemoryDirty () noexcept
	{
		/* One registration per flush: the flag is cleared by updateVideoMemory(). Before creation, create() uploads. */
		if ( m_videoMemoryUpdated || m_renderer == nullptr || !this->isCreated() )
		{
			return;
		}

		m_videoMemoryUpdated = true;

		m_renderer->requestMaterialVideoMemoryUpdate(std::static_pointer_cast< Interface >(this->weak_from_this().lock()));
	}

	uint32_t
	BeamResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	BeamResource::UBOOffset () const noexcept
	{
		/* LOCAL offset within the element's bank (see StandardResource::UBOOffset()). */
		return static_cast< uint32_t >(m_sharedUniformBuffer->getByteOffsetForElement(m_sharedUBOIndex, 0));
	}

	Declaration::UniformBlock
	BeamResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::BeamMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::BeamRadiance);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::BeamShape);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::BeamMotion);

		return block;
	}

	bool
	BeamResource::prepareVertexStage (Generator::Abstract & /*generator*/, AbstractVertexStage & vertexShader) const noexcept
	{
		vertexShader.enableBeamRibbon(MaterialUB(UniformBlock::Component::BeamShape), MaterialUB(UniformBlock::Component::BeamMotion));

		return true;
	}

	bool
	BeamResource::generateVertexShaderCode (Generator::Abstract & generator, AbstractVertexStage & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The beam material '" << this->name() << "' is not created ! It can't generate a vertex shader source code.";

			return false;
		}

		/* The ribbon reads the shape and the motion (AbstractVertexStage::prepareBeamRibbon()). */
		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		if ( !vertexShader.declare(this->getUniformBlock(materialSet, 0)) )
		{
			TraceError{ClassId} << "Unable to declare the material uniform block in the vertex stage of the beam '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	BeamResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & /*lightGenerator*/, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The beam material '" << this->name() << "' is not created ! It can't generate a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, 0) )
		{
			return false;
		}

		const auto radiance = MaterialUB(UniformBlock::Component::BeamRadiance);

		/* The cross-section (1 - side²)^exponent: 1 on the line, 0 at the edge of the ribbon; then the sub-pixel
		 * coverage of a beam narrower than a pixel (its drawn width is one pixel, its light scaled down). */
		Code{fragmentShader, Location::Top} <<
			"const float " << BeamProfile << " = pow(max(1.0 - " << ShaderVariable::BeamCoordinates << ".y * " << ShaderVariable::BeamCoordinates << ".y, 0.0), " << radiance << ".w);" << Line::End <<
			"const float " << BeamLight << " = " << BeamProfile << " * " << ShaderVariable::BeamCoverage << ";";

		return true;
	}

	std::string
	BeamResource::fragmentColor () const noexcept
	{
		/* Radiance in nits, added over the scene (BlendingMode::Add: ONE, ONE). The alpha is a GATE, not an opacity:
		 * the additive state REPLACES the destination alpha, so a fragment that carries light writes 1 (what an opaque
		 * surface wrote there) and the others fall under the blended-material discard (alpha < 0.01) and write
		 * nothing. It is gated on the profile, not the light: a far beam's coverage is small, its pixels must stay. */
		std::stringstream code;

		code << "vec4(" << MaterialUB(UniformBlock::Component::BeamRadiance) << ".rgb * " << BeamLight << ", step(0.01, " << BeamProfile << "))";

		return code.str();
	}

	std::string
	BeamResource::reactiveMaskExpression () const noexcept
	{
		/* The same gate as the alpha (fragmentColor()): the profile, not the light, so a far beam stays reactive. */
		return std::string{"step(0.01, "} + BeamProfile + ")";
	}

	void
	BeamResource::enableBlending (BlendingMode mode) noexcept
	{
		if ( mode != BlendingMode::Add )
		{
			TraceWarning{ClassId} << "A beam adds light: its blending is '" << to_cstring(BlendingMode::Add) << "' whatever is asked ('" << to_cstring(mode) << "' ignored) !";
		}
	}

	void
	BeamResource::updateRadiance () noexcept
	{
		m_properties[RadianceOffset] = m_color.red() * m_luminance;
		m_properties[RadianceOffset + 1] = m_color.green() * m_luminance;
		m_properties[RadianceOffset + 2] = m_color.blue() * m_luminance;
	}

	void
	BeamResource::setColor (const Color< float > & color) noexcept
	{
		m_color = color;

		this->updateRadiance();
		this->markVideoMemoryDirty();
	}

	void
	BeamResource::setLuminance (float nits) noexcept
	{
		m_luminance = std::max(0.0F, nits);

		this->updateRadiance();
		this->markVideoMemoryDirty();
	}

	void
	BeamResource::setHalfWidth (float halfWidth) noexcept
	{
		m_properties[ShapeOffset] = std::max(0.0F, halfWidth);

		this->markVideoMemoryDirty();
	}

	void
	BeamResource::setCoreExponent (float exponent) noexcept
	{
		m_properties[RadianceOffset + 3] = std::max(0.1F, exponent);

		this->markVideoMemoryDirty();
	}

	void
	BeamResource::setArc (float amplitude, float frequency, uint32_t octaves) noexcept
	{
		m_properties[ShapeOffset + 1] = std::max(0.0F, amplitude);
		m_properties[ShapeOffset + 2] = std::max(0.0F, frequency);
		m_properties[ShapeOffset + 3] = static_cast< float >(std::clamp(octaves, 1U, static_cast< uint32_t >(BeamGLSL::MaxOctaves)));

		this->markVideoMemoryDirty();
	}

	void
	BeamResource::setArcMotion (uint32_t seed, float restrikeRate, float drift) noexcept
	{
		/* NOTE: The seed travels as a float and is read back with uint(): exact up to 2^24. */
		m_properties[MotionOffset] = static_cast< float >(seed & 0xFFFFFFU);
		m_properties[MotionOffset + 1] = std::max(0.0F, restrikeRate);
		m_properties[MotionOffset + 2] = drift;

		this->markVideoMemoryDirty();
	}
}
