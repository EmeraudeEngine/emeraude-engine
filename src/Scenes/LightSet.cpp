/*
 * src/Scenes/LightSet.cpp
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

#include "LightSet.hpp"

/* STL inclusions. */
#include <ostream>

/* Local inclusions. */
#include "Saphir/LightGenerator.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir;
	using namespace Vulkan;

	const size_t LightSet::ClassUID{getClassUID(ClassId)};

	LightSet::LightSet (AVConsole::Manager & AVConsoleManager) noexcept
		: m_AVConsoleManager(AVConsoleManager)
	{

	}

	std::unique_ptr< DescriptorSet >
	LightSet::createDescriptorSet (const UniformBufferObject & uniformBufferObject) noexcept
	{
		auto * renderer = Renderer::instance();

		auto descriptorSet = std::make_unique< DescriptorSet >(renderer->descriptorPool(), getDescriptorSetLayout(renderer->layoutManager()));

		if ( !descriptorSet->create() )
		{
			Tracer::error(ClassId, "Unable to create the light descriptor set !");

			return nullptr;
		}

		if ( !descriptorSet->writeUniformBufferObjectDynamic(0, uniformBufferObject) )
		{
			Tracer::error(ClassId, "Unable to write the uniform buffer object to the descriptor set !");

			return nullptr;
		}

		return descriptorSet;
	}

	bool
	LightSet::initialize (Renderer & renderer, const std::string & sceneName) noexcept
	{
		if ( !this->isEnabled() )
		{
			TraceInfo{ClassId} << "Lighting is not enabled for scene '" << sceneName << "'.";

			return true;
		}

		const std::lock_guard< std::mutex > lock{m_lightAccess};

		auto & sharedUBOManager = renderer.sharedUBOManager();

		/* Generate directional light shared uniform buffer. */
		{
			const auto uniformBlock = LightGenerator::getUniformBlock(0, 0, LightType::Directional, false);

			m_directionalLightBuffer = sharedUBOManager.createSharedUniformBuffer(sceneName + "DirectionalLights", createDescriptorSet, uniformBlock.bytes());

			if ( m_directionalLightBuffer == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the directional light shared uniform buffer !");

				return false;
			}

			/* NOTE: Register existing lights. */
			for ( const auto & light : m_directionalLights )
			{
				if ( !light->createOnHardware(*this, renderer, m_AVConsoleManager) )
				{
					TraceError{ClassId} << "Unable to create the directional light '" << light->name() << "' !";

					return false;
				}
			}
		}

		/* Generate point light shared uniform buffer. */
		{
			const auto uniformBlock = LightGenerator::getUniformBlock(0, 0, LightType::Point, false);

			m_pointLightBuffer = sharedUBOManager.createSharedUniformBuffer(sceneName + "PointLights", createDescriptorSet, uniformBlock.bytes());

			if ( m_pointLightBuffer == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the point light shared uniform buffer !");

				return false;
			}

			/* NOTE: Register existing lights. */
			for ( const auto & light : m_pointLights )
			{
				if ( !light->createOnHardware(*this, renderer, m_AVConsoleManager) )
				{
					TraceError{ClassId} << "Unable to create the point light '" << light->name() << "' !";

					return false;
				}
			}
		}

		/* Generate spotlight shared uniform buffer. */
		{
			const auto uniformBlock = LightGenerator::getUniformBlock(0, 0, LightType::Spot, false);

			m_spotLightBuffer = sharedUBOManager.createSharedUniformBuffer(sceneName + "SpotLights", createDescriptorSet, uniformBlock.bytes());

			if ( m_spotLightBuffer == nullptr )
			{
				Tracer::error(ClassId, "Unable to create the spotlight shared uniform buffer !");

				return false;
			}

			/* NOTE: Register existing lights. */
			for ( const auto & light : m_spotLights )
			{
				if ( !light->createOnHardware(*this, renderer, m_AVConsoleManager) )
				{
					TraceError{ClassId} << "Unable to create the spot light '" << light->name() << "' !";

					return false;
				}
			}
		}

		renderer.swapChain()->viewMatrices().updateAmbientLightProperties(m_ambientLightColor, m_ambientLightIntensity);

		m_flags[Initialized] = true;

		return true;
	}

	bool
	LightSet::terminate (Renderer & renderer) noexcept
	{
		if ( !this->isEnabled() )
		{
			return true;
		}

		size_t error = 0;

		auto & sharedUBOManager = renderer.sharedUBOManager();

		m_flags[Initialized] = false;

		/* Release the directional light shared uniform buffer. */
		{
			const auto pointer = m_directionalLightBuffer;

			m_directionalLightBuffer.reset();

			if ( !sharedUBOManager.destroySharedUniformBuffer(pointer) )
			{
				error++;
			}
		}

		/* Release the point light shared uniform buffer. */
		{
			const auto pointer = m_pointLightBuffer;

			m_pointLightBuffer.reset();

			if ( !sharedUBOManager.destroySharedUniformBuffer(pointer) )
			{
				error++;
			}
		}

		/* Release the spotlight shared uniform buffer. */
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
	LightSet::add (const std::shared_ptr< Component::DirectionalLight > & light, Renderer & renderer) noexcept
	{
		/* NOTE: If light set is uninitialized, the light creation will be postponed. */
		if ( m_flags[Initialized] && !light->createOnHardware(*this, renderer, m_AVConsoleManager) )
		{
			TraceError{ClassId} << "Unable to create the directional light '" << light->name() << "' !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_lightAccess};

		m_lights.emplace(light);
		m_directionalLights.emplace(light);

		this->notify(DirectionalLightAdded, light);
	}

	void
	LightSet::add (const std::shared_ptr< Component::PointLight > & light, Renderer & renderer) noexcept
	{
		/* NOTE: If light set is uninitialized, the light creation will be postponed. */
		if ( m_flags[Initialized] && !light->createOnHardware(*this, renderer, m_AVConsoleManager) )
		{
			TraceError{ClassId} << "Unable to create the point light '" << light->name() << "' !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_lightAccess};

		m_lights.emplace(light);
		m_pointLights.emplace(light);

		this->notify(PointLightAdded, light);
	}

	void
	LightSet::add (const std::shared_ptr< Component::SpotLight > & light, Renderer & renderer) noexcept
	{
		/* NOTE: If light set is uninitialized, the light creation will be postponed. */
		if ( m_flags[Initialized] && !light->createOnHardware(*this, renderer, m_AVConsoleManager) )
		{
			TraceError{ClassId} << "Unable to create the spot light '" << light->name() << "' !";

			return;
		}

		const std::lock_guard< std::mutex > lock{m_lightAccess};

		m_lights.emplace(light);
		m_spotLights.emplace(light);

		this->notify(SpotLightAdded, light);
	}

	void
	LightSet::remove (const std::shared_ptr< Component::DirectionalLight > & light) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightAccess};

		m_lights.erase(light);
		m_directionalLights.erase(light);

		this->notify(DirectionalLightRemoved, light);

		light->destroyFromHardware();
	}

	void
	LightSet::remove (const std::shared_ptr< Component::PointLight > & light) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightAccess};

		m_lights.erase(light);
		m_pointLights.erase(light);

		this->notify(PointLightRemoved, light);

		light->destroyFromHardware();
	}

	void
	LightSet::remove (const std::shared_ptr< Component::SpotLight > & light) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightAccess};

		m_lights.erase(light);
		m_spotLights.erase(light);

		this->notify(SpotLightRemoved, light);

		light->destroyFromHardware();
	}

	void
	LightSet::removeAllLights () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightAccess};

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
		const std::lock_guard< std::mutex > lock{obj.m_lightAccess};

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

	std::string
	to_string (const LightSet & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}

	std::shared_ptr< DescriptorSetLayout >
	LightSet::getDescriptorSetLayout (LayoutManager & layoutManager) noexcept
	{
		/* NOTE: Create a unique identifier for the descriptor set layout. */
		const std::string UUID{ClassId};

		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(UUID);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(UUID);
			descriptorSetLayout->setIdentifier(ClassId, "LightProperties", "DescriptorSetLayout");
			descriptorSetLayout->declareUniformBufferDynamic(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	void
	LightSet::createDefaultStaticLighting () noexcept
	{
		if ( m_staticLighting.contains(DefaultStaticLightingName) )
		{
			return;
		}

		StaticLighting staticLighting{};
		staticLighting
			.setAmbientParameters(Blue, 0.005F)
			.setLightParameters(White, 1.5F)
			.setAsDirectionalLight({1.0F, 0.0F, 0.0F});

		m_staticLighting.emplace(DefaultStaticLightingName, staticLighting);
	}

	StaticLighting &
	LightSet::getStaticLighting (const std::string & name) noexcept
	{
		const auto staticLightingIt = m_staticLighting.find(name);

		if ( staticLightingIt != m_staticLighting.cend() )
		{
			return staticLightingIt->second;
		}

		return m_staticLighting[name];
	}

	const StaticLighting *
	LightSet::getStaticLighting (const std::string & name) const noexcept
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

		const std::lock_guard< std::mutex > lock{m_lightAccess};

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
