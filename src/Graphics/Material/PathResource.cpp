/*
 * src/Graphics/Material/PathResource.cpp
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

#include "PathResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <sstream>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SharedUniformBuffer.hpp"
#include "Saphir/AbstractVertexStage.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/PathGLSL.hpp"
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
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	PathResource::PathResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags) noexcept
		: Interface{serviceProvider, name, resourceFlags}
	{
		/* A path emits its colour: nothing lights it. */
		this->enableFlag(UnlitEnabled);

		this->updateRadiance();
	}

	bool
	PathResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* The defaults: a white 10 cm line at 250 nits, mitered. */
		return this->setLoadSuccess(true);
	}

	bool
	PathResource::load (const Json::Value & data) noexcept
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
		this->setWidth(FastJSON::getValue< float >(data, JKHalfWidth).value_or(DefaultHalfWidth), FastJSON::getValue< bool >(data, JKWidthInPixels).value_or(false));
		this->setJoins(FastJSON::getValue< bool >(data, JKRoundJoins).value_or(false), FastJSON::getValue< float >(data, JKMiterLimit).value_or(DefaultMiterLimit));

		return this->setLoadSuccess(true);
	}

	bool
	PathResource::create (Renderer & renderer) noexcept
	{
		/* The renderer flushes the look changes made after creation. */
		m_renderer = &renderer;

		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for path material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for path material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for path material '" << this->name() << "' !";

			return false;
		}

		if ( !this->updateVideoMemory() )
		{
			TraceError{ClassId} << "Unable to upload the initial data of path material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	void
	PathResource::destroy () noexcept
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
	PathResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		if ( !m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex) )
		{
			Tracer::error(ClassId, "Unable to add the path material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	PathResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
	{
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		if ( m_descriptorSetLayout != nullptr )
		{
			return true;
		}

		auto newLayout = layoutManager.prepareNewDescriptorSetLayout(identifier);
		newLayout->setIdentifier(ClassId, identifier, "DescriptorSetLayout");

		/* The vertex stage reads the style (the ribbon), the fragment stage the radiance and the style (round caps). */
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
	PathResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & /*uniformBufferObject*/) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for path material '" << this->name() << "' !";

			return false;
		}

		if ( !m_descriptorSet->writeUniformBuffer(0, m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of path material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	PathResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		/* Lowered BEFORE the properties are read: a change made by another thread during the write registers the
		 * material again (lowered after, it was lost until the next change — a panel could stay lit after its lamp
		 * went out, 2026-10-06). */
		m_videoMemoryUpdated.store(false, std::memory_order_release);

		/* NOTE: Frame region 0, like every material UBO: only the LOOK lives here, and a look change reaching a frame
		 * in flight one frame early is harmless. The points, which move every tick, never come here. */
		if ( !m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, 0, m_properties.data()) )
		{
			return false;
		}

		return true;
	}

	void
	PathResource::markVideoMemoryDirty () noexcept
	{
		/* One registration per flush: the flag is cleared by updateVideoMemory(). Before creation, create() uploads. */
		if ( m_renderer == nullptr || !this->isCreated() || m_videoMemoryUpdated.exchange(true, std::memory_order_acq_rel) )
		{
			return;
		}

		m_renderer->requestMaterialVideoMemoryUpdate(std::static_pointer_cast< Interface >(this->weak_from_this().lock()));
	}

	uint32_t
	PathResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	PathResource::UBOOffset () const noexcept
	{
		/* LOCAL offset within the element's bank (see StandardResource::UBOOffset()). */
		return static_cast< uint32_t >(m_sharedUniformBuffer->getByteOffsetForElement(m_sharedUBOIndex, 0));
	}


	Declaration::UniformBlock
	PathResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::PathMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PathRadiance);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PathStyle);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PathPlacement);

		return block;
	}

	bool
	PathResource::prepareVertexStage (Generator::Abstract & /*generator*/, AbstractVertexStage & vertexShader) const noexcept
	{
		vertexShader.enablePathRibbon(MaterialUB(UniformBlock::Component::PathStyle), MaterialUB(UniformBlock::Component::PathPlacement));

		return true;
	}

	bool
	PathResource::generateVertexShaderCode (Generator::Abstract & generator, AbstractVertexStage & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The path material '" << this->name() << "' is not created ! It can't generate a vertex shader source code.";

			return false;
		}

		/* The ribbon reads the style (AbstractVertexStage::preparePathRibbon()). */
		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		if ( !vertexShader.declare(this->getUniformBlock(materialSet, 0)) )
		{
			TraceError{ClassId} << "Unable to declare the material uniform block in the vertex stage of the path '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	PathResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & /*lightGenerator*/, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The path material '" << this->name() << "' is not created ! It can't generate a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, 0) )
		{
			return false;
		}

		const auto style = MaterialUB(UniformBlock::Component::PathStyle);

		/* Round joins and caps: the capsule around the segment — the distance to the segment beyond the half width is
		 * discarded (the quad was extended by the half width at both ends). */
		Code{fragmentShader, Location::Top} << PathGLSL::roundDiscard(ShaderVariable::PathCoordinates, style);

		return true;
	}

	bool
	PathResource::generateShadowVertexCode (const Generator::Abstract & generator, AbstractVertexStage & vertexShader) const noexcept
	{
		/* The same ribbon as the scene pass (the generator switched the stage to the instance-transforms path and
		 * declared the view block, Generator::ShadowCasting::isPulledVertexGeometry()). */
		vertexShader.enablePathRibbon(MaterialUB(UniformBlock::Component::PathStyle), MaterialUB(UniformBlock::Component::PathPlacement));

		if ( !vertexShader.declare(this->getUniformBlock(generator.shaderProgram()->setIndex(SetType::PerModelLayer), 0)) )
		{
			TraceError{ClassId} << "Unable to declare the material uniform block in the depth-only vertex stage of the path '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	PathResource::generateShadowAlphaTestCode (const Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		if ( !fragmentShader.declare(this->getUniformBlock(generator.shaderProgram()->setIndex(SetType::PerModelLayer), 0)) )
		{
			TraceError{ClassId} << "Unable to declare the material uniform block in the depth-only fragment stage of the path '" << this->name() << "' !";

			return false;
		}

		/* The silhouette of a round path is its capsule, like its colour. */
		Code{fragmentShader, Location::Top} << PathGLSL::roundDiscard(ShaderVariable::PathCoordinates, MaterialUB(UniformBlock::Component::PathStyle));

		return true;
	}

	std::string
	PathResource::fragmentColor () const noexcept
	{
		/* Radiance in nits: the unlit path writes it as it is (SceneRendering's unlit branch), opaque. */
		return std::string{"vec4("} + MaterialUB(UniformBlock::Component::PathRadiance) + ".rgb, 1.0)";
	}

	void
	PathResource::enableBlending (BlendingMode mode) noexcept
	{
		TraceWarning{ClassId} << "A world path is opaque: blending '" << to_cstring(mode) << "' is ignored (the path's debug mode is the translucent one) !";
	}

	void
	PathResource::updateRadiance () noexcept
	{
		m_properties[RadianceOffset] = m_color.red() * m_luminance;
		m_properties[RadianceOffset + 1] = m_color.green() * m_luminance;
		m_properties[RadianceOffset + 2] = m_color.blue() * m_luminance;
	}

	void
	PathResource::setColor (const Color< float > & color) noexcept
	{
		m_color = color;

		this->updateRadiance();
		this->markVideoMemoryDirty();
	}

	void
	PathResource::setLuminance (float nits) noexcept
	{
		m_luminance = std::max(0.0F, nits);

		this->updateRadiance();
		this->markVideoMemoryDirty();
	}

	void
	PathResource::setWidth (float halfWidth, bool inPixels) noexcept
	{
		m_properties[StyleOffset] = std::max(0.0F, halfWidth);
		m_properties[StyleOffset + 1] = inPixels ? 1.0F : 0.0F;

		this->markVideoMemoryDirty();
	}

	void
	PathResource::setJoins (bool round, float miterLimit) noexcept
	{
		m_properties[StyleOffset + 2] = round ? 1.0F : 0.0F;
		m_properties[StyleOffset + 3] = std::max(1.0F, miterLimit);

		this->markVideoMemoryDirty();
	}

	void
	PathResource::setFlat (bool state, const Vector< 3, float > & up) noexcept
	{
		const auto length = up.length();

		/* A zero or non-finite up cannot orient anything: the ribbon faces the eye. */
		const bool flat = state && std::isfinite(length) && length > 1.0e-6F;

		m_properties[PlacementOffset + 1] = flat ? up[X] / length : 0.0F;
		m_properties[PlacementOffset + 2] = flat ? up[Y] / length : 0.0F;
		m_properties[PlacementOffset + 3] = flat ? up[Z] / length : 0.0F;

		this->markVideoMemoryDirty();
	}

	void
	PathResource::setDepthOffset (float offset) noexcept
	{
		m_properties[PlacementOffset] = std::isfinite(offset) ? std::max(0.0F, offset) : 0.0F;

		this->markVideoMemoryDirty();
	}
}
