/*
 * src/Vulkan/DescriptorSet.hpp
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
#include "AbstractObject.hpp"

namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorPool;
		class DescriptorSetLayout;
		class Image;
		class ImageView;
		class Sampler;
		class UniformBufferObject;
	}

	namespace Graphics::TextureResource
	{
		class Abstract;
	}
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The descriptor set wrapper class.
	 * @extends EmEn::Vulkan::AbstractObject This object will use the descriptor pool to get the device.
	 */
	class DescriptorSet final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDescriptorSet"};

			/**
			 * @brief Constructs a descriptor set.
			 * @param descriptorPool The reference to the descriptor pool smart pointer.
			 * @param descriptorSetLayout The reference to the descriptor layout smart pointer.
			 */
			DescriptorSet (const std::shared_ptr< DescriptorPool > & descriptorPool, const std::shared_ptr< DescriptorSetLayout > & descriptorSetLayout) noexcept
				: m_descriptorPool{descriptorPool},
				m_descriptorSetLayout{descriptorSetLayout}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSet (const DescriptorSet & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSet (DescriptorSet && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSet & operator= (const DescriptorSet & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			DescriptorSet & operator= (DescriptorSet && copy) noexcept = default;

			/**
			 * @brief Destructs the descriptor set.
			 */
			~DescriptorSet () override
			{
				this->destroy();
			}

			/**
			 * @brief Creates the descriptor set in video memory.
			 * @return bool
			 */
			bool create () noexcept;

			/**
			 * @brief Destroys the descriptor set from video memory.
			 * @return bool
			 */
			bool destroy () noexcept;

			/**
			 * @brief Returns the descriptor set vulkan handle.
			 * @return VkDescriptorSet
			 */
			[[nodiscard]]
			VkDescriptorSet
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the responsible descriptor pool smart pointer.
			 * @return std::shared_ptr< DescriptorPool >
			 */
			[[nodiscard]]
			std::shared_ptr< DescriptorPool >
			descriptorPool () const noexcept
			{
				return m_descriptorPool;
			}

			/**
			 * @brief Returns the descriptor layout smart pointer.
			 * @return std::shared_ptr< DescriptorSetLayout >
			 */
			[[nodiscard]]
			std::shared_ptr< DescriptorSetLayout >
			descriptorSetLayout () const noexcept
			{
				return m_descriptorSetLayout;
			}

			/**
			 * @brief Writes a uniform buffer object to the descriptor set.
			 * @param bindingIndex The binding index of the texture inside the descriptor set layout.
			 * @param uniformBufferObject A reference to the uniform buffer object smart pointer.
			 * @param elementOffset The element offset inside the uniform buffer object. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeUniformBufferObject (uint32_t bindingIndex, const UniformBufferObject & uniformBufferObject, uint32_t elementOffset = 0) const noexcept;

			/**
			 * @brief Writes a uniform buffer object to the descriptor set.
			 * @param bindingIndex The binding index of the texture inside the descriptor set layout.
			 * @param uniformBufferObject A reference to the uniform buffer object smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeUniformBufferObjectDynamic (uint32_t bindingIndex, const UniformBufferObject & uniformBufferObject) const noexcept;

			/**
			 * @brief Writes a sampled texture to the descriptor set.
			 * @param bindingIndex The binding index of the texture inside the descriptor set layout.
			 * @param texture A reference to the texture resource.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeSampler (uint32_t bindingIndex, const Graphics::TextureResource::Abstract & texture) const noexcept;

			/**
			 * @brief Writes a sampled texture to the descriptor set.
			 * @param bindingIndex The binding index of the first texture inside the descriptor set layout.
			 * @param texture A reference to the texture.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeCombinedImageSampler (uint32_t bindingIndex, const Graphics::TextureResource::Abstract & texture) const noexcept;

			/**
			* @brief Writes a sampled texture to the descriptor set.
			* @param bindingIndex The binding index of the first texture inside the descriptor set layout.
			* @param image A reference to a vulkan image.
			* @param imageView A reference to a vulkan image view.
			* @param sampler A reference to a vulkan sampler.
			* @return bool
			*/
			[[nodiscard]]
			bool writeCombinedImageSampler (uint32_t bindingIndex, const Image & image, const ImageView & imageView, const Sampler & sampler) const noexcept;

		private:

			VkDescriptorSet m_handle{VK_NULL_HANDLE};
			std::shared_ptr< DescriptorPool > m_descriptorPool;
			std::shared_ptr< DescriptorSetLayout > m_descriptorSetLayout;
	};
}
