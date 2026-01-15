/*
 * src/Graphics/SharedUniformBuffer.hpp
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
#include <cmath>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

/* Local inclusions for usages. */
#include "Vulkan/UniformBufferObject.hpp"
#include "Vulkan/DescriptorSet.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The shared uniform buffer class.
	 * @note This is a higher concept to manage an UBO to store multiple data with a fixed-sized structure.
	 */
	class SharedUniformBuffer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SharedUniformBuffer"};

			using descriptor_set_creator_t = std::function< std::unique_ptr< Vulkan::DescriptorSet > (Renderer & renderer, Vulkan::UniformBufferObject & uniformBufferObject) >;

			/**
			 * @brief Constructs a shared uniform buffer.
			 * @param device A reference to the device smart pointer.
			 * @param uniformBlockSize The size of the uniform block.
			 * @param maxElementCount The max number of elements to hold in one UBO. Default, compute the maximum according to structure size and UBO properties. Default is the max limit.
			 */
			SharedUniformBuffer (const std::shared_ptr< Vulkan::Device > & device, uint32_t uniformBlockSize, uint32_t maxElementCount = 0) noexcept;

			/**
			 * @brief Constructs a shared uniform buffer with a unique descriptor set.
			 * @note This version uses a dynamic uniform buffer to switch from element to element instead of binding another descriptor set.
			 * @warning The descriptor set is unique for all elements, so all other binds will be the same for each element.
			 * @param device A reference to the device smart pointer.
			 * @param renderer A reference to the graphics renderer.
			 * @param descriptorSetCreator A reference to a lambda to build the associated descriptor set.
			 * @param uniformBlockSize The size of the uniform block.
			 * @param maxElementCount The max number of elements to hold in one UBO. Default, compute the maximum according to structure size and UBO properties. Default is the max limit.
			 */
			SharedUniformBuffer (const std::shared_ptr< Vulkan::Device > & device, Renderer & renderer, const descriptor_set_creator_t & descriptorSetCreator, uint32_t uniformBlockSize, uint32_t maxElementCount = 0) noexcept;

			/**
			 * @brief Returns whether the shared uniform buffer is usable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usable () const noexcept
			{
				return !m_uniformBufferObjects.empty();
			}

			/**
			 * @brief Returns whether the shared uniform buffer is dynamic (use a single descriptor set).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDynamic () const noexcept
			{
				return !m_descriptorSets.empty();
			}

			/**
			 * @brief Returns the uniform buffer object pointer.
			 * @param index The element index.
			 * @return const UniformBufferObject *
			 */
			[[nodiscard]]
			const Vulkan::UniformBufferObject * uniformBufferObject (uint32_t index) const noexcept;

			/**
			 * @brief Returns the uniform buffer object pointer.
			 * @param index The element index.
			 * @return UniformBufferObject *
			 */
			[[nodiscard]]
			Vulkan::UniformBufferObject * uniformBufferObject (uint32_t index) noexcept;

			/**
			 * @brief Returns the descriptor set pointer.
			 * @param index The element index.
			 * @return UniformBufferObject *
			 */
			[[nodiscard]]
			Vulkan::DescriptorSet * descriptorSet (uint32_t index) const noexcept;

			/**
			 * @brief Adds a new element to the uniform buffer object.
			 * @param element A raw pointer to link the element.
			 * @param offset A reference to an unsigned integer to get the offset.
			 * @return bool
			 */
			[[nodiscard]]
			bool addElement (const void * element, uint32_t & offset) noexcept;

			/**
			 * @brief Removes an element from the uniform buffer object.
			 * @param element A raw pointer from the linked element.
			 * @return void
			 */
			void removeElement (const void * element) noexcept;

			/**
			 * @brief Returns the number of elements present in the buffer.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t elementCount () const noexcept;

			/**
			 * @brief Writes element data to the UBO.
			 * @note There is no check in this function. All have been done at initialization time.
			 * @param index The element index.
			 * @param data A pointer to the source data.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeElementData (uint32_t index, const void * data) noexcept;

			/**
			 * @brief Returns the element aligned size in the UBO.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			blockAlignedSize () const noexcept
			{
				return m_blockAlignedSize;
			}

			/**
			 * @brief Returns the byte offset for an element within its UBO.
			 * @note This is the LOCAL offset within the specific UBO that holds the element.
			 *       Use this for descriptor set binding or direct buffer access.
			 * @param elementIndex The global element index.
			 * @return VkDeviceSize The byte offset within the UBO.
			 */
			[[nodiscard]]
			VkDeviceSize
			getByteOffsetForElement (uint32_t elementIndex) const noexcept
			{
				/* NOTE: When multiple UBOs exist, convert global index to local index within that UBO.
				 * Example: With maxElementCountPerUBO=256, element 300 is at local index 44 in UBO #1. */
				const auto localIndex = elementIndex % m_maxElementCountPerUBO;
				return static_cast< VkDeviceSize >(localIndex) * m_blockAlignedSize;
			}

			/**
			 * @brief Returns a fully configured VkDescriptorBufferInfo for an element.
			 * @note This is the preferred method for getting descriptor info as it ensures
			 *       the correct byte offset calculation is always used.
			 * @param elementIndex The global element index.
			 * @return VkDescriptorBufferInfo Ready to use in vkUpdateDescriptorSets.
			 */
			[[nodiscard]]
			VkDescriptorBufferInfo getDescriptorInfoForElement (uint32_t elementIndex) const noexcept;

		private:

			/**
			 * @brief Computes internal sizes of the UBO.
			 * @param elementCount The desired element count for the whole shared uniform buffer.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t computeBlockAlignment (uint32_t elementCount) noexcept;

			/**
			 * @brief Adds a buffer to the UBO list without creating a descriptor set associated.
			 * @return bool
			 */
			[[nodiscard]]
			bool addBuffer () noexcept;

			/**
			 * @brief Adds a buffer to the UBO list.
			 * @param renderer A reference to the renderer.
			 * @param descriptorSetCreator A reference to a lambda to build the associated descriptor set.
			 * @return bool
			 */
			[[nodiscard]]
			bool addBuffer (Renderer & renderer, const descriptor_set_creator_t & descriptorSetCreator) noexcept;

			/**
			 * @brief Returns the right UBO index from element index.
			 * @param index The element index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			bufferIndex (uint32_t index) const noexcept
			{
				return static_cast< uint32_t >(std::floor(static_cast< double >(index) / static_cast< double >(m_maxElementCountPerUBO)));
			}

			std::shared_ptr< Vulkan::Device > m_device;
			uint32_t m_uniformBlockSize;
			uint32_t m_maxElementCountPerUBO{0};
			uint32_t m_blockAlignedSize{0};
			std::vector< std::unique_ptr< Vulkan::UniformBufferObject > > m_uniformBufferObjects;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::vector< const void * > m_elements;
			mutable std::mutex m_memoryAccess;
	};
}
