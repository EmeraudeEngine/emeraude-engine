/*
 * src/Graphics/SharedUniformBuffer.cpp
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

#include "SharedUniformBuffer.hpp"

#include <algorithm>

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Math/Base.hpp"
#include "Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/MemoryRegion.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;

	SharedUniformBuffer::SharedUniformBuffer (const std::shared_ptr< Device > & device, uint32_t uniformBlockSize, uint32_t maxElementCount, uint32_t frameCount) noexcept
		: m_device{device},
		m_uniformBlockSize{uniformBlockSize}
	{
		m_frameCount = std::max(1U, frameCount);

		/* ⚠️ Reserved ONCE, so no later growth can reallocate these vectors under a reader. See
		 * MaxBankCount. */
		m_uniformBufferObjects.reserve(MaxBankCount);

		const auto bufferCount = this->computeBlockAlignment(maxElementCount);

		for ( uint32_t index = 0; index < bufferCount; index++ )
		{
			if ( !this->addBuffer() )
			{
				break;
			}
		}
	}

	SharedUniformBuffer::SharedUniformBuffer (const std::shared_ptr< Device > & device, Renderer & renderer, const descriptor_set_creator_t & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount, uint32_t frameCount) noexcept
		: m_device(device),
		m_renderer(&renderer),
		m_descriptorSetCreator(descriptorSetCreator),
		m_uniformBlockSize(uniformBlockSize)
	{
		m_frameCount = std::max(1U, frameCount);

		/* ⚠️ Reserved ONCE, so no later growth can reallocate these vectors under a reader. See
		 * MaxBankCount. */
		m_uniformBufferObjects.reserve(MaxBankCount);
		m_descriptorSets.reserve(MaxBankCount);

		const auto bufferCount = this->computeBlockAlignment(maxElementCount);

		for ( uint32_t index = 0; index < bufferCount; index++ )
		{
			if ( !this->addBuffer(renderer, descriptorSetCreator) )
			{
				break;
			}
		}
	}

	uint32_t
	SharedUniformBuffer::computeBlockAlignment (uint32_t maxElementCount) noexcept
	{
		/* NOTE: nvidia GTX 1070 : 65536 bytes and 256 bytes alignment, so 256 optimal elements. */
		const auto & limits = m_device->physicalDevice()->propertiesVK10().limits;
		const auto maxUBOSize = this->bankSize();
		const auto minUBOAlignment = limits.minUniformBufferOffsetAlignment;

		m_blockAlignedSize = static_cast< uint32_t >(minUBOAlignment) * Math::alignCount(m_uniformBlockSize, static_cast< uint32_t >(minUBOAlignment));

		/* ⚠️ Each bank is split into m_frameCount regions, so a frame-in-flight never writes the
		 * bytes another one is still reading. The capacity per bank drops by the same factor; past
		 * it the class allocates another bank on its own, so this costs elements, never correctness.
		 * m_frameStride stays a multiple of m_blockAlignedSize (itself a multiple of
		 * minUniformBufferOffsetAlignment), so the resulting dynamic offset remains legally aligned. */
		m_maxElementCountPerUBO = (maxUBOSize / m_frameCount) / m_blockAlignedSize;
		m_frameStride = m_maxElementCountPerUBO * m_blockAlignedSize;

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

		if ( bufferIndex >= m_bankCount.load(std::memory_order_acquire) )
		{
			TraceError{ClassId} << "There is no uniform buffer object #" << bufferIndex << " !";

			return nullptr;
		}

		return m_uniformBufferObjects[bufferIndex].get();
	}

	UniformBufferObject *
	SharedUniformBuffer::uniformBufferObject (uint32_t index) noexcept
	{
		const auto bufferIndex = this->bufferIndex(index);

		if ( bufferIndex >= m_bankCount.load(std::memory_order_acquire) )
		{
			TraceError{ClassId} << "There is no uniform buffer object #" << bufferIndex << " !";

			return nullptr;
		}

		return m_uniformBufferObjects[bufferIndex].get();
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

		if ( bufferIndex >= m_bankCount.load(std::memory_order_acquire) )
		{
			TraceError{ClassId} << "There is no descriptor set #" << bufferIndex << " !";

			return nullptr;
		}

		return m_descriptorSets[bufferIndex].get();
	}

	uint32_t
	SharedUniformBuffer::bankSize () const noexcept
	{
		/* 64 KiB is the size every desktop device handles well and the value this engine has always
		 * used; the device limit only ever makes it SMALLER. See the header note for why the raw
		 * limit must never be allocated. */
		constexpr uint32_t PreferredBankSize{65536};

		return std::min(m_device->physicalDevice()->propertiesVK10().limits.maxUniformBufferRange, PreferredBankSize);
	}

	bool
	SharedUniformBuffer::growOneBank () noexcept
	{
		if ( m_bankCount.load(std::memory_order_relaxed) >= MaxBankCount )
		{
			return false;
		}

		if ( m_renderer != nullptr )
		{
			return this->addBuffer(*m_renderer, m_descriptorSetCreator);
		}

		return this->addBuffer();
	}

	Base::ScopedRegistration
	SharedUniformBuffer::addElement (const void * element, uint32_t & offset) noexcept
	{
		/* NOTE: Scanning for a free seat and claiming it MUST be atomic, otherwise two concurrent
		 * owners both see the same seat as free and both take it, ending up sharing a single UBO
		 * offset. The corruption is silent: no error is reported, the two owners simply overwrite
		 * each other's uniform block. */
		const Base::MutexLock lock{m_elementsAccess};

		offset = 0;

		for ( auto & seat : m_elements )
		{
			if ( seat == nullptr )
			{
				seat = element;

				return this->seatToken(offset, element);
			}

			offset++;
		}

		/* ⚠️ Every seat is taken, and this used to be the end of it: the owner failed to load, and
		 * `Material::Interface::getSharedUniformBuffer()`'s own comment spells out the consequence —
		 * its sub-meshes leave the scene, with no VUID and no black frame to say so. Measured on the
		 * Khronos iridescence grids: 344 materials against 146 seats, 197 spheres silently absent.
		 * A bank is allocated instead, which is what this class's sizing comment always claimed it
		 * did. The new seats are appended, so `offset` — already walked to the end of the previous
		 * table — is exactly the first of them. */
		if ( !this->growOneBank() )
		{
			TraceError{ClassId} <<
				"The shared uniform buffer is full at " << m_elements.size() << " elements "
				"(" << m_uniformBufferObjects.size() << " banks of " << m_maxElementCountPerUBO << ") "
				"and cannot grow any further ! The owner will fail to load and its geometry will "
				"leave the scene.";

			return {};
		}

		m_elements[offset] = element;

		return this->seatToken(offset, element);
	}

	Base::ScopedRegistration
	SharedUniformBuffer::seatToken (uint32_t offset, const void * element) noexcept
	{
		/* NOTE: By index, not by address: an element destroyed and another one built at the same address cannot free a
		 * seat that is not theirs. The weak reference lets a seat outlive the buffer. */
		return Base::ScopedRegistration{[weakThis = this->weak_from_this(), offset, element] () {
			if ( const auto self = weakThis.lock(); self != nullptr )
			{
				self->releaseSeat(offset, element);
			}
		}};
	}

	void
	SharedUniformBuffer::releaseSeat (uint32_t offset, const void * element) noexcept
	{
		const Base::MutexLock lock{m_elementsAccess};

		if ( offset < m_elements.size() && m_elements[offset] == element )
		{
			m_elements[offset] = nullptr;
		}
	}

	uint32_t
	SharedUniformBuffer::elementCount () const noexcept
	{
		const Base::MutexLock lock{m_elementsAccess};

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
	SharedUniformBuffer::writeElementData (uint32_t index, uint32_t frameIndex, const void * data) noexcept
	{
		const auto bufferIndex = this->bufferIndex(index);

		if ( bufferIndex >= m_bankCount.load(std::memory_order_acquire) )
		{
			TraceError{ClassId} << "There is no uniform buffer object #" << bufferIndex << " !";

			return false;
		}

		/* NOTE: Use LOCAL offset within the specific UBO, not global offset.
		 * Example: Element 300 with maxElementCountPerUBO=256 is at local index 44 in UBO #1.
		 * The byte offset is 44 * blockAlignedSize, not 300 * blockAlignedSize! */
		const auto byteOffset = static_cast< uint32_t >(this->getByteOffsetForElement(index, frameIndex));

		return m_uniformBufferObjects[bufferIndex]->writeData({data, m_uniformBlockSize, byteOffset});
	}

	VkDescriptorBufferInfo
	SharedUniformBuffer::getDescriptorInfoForElement (uint32_t elementIndex) const noexcept
	{
		VkDescriptorBufferInfo descriptorInfo{};

		const auto * ubo = this->uniformBufferObject(elementIndex);

		if ( ubo == nullptr )
		{
			TraceError{ClassId} << "Cannot get descriptor info for element #" << elementIndex << " - no UBO found!";

			return descriptorInfo;
		}

		descriptorInfo.buffer = ubo->handle();
		descriptorInfo.offset = this->getByteOffsetForElement(elementIndex);
		descriptorInfo.range = m_blockAlignedSize;

		return descriptorInfo;
	}

	bool
	SharedUniformBuffer::addBuffer () noexcept
	{
		const auto chunkId = (std::stringstream{} << "Chunk#" << m_uniformBufferObjects.size()).str();

		const auto UBOMaxSize = this->bankSize();

		auto * uniformBufferObject = m_uniformBufferObjects.emplace_back(std::make_unique< UniformBufferObject >(m_device, UBOMaxSize, m_blockAlignedSize)).get();
		uniformBufferObject->setIdentifier(ClassId, chunkId, "UniformBufferObject");

		if ( !uniformBufferObject->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create an UBO of " << UBOMaxSize << " bytes !";

			/* NOTE: A bank that failed is not kept: it was counted by usable() and could have been read. */
			m_uniformBufferObjects.pop_back();

			return false;
		}

		m_elements.resize(m_uniformBufferObjects.size() * m_maxElementCountPerUBO, nullptr);

		m_bankCount.store(static_cast< uint32_t >(m_uniformBufferObjects.size()), std::memory_order_release);

		return true;
	}

	bool
	SharedUniformBuffer::addBuffer (Renderer & renderer, const descriptor_set_creator_t & descriptorSetCreator) noexcept
	{
		const auto chunkId = (std::stringstream{} << "DynamicChunk#" << m_uniformBufferObjects.size()).str();

		const auto UBOMaxSize = this->bankSize();

		auto * uniformBufferObject = m_uniformBufferObjects.emplace_back(std::make_unique< UniformBufferObject >(m_device, UBOMaxSize, m_blockAlignedSize)).get();
		uniformBufferObject->setIdentifier(ClassId, chunkId, "UniformBufferObject");

		if ( !uniformBufferObject->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create an UBO of " << UBOMaxSize << " bytes !";

			/* NOTE: A bank that failed is not kept: it was counted by usable() and could have been read. */
			m_uniformBufferObjects.pop_back();

			return false;
		}

		auto descriptorSet = descriptorSetCreator(renderer, *uniformBufferObject);

		if ( descriptorSet == nullptr )
		{
			TraceError{ClassId} << "Unable to create the descriptor set of the bank '" << chunkId << "' !";

			m_uniformBufferObjects.pop_back();

			return false;
		}

		descriptorSet->setIdentifier(ClassId, chunkId, "DescriptorSet");

		m_descriptorSets.emplace_back(std::move(descriptorSet));

		m_elements.resize(m_uniformBufferObjects.size() * m_maxElementCountPerUBO, nullptr);

		m_bankCount.store(static_cast< uint32_t >(m_uniformBufferObjects.size()), std::memory_order_release);

		return true;
	}
}
