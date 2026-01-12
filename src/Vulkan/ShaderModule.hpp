/*
 * src/Vulkan/ShaderModule.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The ShaderModule class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This Vulkan object needs a device.
	 */
	class ShaderModule final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanShaderModule"};

			/**
			 * @brief Constructs a shader module.
			 * @param device A reference to a device smart pointer.
			 * @param shaderType The vulkan shader type.
			 * @param binaryCode A reference to a binary data vector.
			 * @param createFlags The createInfo flags. Default none.
			 */
			ShaderModule (const std::shared_ptr< Device > & device, VkShaderStageFlagBits shaderType, const std::vector< uint32_t > & binaryCode, VkShaderModuleCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{device},
				m_shaderType{shaderType},
				m_binaryCode{binaryCode}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.codeSize = 0;
				m_createInfo.pCode = nullptr;
			}

			/**
			 * @brief Constructs a shader module with a createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to the createInfo.
			 * @param shaderType The vulkan shader type.
			 * @param binaryCode A reference to a binary data vector.
			 */
			ShaderModule (const std::shared_ptr< Device > & device, const VkShaderModuleCreateInfo & createInfo, VkShaderStageFlagBits shaderType, const std::vector< uint32_t > & binaryCode) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo},
				m_shaderType{shaderType},
				m_binaryCode{binaryCode}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ShaderModule (const ShaderModule & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ShaderModule (ShaderModule && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			ShaderModule & operator= (const ShaderModule & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			ShaderModule & operator= (ShaderModule && copy) noexcept = default;

			/**
			 * @brief Destructs a shader module.
			 */
			~ShaderModule () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the shader module handle.
			 * @return VkShaderModule
			 */
			[[nodiscard]]
			VkShaderModule
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the shader module createInfo.
			 * @return const VkShaderModuleCreateInfo &
			 */
			[[nodiscard]]
			const VkShaderModuleCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the pipeline shader stage createInfo.
			 * @return const VkPipelineShaderStageCreateInfo &
			 */
			[[nodiscard]]
			const VkPipelineShaderStageCreateInfo &
			pipelineShaderStageCreateInfo () const noexcept
			{
				return m_pipelineShaderStageCreateInfo;
			}

		private:

			/**
			 * @brief Prepares the pipeline shader createInfo.
			 * @return bool
			 */
			bool preparePipelineShaderStageCreateInfo () noexcept;

			VkShaderModule m_handle{VK_NULL_HANDLE};
			VkShaderModuleCreateInfo m_createInfo{};
			VkShaderStageFlagBits m_shaderType{};
			std::vector< uint32_t > m_binaryCode;
			Libs::StaticVector< VkSpecializationMapEntry, 8 > m_mapEntries;
			VkSpecializationInfo m_specializationInfo{};
			VkPipelineShaderStageCreateInfo m_pipelineShaderStageCreateInfo{};
	};
}
