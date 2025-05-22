/*
 * src/Vulkan/DescriptorPool.cpp
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

#include "DescriptorPool.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "DescriptorSetLayout.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	bool
	DescriptorPool::createOnHardware() noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this descriptor pool !");

			return false;
		}

		/* Refresh pool sizes. */
		m_createInfo.poolSizeCount = static_cast< uint32_t >(m_descriptorPoolSizes.size());
		m_createInfo.pPoolSizes = m_descriptorPoolSizes.data();

		const auto result = vkCreateDescriptorPool(
			this->device()->handle(),
			&m_createInfo,
			VK_NULL_HANDLE,
			&m_handle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create descriptor pool : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	DescriptorPool::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to destroy this descriptor pool !");

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying descriptor pool");

			vkDestroyDescriptorPool(
				this->device()->handle(),
				m_handle,
				VK_NULL_HANDLE
			);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	VkDescriptorSet
	DescriptorPool::allocateDescriptorSet (const DescriptorSetLayout & descriptorSetLayout) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_allocationMutex};

		auto * descriptorSetLayoutHandle = descriptorSetLayout.handle();

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = m_handle;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorSetLayoutHandle;

		VkDescriptorSet descriptorSetHandle = VK_NULL_HANDLE;

		const auto result = vkAllocateDescriptorSets(
			this->device()->handle(),
			&allocateInfo,
			&descriptorSetHandle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to allocate a descriptor set : " << vkResultToCString(result) << " !";

			return VK_NULL_HANDLE;
		}

		return descriptorSetHandle;
	}

	bool
	DescriptorPool::freeDescriptorSet (VkDescriptorSet descriptorSetHandle) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_allocationMutex};

		this->device()->waitIdle("Freeing descriptor set");

		const auto result = vkFreeDescriptorSets(
			this->device()->handle(),
			m_handle,
			1, &descriptorSetHandle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to allocate a descriptor set : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
}
