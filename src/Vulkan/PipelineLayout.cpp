/*
 * src/Vulkan/PipelineLayout.cpp
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

#include "PipelineLayout.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Device.hpp"
#include "DescriptorSetLayout.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	PipelineLayout::operator== (const PipelineLayout & operand) const noexcept
	{
		if ( this == &operand )
		{
			return true;
		}

		/* FIXME: Find a way to serialize the createInfo to reduce the equality check complexity. */

		if ( m_createInfo.flags != operand.m_createInfo.flags )
		{
			return false;
		}

		if ( m_createInfo.setLayoutCount != operand.m_createInfo.setLayoutCount )
		{
			return false;
		}

		if ( m_createInfo.pushConstantRangeCount != operand.m_createInfo.pushConstantRangeCount )
		{
			return false;
		}

		/* NOTE: Vector comparison. */
		if ( m_descriptorSetLayouts != operand.m_descriptorSetLayouts )
		{
			return false;
		}

		/* NOTE: Vector comparison. */
		for ( size_t i = 0; i < m_pushConstantRanges.size(); i++ )
		{
			if ( m_pushConstantRanges[i].stageFlags != operand.m_pushConstantRanges[i].stageFlags )
			{
				return false;
			}

			if ( m_pushConstantRanges[i].offset != operand.m_pushConstantRanges[i].offset )
			{
				return false;
			}

			if ( m_pushConstantRanges[i].size != operand.m_pushConstantRanges[i].size )
			{
				return false;
			}
		}

		return false;
	}

	bool
	PipelineLayout::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this pipeline layout !");

			return false;
		}

		/* Repack vulkan handles to use within the createInfo. */
		StaticVector< VkDescriptorSetLayout, 4 > setLayouts{};

		std::ranges::transform(m_descriptorSetLayouts, std::back_inserter(setLayouts), [] (const auto & item) -> VkDescriptorSetLayout {
			return item->handle();
		});

		m_createInfo.setLayoutCount = static_cast< uint32_t >(setLayouts.size());
		m_createInfo.pSetLayouts = setLayouts.data();
		m_createInfo.pushConstantRangeCount = static_cast< uint32_t >(m_pushConstantRanges.size());
		m_createInfo.pPushConstantRanges = m_pushConstantRanges.data();

		const auto result = vkCreatePipelineLayout(
			this->device()->handle(),
			&m_createInfo,
			VK_NULL_HANDLE,
			&m_handle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a pipeline layout : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	PipelineLayout::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the pipeline layout " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyPipelineLayout(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	size_t
	PipelineLayout::computeHash (const StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > & descriptorSetLayouts, const StaticVector< VkPushConstantRange, 4 > & pushConstantRanges, VkPipelineLayoutCreateFlags flags) noexcept
	{
		/* FIXME: Weak and unstable hash method !! */
		size_t hash = 0;

		hash += descriptorSetLayouts.size();
		hash += pushConstantRanges.size();
		hash += flags;

		for ( const auto & descriptorSetLayout : descriptorSetLayouts )
		{
			hash += descriptorSetLayout->createInfo().bindingCount;
			hash += descriptorSetLayout->createInfo().flags;
		}

		for ( const auto & pushConstantRange : pushConstantRanges )
		{
			hash += pushConstantRange.offset;
			hash += pushConstantRange.size;
			hash += pushConstantRange.stageFlags;
		}

		return hash;
	}

	std::ostream &
	operator<< (std::ostream & out, const PipelineLayout & obj)
	{
		out << "Pipeline layout @" << &obj << " :" "\n";

		for ( const auto & descriptorSetLayout : obj.m_descriptorSetLayouts )
		{
			out << descriptorSetLayout;
		}

		return out;
	}

	std::string
	to_string (const PipelineLayout & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
