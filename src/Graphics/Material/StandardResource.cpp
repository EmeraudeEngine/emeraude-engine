/*
 * src/Graphics/Material/StandardResource.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "StandardResource.hpp"

/* STL inclusions. */
#include <cstdlib>
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/FastJSON.hpp"
#include "Saphir/Declaration/StageOutput.hpp"
#include "Saphir/Declaration/Types.hpp"
#include "Saphir/Declaration/UniformBlock.hpp"
#include "Saphir/Generator/Abstract.hpp"
#include "Saphir/VertexShader.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Types.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Resources/Manager.hpp"
#include "Component/Color.hpp"
#include "Component/Interface.hpp"
#include "Component/Texture.hpp"
#include "Component/Value.hpp"
#include "Helpers.hpp"
#include "Tracer.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::Material::StandardResource >::ClassId{"StandardResourceContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::Material::StandardResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::Material
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Saphir;
	using namespace EmEn::Saphir::Keys;
	using namespace EmEn::Graphics::Material::Component;
	using namespace EmEn::Vulkan;

	const size_t StandardResource::ClassUID{getClassUID(ClassId)};

	bool
	StandardResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		this->setAmbientComponent(DefaultAmbientColor);
		this->setDiffuseComponent(DefaultDiffuseColor);
		this->setSpecularComponent(DefaultSpecularColor, DefaultShininess);

		return this->setLoadSuccess(true);
	}

	bool
	StandardResource::parseAmbientComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, AmbientString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);

				if ( !this->setAmbientComponent(color) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Ambient, std::make_unique< Texture >(Uniform::AmbientSampler, SurfaceAmbientColor, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for material '" << this->name() << "' resource ambient component !";

				return false;
		}
	}

	bool
	StandardResource::parseDiffuseComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, DiffuseString, fillingType, componentData, false) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);

				if ( !this->setDiffuseComponent(color) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Diffuse, std::make_unique< Texture >(Uniform::DiffuseSampler, SurfaceDiffuseColor, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);
			}
				return true;

			case FillingType::None :
			default:
				TraceError{ClassId} << "The diffuse component (mandatory) is not present or invalid in material '" << this->name() << "' resource JSON file !";

				return false;
		}
	}

	bool
	StandardResource::parseSpecularComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, SpecularString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);
				const auto shininess = FastJSON::getNumber< float >(data[SpecularString], JKShininess, DefaultShininess);

				if ( !this->setSpecularComponent(color, shininess) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Specular, std::make_unique< Texture >(Uniform::SpecularSampler, SurfaceSpecularColor, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);

				this->setShininess(FastJSON::getNumber< float >(data[SpecularString], JKShininess, DefaultShininess));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for material '" << this->name() << "' resource specular component !";

				return false;
		}
	}

	bool
	StandardResource::parseOpacityComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, OpacityString, fillingType, componentData, true) )
		{
			return false;
		}

		m_alphaThresholdToDiscard = FastJSON::getNumber< float >(data[OpacityString], JKAlphaThreshold, 0.1F);

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto value = parseValueComponent(componentData);

				if ( !this->setOpacityComponent(value) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Opacity, std::make_unique< Texture >(Uniform::OpacitySampler, SurfaceOpacityAmount, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(BlendingEnabled);
				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);

				this->setOpacity(FastJSON::getNumber< float >(data[OpacityString], JKAmount, DefaultOpacity));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for material '" << this->name() << "' resource opacity component !";

				return false;
		}
	}

	bool
	StandardResource::parseAutoIlluminationComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, AutoIlluminationString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);
				const auto amount = FastJSON::getNumber< float >(data[AutoIlluminationString], JKAmount, DefaultAutoIlluminationAmount);

				if ( !this->setAutoIlluminationComponent(color, amount) )
				{
					return false;
				}
			}
				return true;

			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Texture >(Uniform::AutoIlluminationSampler, SurfaceAutoIlluminationColor, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);

				this->setAutoIlluminationAmount(FastJSON::getNumber< float >(data[AutoIlluminationString], JKAmount, DefaultAutoIlluminationAmount));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for material '" << this->name() << "' resource auto-illumination component !";

				return false;
		}
	}

	bool
	StandardResource::parseNormalComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, NormalString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Normal, std::make_unique< Texture >(Uniform::NormalSampler, SurfaceNormalVector, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);

				this->setNormalScale(FastJSON::getNumber< float >(data[NormalString], JKScale, DefaultNormalScale));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for material '" << this->name() << "' resource normal component !";

				return false;
		}
	}

	bool
	StandardResource::parseReflectionComponent (const Json::Value & data, Resources::Manager & resources) noexcept
	{
		FillingType fillingType{};

		Json::Value componentData{};

		if ( !parseComponentBase(data, ReflectionString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Gradient :
			case FillingType::Texture :
			case FillingType::VolumeTexture :
			case FillingType::Cubemap :
			case FillingType::AnimatedTexture :
			{
				const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, componentData, fillingType, resources));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TexturingEnabled);
				// FIXME: Check UVW channel number
				this->enableFlag(UsesPrimaryTextureCoordinates);

				this->setReflectionAmount(FastJSON::getNumber< float >(data[ReflectionString], JKAmount, DefaultReflectionAmount));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for material '" << this->name() << "' resource reflection component !";

				return false;
		}
	}

	bool
	StandardResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		auto * resources = Resources::Manager::instance();

		if ( !this->parseAmbientComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the ambient component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseDiffuseComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the diffuse component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseSpecularComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the specular component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseAutoIlluminationComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the auto-illumination component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseOpacityComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the opacity component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseNormalComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the normal component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseReflectionComponent(data, *resources) )
		{
			TraceError{ClassId} << "Error while parsing the reflection component for material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( m_components.empty() )
		{
			TraceError{ClassId} << "No component could be read from material '" << this->name() << "' resource JSON file !";

			return this->setLoadSuccess(false);
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() != Type::Texture )
			{
				continue;
			}

			const auto textureResource = component->textureResource();

			if ( !this->addDependency(textureResource) )
			{
				TraceError{ClassId} << "Unable to link the texture '" << textureResource->name() << "' dependency to material '" << this->name() << "' for ambient component !";

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	StandardResource::createOnHardware (Renderer & renderer) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The material resource '" << this->name() << "' is already created !";

			return true;
		}

		if ( m_components.empty() )
		{
			TraceError{ClassId} << "The material resource '" << this->name() << "' has no component !";

			return false;
		}

		/* Component creation (optional). */
		if ( this->usingTexture() )
		{
			/* NOTE: Starts to 1 because there is the UBO in the first place. */
			uint32_t binding = 1;

			for ( const auto & [componentType, component] : m_components )
			{
				if ( component->type() != Type::Texture )
				{
					continue;
				}

				if ( !component->create(renderer, binding))
				{
					TraceError{ClassId} << "Unable to create component '" << to_cstring(componentType) << "' of material resource '" << this->name() << "' !";

					return false;
				}
			}
		}

		/* Create the material UBO, sampler, descriptor set ... */
		if ( !this->createVideoMemory(renderer) )
		{
			Tracer::error(ClassId, "Unable to the material onto the GPU !");

			return false;
		}

		/* Initialize the material data in the GPU. */
		if ( !this->updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the initial video memory !");

			return false;
		}

		this->enableFlag(Created);

		return true;
	}

	std::string
	StandardResource::getSharedUniformBufferIdentifier () const noexcept
	{
		std::stringstream identifier{};

		uint32_t textureCount = 0;

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() == Type::Texture )
			{
				textureCount++;
			}
		}

		identifier << ClassId;

		if ( textureCount > 0 )
		{
			identifier << textureCount << "Textures";
		}
		else
		{
			identifier << "Simple";
		}

		return identifier.str();
	}

	bool
	StandardResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		if ( !m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex) )
		{
			Tracer::error(ClassId, "Unable to add the material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	StandardResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
	{
		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(identifier);

		if ( m_descriptorSetLayout == nullptr )
		{
			uint32_t bindingPoint = 0;

			m_descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(identifier);
			m_descriptorSetLayout->setIdentifier(ClassId, identifier, "DescriptorSetLayout");

			/* Declare the UBO for the material properties. */
			m_descriptorSetLayout->declareUniformBuffer(bindingPoint++, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

			/* Declare every sampler used by the material. */
			for ( const auto & component : std::ranges::views::values(m_components) )
			{
				if ( component->type() == Type::Texture )
				{
					m_descriptorSetLayout->declareCombinedImageSampler(bindingPoint++, VK_SHADER_STAGE_FRAGMENT_BIT);
				}
			}

			if ( !layoutManager.createDescriptorSetLayout(m_descriptorSetLayout) )
			{
				return false;
			}
		}

		return true;
	}

	bool
	StandardResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & uniformBufferObject) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for material '" << this->name() << "' !";

			return false;
		}

		uint32_t bindingPoint = 0;

		if ( !m_descriptorSet->writeUniformBufferObject(bindingPoint++, uniformBufferObject, m_sharedUBOIndex) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer object to the descriptor set of material '" << this->name() << "' !";

			return false;
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() != Type::Texture )
			{
				continue;
			}

			const auto texture = component->textureResource();

			if ( !m_descriptorSet->writeCombinedImageSampler(bindingPoint++, *texture) )
			{
				TraceError{ClassId} << "Unable to write the texture object '" << texture->name() << "' to the descriptor set of material '" << this->name() << "' !";

				return false;
			}
		}

		return true;
	}

	void
	StandardResource::onMaterialLoaded () noexcept
	{
		// Nothing to do for now
	}

	bool
	StandardResource::createVideoMemory (Renderer & renderer) noexcept
	{
		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	StandardResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		if ( !m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, m_materialProperties.data()) )
		{
			return false;
		}

		m_flags[VideoMemoryUpdateRequested] = false;

		return true;
	}

	void
	StandardResource::destroyFromHardware () noexcept
	{
		/* FIXME: Crash here on application closing ! */
		m_sharedUniformBuffer->removeElement(this);

		/* Reset to defaults. */
		this->resetFlags();

		/* Reset member variables. */
		m_physicalSurfaceProperties.reset();
		m_components.clear();
		m_blendingMode = BlendingMode::None;
		m_materialProperties = {
			/* Ambient color (4), */
			DefaultAmbientColor.red(), DefaultAmbientColor.green(), DefaultAmbientColor.blue(), DefaultDiffuseColor.alpha(),
			/* Diffuse color (4), */
			DefaultDiffuseColor.red(), DefaultDiffuseColor.green(), DefaultDiffuseColor.blue(), DefaultDiffuseColor.alpha(),
			/* Specular color (4), */
			DefaultSpecularColor.red(), DefaultSpecularColor.green(), DefaultSpecularColor.blue(), DefaultSpecularColor.alpha(),
			/* Auto-illumination color (4), */
			DefaultAutoIlluminationColor.red(), DefaultAutoIlluminationColor.green(), DefaultAutoIlluminationColor.blue(), DefaultAutoIlluminationColor.alpha(),
			/* Shininess (1), Opacity (1), AutoIlluminationColor (1), NormalScale (1). */
			DefaultShininess, DefaultOpacity, DefaultAutoIlluminationAmount, DefaultNormalScale,
			/* ReflectionAmount (1), Unused (1), Unused (1), Unused (1). */
			DefaultReflectionAmount, 0.0F, 0.0F, 0.0F
		};
		m_descriptorSetLayout.reset();
		m_descriptorSet.reset();
		m_sharedUniformBuffer.reset();
		m_sharedUBOIndex = 0;
	}

	bool
	StandardResource::isCreated () const noexcept
	{
		return this->isFlagEnabled(Created);
	}

	bool
	StandardResource::isComplex () const noexcept
	{
		return this->isComponentPresent(ComponentType::Reflection);
	}

	const Physics::PhysicalSurfaceProperties &
	StandardResource::physicalSurfaceProperties () const noexcept
	{
		return m_physicalSurfaceProperties;
	}

	Physics::PhysicalSurfaceProperties &
	StandardResource::physicalSurfaceProperties () noexcept
	{
		return m_physicalSurfaceProperties;
	}

	uint32_t
	StandardResource::frameCount () const noexcept
	{
		if ( !this->isFlagEnabled(Animated) )
		{
			return 1;
		}

		// TODO: Which component is animated ?

		return 1;
	}

	uint32_t
	StandardResource::duration () const noexcept
	{
		if ( !this->isFlagEnabled(Animated) )
		{
			return 0;
		}

		// TODO: Which component is animated ?

		return 0;
	}

	uint32_t
	StandardResource::frameIndexAt (uint32_t /*sceneTime*/) const noexcept
	{
		/*if ( !this->isFlagEnabled(Animated) )
		{
			return 0;
		}*/

		// TODO: Which component is animated ?

		return 0;
	}

	void
	StandardResource::enableBlending (BlendingMode mode) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The resource '" << this->name() << "' is already created ! Unable to enabled a blending mode.";

			return;
		}

		this->enableFlag(BlendingEnabled);

		m_blendingMode = mode;
	}

	BlendingMode
	StandardResource::blendingMode () const noexcept
	{
		if ( !this->isFlagEnabled(BlendingEnabled) )
		{
			return BlendingMode::None;
		}

		return m_blendingMode;
	}

	bool
	StandardResource::setupLightGenerator (LightGenerator & lightGenerator) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The standard material '" << this->name() << "' is not created !"
				"It can't configure the light generator.";

			return false;
		}

		/* Ambient component */
		{
			const auto componentIt = m_components.find(ComponentType::Ambient);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAmbient(componentIt->second->variableName());
			}
		}

		/* Diffuse component */
		{
			const auto componentIt = m_components.find(ComponentType::Diffuse);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceDiffuse(componentIt->second->variableName());
			}
		}

		/* Specular component */
		{
			const auto componentIt = m_components.find(ComponentType::Specular);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceSpecular(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::Shininess));
			}
		}

		/* Opacity component */
		{
			const auto componentIt = m_components.find(ComponentType::Opacity);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceOpacity(componentIt->second->variableName());
			}
		}

		/* Auto-illumination component */
		{
			const auto componentIt = m_components.find(ComponentType::AutoIllumination);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAutoIllumination(componentIt->second->variableName());
			}
		}

		/* Normal component */
		if ( !lightGenerator.isAmbientPass() )
		{
			const auto componentIt = m_components.find(ComponentType::Normal);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceNormal(componentIt->second->variableName());
			}
		}

		/* Reflection component */
		{
			const auto componentIt = m_components.find(ComponentType::Reflection);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceReflection(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::ReflectionAmount));
			}
		}

		return true;
	}

	std::string
	StandardResource::fragmentColor () const noexcept
	{
		std::string base;

		if ( this->isComponentPresent(ComponentType::Diffuse) )
		{
			base = m_components.at(ComponentType::Diffuse)->variableName();
		}
		else if ( this->isComponentPresent(ComponentType::Ambient) )
		{
			base = m_components.at(ComponentType::Ambient)->variableName();
		}
		else
		{
			base = "vec4(0.5, 0.5, 0.5, 1.0)";
		}

		if ( this->isComponentPresent(ComponentType::Reflection) )
		{
			std::stringstream subCode;
			subCode << "mix(" << base << ", " << m_components.at(ComponentType::Reflection)->variableName() << ", " << MaterialUB(UniformBlock::Component::ReflectionAmount) << ")";

			base = subCode.str();
		}

		std::stringstream code;

		if ( this->isComponentPresent(ComponentType::Opacity) )
		{
			code << "vec4((" << base << ").xyz, " << m_components.at(ComponentType::Opacity)->variableName() << ")";
		}
		else
		{
			code << '(' << base << ')';
		}

		return code.str();
	}

	std::shared_ptr< DescriptorSetLayout >
	StandardResource::descriptorSetLayout () const noexcept
	{
		return m_descriptorSetLayout;
	}

	uint32_t
	StandardResource::UBOIndex () const noexcept
	{
		return m_sharedUBOIndex;
	}

	uint32_t
	StandardResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	StandardResource::UBOOffset () const noexcept
	{
		return m_sharedUBOIndex * m_sharedUniformBuffer->blockAlignedSize();
	}

	const DescriptorSet *
	StandardResource::descriptorSet () const noexcept
	{
		/* NOTE: This is no more a dynamic. */
		//return m_sharedUniformBuffer->descriptorSet(m_sharedUBOIndex);
		return m_descriptorSet.get();
	}

	Declaration::UniformBlock
	StandardResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::StandardMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AmbientColor);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::DiffuseColor);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::SpecularColor);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AutoIlluminationColor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Shininess);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Opacity);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AutoIlluminationAmount);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::NormalScale);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ReflectionAmount);

		return block;
	}

	bool
	StandardResource::generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The standard material '" << this->name() << "' is not created !"
				"It can't generates a vertex shader source code.";

			return false;
		}

		const auto * geometry = generator.getGeometryInterface();

		if ( !generator.highQualityLightEnabled() && !generator.declareMaterialUniformBlock(*this, vertexShader, 0) )
		{
			return false;
		}

		/* Check texture coordinate attributes. */
		if ( this->usingTexture() )
		{
			if ( this->usingPrimaryTextureCoordinates() && !checkPrimaryTextureCoordinates(generator, vertexShader, *this, *geometry) )
			{
				return false;
			}

			if ( this->usingSecondaryTextureCoordinates() && !checkSecondaryTextureCoordinates(generator, vertexShader, *this, *geometry) )
			{
				return false;
			}
		}

		/* Check if the material needs vertex colors. */
		if ( this->usingVertexColors() )
		{
			if ( !geometry->vertexColorEnabled() )
			{
				TraceError{ClassId} <<
					"The geometry " << geometry->name() << " has no vertex color "
					"for standard material '" << this->name() << "' !";

				return false;
			}

			vertexShader.requestSynthesizeInstruction(ShaderVariable::PrimaryVertexColor);
		}

		if ( this->isComponentPresent(ComponentType::Reflection) )
		{
			if ( generator.highQualityReflectionEnabled() )
			{
				vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace);

				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					vertexShader.requestSynthesizeInstruction(ShaderVariable::WorldToTangentMatrix);
				}
				else
				{
					vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace);
				}

				/* TODO: If this is requested more than once, create a new synthesization instruction.
				 * TODO²: Checks if the inverse view matrix is not already performed for the light calculations. */
				vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, "CameraWorldPosition", GLSL::Flat});

				Code(vertexShader) <<
					"const mat4 InverseViewMatrix = inverse(" << MatrixPC(PushConstant::Component::ViewMatrix) << ");" << Line::End <<
					"CameraWorldPosition = InverseViewMatrix[3].xyz;";
			}
			else
			{
				vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, VariableScope::Local);
				vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace, VariableScope::Local);

				vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::ReflectionTextureCoordinates, GLSL::Smooth});

				Code(vertexShader) << ShaderVariable::ReflectionTextureCoordinates << " = reflect(normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - " << ViewUB(UniformBlock::Component::PositionWorldSpace) << ".xyz), " << ShaderVariable::NormalWorldSpace << ");";
			}
		}

		return true;
	}

	bool
	StandardResource::generateTextureComponentFragmentShader (ComponentType componentType, const std::function< bool (FragmentShader &, const Texture *) > & codeGenerator, FragmentShader & fragmentShader, uint32_t materialSet) const noexcept
	{
		const auto componentIt = m_components.find(componentType);

		/* NOTE: The component must exist and use a texture. */
		if ( componentIt == m_components.cend() || componentIt->second->type() != Type::Texture )
		{
			return true;
		}

		const auto * component = dynamic_cast< const Texture * >(componentIt->second.get());

		if ( !fragmentShader.declare(Declaration::Sampler{materialSet, component->binding(), component->textureType(), component->samplerName()}) )
		{
			return false;
		}

		return codeGenerator(fragmentShader, component);
	}

	const char *
	StandardResource::textCoords (const Texture * component) noexcept
	{
		return component->isVolumetricTexture() ?
			ShaderVariable::Primary3DTextureCoordinates :
			ShaderVariable::Primary2DTextureCoordinates;
	}

	bool
	StandardResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & lightGenerator, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The standard material '" << this->name() << "' is not created !"
				"It can't generates a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, 0) )
		{
			return false;
		}

		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		/* Normal component.
		 * NOTE: Get a sample from a texture in range [0,1], convert it to a normalized range of [-1, 1]. */
		if ( this->isComponentPresent(ComponentType::Reflection) || !lightGenerator.isAmbientPass() )
		{
			if ( !this->generateTextureComponentFragmentShader(ComponentType::Normal, [] (FragmentShader & shader, const Texture * component) {
				Code{shader, Location::Top} << "const vec3 " << component->variableName() << " = normalize(texture(" << component->samplerName() << ", " << textCoords(component) << ").rgb * 2.0 - 1.0);";

				return true;
			}, fragmentShader, materialSet) )
			{
				TraceError{ClassId} << "Unable to generate fragment code for the normal component of material '" << this->name() << "' !";

				return false;
			}
		}

		/* Ambient, Diffuse and Specular components. */
		{
			auto simpleGenerator = [] (FragmentShader & shader, const Texture * component) {
				Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ");";

				return true;
			};

			if ( !this->generateTextureComponentFragmentShader(ComponentType::Ambient, simpleGenerator, fragmentShader, materialSet) )
			{
				TraceError{ClassId} << "Unable to generate fragment code for the ambient component of material '" << this->name() << "' !";

				return false;
			}

			if ( this->isComponentPresent(ComponentType::AutoIllumination) || !lightGenerator.isAmbientPass() )
			{
				if ( !this->generateTextureComponentFragmentShader(ComponentType::Diffuse, simpleGenerator, fragmentShader, materialSet) )
				{
					TraceError{ClassId} << "Unable to generate fragment code for the diffuse component of material '" << this->name() << "' !";

					return false;
				}
			}

			if ( !this->generateTextureComponentFragmentShader(ComponentType::Specular, simpleGenerator, fragmentShader, materialSet) )
			{
				TraceError{ClassId} << "Unable to generate fragment code for the specular component of material '" << this->name() << "' !";

				return false;
			}
		}

		/* Opacity component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Opacity, [this] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} <<
				"const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r * " << MaterialUB(UniformBlock::Component::Opacity) << ";" << Line::End <<
				"if ( " << component->variableName() << " <= " << m_alphaThresholdToDiscard << " ) { discard; }" << Line::End;

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the opacity component of material '" << this->name() << "' !";

			return false;
		}

		/* Auto-illumination component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::AutoIllumination, [] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ") * " << MaterialUB(UniformBlock::Component::AutoIlluminationAmount) << ';';

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the auto-illumination component of material '" << this->name() << "' !";

			return false;
		}

		/* Reflection component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Reflection, [&] (FragmentShader & shader, const Texture * component) {
			if ( generator.highQualityReflectionEnabled() )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::WorldToTangentMatrix << " * " << SurfaceNormalVector << ");";
				}
				else
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				Code(shader, Location::Top) <<
					"const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);" << Line::End <<
					"const vec3 " << ShaderVariable::ReflectionTextureCoordinates << " = reflect(reflectionI, reflectionNormal);" << Line::End <<
					"const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ");";
			}
			else
			{
				Code(shader, Location::Top) << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ");";
			}

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the reflection component of material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	StandardResource::setAmbientComponent (const PixelFactory::Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the ambient component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AmbientColor);

		const auto result = m_components.emplace(ComponentType::Ambient, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setAmbientColor(color);

		return true;
	}

	bool
	StandardResource::setAmbientComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the ambient component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Ambient, std::make_unique< Texture >(Uniform::AmbientSampler, SurfaceAmbientColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for ambient component !";

			return false;
		}

		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::setDiffuseComponent (const PixelFactory::Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the diffuse component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::DiffuseColor);

		const auto result = m_components.emplace(ComponentType::Diffuse, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setDiffuseColor(color);

		return true;
	}

	bool
	StandardResource::setDiffuseComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the diffuse component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Diffuse, std::make_unique< Texture >(Uniform::DiffuseSampler, SurfaceDiffuseColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for diffuse component !";

			return false;
		}

		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		return true;
	}

	bool
	StandardResource::setSpecularComponent (const PixelFactory::Color< float > & color, float shininess) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the specular component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::SpecularColor);

		const auto result = m_components.emplace(ComponentType::Specular, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setSpecularColor(color);
		this->setShininess(shininess);

		return true;
	}

	bool
	StandardResource::setSpecularComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float shininess) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the specular component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Specular, std::make_unique< Texture >(Uniform::SpecularSampler, SurfaceSpecularColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for specular component !";

			return false;
		}

		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		this->setShininess(shininess);

		return true;
	}

	bool
	StandardResource::setOpacityComponent (float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the opacity component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::Opacity);

		const auto result = m_components.emplace(ComponentType::Opacity, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->enableFlag(BlendingEnabled);

		this->setOpacity(amount);

		return true;
	}

	bool
	StandardResource::setOpacityComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the opacity component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Opacity, std::make_unique< Texture >(Uniform::OpacitySampler, SurfaceOpacityAmount, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for opacity component !";

			return false;
		}

		this->enableFlag(BlendingEnabled);
		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		this->setOpacity(amount);

		return true;
	}

	bool
	StandardResource::setAutoIlluminationComponent (float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the auto-illumination component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AutoIlluminationColor);

		m_components[ComponentType::AutoIllumination] = std::make_unique< Value >(uniform);

		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	StandardResource::setAutoIlluminationComponent (const PixelFactory::Color< float > & color, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the auto-illumination component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AutoIlluminationColor);

		const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setAutoIlluminationColor(color);
		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	StandardResource::setAutoIlluminationComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the auto-illumination component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Texture >(Uniform::AutoIlluminationSampler, SurfaceAutoIlluminationColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for auto-illumination component !";

			return false;
		}

		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	StandardResource::setNormalComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the normal component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Normal, std::make_unique< Texture >(Uniform::NormalSampler, SurfaceNormalVector, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for normal component !";

			return false;
		}

		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		this->setNormalScale(scale);

		return true;
	}

	bool
	StandardResource::setReflectionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflection component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to material '" << this->name() << "' for reflection component !";

			return false;
		}


		this->enableFlag(TexturingEnabled);
		this->enableFlag(UsesPrimaryTextureCoordinates);

		this->setReflectionAmount(amount);

		return true;
	}

	bool
	StandardResource::isComponentPresent (ComponentType componentType) const noexcept
	{
		return m_components.contains(componentType);
	}

	void
	StandardResource::setAmbientColor (const PixelFactory::Color< float > & color) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Ambient) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no ambient component !";

			return;
		}

		m_materialProperties[AmbientColorOffset] = color.red();
		m_materialProperties[AmbientColorOffset+1] = color.green();
		m_materialProperties[AmbientColorOffset+2] = color.blue();
		m_materialProperties[AmbientColorOffset+3] = color.alpha();

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setDiffuseColor (const PixelFactory::Color< float > & color) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Diffuse) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no diffuse component !";

			return;
		}

		m_materialProperties[DiffuseColorOffset] = color.red();
		m_materialProperties[DiffuseColorOffset+1] = color.green();
		m_materialProperties[DiffuseColorOffset+2] = color.blue();
		m_materialProperties[DiffuseColorOffset+3] = color.alpha();

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setSpecularColor (const PixelFactory::Color< float > & color) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Specular) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no specular component !";

			return;
		}

		m_materialProperties[SpecularColorOffset] = color.red();
		m_materialProperties[SpecularColorOffset+1] = color.green();
		m_materialProperties[SpecularColorOffset+2] = color.blue();
		m_materialProperties[SpecularColorOffset+3] = color.alpha();

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setAutoIlluminationColor (const PixelFactory::Color< float > & color) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::AutoIllumination) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no auto-illumination component !";

			return;
		}

		m_materialProperties[AutoIlluminationColorOffset] = color.red();
		m_materialProperties[AutoIlluminationColorOffset+1] = color.green();
		m_materialProperties[AutoIlluminationColorOffset+2] = color.blue();
		m_materialProperties[AutoIlluminationColorOffset+3] = color.alpha();

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setShininess (float value) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Specular) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no specular component !";

			return;
		}

		m_materialProperties[ShininessOffset] = std::abs(value);

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setOpacity (float value) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Opacity) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no opacity component !";

			return;
		}

		m_materialProperties[OpacityOffset] = clampToUnit(value);

		m_flags[VideoMemoryUpdateRequested] = true;
	}

	void
	StandardResource::setAlphaThresholdToDiscard (float value) noexcept
	{
		m_alphaThresholdToDiscard = clampToUnit(value);
	}
	
	void
	StandardResource::setAutoIlluminationAmount (float value) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::AutoIllumination) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no auto-illumination component !";

			return;
		}

		m_materialProperties[AutoIlluminationAmountOffset] = std::abs(value);

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setNormalScale (float value) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Normal) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no normal component !";

			return;
		}

		m_materialProperties[NormalScaleOffset] = value;

		m_flags[VideoMemoryUpdateRequested] = true;
	}
	
	void
	StandardResource::setReflectionAmount (float value) noexcept
	{
		if ( !this->isComponentPresent(ComponentType::Reflection) )
		{
			TraceWarning{ClassId} << "The material '" << this->name() << "' has no reflection component !";

			return;
		}

		m_materialProperties[ReflectionAmountOffset] = clampToUnit(value);

		m_flags[VideoMemoryUpdateRequested] = true;
	}
}
