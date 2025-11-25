/*
 * src/Vulkan/ComputePipeline.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"
#include "PipelineLayout.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The ComputePipeline class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class ComputePipeline final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanComputePipeline"};

			/**
			 * @brief Constructs a compute pipeline.
			 * @param pipelineLayout A reference to pipeline layout smart pointer.
			 * @param createFlags The createInfo flags. Default none.
			 */
			explicit
			ComputePipeline (const std::shared_ptr< PipelineLayout > & pipelineLayout, VkPipelineCreateFlags createFlags = 0) noexcept
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

			/**
			 * @brief Constructs a compute pipeline with a createInfo.
			 * @param pipelineLayout A reference to pipeline layout smart pointer.
			 * @param createInfo A reference to a createInfo.
			 */
			ComputePipeline (const std::shared_ptr< PipelineLayout > & pipelineLayout, const VkComputePipelineCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{pipelineLayout->device()},
				m_createInfo{createInfo},
				m_pipelineLayout{pipelineLayout}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ComputePipeline (const ComputePipeline & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ComputePipeline (ComputePipeline && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			ComputePipeline & operator= (const ComputePipeline & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			ComputePipeline & operator= (ComputePipeline && copy) noexcept = default;

			/**
			 * @brief Destructs the compute pipeline.
			 */
			~ComputePipeline () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the pipeline vulkan handle.
			 * @return VkPipeline
			 */
			[[nodiscard]]
			VkPipeline
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the pipeline createInfo.
			 * @return const VkComputePipelineCreateInfo &
			 */
			[[nodiscard]]
			const VkComputePipelineCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the associated pipeline layout smart pointer.
			 * @return std::shared_ptr< PipelineLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< PipelineLayout >
			pipelineLayout () const noexcept
			{
				return m_pipelineLayout;
			}

			/**
			 * @brief Returns a hash for this graphics pipeline according to constructor params.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t getHash () const noexcept;

		private:

			VkPipeline m_handle{VK_NULL_HANDLE};
			VkComputePipelineCreateInfo m_createInfo{};
			std::shared_ptr< PipelineLayout > m_pipelineLayout;
	};
}
