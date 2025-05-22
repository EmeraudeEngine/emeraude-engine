/*
 * src/Vulkan/SharedUBOManager.cpp
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

#include "SharedUBOManager.hpp"

/* STL inclusions. */
#include <exception>
#include <iostream>

/* Local inclusions. */
#include "Device.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	const size_t SharedUBOManager::ClassUID{getClassUID(ClassId)};

	std::array< SharedUBOManager *, 2 > SharedUBOManager::s_instances{nullptr, nullptr};

	SharedUBOManager::SharedUBOManager (GPUWorkType type) noexcept
		: ServiceInterface(ClassId)
	{
		auto & instance = s_instances.at(static_cast< size_t >(type));

		if ( instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		instance = this;
	}

	SharedUBOManager::~SharedUBOManager ()
	{
		for ( auto & pointer : s_instances )
		{
			if ( pointer == this )
			{
				pointer = nullptr;

				break;
			}
		}
	}

	std::shared_ptr< SharedUniformBuffer >
	SharedUBOManager::createSharedUniformBuffer (const std::string & name, uint32_t uniformBlockSize, uint32_t maxElementCount) noexcept
	{
		if ( m_sharedUniformBuffers.contains(name) )
		{
			TraceError{ClassId} << "A shared uniform buffer named '" << name << "' already exists !";

			return nullptr;
		}

		auto sharedUniformBuffer = std::make_shared< SharedUniformBuffer >(m_device, uniformBlockSize, maxElementCount);

		if ( !sharedUniformBuffer->usable() )
		{
			TraceError{ClassId} << "Unable to create a shared uniform buffer (Name: '" << name << "', block size: " << uniformBlockSize << ", element count: " << maxElementCount << ", Dynamic: false) !";

			return nullptr;
		}

		return m_sharedUniformBuffers.emplace(name, sharedUniformBuffer).first->second;
	}

	std::shared_ptr< SharedUniformBuffer >
	SharedUBOManager::createSharedUniformBuffer (const std::string & name, const SharedUniformBuffer::DescriptorSetCreator & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount) noexcept
	{
		if ( m_sharedUniformBuffers.contains(name) )
		{
			TraceError{ClassId} << "A shared uniform buffer named '" << name << "' already exists !";

			return nullptr;
		}

		auto sharedUniformBuffer = std::make_shared< SharedUniformBuffer >(m_device, descriptorSetCreator, uniformBlockSize, maxElementCount);

		if ( !sharedUniformBuffer->usable() )
		{
			TraceError{ClassId} << "Unable to create a shared uniform buffer (Name: '" << name << "', block size: " << uniformBlockSize << ", element count: " << maxElementCount << ", Dynamic: true) !";

			return nullptr;
		}

		return m_sharedUniformBuffers.emplace(name, sharedUniformBuffer).first->second;
	}

	std::shared_ptr< SharedUniformBuffer >
	SharedUBOManager::getSharedUniformBuffer (const std::string & name) const noexcept
	{
		const auto sharedUniformBufferIt = m_sharedUniformBuffers.find(name);

		if ( sharedUniformBufferIt == m_sharedUniformBuffers.cend() )
		{
			TraceInfo{ClassId} << "There is no shared uniform buffer named '" << name << "' !";

			return nullptr;
		}

		return sharedUniformBufferIt->second;
	}

	bool
	SharedUBOManager::destroySharedUniformBuffer (const std::shared_ptr< SharedUniformBuffer > & pointer) noexcept
	{
		for ( const auto & [name, buffer] : m_sharedUniformBuffers )
		{
			if ( buffer.get() == pointer.get() )
			{
				return this->destroySharedUniformBuffer(name);
			}
		}

		return false;
	}

	bool
	SharedUBOManager::destroySharedUniformBuffer (const std::string & name) noexcept
	{
		const auto sharedUniformBufferIt = m_sharedUniformBuffers.find(name);

		if ( sharedUniformBufferIt == m_sharedUniformBuffers.cend() )
		{
			TraceWarning{ClassId} << "There is no shared uniform buffer named '" << name << "' !";

			return false;
		}

		m_sharedUniformBuffers.erase(sharedUniformBufferIt);

		return true;
	}

	bool
	SharedUBOManager::onInitialize () noexcept
	{
		if ( m_device == nullptr || !m_device->isCreated() )
		{
			Tracer::error(ClassId, "No device set !");

			return false;
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	SharedUBOManager::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		m_sharedUniformBuffers.clear();

		m_device.reset();

		return true;
	}
}
