/*
 * src/Vulkan/ShaderModule.cpp
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

#include "ShaderModule.hpp"

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"
#include "Libs/Hash/FNV1a.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

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
		m_specializationInfo.pMapEntries = m_mapEntries.empty() ? nullptr : m_mapEntries.data();
		m_specializationInfo.dataSize = static_cast< uint32_t >(m_specializationData.size());
		m_specializationInfo.pData = m_specializationData.empty() ? nullptr : m_specializationData.data();

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

	void
	ShaderModule::setSpecializationConstant (uint32_t constantId, bool value) noexcept
	{
		/* GLSL booleans are 4 bytes (VK_TRUE/VK_FALSE). */
		const VkBool32 vkValue = value ? VK_TRUE : VK_FALSE;
		const auto offset = m_specializationData.size();

		/* Append the value to the data buffer. */
		m_specializationData.resize(offset + sizeof(VkBool32));
		std::memcpy(m_specializationData.data() + offset, &vkValue, sizeof(VkBool32));

		/* Add the map entry. */
		VkSpecializationMapEntry entry{};
		entry.constantID = constantId;
		entry.offset = static_cast< uint32_t >(offset);
		entry.size = sizeof(VkBool32);
		m_mapEntries.push_back(entry);
	}

	void
	ShaderModule::setSpecializationConstant (uint32_t constantId, int32_t value) noexcept
	{
		const auto offset = m_specializationData.size();

		/* Append the value to the data buffer. */
		m_specializationData.resize(offset + sizeof(int32_t));
		std::memcpy(m_specializationData.data() + offset, &value, sizeof(int32_t));

		/* Add the map entry. */
		VkSpecializationMapEntry entry{};
		entry.constantID = constantId;
		entry.offset = static_cast< uint32_t >(offset);
		entry.size = sizeof(int32_t);
		m_mapEntries.push_back(entry);
	}

	void
	ShaderModule::setSpecializationConstant (uint32_t constantId, uint32_t value) noexcept
	{
		const auto offset = m_specializationData.size();

		/* Append the value to the data buffer. */
		m_specializationData.resize(offset + sizeof(uint32_t));
		std::memcpy(m_specializationData.data() + offset, &value, sizeof(uint32_t));

		/* Add the map entry. */
		VkSpecializationMapEntry entry{};
		entry.constantID = constantId;
		entry.offset = static_cast< uint32_t >(offset);
		entry.size = sizeof(uint32_t);
		m_mapEntries.push_back(entry);
	}

	void
	ShaderModule::setSpecializationConstant (uint32_t constantId, float value) noexcept
	{
		const auto offset = m_specializationData.size();

		/* Append the value to the data buffer. */
		m_specializationData.resize(offset + sizeof(float));
		std::memcpy(m_specializationData.data() + offset, &value, sizeof(float));

		/* Add the map entry. */
		VkSpecializationMapEntry entry{};
		entry.constantID = constantId;
		entry.offset = static_cast< uint32_t >(offset);
		entry.size = sizeof(float);
		m_mapEntries.push_back(entry);
	}

	size_t
	ShaderModule::specializationConstantsHash () const noexcept
	{
		if ( m_mapEntries.empty() || m_specializationData.empty() )
		{
			return 0;
		}

		/* Hash the specialization data using FNV-1a algorithm.
		 * NOTE: Create a string_view from the raw bytes for hashing. */
		return Hash::FNV1a(std::string_view{reinterpret_cast< const char * >(m_specializationData.data()), m_specializationData.size()});
	}

	bool
	ShaderModule::rebuildPipelineShaderStageCreateInfo () noexcept
	{
		if ( m_handle == VK_NULL_HANDLE )
		{
			Tracer::error(ClassId, "Cannot rebuild pipeline shader stage create info - shader module not created !");

			return false;
		}

		return this->preparePipelineShaderStageCreateInfo();
	}
}
