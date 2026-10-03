/*
 * src/Vulkan/ComputePipeline.cpp
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

#include "ComputePipeline.hpp"

/* Local inclusions. */
#include <chrono>

#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

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

		/* Timed, and asked whether the pipeline cache served it (VkPipelineCreationFeedbackCreateInfo, Vulkan 1.3): a
		 * runtime compile stalls its thread and must be seen (reportPipelineCreation()). The feedback rides the pNext chain
		 * for this call only. */
		VkPipelineCreationFeedback feedback{};
		VkPipelineCreationFeedbackCreateInfo feedbackInfo{};
		const bool withFeedback = pipelineCreationFeedbackAvailable(*this->device());

		if ( withFeedback )
		{
			feedbackInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO;
			feedbackInfo.pNext = m_createInfo.pNext;
			feedbackInfo.pPipelineCreationFeedback = &feedback;
			m_createInfo.pNext = &feedbackInfo;
		}

		const auto start = std::chrono::steady_clock::now();
		const auto result = vkCreateComputePipelines(this->device()->handle(), this->device()->pipelineCache(), 1, &m_createInfo, nullptr, &m_handle);
		const auto elapsed = std::chrono::steady_clock::now() - start;

		if ( withFeedback )
		{
			m_createInfo.pNext = feedbackInfo.pNext;
		}

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a compute pipeline : " << vkResultToCString(result) << " !";

			return false;
		}

		reportPipelineCreation(ClassId, std::string{}, elapsed, withFeedback ? &feedback : nullptr);

		this->setVulkanObjectName(this->device()->handle(), VK_OBJECT_TYPE_PIPELINE, reinterpret_cast< uint64_t >(m_handle));

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
			vkDestroyPipeline(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	size_t
	ComputePipeline::getHash () const noexcept
	{
		/* TODO: ... */
		return 0;
	}
}
