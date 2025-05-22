/*
 * src/Physics/Manager.cpp
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

#include "Manager.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Vulkan/Instance.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Physics
{
	using namespace Vulkan;

	const size_t Manager::ClassUID{getClassUID(ClassId)};

	Manager::Manager (PrimaryServices & primaryServices, Instance & instance) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices),
		m_vulkanInstance(instance)
	{

	}

	bool
	Manager::onInitialize () noexcept
	{
		m_flags[AccelerationAvailable] =
			!m_primaryServices.arguments().get("--disable-physics-acceleration").isPresent() &&
			m_primaryServices.settings().get< bool >(EnablePhysicsAccelerationKey, DefaultEnablePhysicsAcceleration);

		if ( !m_flags[AccelerationAvailable] )
		{
			Tracer::warning(ClassId, "Physics acceleration disabled at startup.");

			return true;
		}

		if ( !m_vulkanInstance.usable() )
		{
			Tracer::fatal(ClassId, "The Vulkan instance is not usable !");

			return false;
		}

		/* Compute device selection. */
		m_device = m_vulkanInstance.getComputeDevice();

		if ( m_device == nullptr )
		{
			Tracer::fatal(ClassId, "Unable to find a suitable compute device !");

			return false;
		}

		/* Initialize transfer manager for physics. */
		m_transferManager.setDevice(m_device);

		if ( m_transferManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_transferManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_transferManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize layout manager for physics. */
		m_layoutManager.setDevice(m_device);

		if ( m_layoutManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_layoutManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_layoutManager.name() << " service failed to execute !";

			return false;
		}

		/* Create a descriptor pool
		 * FIXME: maybe not the right place ! */
		const auto sizes = std::vector< VkDescriptorPoolSize >{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16}
		};

		m_descriptorPool = std::make_shared< DescriptorPool >(m_device, sizes, 64, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
		m_descriptorPool->setIdentifier(ClassId, "Main", "DescriptorPool");

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::fatal(ClassId, "Unable to create the descriptor pool !");

			return false;
		}

		m_commandPool = std::make_shared< CommandPool >(m_device, m_device->getComputeFamilyIndex(), false, true, false);
		m_commandPool->setIdentifier(ClassId, "Main", "CommandPool");

		if ( !m_commandPool->createOnHardware() )
		{
			Tracer::fatal(ClassId, "Unable to create the command pool !");

			return false;
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		size_t error = 0;

		m_flags[ServiceInitialized] = false;

		m_pipelines.clear();
		m_pipelineLayouts.clear();

		m_descriptorPool.reset();

		this->clearCommandBuffers();

		m_commandPool.reset();

		/* Terminate sub-services. */
		for ( auto * service : std::ranges::reverse_view(m_subServicesEnabled) )
		{
			if ( service->terminate() )
			{
				TraceSuccess{ClassId} << service->name() << " sub-service terminated gracefully !";
			}
			else
			{
				error++;

				TraceError{ClassId} << service->name() << " sub-service failed to terminate properly !";
			}
		}

		m_subServicesEnabled.clear();

		m_device.reset();

		return error == 0;
	}

	void
	Manager::clearCommandBuffers () noexcept
	{
		if ( m_commandBuffers.empty() )
		{
			return;
		}

		m_device->waitIdle("Clearing the command buffers");

		// FIXME !!! Check with the bit flag a command pool creation.
		//m_commandPool->freeCommandBuffers(static_cast< uint32_t >(m_commandBuffers.size()), m_commandBuffers.data());

		m_commandBuffers.clear();
	}

	size_t
	Manager::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	Manager::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	Manager::usable () const noexcept
	{
		if ( !m_vulkanInstance.usable() )
		{
			return false;
		}

		return m_flags[ServiceInitialized];
	}

	std::shared_ptr< ComputePipeline >
	Manager::getPipeline (const std::shared_ptr< PipelineLayout > & pipelineLayout) noexcept
	{
		const auto hash = ComputePipeline::getHash();

		const auto pipelineIt = m_pipelines.find(hash);

		if ( pipelineIt != m_pipelines.cend() )
		{
			return pipelineIt->second;
		}

		auto pipeline = std::make_shared< ComputePipeline >(pipelineLayout);
		pipeline->setIdentifier(ClassId, "Main", "ComputePipeline");

		if ( !pipeline->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the compute pipeline !");

			return {};
		}

		auto result = m_pipelines.emplace(hash, pipeline);

		return result.first->second;
	}
}
