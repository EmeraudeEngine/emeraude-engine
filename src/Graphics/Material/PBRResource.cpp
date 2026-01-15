/*
 * src/Graphics/Material/PBRResource.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "PBRResource.hpp"

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
#include "Vulkan/TextureInterface.hpp"
#include "Resources/Manager.hpp"
#include "Component/Color.hpp"
#include "Component/Interface.hpp"
#include "Component/Texture.hpp"
#include "Component/Value.hpp"
#include "Helpers.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::Material
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics::Material::Component;
	using namespace Vulkan;

	bool
	PBRResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Default PBR material: grey dielectric with medium roughness. */
		this->setAlbedoComponent(DefaultAlbedoColor);
		this->setRoughnessComponent(DefaultRoughness);
		this->setMetalnessComponent(DefaultMetalness);

		return this->setLoadSuccess(true);
	}

	bool
	PBRResource::parseAlbedoComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		/* Try "Albedo" first, fallback to "Diffuse" for Standard material compatibility. */
		if ( !parseComponentBase(data, AlbedoString, fillingType, componentData, true) )
		{
			return false;
		}

		if ( fillingType == FillingType::None )
		{
			/* Fallback: try "Diffuse" key from Standard material format. */
			if ( !parseComponentBase(data, DiffuseString, fillingType, componentData, true) )
			{
				return false;
			}
		}

		switch ( fillingType )
		{
			case FillingType::Color :
			{
				const auto color = parseColorComponent(componentData);

				if ( !this->setAlbedoComponent(color) )
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
				const auto result = m_components.emplace(ComponentType::Albedo, std::make_unique< Texture >(Uniform::AlbedoSampler, SurfaceAlbedoColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);
			}
				return true;

			case FillingType::None :
			default:
				TraceError{ClassId} << "The albedo component (mandatory) is not present or invalid in PBR material '" << this->name() << "' resource JSON file ! Tried both 'Albedo' and 'Diffuse' keys.";

				return false;
		}
	}

	bool
	PBRResource::parseRoughnessComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, RoughnessString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto value = parseValueComponent(componentData);

				if ( !this->setRoughnessComponent(value) )
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
				const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Texture >(Uniform::RoughnessSampler, SurfaceRoughness, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setRoughness(FastJSON::getValue< float >(data[RoughnessString], JKValue).value_or(DefaultRoughness));
			}
				return true;

			case FillingType::None :
			{
				/* Fallback: try "Specular" key from Standard material format (inverted). */
				if ( !parseComponentBase(data, SpecularString, fillingType, componentData, true) )
				{
					return false;
				}

				switch ( fillingType )
				{
					case FillingType::Value :
					{
						/* Specular value → Roughness (inverted): high specular = low roughness. */
						const auto specularValue = parseValueComponent(componentData);

						if ( !this->setRoughnessComponent(1.0F - specularValue) )
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
						/* Specular texture → Roughness texture (inverted automatically). */
						const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Texture >(Uniform::RoughnessSampler, SurfaceRoughness, componentData, fillingType, serviceProvider));

						if ( !result.second || result.first->second == nullptr )
						{
							return false;
						}

						this->enableFlag(TextureEnabled);
						this->enableFlag(UsePrimaryTextureCoordinates);

						/* Auto-invert since we're using a specular/gloss map as roughness source. */
						m_invertRoughness = true;

						this->setRoughness(FastJSON::getValue< float >(data[SpecularString], JKValue).value_or(DefaultRoughness));
					}
						return true;

					case FillingType::None :
					{
						/* Last fallback: try Shininess value from Specular component. */
						const auto shininessOpt = FastJSON::getValue< float >(data[SpecularString], JKShininess);

						if ( shininessOpt.has_value() )
						{
							/* Shininess → Roughness conversion: high shininess = low roughness.
							 * Formula: roughness = 1.0 - sqrt(shininess / 128.0), clamped to [0,1]. */
							const auto shininess = std::clamp(shininessOpt.value(), 1.0F, 128.0F);
							const auto roughness = 1.0F - std::sqrt(shininess / 128.0F);

							this->setRoughnessComponent(roughness);
						}
						else
						{
							/* No roughness, no specular texture, no shininess - use default. */
							this->setRoughnessComponent(DefaultRoughness);
						}
					}
						return true;

					default:
						TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource roughness component (from Specular fallback) !";

						return false;
				}
			}

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource roughness component !";

				return false;
		}
	}

	bool
	PBRResource::parseMetalnessComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, MetalnessString, fillingType, componentData, true) )
		{
			return false;
		}

		switch ( fillingType )
		{
			case FillingType::Value :
			{
				const auto value = parseValueComponent(componentData);

				if ( !this->setMetalnessComponent(value) )
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
				const auto result = m_components.emplace(ComponentType::Metalness, std::make_unique< Texture >(Uniform::MetalnessSampler, SurfaceMetalness, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setMetalness(FastJSON::getValue< float >(data[MetalnessString], JKValue).value_or(DefaultMetalness));
			}
				return true;

			case FillingType::None :
				/* Metalness is optional, use default (dielectric). */
				this->setMetalnessComponent(DefaultMetalness);
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource metalness component !";

				return false;
		}
	}

	bool
	PBRResource::parseNormalComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
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
				const auto result = m_components.emplace(ComponentType::Normal, std::make_unique< Texture >(Uniform::NormalSampler, SurfaceNormalVector, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setNormalScale(FastJSON::getValue< float >(data[NormalString], JKScale).value_or(DefaultNormalScale));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource normal component !";

				return false;
		}
	}

	bool
	PBRResource::parseReflectionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		/* Check for "Automatic" keyword - use scene environment cubemap at render time. */
		if ( data.isMember(ReflectionString) && data[ReflectionString].isString() )
		{
			if ( data[ReflectionString].asString() == AutomaticString )
			{
				this->enableAutomaticReflection(DefaultIBLIntensity);

				return true;
			}
		}

		/* Check for "Automatic" inside object with IBLIntensity. */
		if ( data.isMember(ReflectionString) && data[ReflectionString].isObject() )
		{
			const auto & reflectionData = data[ReflectionString];

			if ( reflectionData.isMember(JKCubemap) && reflectionData[JKCubemap].isString() )
			{
				if ( reflectionData[JKCubemap].asString() == AutomaticString )
				{
					const auto iblIntensity = FastJSON::getValue< float >(reflectionData, JKIBLIntensity).value_or(DefaultIBLIntensity);

					this->enableAutomaticReflection(iblIntensity);

					return true;
				}
			}
		}

		/* Standard parsing for explicit cubemap texture. */
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
				const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource reflection component !";

				return false;
		}
	}

	bool
	PBRResource::parseRefractionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, RefractionString, fillingType, componentData, true) )
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
				const auto result = m_components.emplace(ComponentType::Refraction, std::make_unique< Texture >(Uniform::RefractionSampler, SurfaceRefractionColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setIOR(FastJSON::getValue< float >(data[RefractionString], JKValue).value_or(DefaultIOR));
			}
				return true;

			case FillingType::None :
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource refraction component !";

				return false;
		}
	}

	bool
	PBRResource::parseAutoIlluminationComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
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
				const auto amount = FastJSON::getValue< float >(data[AutoIlluminationString], JKAmount).value_or(DefaultAutoIlluminationAmount);

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
				const auto result = m_components.emplace(ComponentType::AutoIllumination, std::make_unique< Texture >(Uniform::AutoIlluminationSampler, SurfaceAutoIlluminationColor, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setAutoIlluminationAmount(FastJSON::getValue< float >(data[AutoIlluminationString], JKAmount).value_or(DefaultAutoIlluminationAmount));
			}
				return true;

			case FillingType::None :
				/* AutoIllumination is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource auto-illumination component !";

				return false;
		}
	}

	bool
	PBRResource::parseAmbientOcclusionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		FillingType fillingType{};
		Json::Value componentData{};

		if ( !parseComponentBase(data, AmbientOcclusionString, fillingType, componentData, true) )
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
				const auto result = m_components.emplace(ComponentType::AmbientOcclusion, std::make_unique< Texture >(Uniform::AmbientOcclusionSampler, SurfaceAmbientOcclusion, componentData, fillingType, serviceProvider));

				if ( !result.second || result.first->second == nullptr )
				{
					return false;
				}

				this->enableFlag(TextureEnabled);
				this->enableFlag(UsePrimaryTextureCoordinates);

				this->setAOIntensity(FastJSON::getValue< float >(data[AmbientOcclusionString], JKAmount).value_or(DefaultAOIntensity));
			}
				return true;

			case FillingType::None :
				/* AmbientOcclusion is optional. */
				return true;

			default:
				TraceError{ClassId} << "Invalid filling type for PBR material '" << this->name() << "' resource ambient occlusion component !";

				return false;
		}
	}

	bool
	PBRResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->parseAlbedoComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the albedo component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseRoughnessComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the roughness component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseMetalnessComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the metalness component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseNormalComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the normal component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseReflectionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the reflection component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseRefractionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the refraction component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseAutoIlluminationComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the auto-illumination component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( !this->parseAmbientOcclusionComponent(data, serviceProvider) )
		{
			TraceError{ClassId} << "Error while parsing the ambient occlusion component for PBR material '" << this->name() << "' resource from JSON file !" "\n" "Data : " << data;

			return this->setLoadSuccess(false);
		}

		if ( m_components.empty() )
		{
			TraceError{ClassId} << "No component could be read from PBR material '" << this->name() << "' resource JSON file !";

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
				TraceError{ClassId} << "Unable to link the texture '" << textureResource->name() << "' dependency to PBR material '" << this->name() << "' !";

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	PBRResource::create (Renderer & renderer) noexcept
	{
		if ( m_components.empty() )
		{
			TraceError{ClassId} << "The PBR material resource '" << this->name() << "' has no component !";

			return false;
		}

		/* Component creation (optional). */
		if ( this->usingTexture() )
		{
			/* NOTE: Starts at 1 because there is the UBO in the first place. */
			uint32_t binding = 1;

			for ( const auto & [componentType, component] : m_components )
			{
				if ( component->type() != Type::Texture )
				{
					continue;
				}

				if ( !component->create(renderer, binding))
				{
					TraceError{ClassId} << "Unable to create component '" << to_cstring(componentType) << "' of PBR material resource '" << this->name() << "' !";

					return false;
				}
			}
		}

		const auto identifier = this->getSharedUniformBufferIdentifier();

		if ( !this->createElementInSharedBuffer(renderer, identifier) )
		{
			TraceError{ClassId} << "Unable to create the data inside the shared uniform buffer '" << identifier << "' for PBR material '" << this->name() << "' !";

			return false;
		}

		if ( !this->createDescriptorSetLayout(renderer.layoutManager(), identifier) )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout for PBR material '" << this->name() << "' !";

			return false;
		}

		/* NOTE: When automatic reflection is enabled, defer descriptor set creation until
		 * updateAutomaticReflectionCubemap() is called with the scene's environment cubemap.
		 * This is because descriptor sets must have all bindings written before use. */
		if ( !m_useAutomaticReflection )
		{
			if ( !this->createDescriptorSet(renderer, *m_sharedUniformBuffer->uniformBufferObject(m_sharedUBOIndex)) )
			{
				TraceError{ClassId} << "Unable to create the descriptor set for PBR material '" << this->name() << "' !";

				return false;
			}
		}

		/* Initialize the material data in the GPU. */
		if ( !this->updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the initial video memory !");

			return false;
		}

		return true;
	}

	std::string
	PBRResource::getSharedUniformBufferIdentifier () const noexcept
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

		/* Account for automatic reflection cubemap slot. */
		if ( m_useAutomaticReflection )
		{
			textureCount++;
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
	PBRResource::createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept
	{
		m_sharedUniformBuffer = this->getSharedUniformBuffer(renderer, identifier);

		if ( m_sharedUniformBuffer == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the shared uniform buffer !");

			return false;
		}

		if ( !m_sharedUniformBuffer->addElement(this, m_sharedUBOIndex) )
		{
			Tracer::error(ClassId, "Unable to add the PBR material to the shared uniform buffer !");

			return false;
		}

		return true;
	}

	bool
	PBRResource::createDescriptorSetLayout (LayoutManager & layoutManager, const std::string & identifier) noexcept
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

			/* Declare cubemap sampler for automatic reflection (will be bound later with scene's environment cubemap). */
			if ( m_useAutomaticReflection )
			{
				m_automaticReflectionBindingPoint = bindingPoint;
				m_descriptorSetLayout->declareCombinedImageSampler(bindingPoint++, VK_SHADER_STAGE_FRAGMENT_BIT);
			}

			if ( !layoutManager.createDescriptorSetLayout(m_descriptorSetLayout) )
			{
				return false;
			}
		}

		return true;
	}

	bool
	PBRResource::createDescriptorSet (Renderer & renderer, const UniformBufferObject & uniformBufferObject) noexcept
	{
		m_descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), m_descriptorSetLayout);
		m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !m_descriptorSet->create() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set for PBR material '" << this->name() << "' !";

			return false;
		}

		uint32_t bindingPoint = 0;

		const auto descriptorInfo = m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex);

		if ( !m_descriptorSet->writeUniformBuffer(bindingPoint++, descriptorInfo) )
		{
			TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of PBR material '" << this->name() << "' !";

			return false;
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( component->type() != Type::Texture )
			{
				continue;
			}

			if ( !m_descriptorSet->writeCombinedImageSampler(bindingPoint++, *component->texture()) )
			{
				TraceError{ClassId} << "Unable to write the texture to the descriptor set of PBR material '" << this->name() << "' !";

				return false;
			}
		}

		return true;
	}

	bool
	PBRResource::updateAutomaticReflectionCubemap (const TextureInterface & cubemap) noexcept
	{
		if ( !m_useAutomaticReflection )
		{
			TraceWarning{ClassId} << "PBR material '" << this->name() << "' does not use automatic reflection !";

			return false;
		}

		/* Create descriptor set if not already created (deferred from create()). */
		if ( m_descriptorSet == nullptr )
		{
			if ( s_graphicsRenderer == nullptr )
			{
				Tracer::error(ClassId, "The static renderer pointer is null !");

				return false;
			}

			m_descriptorSet = std::make_unique< DescriptorSet >(s_graphicsRenderer->descriptorPool(), m_descriptorSetLayout);
			m_descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

			if ( !m_descriptorSet->create() )
			{
				TraceError{ClassId} << "Unable to create the descriptor set for PBR material '" << this->name() << "' !";

				return false;
			}

			uint32_t bindingPoint = 0;

			/* Write UBO binding. */
			const auto descriptorInfo = m_sharedUniformBuffer->getDescriptorInfoForElement(m_sharedUBOIndex);

			if ( !m_descriptorSet->writeUniformBuffer(bindingPoint++, descriptorInfo) )
			{
				TraceError{ClassId} << "Unable to write the uniform buffer to the descriptor set of PBR material '" << this->name() << "' !";

				return false;
			}

			/* Write component texture bindings. */
			for ( const auto & component : std::ranges::views::values(m_components) )
			{
				if ( component->type() != Type::Texture )
				{
					continue;
				}

				if ( !m_descriptorSet->writeCombinedImageSampler(bindingPoint++, *component->texture()) )
				{
					TraceError{ClassId} << "Unable to write the texture to the descriptor set of PBR material '" << this->name() << "' !";

					return false;
				}
			}
		}

		/* Write the automatic reflection cubemap at the reserved binding point. */
		if ( !m_descriptorSet->writeCombinedImageSampler(m_automaticReflectionBindingPoint, cubemap) )
		{
			TraceError{ClassId} << "Unable to write the automatic reflection cubemap to the descriptor set of PBR material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	PBRResource::updateVideoMemory () noexcept
	{
		if ( m_sharedUniformBuffer == nullptr )
		{
			return false;
		}

		if ( !m_sharedUniformBuffer->writeElementData(m_sharedUBOIndex, m_materialProperties.data()) )
		{
			return false;
		}

		m_videoMemoryUpdated = false;

		return true;
	}

	void
	PBRResource::destroy () noexcept
	{
		if ( m_sharedUniformBuffer != nullptr )
		{
			m_sharedUniformBuffer->removeElement(this);
		}

		/* Reset to defaults. */
		this->resetFlags();

		/* Reset member variables. */
		m_physicalSurfaceProperties.reset();
		m_components.clear();
		m_blendingMode = BlendingMode::None;
		m_materialProperties = {
			/* Albedo color (4) */
			DefaultAlbedoColor.red(), DefaultAlbedoColor.green(), DefaultAlbedoColor.blue(), DefaultAlbedoColor.alpha(),
			/* Roughness (1), Metalness (1), NormalScale (1), F0 (1) */
			DefaultRoughness, DefaultMetalness, DefaultNormalScale, DefaultF0,
			/* IOR (1), IBLIntensity (1), AutoIlluminationAmount (1), AOIntensity (1) */
			DefaultIOR, DefaultIBLIntensity, DefaultAutoIlluminationAmount, DefaultAOIntensity,
			/* AutoIlluminationColor (4) */
			DefaultAutoIlluminationColor.red(), DefaultAutoIlluminationColor.green(), DefaultAutoIlluminationColor.blue(), DefaultAutoIlluminationColor.alpha()
		};
		m_descriptorSetLayout.reset();
		m_descriptorSet.reset();
		m_sharedUniformBuffer.reset();
		m_sharedUBOIndex = 0;
	}

	bool
	PBRResource::isComplex () const noexcept
	{
		return this->isComponentPresent(ComponentType::Reflection) || this->isComponentPresent(ComponentType::Refraction);
	}

	const Physics::SurfacePhysicalProperties &
	PBRResource::surfacePhysicalProperties () const noexcept
	{
		return m_physicalSurfaceProperties;
	}

	Physics::SurfacePhysicalProperties &
	PBRResource::surfacePhysicalProperties () noexcept
	{
		return m_physicalSurfaceProperties;
	}

	uint32_t
	PBRResource::frameCount () const noexcept
	{
		return 1;
	}

	uint32_t
	PBRResource::duration () const noexcept
	{
		return 0;
	}

	uint32_t
	PBRResource::frameIndexAt (uint32_t /*sceneTime*/) const noexcept
	{
		return 0;
	}

	void
	PBRResource::enableBlending (BlendingMode mode) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} << "The resource '" << this->name() << "' is already created ! Unable to enable a blending mode.";

			return;
		}

		this->enableFlag(BlendingEnabled);

		m_blendingMode = mode;
	}

	BlendingMode
	PBRResource::blendingMode () const noexcept
	{
		if ( !this->isFlagEnabled(BlendingEnabled) )
		{
			return BlendingMode::None;
		}

		return m_blendingMode;
	}

	bool
	PBRResource::setupLightGenerator (LightGenerator & lightGenerator) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The PBR material '" << this->name() << "' is not created !"
				"It can't configure the light generator.";

			return false;
		}

		/* Enable PBR mode in the light generator. */
		lightGenerator.enablePBRMode();

		/* Albedo component */
		{
			const auto componentIt = m_components.find(ComponentType::Albedo);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAlbedo(componentIt->second->variableName());
			}
			else
			{
				/* Use uniform color value. */
				lightGenerator.declareSurfaceAlbedo(MaterialUB(UniformBlock::Component::AlbedoColor));
			}
		}

		/* Roughness component */
		{
			const auto componentIt = m_components.find(ComponentType::Roughness);

			if ( componentIt != m_components.cend() )
			{
				/* If invert is enabled, treat texture as smoothness/gloss map and invert it. */
				if ( m_invertRoughness )
				{
					lightGenerator.declareSurfaceRoughness("(1.0 - " + componentIt->second->variableName() + ")");
				}
				else
				{
					lightGenerator.declareSurfaceRoughness(componentIt->second->variableName());
				}
			}
			else
			{
				/* Use uniform value. */
				lightGenerator.declareSurfaceRoughness(MaterialUB(UniformBlock::Component::Roughness));
			}
		}

		/* Metalness component */
		{
			const auto componentIt = m_components.find(ComponentType::Metalness);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceMetalness(componentIt->second->variableName());
			}
			else
			{
				/* Use uniform value. */
				lightGenerator.declareSurfaceMetalness(MaterialUB(UniformBlock::Component::Metalness));
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

		/* Reflection/IBL component */
		{
			const auto componentIt = m_components.find(ComponentType::Reflection);

			if ( componentIt != m_components.cend() )
			{
				/* NOTE: For PBR, reflection amount is controlled by roughness/metalness, not a separate uniform.
				 * We pass "1.0" as the amount since IBL contribution is computed in the BRDF. */
				lightGenerator.declareSurfaceReflection(componentIt->second->variableName(), "1.0");
			}
		}

		/* Refraction component (for glass-like materials) */
		{
			const auto componentIt = m_components.find(ComponentType::Refraction);

			if ( componentIt != m_components.cend() )
			{
				/* NOTE: For PBR glass materials, Fresnel will automatically blend between reflection and refraction.
				 * We pass "1.0" as the amount since the Fresnel equation handles the blend ratio. */
				lightGenerator.declareSurfaceRefraction(componentIt->second->variableName(), "1.0", MaterialUB(UniformBlock::Component::RefractionIOR));
			}
		}

		/* IBL Intensity - controls the contribution of environment cubemaps (reflection/refraction). */
		lightGenerator.declareSurfaceIBLIntensity(MaterialUB(UniformBlock::Component::IBLIntensity));

		/* Auto-Illumination (emissive) component */
		{
			const auto componentIt = m_components.find(ComponentType::AutoIllumination);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAutoIllumination(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::AutoIlluminationAmount));
			}
			else
			{
				/* Use uniform color value with amount. */
				lightGenerator.declareSurfaceAutoIllumination(MaterialUB(UniformBlock::Component::AutoIlluminationColor), MaterialUB(UniformBlock::Component::AutoIlluminationAmount));
			}
		}

		/* Ambient Occlusion component (texture-based only) */
		{
			const auto componentIt = m_components.find(ComponentType::AmbientOcclusion);

			if ( componentIt != m_components.cend() )
			{
				lightGenerator.declareSurfaceAmbientOcclusion(componentIt->second->variableName(), MaterialUB(UniformBlock::Component::AOIntensity));
			}
		}

		return true;
	}

	std::string
	PBRResource::fragmentColor () const noexcept
	{
		/* For PBR, the fragment color is the albedo (base color).
		 * The actual shading is computed by the BRDF in the light generator. */
		if ( this->isComponentPresent(ComponentType::Albedo) )
		{
			return m_components.at(ComponentType::Albedo)->variableName();
		}

		return MaterialUB(UniformBlock::Component::AlbedoColor);
	}

	std::shared_ptr< DescriptorSetLayout >
	PBRResource::descriptorSetLayout () const noexcept
	{
		return m_descriptorSetLayout;
	}

	uint32_t
	PBRResource::UBOIndex () const noexcept
	{
		return m_sharedUBOIndex;
	}

	uint32_t
	PBRResource::UBOAlignment () const noexcept
	{
		return m_sharedUniformBuffer->blockAlignedSize();
	}

	uint32_t
	PBRResource::UBOOffset () const noexcept
	{
		return m_sharedUBOIndex * m_sharedUniformBuffer->blockAlignedSize();
	}

	const DescriptorSet *
	PBRResource::descriptorSet () const noexcept
	{
		return m_descriptorSet.get();
	}

	Declaration::UniformBlock
	PBRResource::getUniformBlock (uint32_t set, uint32_t binding) const noexcept
	{
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::PBRMaterial, UniformBlock::Material};
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AlbedoColor);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Roughness);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Metalness);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::NormalScale);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::F0);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::RefractionIOR);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::IBLIntensity);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AutoIlluminationAmount);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::AOIntensity);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AutoIlluminationColor);

		return block;
	}

	bool
	PBRResource::generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The PBR material '" << this->name() << "' is not created !"
				"It can't generate a vertex shader source code.";

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
					"for PBR material '" << this->name() << "' !";

				return false;
			}

			vertexShader.requestSynthesizeInstruction(ShaderVariable::PrimaryVertexColor);
		}

		/* Reflection/IBL component setup. */
		if ( this->isComponentPresent(ComponentType::Reflection) || this->isComponentPresent(ComponentType::Refraction) )
		{
			const auto isCubemap = generator.renderTarget()->isCubemap();

			if ( generator.highQualityReflectionEnabled() )
			{
				vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace);

				vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace);

				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					vertexShader.requestSynthesizeInstruction(ShaderVariable::TangentToWorldMatrix);
				}

				/* NOTE: Camera world position is read directly from View UBO instead of computing inverse(ViewMatrix). */
				vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, "CameraWorldPosition", GLSL::Flat});

				Code(vertexShader) <<
					"CameraWorldPosition = " << ViewUB(UniformBlock::Component::PositionWorldSpace, isCubemap) << ".xyz;";
			}
			else
			{
				vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, VariableScope::Local);
				vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace, VariableScope::Local);

				if ( this->isComponentPresent(ComponentType::Reflection) )
				{
					vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::ReflectionTextureCoordinates, GLSL::Smooth});

					/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox). */
					Code(vertexShader) <<
						"vec3 reflectDir = reflect(normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - " << ViewUB(UniformBlock::Component::PositionWorldSpace, isCubemap) << ".xyz), " << ShaderVariable::NormalWorldSpace << ");" << Line::End <<
						ShaderVariable::ReflectionTextureCoordinates << " = vec3(reflectDir.x, -reflectDir.y, reflectDir.z);";
				}

				if ( this->isComponentPresent(ComponentType::Refraction) )
				{
					vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector3, ShaderVariable::RefractionTextureCoordinates, GLSL::Smooth});

					/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox).
					 * For refraction, we use IOR ratio: eta = 1.0 / IOR (air to material). */
					Code(vertexShader) <<
						"float eta = 1.0 / " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End <<
						"vec3 refractDir = refract(normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - " << ViewUB(UniformBlock::Component::PositionWorldSpace, isCubemap) << ".xyz), " << ShaderVariable::NormalWorldSpace << ", eta);" << Line::End <<
						ShaderVariable::RefractionTextureCoordinates << " = vec3(refractDir.x, -refractDir.y, refractDir.z);";
				}
			}
		}

		return true;
	}

	bool
	PBRResource::generateTextureComponentFragmentShader (ComponentType componentType, const std::function< bool (FragmentShader &, const Texture *) > & codeGenerator, FragmentShader & fragmentShader, uint32_t materialSet) const noexcept
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
	PBRResource::textCoords (const Texture * component) noexcept
	{
		return component->isVolumetricTexture() ?
			ShaderVariable::Primary3DTextureCoordinates :
			ShaderVariable::Primary2DTextureCoordinates;
	}

	bool
	PBRResource::generateFragmentShaderCode (Generator::Abstract & generator, LightGenerator & lightGenerator, FragmentShader & fragmentShader) const noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} <<
				"The PBR material '" << this->name() << "' is not created !"
				"It can't generate a fragment shader source code.";

			return false;
		}

		if ( !generator.declareMaterialUniformBlock(*this, fragmentShader, 0) )
		{
			return false;
		}

		const uint32_t materialSet = generator.shaderProgram()->setIndex(SetType::PerModelLayer);

		/* Normal component.
		 * NOTE: Get a sample from a texture in range [0,1], convert it to a normalized range of [-1, 1]. */
		if ( this->isComponentPresent(ComponentType::Reflection) || this->isComponentPresent(ComponentType::Refraction) || !lightGenerator.isAmbientPass() )
		{
			if ( !this->generateTextureComponentFragmentShader(ComponentType::Normal, [] (FragmentShader & shader, const Texture * component) {
				Code{shader, Location::Top} << "const vec3 " << component->variableName() << " = normalize(texture(" << component->samplerName() << ", " << textCoords(component) << ").rgb * 2.0 - 1.0);";

				return true;
			}, fragmentShader, materialSet) )
			{
				TraceError{ClassId} << "Unable to generate fragment code for the normal component of PBR material '" << this->name() << "' !";

				return false;
			}
		}

		/* Albedo component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Albedo, [] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ");";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the albedo component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Roughness component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Roughness, [] (FragmentShader & shader, const Texture * component) {
			/* NOTE: Roughness is typically stored in the green channel of a combined texture,
			 * but we support single-channel textures too. */
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the roughness component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Metalness component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Metalness, [] (FragmentShader & shader, const Texture * component) {
			/* NOTE: Metalness is typically stored in the blue channel of a combined texture,
			 * but we support single-channel textures too. */
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the metalness component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Reflection/IBL component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Reflection, [&] (FragmentShader & shader, const Texture * component) {
			if ( generator.highQualityReflectionEnabled() )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::TangentToWorldMatrix << "[0] * " << SurfaceNormalVector << ".x + " << ShaderVariable::TangentToWorldMatrix << "[1] * " << SurfaceNormalVector << ".y + " << ShaderVariable::NormalWorldSpace << " * " << SurfaceNormalVector << ".z);";
				}
				else
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox). */
				Code(shader, Location::Top) <<
					"const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);" << Line::End <<
					"const vec3 reflectDir = reflect(reflectionI, reflectionNormal);" << Line::End <<
					"const vec3 " << ShaderVariable::ReflectionTextureCoordinates << " = vec3(reflectDir.x, -reflectDir.y, reflectDir.z);" << Line::End <<
					"const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ");";
			}
			else
			{
				Code(shader, Location::Top) << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::ReflectionTextureCoordinates << ");";
			}

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the reflection component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Refraction component (for glass-like materials). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::Refraction, [&] (FragmentShader & shader, const Texture * component) {
			if ( generator.highQualityReflectionEnabled() )
			{
				if ( this->isComponentPresent(ComponentType::Normal) )
				{
					/* Reuse reflectionNormal if already declared by reflection, otherwise declare it. */
					if ( !this->isComponentPresent(ComponentType::Reflection) )
					{
						Code(shader, Location::Top) <<
							"const vec3 reflGeomN = normalize(" << ShaderVariable::NormalWorldSpace << ");" << Line::End <<
							"const vec3 reflRawT = " << ShaderVariable::TangentToWorldMatrix << "[0];" << Line::End <<
							"const vec3 reflGeomT = normalize(reflRawT - reflGeomN * dot(reflGeomN, reflRawT));" << Line::End <<
							"const vec3 reflGeomB = cross(reflGeomN, reflGeomT) * sign(dot(cross(reflGeomN, reflGeomT), " << ShaderVariable::TangentToWorldMatrix << "[1]));" << Line::End <<
							"const vec3 reflectionNormal = normalize(reflGeomT * " << SurfaceNormalVector << ".x + reflGeomB * " << SurfaceNormalVector << ".y + reflGeomN * " << SurfaceNormalVector << ".z);";
					}
				}
				else if ( !this->isComponentPresent(ComponentType::Reflection) )
				{
					Code(shader, Location::Top) << "const vec3 reflectionNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");";
				}

				/* NOTE: Negate Y to convert from engine Y-DOWN to cubemap Y-UP convention (same as skybox). */
				Code(shader, Location::Top) <<
					"const float eta = 1.0 / " << MaterialUB(UniformBlock::Component::RefractionIOR) << ";" << Line::End;

				/* Reuse reflectionI if already declared by reflection, otherwise declare it. */
				if ( !this->isComponentPresent(ComponentType::Reflection) )
				{
					Code(shader, Location::Top) << "const vec3 reflectionI = normalize(" << ShaderVariable::PositionWorldSpace << ".xyz - CameraWorldPosition);" << Line::End;
				}

				Code(shader, Location::Top) <<
					"const vec3 refractDir = refract(reflectionI, reflectionNormal, eta);" << Line::End <<
					"const vec3 " << ShaderVariable::RefractionTextureCoordinates << " = vec3(refractDir.x, -refractDir.y, refractDir.z);" << Line::End <<
					"const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::RefractionTextureCoordinates << ");";
			}
			else
			{
				Code(shader, Location::Top) << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << ShaderVariable::RefractionTextureCoordinates << ");";
			}

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the refraction component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Auto-Illumination (emissive) component. */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::AutoIllumination, [] (FragmentShader & shader, const Texture * component) {
			Code{shader, Location::Top} << "const vec4 " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ");";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the auto-illumination component of PBR material '" << this->name() << "' !";

			return false;
		}

		/* Ambient Occlusion component (baked texture). */
		if ( !this->generateTextureComponentFragmentShader(ComponentType::AmbientOcclusion, [] (FragmentShader & shader, const Texture * component) {
			/* NOTE: AO is typically stored in a grayscale texture (red channel). */
			Code{shader, Location::Top} << "const float " << component->variableName() << " = texture(" << component->samplerName() << ", " << textCoords(component) << ").r;";

			return true;
		}, fragmentShader, materialSet) )
		{
			TraceError{ClassId} << "Unable to generate fragment code for the ambient occlusion component of PBR material '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	/* ==================== Component Setters ==================== */

	bool
	PBRResource::setAlbedoComponent (const PixelFactory::Color< float > & color) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the albedo component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::AlbedoColor);

		const auto result = m_components.emplace(ComponentType::Albedo, std::make_unique< Color >(uniform, color));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setAlbedoColor(color);

		return true;
	}

	bool
	PBRResource::setAlbedoComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the albedo component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Albedo, std::make_unique< Texture >(Uniform::AlbedoSampler, SurfaceAlbedoColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for albedo component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	PBRResource::setRoughnessComponent (float value) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the roughness component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::Roughness);

		const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setRoughness(value);

		return true;
	}

	bool
	PBRResource::setRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value, bool invert) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the roughness component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Roughness, std::make_unique< Texture >(Uniform::RoughnessSampler, SurfaceRoughness, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for roughness component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setRoughness(value);

		m_invertRoughness = invert;

		return true;
	}

	bool
	PBRResource::setMetalnessComponent (float value) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the metalness component.";

			return false;
		}

		const auto uniform = MaterialUB(UniformBlock::Component::Metalness);

		const auto result = m_components.emplace(ComponentType::Metalness, std::make_unique< Value >(uniform));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		this->setMetalness(value);

		return true;
	}

	bool
	PBRResource::setMetalnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the metalness component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Metalness, std::make_unique< Texture >(Uniform::MetalnessSampler, SurfaceMetalness, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for metalness component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setMetalness(value);

		return true;
	}

	bool
	PBRResource::setNormalComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale) noexcept
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
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for normal component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setNormalScale(scale);

		return true;
	}

	bool
	PBRResource::setReflectionComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept
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
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for reflection component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	PBRResource::setReflectionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the reflection component.";

			return false;
		}

		/* NOTE: Using TextureInterface constructor - no resource dependency tracking. */
		const auto result = m_components.emplace(ComponentType::Reflection, std::make_unique< Texture >(Uniform::ReflectionSampler, SurfaceReflectionColor, renderTarget));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		/* NOTE: No addDependency() for TextureInterface - it's not a loadable resource. */

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		return true;
	}

	bool
	PBRResource::setRefractionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float ior) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the refraction component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::Refraction, std::make_unique< Texture >(Uniform::RefractionSampler, SurfaceRefractionColor, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for refraction component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setIOR(ior);

		return true;
	}

	bool
	PBRResource::setRefractionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget, float ior) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the refraction component.";

			return false;
		}

		/* NOTE: Using TextureInterface constructor - no resource dependency tracking. */
		const auto result = m_components.emplace(ComponentType::Refraction, std::make_unique< Texture >(Uniform::RefractionSampler, SurfaceRefractionColor, renderTarget));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		/* NOTE: No addDependency() for TextureInterface - it's not a loadable resource. */

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setIOR(ior);

		return true;
	}

	bool
	PBRResource::setAutoIlluminationComponent (const PixelFactory::Color< float > & color, float amount) noexcept
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
	PBRResource::setAutoIlluminationComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount) noexcept
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
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for auto-illumination component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setAutoIlluminationAmount(amount);

		return true;
	}

	bool
	PBRResource::setAmbientOcclusionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float intensity) noexcept
	{
		if ( this->isCreated() )
		{
			TraceWarning{ClassId} <<
				"The resource '" << this->name() << "' is created ! "
				"Unable to create or change the ambient occlusion component.";

			return false;
		}

		const auto result = m_components.emplace(ComponentType::AmbientOcclusion, std::make_unique< Texture >(Uniform::AmbientOcclusionSampler, SurfaceAmbientOcclusion, texture));

		if ( !result.second || result.first->second == nullptr )
		{
			return false;
		}

		if ( !this->addDependency(texture) )
		{
			TraceError{ClassId} << "Unable to link the texture '" << texture->name() << "' dependency to PBR material '" << this->name() << "' for ambient occlusion component !";

			return false;
		}

		this->enableFlag(TextureEnabled);
		this->enableFlag(UsePrimaryTextureCoordinates);

		this->setAOIntensity(intensity);

		return true;
	}

	bool
	PBRResource::isComponentPresent (ComponentType componentType) const noexcept
	{
		return m_components.contains(componentType);
	}

	/* ==================== Dynamic Property Setters ==================== */

	void
	PBRResource::setAlbedoColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[AlbedoColorOffset] = color.red();
		m_materialProperties[AlbedoColorOffset+1] = color.green();
		m_materialProperties[AlbedoColorOffset+2] = color.blue();
		m_materialProperties[AlbedoColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setRoughness (float value) noexcept
	{
		m_materialProperties[RoughnessOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setMetalness (float value) noexcept
	{
		m_materialProperties[MetalnessOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setNormalScale (float value) noexcept
	{
		m_materialProperties[NormalScaleOffset] = value;

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setIOR (float value) noexcept
	{
		m_materialProperties[IOROffset] = std::clamp(value, 1.0F, 3.0F);

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setIBLIntensity (float value) noexcept
	{
		m_materialProperties[IBLIntensityOffset] = std::clamp(value, 0.0F, 1.0F);

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::enableAutomaticReflection (float iblIntensity) noexcept
	{
		m_useAutomaticReflection = true;

		m_materialProperties[IBLIntensityOffset] = std::clamp(iblIntensity, 0.0F, 1.0F);
	}

	void
	PBRResource::setAutoIlluminationColor (const PixelFactory::Color< float > & color) noexcept
	{
		m_materialProperties[AutoIlluminationColorOffset] = color.red();
		m_materialProperties[AutoIlluminationColorOffset+1] = color.green();
		m_materialProperties[AutoIlluminationColorOffset+2] = color.blue();
		m_materialProperties[AutoIlluminationColorOffset+3] = color.alpha();

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setAutoIlluminationAmount (float value) noexcept
	{
		m_materialProperties[AutoIlluminationAmountOffset] = std::max(0.0F, value);

		m_videoMemoryUpdated = true;
	}

	void
	PBRResource::setAOIntensity (float value) noexcept
	{
		m_materialProperties[AOIntensityOffset] = clampToUnit(value);

		m_videoMemoryUpdated = true;
	}
}
