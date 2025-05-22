/*
 * src/Vulkan/ShaderModule.cpp
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

#include "ShaderModule.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	bool
	ShaderModule::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this shader module !");

			return false;
		}

		if ( m_binaryCode.empty() )
		{
			Tracer::error(ClassId, "There is no shader binary !");

			return false;
		}

		m_createInfo.codeSize = m_binaryCode.size() * sizeof(uint32_t);
		m_createInfo.pCode = m_binaryCode.data();

		const auto result = vkCreateShaderModule(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a shader module : " << vkResultToCString(result) << " !";

			return false;
		}

		if ( !this->preparePipelineShaderStageCreateInfo() )
		{
			Tracer::error(ClassId, "Unable to prepare the pipeline shader stage createInfo !");

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	ShaderModule::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the shader module " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a shader module");

			vkDestroyShaderModule(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	bool
	ShaderModule::preparePipelineShaderStageCreateInfo () noexcept
	{
		if ( m_shaderType == 0 )
		{
			Tracer::error(ClassId, "Unable to determine the vulkan shader type !");

			return false;
		}

		/*VkSpecializationMapEntry mapEntry{};
		mapEntry.constantID = 0;
		mapEntry.offset = 0;
		mapEntry.size = 0;*/

		m_specializationInfo.mapEntryCount = static_cast< uint32_t >(m_mapEntries.size());
		m_specializationInfo.pMapEntries = m_mapEntries.data();
		m_specializationInfo.dataSize = 0;
		m_specializationInfo.pData = nullptr;

		m_pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_pipelineShaderStageCreateInfo.pNext = nullptr;
		/*
		VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT = 0x00000001,
		VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT = 0x00000002,
		*/
		m_pipelineShaderStageCreateInfo.flags = 0;
		m_pipelineShaderStageCreateInfo.stage = m_shaderType;
		m_pipelineShaderStageCreateInfo.module = m_handle;
		m_pipelineShaderStageCreateInfo.pName = "main";
		m_pipelineShaderStageCreateInfo.pSpecializationInfo = m_mapEntries.empty() ? nullptr : &m_specializationInfo;

		return true;
	}
}
