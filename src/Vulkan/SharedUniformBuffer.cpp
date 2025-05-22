/*
 * src/Vulkan/SharedUniformBuffer.cpp
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

#include "SharedUniformBuffer.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "MemoryRegion.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	SharedUniformBuffer::SharedUniformBuffer (const std::shared_ptr< Device > & device, uint32_t uniformBlockSize, uint32_t maxElementCount) noexcept
		: m_device(device),
		m_uniformBlockSize(uniformBlockSize)
	{
		const auto bufferCount = this->computeBlockAlignment(maxElementCount);

		for ( uint32_t index = 0; index < bufferCount; index++ )
		{
			if ( !this->addBuffer() )
			{
				break;
			}
		}
	}

	SharedUniformBuffer::SharedUniformBuffer (const std::shared_ptr< Device > & device, const DescriptorSetCreator & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount) noexcept
		: m_device(device),
		m_uniformBlockSize(uniformBlockSize)
	{
		const auto bufferCount = this->computeBlockAlignment(maxElementCount);

		for ( uint32_t index = 0; index < bufferCount; index++ )
		{
			if ( !this->addBuffer(descriptorSetCreator) )
			{
				break;
			}
		}
	}

	uint32_t
	SharedUniformBuffer::computeBlockAlignment (uint32_t maxElementCount) noexcept
	{
		/* NOTE: nvidia GTX 1070 : 65536 bytes and 256 bytes alignment, so 256 optimal elements. */
		const auto & limits = m_device->physicalDevice()->properties().limits;
		const auto maxUBOSize = limits.maxUniformBufferRange;
		const auto minUBOAlignment = limits.minUniformBufferOffsetAlignment;

		m_blockAlignedSize = minUBOAlignment * Math::alignCount(m_uniformBlockSize, static_cast< uint32_t >(minUBOAlignment));
		m_maxElementCountPerUBO = maxUBOSize / m_blockAlignedSize;

		/*TraceInfo{ClassId} <<
			"Shared uniform buffer data :" "\n"
			"UBO maximum size : " << maxUBOSize << " bytes" "\n"
			"UBO minimum alignment : " << minUBOAlignment << " bytes" "\n"
			"Uniform block structure size : " << m_uniformBlockSize << " bytes" "\n"
			"Uniform block aligned size : " << m_blockAlignedSize << " bytes" "\n"
			"Max element per UBO : " << m_maxElementCountPerUBO << "\n";*/

		if ( maxElementCount == 0 )
		{
			return 1;
		}

		return Math::alignCount(maxElementCount * m_blockAlignedSize, maxUBOSize);
	}

	const UniformBufferObject *
	SharedUniformBuffer::uniformBufferObject (uint32_t index) const noexcept
	{
		const auto bufferIndex = this->bufferIndex(index);

		if ( bufferIndex >= m_uniformBufferObjects.size() )
		{
			TraceError{ClassId} << "There is no uniform buffer object #" << bufferIndex << " !";

			return nullptr;
		}

		return m_uniformBufferObjects.at(bufferIndex).get();
	}

	UniformBufferObject *
	SharedUniformBuffer::uniformBufferObject (uint32_t index) noexcept
	{
		const auto bufferIndex = this->bufferIndex(index);

		if ( bufferIndex >= m_uniformBufferObjects.size() )
		{
			TraceError{ClassId} << "There is no uniform buffer object #" << bufferIndex << " !";

			return nullptr;
		}

		return m_uniformBufferObjects.at(bufferIndex).get();
	}

	DescriptorSet *
	SharedUniformBuffer::descriptorSet (uint32_t index) const noexcept
	{
		if ( !this->isDynamic() )
		{
			Tracer::warning(ClassId, "This shared uniform buffer don't use a dynamic uniform buffer with a single descriptor set.");

			return nullptr;
		}

		const auto bufferIndex = this->bufferIndex(index);

		if ( bufferIndex >= m_descriptorSets.size() )
		{
			TraceError{ClassId} << "There is no descriptor set #" << bufferIndex << " !";

			return nullptr;
		}

		return m_descriptorSets.at(bufferIndex).get();
	}

	bool
	SharedUniformBuffer::addElement (const void * element, uint32_t & offset) noexcept
	{
		offset = 0;

		for ( auto & seat : m_elements )
		{
			if ( seat == nullptr )
			{
				seat = element;

				return true;
			}

			offset++;
		}

		return false;
	}

	void
	SharedUniformBuffer::removeElement (const void * element) noexcept
	{
		const auto elementIt = std::ranges::find_if(m_elements, [element] (const auto & seat) {
			return seat == element;
		});

		if ( elementIt != m_elements.end() )
		{
			*elementIt = nullptr;
		}
	}

	uint32_t
	SharedUniformBuffer::elementCount () const noexcept
	{
		uint32_t count = 0;

		for ( const auto & seat : m_elements )
		{
			if ( seat != nullptr )
			{
				count++;
			}
		}

		return count;
	}

	bool
	SharedUniformBuffer::writeElementData (uint32_t index, const void * data) noexcept
	{
		const auto bufferIndex = this->bufferIndex(index);

		if ( bufferIndex >= m_uniformBufferObjects.size() )
		{
			TraceError{ClassId} << "There is no uniform buffer object #" << bufferIndex << " !";

			return false;
		}

		return m_uniformBufferObjects.at(bufferIndex)->writeData({data, m_uniformBlockSize, m_blockAlignedSize * index});
	}

	bool
	SharedUniformBuffer::addBuffer () noexcept
	{
		const auto & limits = m_device->physicalDevice()->properties().limits;
		const auto chunkId = (std::stringstream{} << "Chunk#" << m_uniformBufferObjects.size()).str();

		auto * uniformBufferObject = m_uniformBufferObjects.emplace_back(std::make_unique< UniformBufferObject >(m_device, limits.maxUniformBufferRange, m_blockAlignedSize)).get();
		uniformBufferObject->setIdentifier(ClassId, chunkId, "UniformBufferObject");

		if ( !uniformBufferObject->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create an UBO of " << limits.maxUniformBufferRange << " bytes !";

			return false;
		}

		m_elements.resize(m_uniformBufferObjects.size() * m_maxElementCountPerUBO, nullptr);

		return true;
	}

	bool
	SharedUniformBuffer::addBuffer (const DescriptorSetCreator & descriptorSetCreator) noexcept
	{
		const auto & limits = m_device->physicalDevice()->properties().limits;
		const auto chunkId = (std::stringstream{} << "DynamicChunk#" << m_uniformBufferObjects.size()).str();

		auto * uniformBufferObject = m_uniformBufferObjects.emplace_back(std::make_unique< UniformBufferObject >(m_device, limits.maxUniformBufferRange, m_blockAlignedSize)).get();
		uniformBufferObject->setIdentifier(ClassId, chunkId, "UniformBufferObject");

		if ( !uniformBufferObject->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create an UBO of " << limits.maxUniformBufferRange << " bytes !";

			return false;
		}

		auto * descriptorSet = m_descriptorSets.emplace_back(descriptorSetCreator(*uniformBufferObject)).get();
		descriptorSet->setIdentifier(ClassId, chunkId, "DescriptorSet");

		m_elements.resize(m_uniformBufferObjects.size() * m_maxElementCountPerUBO, nullptr);

		return true;
	}
}
