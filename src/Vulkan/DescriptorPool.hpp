/*
 * src/Vulkan/DescriptorPool.hpp
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
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class DescriptorSetLayout;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The DescriptorPool class
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This vulkan object needs a device.
	 */
	class DescriptorPool final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDescriptorPool"};

			/**
			 * @brief Constructs a descriptor pool.
			 * @param device A reference to a smart pointer of a device.
			 * @param descriptorPoolSizes A reference to a list of descriptor pool sizes.
			 * @param maxSets The max sets.
			 * @param createFlags The createInfo flags. Default none.
			 */
			DescriptorPool (const std::shared_ptr< Device > & device, const std::vector< VkDescriptorPoolSize > & descriptorPoolSizes, uint32_t maxSets, VkDescriptorPoolCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{device},
				m_descriptorPoolSizes{descriptorPoolSizes}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.maxSets = maxSets;
				m_createInfo.poolSizeCount = static_cast< uint32_t >(m_descriptorPoolSizes.size());
				m_createInfo.pPoolSizes = m_descriptorPoolSizes.data();
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorPool (const DescriptorPool & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorPool (DescriptorPool && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return DescriptorPool &
			 */
			DescriptorPool & operator= (const DescriptorPool & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return DescriptorPool &
			 */
			DescriptorPool & operator= (DescriptorPool && copy) noexcept = delete;

			/**
			 * @brief Destructs the descriptor pool.
			 */
			~DescriptorPool () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the descriptor pool handle.
			 * @return VkDescriptorPool
			 */
			[[nodiscard]]
			VkDescriptorPool
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the descriptor pool createInfo.
			 * @return const VkDescriptorPoolCreateInfo &
			 */
			[[nodiscard]]
			const VkDescriptorPoolCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the list of descriptor pool sizes.
			 * @return const std::vector< VkDescriptorPoolSize > &
			 */
			[[nodiscard]]
			const std::vector< VkDescriptorPoolSize > &
			descriptorPoolSizes () const noexcept
			{
				return m_descriptorPoolSizes;
			}

			/**
			 * @brief Returns the max sets.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			maxSets () const noexcept
			{
				return m_createInfo.maxSets;
			}

			/**
			 * @brief Allocates one descriptor set.
			 * @param descriptorSetLayout A reference to a descriptor set layout.
			 * @return VkDescriptorSet
			 */
			[[nodiscard]]
			VkDescriptorSet allocateDescriptorSet (const DescriptorSetLayout & descriptorSetLayout) const noexcept;

			/**
			 * @brief Frees one descriptor set.
			 * @param descriptorSetHandle A Vulkan handle to the descriptor set.
			 * @return bool
			 */
			bool freeDescriptorSet (VkDescriptorSet descriptorSetHandle) const noexcept;

			/**
			 * @brief Resets the descriptor pool.
			 * @return bool
			 */
			bool reset () const noexcept;

		private:

			VkDescriptorPool m_handle{VK_NULL_HANDLE};
			VkDescriptorPoolCreateInfo m_createInfo{};
			std::vector< VkDescriptorPoolSize > m_descriptorPoolSizes;
			mutable std::mutex m_descriptorPoolAccess;
	};
}
