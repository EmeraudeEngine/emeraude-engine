/*
 * src/Scenes/LightSet.cpp
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

#include "LightSet.hpp"

/* STL inclusions. */
#include <ostream>

/* Local inclusions. */
#include "Graphics/DummyShadowTexture.hpp"
#include "Scenes/AVConsole/Manager.hpp"
#include "Saphir/LightGenerator.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir;
	using namespace Vulkan;

	std::unique_ptr< DescriptorSet >
	LightSet::createDescriptorSet (Renderer & renderer, const UniformBufferObject & uniformBufferObject) noexcept
	{
		auto descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), getDescriptorSetLayout(renderer.layoutManager()));

		if ( !descriptorSet->create() )
		{
			Tracer::error(ClassId, "Unable to create the light descriptor set !");

			return nullptr;
		}

		/* Binding 0: Light UBO with dynamic offset. */
		if ( !descriptorSet->writeUniformBufferObjectDynamic(0, uniformBufferObject) )
		{
			Tracer::error(ClassId, "Unable to write the uniform buffer object to the descriptor set !");

			return nullptr;
		}

		/* Binding 1: Default dummy shadow texture.
		 * This will be overwritten with a real shadow map if the light uses shadow mapping. */
		const auto dummyShadowTexture = renderer.getDummyShadowTexture2D();

		if ( dummyShadowTexture != nullptr )
		{
			if ( !descriptorSet->writeCombinedImageSampler(1, *dummyShadowTexture) )
			{
				Tracer::error(ClassId, "Unable to write the dummy shadow texture to the descriptor set !");

				return nullptr;
			}
		}
		else
		{
			Tracer::warning(ClassId, "No dummy shadow texture available, binding 1 will be uninitialized !");
		}

		return descriptorSet;
	}

	bool
	LightSet::initialize (Scene & scene) noexcept
	{
		if ( !this->isEnabled() )
		{
			TraceInfo{ClassId} << "Lighting is not enabled for scene '" << scene.name() << "'.";

			return true;
		}

		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		auto & renderer = scene.AVConsoleManager().graphicsRenderer();
		auto & sharedUBOManager = renderer.sharedUBOManager();

		/* Generate directional light shared uniform buffer.
		 * NOTE: Always allocate space for the light space matrix (useShadow=true) even if some lights
		 * don't use shadow mapping. This ensures the UBO size matches DirectionalLight::m_buffer which
		 * always includes the matrix. Lights without shadows will have an identity matrix. */
		{
			const auto uniformBlock = LightGenerator::getUniformBlock(0, 0, LightType::Directional, true);

			m_directionalLightBuffer = sharedUBOManager.createSharedUniformBuffer(scene.name() + "DirectionalLights", createDescriptorSet, uniformBlock.bytes());

			if ( m_directionalLightBuffer == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the directional light shared uniform buffer !");

				return false;
			}

			/* NOTE: Register existing lights. */
			for ( const auto & light : m_directionalLights )
			{
				if ( !light->createOnHardware(scene) )
				{
					TraceError{ClassId} << "Unable to create the directional light '" << light->name() << "' !";

					return false;
				}
			}
		}

		/* Generate point light shared uniform buffer.
		 * NOTE: Always allocate space for the light space matrix (useShadow=true) even if some lights
		 * don't use shadow mapping. This ensures the UBO size matches PointLight::m_buffer which
		 * always includes the matrix. Lights without shadows will have an identity matrix. */
		{
			const auto uniformBlock = LightGenerator::getUniformBlock(0, 0, LightType::Point, true);

			m_pointLightBuffer = sharedUBOManager.createSharedUniformBuffer(scene.name() + "PointLights", createDescriptorSet, uniformBlock.bytes());

			if ( m_pointLightBuffer == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the point light shared uniform buffer !");

				return false;
			}

			/* NOTE: Register existing lights. */
			for ( const auto & light : m_pointLights )
			{
				if ( !light->createOnHardware(scene) )
				{
					TraceError{ClassId} << "Unable to create the point light '" << light->name() << "' !";

					return false;
				}
			}
		}

		/* Generate spotlight shared uniform buffer.
		 * NOTE: Always allocate space for the light space matrix (useShadow=true) even if some lights
		 * don't use shadow mapping. This ensures the UBO size matches SpotLight::m_buffer which
		 * always includes the matrix. Lights without shadows will have an identity matrix. */
		{
			const auto uniformBlock = LightGenerator::getUniformBlock(0, 0, LightType::Spot, true);

			m_spotLightBuffer = sharedUBOManager.createSharedUniformBuffer(scene.name() + "SpotLights", createDescriptorSet, uniformBlock.bytes());

			if ( m_spotLightBuffer == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the spotlight shared uniform buffer !");

				return false;
			}

			/* NOTE: Register existing lights. */
			for ( const auto & light : m_spotLights )
			{
				if ( !light->createOnHardware(scene) )
				{
					TraceError{ClassId} << "Unable to create the spot light '" << light->name() << "' !";

					return false;
				}
			}
		}

		/* FIXME: Take only in account the main view! */
		renderer.mainRenderTarget()->viewMatrices().updateAmbientLightProperties(m_ambientLightColor, m_ambientLightIntensity);

		m_initialized = true;

		return true;
	}

	bool
	LightSet::terminate (Scene & scene) noexcept
	{
		if ( !this->isEnabled() )
		{
			return true;
		}

		size_t error = 0;

		auto & renderer = scene.AVConsoleManager().graphicsRenderer();
		auto & sharedUBOManager = renderer.sharedUBOManager();

		m_initialized = false;

		/* Release the directional light SharedUniformBuffer. */
		{
			const auto pointer = m_directionalLightBuffer;

			m_directionalLightBuffer.reset();

			if ( !sharedUBOManager.destroySharedUniformBuffer(pointer) )
			{
				error++;
			}
		}

		/* Release the point light SharedUniformBuffer. */
		{
			const auto pointer = m_pointLightBuffer;

			m_pointLightBuffer.reset();

			if ( !sharedUBOManager.destroySharedUniformBuffer(pointer) )
			{
				error++;
			}
		}

		/* Release the spotlight SharedUniformBuffer. */
		{
			const auto pointer = m_spotLightBuffer;

			m_spotLightBuffer.reset();

			if ( !sharedUBOManager.destroySharedUniformBuffer(pointer) )
			{
				error++;
			}
		}

		return error == 0;
	}

	void
	LightSet::add (Scene & scene, const std::shared_ptr< Component::DirectionalLight > & light) noexcept
	{
		/* NOTE: If the light set is uninitialized, the light creation will be postponed. */
		if ( m_initialized && !light->createOnHardware(scene) )
		{
			TraceError{ClassId} << "Unable to create the directional light '" << light->name() << "' !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.emplace(light);
		m_directionalLights.emplace(light);

		this->notify(DirectionalLightAdded, light);
	}

	void
	LightSet::add (Scene & scene, const std::shared_ptr< Component::PointLight > & light) noexcept
	{
		/* NOTE: If the light set is uninitialized, the light creation will be postponed. */
		if ( m_initialized && !light->createOnHardware(scene) )
		{
			TraceError{ClassId} << "Unable to create the point light '" << light->name() << "' !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.emplace(light);
		m_pointLights.emplace(light);

		this->notify(PointLightAdded, light);
	}

	void
	LightSet::add (Scene & scene, const std::shared_ptr< Component::SpotLight > & light) noexcept
	{
		/* NOTE: If the light set is uninitialized, the light creation will be postponed. */
		if ( m_initialized && !light->createOnHardware(scene) )
		{
			TraceError{ClassId} << "Unable to create the spot light '" << light->name() << "' !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.emplace(light);
		m_spotLights.emplace(light);

		this->notify(SpotLightAdded, light);
	}

	void
	LightSet::remove (Scene & scene, const std::shared_ptr< Component::DirectionalLight > & light) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.erase(light);
		m_directionalLights.erase(light);

		this->notify(DirectionalLightRemoved, light);

		light->destroyFromHardware(scene);
	}

	void
	LightSet::remove (Scene & scene, const std::shared_ptr< Component::PointLight > & light) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.erase(light);
		m_pointLights.erase(light);

		this->notify(PointLightRemoved, light);

		light->destroyFromHardware(scene);
	}

	void
	LightSet::remove (Scene & scene, const std::shared_ptr< Component::SpotLight > & light) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.erase(light);
		m_spotLights.erase(light);

		this->notify(SpotLightRemoved, light);

		light->destroyFromHardware(scene);
	}

	void
	LightSet::removeAllLights () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		m_lights.clear();
		m_directionalLights.clear();
		m_pointLights.clear();
		m_spotLights.clear();
		m_ambientLightColor = Black;
		m_lightPercentToAmbient = DefaultLightPercentToAmbient;
	}

	std::ostream &
	operator<< (std::ostream & out, const LightSet & obj)
	{
		const std::lock_guard< std::mutex > lock{obj.m_lightsAccess};

		out <<
			"Ambient light color : " << obj.m_ambientLightColor << "\n"
			"Ambient light intensity : " << obj.m_ambientLightIntensity << "\n";

		if ( obj.m_directionalLights.empty() )
		{
			out << "No directional light." << "\n";
		}
		else
		{
			out << "Directional lights : " << obj.m_directionalLights.size() << "\n";

			for ( const auto & light : obj.m_directionalLights )
			{
				out << " - light #" << light->UBOIndex() << " color : " << light->color() << ", intensity : " << light->intensity() << "\n";
			}
		}

		if ( obj.m_pointLights.empty() )
		{
			out << "No point light." << "\n";
		}
		else
		{
			out << "Point lights : " << obj.m_pointLights.size() << "\n";

			for ( const auto & light : obj.m_pointLights )
			{
				out << " - light #" << light->UBOIndex() << " color : " << light->color() << ", intensity : " << light->intensity() << "\n";
			}
		}

		if ( obj.m_spotLights.empty() )
		{
			out << "No spotlight." << "\n";
		}
		else
		{
			out << "Spot lights : " << obj.m_spotLights.size() << "\n";

			for ( const auto & light : obj.m_spotLights )
			{
				out << " - light #" << light->UBOIndex() << " color : " << light->color() << ", intensity : " << light->intensity() << "\n";
			}
		}

		return out;
	}

	std::shared_ptr< DescriptorSetLayout >
	LightSet::getDescriptorSetLayout (LayoutManager & layoutManager) noexcept
	{
		/* NOTE: Unified descriptor set layout with 2 bindings.
		 * Binding 0: Light UBO (dynamic offset for shared buffer)
		 * Binding 1: Shadow map sampler (real shadow map or dummy texture)
		 *
		 * The specialization constant scUseShadow controls whether the
		 * shadow sampling code is executed in the shader. When false,
		 * the Vulkan driver eliminates the dead code. */
		return getDescriptorSetLayoutWithShadow(layoutManager);
	}

	std::shared_ptr< DescriptorSetLayout >
	LightSet::getDescriptorSetLayoutWithShadow (LayoutManager & layoutManager) noexcept
	{
		/* NOTE: Create a unique identifier for the unified descriptor set layout.
		 * This layout is used for ALL light passes (shadow and no-shadow).
		 * Lights without shadow maps bind the dummy shadow texture. */
		const std::string UUID{ClassId};

		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(UUID);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(UUID);
			descriptorSetLayout->setIdentifier(ClassId, "LightProperties", "DescriptorSetLayout");

			/* Binding 0: Light UBO (dynamic offset for shared buffer). */
			descriptorSetLayout->declareUniformBufferDynamic(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

			/* Binding 1: Shadow map sampler (real shadow map or dummy 1x1 depth texture). */
			descriptorSetLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	StaticLighting &
	LightSet::getOrCreateDefaultStaticLighting () noexcept
	{
		const auto staticLightingIt = m_staticLighting.find(DefaultStaticLightingName);

		if ( staticLightingIt != m_staticLighting.cend() )
		{
			return staticLightingIt->second;
		}

		StaticLighting staticLighting;
		staticLighting
			.setAmbientParameters(Blue, 0.005F)
			.setLightParameters(White, 1.5F)
			.setAsDirectionalLight({0.0F, 1.0F, 0.0F}, true);

		return m_staticLighting.emplace(DefaultStaticLightingName, staticLighting).first->second;
	}

	StaticLighting &
	LightSet::getOrCreateStaticLighting (const std::string & name) noexcept
	{
		if ( const auto staticLightingIt = m_staticLighting.find(name); staticLightingIt != m_staticLighting.cend() )
		{
			return staticLightingIt->second;
		}

		return m_staticLighting.emplace(DefaultStaticLightingName, StaticLighting{}).first->second;
	}

	const StaticLighting *
	LightSet::getStaticLightingPointer (const std::string & name) const noexcept
	{
		const auto staticLightingIt = m_staticLighting.find(name);

		if ( staticLightingIt == m_staticLighting.cend() )
		{
			return nullptr;
		}

		return &staticLightingIt->second;
	}

	bool
	LightSet::updateVideoMemory () const noexcept
	{
		if ( !this->isEnabled() || this->isUsingStaticLighting() )
		{
			return true;
		}

		const std::lock_guard< std::mutex > lock{m_lightsAccess};

		size_t errors = 0;

		for ( const auto & light : m_directionalLights )
		{
			if ( !light->updateVideoMemory() )
			{
				errors++;
			}
		}

		for ( const auto & light : m_pointLights )
		{
			if ( !light->updateVideoMemory() )
			{
				errors++;
			}
		}

		for ( const auto & light : m_spotLights )
		{
			if ( !light->updateVideoMemory() )
			{
				errors++;
			}
		}

		return errors == 0;
	}
}
