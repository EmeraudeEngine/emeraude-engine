/*
 * src/Vulkan/UniformBufferObject.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractHostBuffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines a convenient way to build a uniform buffer object (UBO).
	 * @note Vulkan guaranteed a minimum size of 16Kb.
	 * @extends EmEn::Vulkan::AbstractHostBuffer This is a host-side buffer.
	 */
	class UniformBufferObject final : public AbstractHostBuffer
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanUniformBufferObject"};

			static constexpr auto Vector2Room{4U};
			static constexpr auto Vector2iSize{2U * sizeof(int)};
			static constexpr auto Vector2fSize{2U * sizeof(float)};
			static constexpr auto Vector2dSize{2U * sizeof(double)};
			static constexpr auto Vector3Room{4U};
			static constexpr auto Vector3iSize{3U * sizeof(int)};
			static constexpr auto Vector3fSize{3U * sizeof(float)};
			static constexpr auto Vector3dSize{3U * sizeof(double)};
			static constexpr auto Vector4Room{4U};
			static constexpr auto Vector4iSize{4U * sizeof(int)};
			static constexpr auto Vector4fSize{4U * sizeof(float)};
			static constexpr auto Vector4dSize{4U * sizeof(double)};
			static constexpr auto Matrix2Room{8U};
			static constexpr auto Matrix2fSize{4U * sizeof(float)};
			static constexpr auto Matrix2dSize{4U * sizeof(double)};
			static constexpr auto Matrix3Room{12U};
			static constexpr auto Matrix3fSize{9U * sizeof(float)};
			static constexpr auto Matrix3dSize{9U * sizeof(double)};
			static constexpr auto Matrix4Room{16U};
			static constexpr auto Matrix4fSize{16U * sizeof(float)};
			static constexpr auto Matrix4dSize{16U * sizeof(double)};

			using Buffer::getDescriptorInfo;

			/**
			 * @brief Constructs a uniform buffer object (UBO).
			 * @param device A reference to a smart pointer to the device where the buffer will be created.
			 * @param size The size of the buffer.
			 * @param blockAlignedSize The size of a block. Default, no sub-objects.
			 */
			UniformBufferObject (const std::shared_ptr< Device > & device, VkDeviceSize size, uint32_t blockAlignedSize = 0) noexcept;

			/**
			 * @brief Returns whether this uniform buffer can be used for multiple elements.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShared () const noexcept
			{
				return m_blockAlignedSize > 0;
			}

			/**
			 * @brief Returns the aligned block size.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			blockAlignedSize () const noexcept
			{
				return m_blockAlignedSize;
			}

			/**
			 * @brief Returns the possible element count inside this buffer.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t maxElementCount () const noexcept;

			/**
			 * @brief Returns the descriptor buffer info.
			 * @return VkDescriptorBufferInfo
			 */
			[[nodiscard]]
			VkDescriptorBufferInfo
			getDescriptorInfo () const noexcept
			{
				return this->getDescriptorInfo(0, this->bytes());
			}

			/**
			 * @brief Returns the descriptor buffer info.
			 * @param elementOffset The element offset inside the UBO.
			 * @return VkDescriptorBufferInfo
			 */
			[[nodiscard]]
			VkDescriptorBufferInfo getDescriptorInfo (uint32_t elementOffset) const noexcept;

		private:

			uint32_t m_blockAlignedSize{0};
	};
}
