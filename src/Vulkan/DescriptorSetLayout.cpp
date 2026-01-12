/*
 * src/Vulkan/DescriptorSetLayout.cpp
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

#include "DescriptorSetLayout.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	DescriptorSetLayout::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this descriptor set layout !");

			return false;
		}

		if ( m_setLayoutBindings.empty() )
		{
			Tracer::error(ClassId, "No binding to create this descriptor set layout !");

			return false;
		}

		m_createInfo.bindingCount = static_cast< uint32_t >(m_setLayoutBindings.size());
		m_createInfo.pBindings = m_setLayoutBindings.data();

		const auto result = vkCreateDescriptorSetLayout(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create descriptor set layout : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	DescriptorSetLayout::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the descriptor set layout " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyDescriptorSetLayout(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		/* FIXME: Descriptor set layout can be instanced without the need of being created. */
		this->setCreated();
		this->setDestroyed();

		return true;
	}

	bool
	DescriptorSetLayout::declare (VkDescriptorSetLayoutBinding setLayoutBinding) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::error(ClassId, "Cannot declares binding when the descriptor set layout is created !");

			return false;
		}

		if ( setLayoutBinding.descriptorType >= VK_DESCRIPTOR_TYPE_MAX_ENUM )
		{
			Tracer::error(ClassId, "Invalid descriptor type !");

			return false;
		}

		auto bindingExists = std::ranges::any_of(m_setLayoutBindings, [setLayoutBinding] (const auto & current) {
			return current.binding == setLayoutBinding.binding;
		});

		if ( bindingExists )
		{
			TraceError{ClassId} << "Binding #" << setLayoutBinding.binding << " already declared in this descriptor set layout !";

			return false;
		}

		m_setLayoutBindings.emplace_back(setLayoutBinding);

		return true;
	}

	size_t
	DescriptorSetLayout::computeHash (const Libs::StaticVector< VkDescriptorSetLayoutBinding, 16 > & bindings, VkDescriptorSetLayoutCreateFlags flags) noexcept
	{
		/* FIXME: Weak and unstable hash method !! */
		size_t hashValue = 0;
		hashValue += bindings.size();
		hashValue += flags;

		for ( const auto & binding : bindings )
		{
			hashValue += binding.binding;
			hashValue += binding.descriptorCount;
			hashValue += binding.stageFlags;
		}

		return hashValue;
	}
}
