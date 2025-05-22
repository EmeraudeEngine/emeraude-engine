/*
 * src/Vulkan/ComputePipeline.cpp
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

#include "ComputePipeline.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	size_t ComputePipeline::s_fakeHash = 0;

	ComputePipeline::ComputePipeline (const std::shared_ptr< PipelineLayout > & pipelineLayout, VkPipelineCreateFlags createFlags) noexcept
		: AbstractDeviceDependentObject(pipelineLayout->device()), m_pipelineLayout(pipelineLayout)
	{
		/* FIXME: The compute pipeline is not developed yet. */
		m_createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = createFlags;
		m_createInfo.stage = VkPipelineShaderStageCreateInfo{};
		m_createInfo.layout = m_pipelineLayout->handle();
		m_createInfo.basePipelineHandle = nullptr; // VkPipeline
		m_createInfo.basePipelineIndex = 0;
	}

	ComputePipeline::ComputePipeline (const std::shared_ptr< PipelineLayout > & pipelineLayout, const VkComputePipelineCreateInfo & createInfo) noexcept
		: AbstractDeviceDependentObject(pipelineLayout->device()), m_createInfo(createInfo), m_pipelineLayout(pipelineLayout)
	{

	}

	ComputePipeline::~ComputePipeline ()
	{
		this->destroyFromHardware();
	}

	bool
	ComputePipeline::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this compute pipeline !");

			return false;
		}

		if ( m_pipelineLayout == nullptr )
		{
			Tracer::error(ClassId, "No pipeline layout to create this compute pipeline !");

			return false;
		}

		const auto result = vkCreateComputePipelines(this->device()->handle(), VK_NULL_HANDLE, 1, &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a compute pipeline : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	ComputePipeline::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to destroy this compute pipeline !");

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a compute pipeline");

			vkDestroyPipeline(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	size_t
	ComputePipeline::getHash () noexcept
	{
		/* FIXME: Weak and unstable hash method !! */
		return s_fakeHash++;
	}
}
