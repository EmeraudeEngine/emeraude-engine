/*
 * src/Vulkan/Sampler.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The sampler class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class Sampler  final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanSampler"};

			/**
			 * @brief Constructs a sampler with createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to a createInfo.
			 */
			Sampler (const std::shared_ptr< Device > & device, const VkSamplerCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Sampler (const Sampler & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Sampler (Sampler && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Sampler & operator= (const Sampler & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Sampler & operator= (Sampler && copy) noexcept = default;

			/**
			 * @brief Destructs the sampler.
			 */
			~Sampler () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the sampler vulkan handle.
			 * @return VkSampler
			 */
			[[nodiscard]]
			VkSampler
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the sampler createInfo.
			 * @return const VkSamplerCreateInfo &
			 */
			[[nodiscard]]
			const VkSamplerCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

		private:

			VkSampler m_handle{VK_NULL_HANDLE};
			VkSamplerCreateInfo m_createInfo{};
	};
}
