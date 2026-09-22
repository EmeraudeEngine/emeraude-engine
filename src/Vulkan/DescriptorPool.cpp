/*
 * src/Vulkan/DescriptorPool.cpp
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

#include "DescriptorPool.hpp"

/* Local inclusions. */
#include "DescriptorSetLayout.hpp"
#include "Device.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

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

		this->setVulkanObjectName(this->device()->handle(), VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast< uint64_t >(m_handle));

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

		{
			const std::lock_guard< std::mutex > lock{m_descriptorPoolAccess};

			for ( auto * page : m_extraPages )
			{
				vkDestroyDescriptorPool(this->device()->handle(), page, VK_NULL_HANDLE);
			}

			m_extraPages.clear();
			m_setPages.clear();
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyDescriptorPool(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	VkResult
	DescriptorPool::allocateFromPage (VkDescriptorPool pool, VkDescriptorSetLayout descriptorSetLayoutHandle, VkDescriptorSet & descriptorSetHandle) const noexcept
	{
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.descriptorPool = pool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorSetLayoutHandle;

		return vkAllocateDescriptorSets(this->device()->handle(), &allocateInfo, &descriptorSetHandle);
	}

	VkDescriptorPool
	DescriptorPool::createPage () const noexcept
	{
		VkDescriptorPoolCreateInfo createInfo = m_createInfo;
		createInfo.poolSizeCount = static_cast< uint32_t >(m_descriptorPoolSizes.size());
		createInfo.pPoolSizes = m_descriptorPoolSizes.data();

		VkDescriptorPool page = VK_NULL_HANDLE;

		if ( const auto result = vkCreateDescriptorPool(this->device()->handle(), &createInfo, VK_NULL_HANDLE, &page); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a new page for the descriptor pool '" << this->identifier() << "' : " << vkResultToCString(result) << " !";

			return VK_NULL_HANDLE;
		}

		return page;
	}

	VkDescriptorSet
	DescriptorPool::allocateDescriptorSet (const DescriptorSetLayout & descriptorSetLayout) const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkAllocateDescriptorSets() */
		const std::lock_guard< std::mutex > lock{m_descriptorPoolAccess};

		auto * descriptorSetLayoutHandle = descriptorSetLayout.handle();
		VkDescriptorSet descriptorSetHandle = VK_NULL_HANDLE;

		/* The current page is the last one created. */
		auto * page = m_extraPages.empty() ? m_handle : m_extraPages.back();

		auto result = this->allocateFromPage(page, descriptorSetLayoutHandle, descriptorSetHandle);

		/* An exhausted page is not an error: add one of the same sizes and retry once there. */
		if ( result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL )
		{
			page = this->createPage();

			if ( page == VK_NULL_HANDLE )
			{
				return VK_NULL_HANDLE;
			}

			m_extraPages.emplace_back(page);

			TraceInfo{ClassId} << "The descriptor pool '" << this->identifier() << "' was exhausted (" << vkResultToCString(result) << "): page #" << m_extraPages.size() + 1 << " added.";

			result = this->allocateFromPage(page, descriptorSetLayoutHandle, descriptorSetHandle);
		}

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to allocate a descriptor set : " << vkResultToCString(result) << " !";

			return VK_NULL_HANDLE;
		}

		if ( page != m_handle )
		{
			m_setPages.emplace(descriptorSetHandle, page);
		}

		return descriptorSetHandle;
	}

	bool
	DescriptorPool::freeDescriptorSet (VkDescriptorSet descriptorSetHandle) const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkFreeDescriptorSets() */
		const std::lock_guard< std::mutex > lock{m_descriptorPoolAccess};

		auto * page = m_handle;

		if ( const auto pageIt = m_setPages.find(descriptorSetHandle); pageIt != m_setPages.end() )
		{
			page = pageIt->second;

			m_setPages.erase(pageIt);
		}

		if ( const auto result = vkFreeDescriptorSets(this->device()->handle(), page, 1, &descriptorSetHandle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to free a descriptor set : " << vkResultToCString(result) << " ! "
				"Was the pool created with VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT ?";

			return false;
		}

		return true;
	}

	bool
	DescriptorPool::reset () const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkResetDescriptorPool() */
		const std::lock_guard< std::mutex > lock{m_descriptorPoolAccess};

		if ( const auto result = vkResetDescriptorPool(this->device()->handle(), m_handle, 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset the descriptor pool : " << vkResultToCString(result) << " !";

			return false;
		}

		/* NOTE: A reset returns every set of every page; the extra pages are kept for the next allocations. */
		for ( auto * page : m_extraPages )
		{
			if ( const auto result = vkResetDescriptorPool(this->device()->handle(), page, 0); result != VK_SUCCESS )
			{
				TraceError{ClassId} << "Unable to reset a page of the descriptor pool : " << vkResultToCString(result) << " !";

				return false;
			}
		}

		m_setPages.clear();

		return true;
	}
}
