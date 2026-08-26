/*
 * src/Graphics/SharedUBOManager.cpp
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

#include "SharedUBOManager.hpp"

/* STL inclusions. */
#include <mutex>

/* Local inclusions. */
#include "Tracer.hpp"
#include "Vulkan/Device.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	std::shared_ptr< SharedUniformBuffer >
	SharedUBOManager::getOrCreateSharedUniformBuffer (const std::string & name, uint32_t uniformBlockSize, uint32_t maxElementCount) noexcept
	{
		/* NOTE: The lookup and the insertion MUST stay in the same critical section. Material resources
		 * are loaded concurrently from the resource thread pool and several of them legitimately share
		 * one buffer identifier (it only encodes the material kind and its texture count), so a
		 * get() then create() sequence lets every loser of the race receive a nullptr and fail to load
		 * its whole material, which silently removes the sub-meshes using it from the scene.
		 * The hardware buffer creation is serialized as a consequence: this is accepted, it happens
		 * once per distinct identifier at load time and never in the rendering path. */
		const std::lock_guard< std::mutex > lock{m_access};

		if ( const auto sharedUniformBufferIt = m_sharedUniformBuffers.find(name); sharedUniformBufferIt != m_sharedUniformBuffers.cend() )
		{
			return sharedUniformBufferIt->second;
		}

		/* NOTE: One region — a material's block does not change while the GPU reads it. */
		auto sharedUniformBuffer = std::make_shared< SharedUniformBuffer >(m_device, uniformBlockSize, maxElementCount);

		if ( !sharedUniformBuffer->usable() )
		{
			TraceError{ClassId} << "Unable to create a shared uniform buffer (Name: '" << name << "', block size: " << uniformBlockSize << ", element count: " << maxElementCount << ", Dynamic: false) !";

			return nullptr;
		}

		return m_sharedUniformBuffers.emplace(name, sharedUniformBuffer).first->second;
	}

	std::shared_ptr< SharedUniformBuffer >
	SharedUBOManager::createSharedUniformBuffer (const std::string & name, uint32_t uniformBlockSize, uint32_t maxElementCount, uint32_t frameCount) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

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
	SharedUBOManager::createSharedUniformBuffer (const std::string & name, const SharedUniformBuffer::descriptor_set_creator_t & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount, uint32_t frameCount) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		if ( m_sharedUniformBuffers.contains(name) )
		{
			TraceError{ClassId} << "A shared uniform buffer named '" << name << "' already exists !";

			return nullptr;
		}

		auto sharedUniformBuffer = std::make_shared< SharedUniformBuffer >(m_device, m_renderer, descriptorSetCreator, uniformBlockSize, maxElementCount, frameCount);

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
		const std::lock_guard< std::mutex > lock{m_access};

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
		/* NOTE: The erasure is performed here instead of delegating to the overload taking a name: the
		 * lock is not recursive, so delegating under it would deadlock. */
		const std::lock_guard< std::mutex > lock{m_access};

		for ( auto sharedUniformBufferIt = m_sharedUniformBuffers.begin(); sharedUniformBufferIt != m_sharedUniformBuffers.end(); ++sharedUniformBufferIt )
		{
			if ( sharedUniformBufferIt->second.get() == pointer.get() )
			{
				m_sharedUniformBuffers.erase(sharedUniformBufferIt);

				return true;
			}
		}

		return false;
	}

	bool
	SharedUBOManager::destroySharedUniformBuffer (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

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

		return true;
	}

	bool
	SharedUBOManager::onTerminate () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		m_sharedUniformBuffers.clear();

		m_device.reset();

		return true;
	}
}
