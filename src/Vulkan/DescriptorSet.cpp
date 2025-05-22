/*
 * src/Vulkan/DescriptorSet.cpp
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

#include "DescriptorSet.hpp"

/* Local inclusions. */
#include "Graphics/TextureResource/Abstract.hpp"
#include "Device.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "DescriptorSet.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Sampler.hpp"
#include "UniformBufferObject.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	bool
	DescriptorSet::create () noexcept
	{
		if ( m_descriptorPool == nullptr || !m_descriptorPool->isCreated() )
		{
			Tracer::error(ClassId, "Descriptor pool is null or not created to allocate this descriptor set !");

			return false;
		}

		m_handle = m_descriptorPool->allocateDescriptorSet(*m_descriptorSetLayout);

		if ( m_handle == VK_NULL_HANDLE )
		{
			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	DescriptorSet::destroy () noexcept
	{
		if ( m_descriptorPool == nullptr || !m_descriptorPool->isCreated() )
		{
			Tracer::error(ClassId, "No or uninitialized descriptor pool to destroy this descriptor set !");

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			m_descriptorPool->freeDescriptorSet(m_handle);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	bool
	DescriptorSet::writeUniformBufferObject (uint32_t bindingIndex, const UniformBufferObject & uniformBufferObject, uint32_t elementOffset) const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The descriptor set is not yet created ! Unable to write into it.");

			return false;
		}

		if ( !uniformBufferObject.isCreated() )
		{
			Tracer::error(ClassId, "The uniform buffer object is not created !");

			return false;
		}

		const auto descriptorInfo =
			uniformBufferObject.isShared() ?
			uniformBufferObject.getDescriptorInfo(elementOffset) :
			uniformBufferObject.getDescriptorInfo();

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = m_handle;
		writeDescriptorSet.dstBinding = bindingIndex;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.pImageInfo = nullptr;
		writeDescriptorSet.pBufferInfo = &descriptorInfo;
		writeDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			m_descriptorPool->device()->handle(),
			1, &writeDescriptorSet,
			0, VK_NULL_HANDLE
		);

		return true;
	}

	bool
	DescriptorSet::writeUniformBufferObjectDynamic (uint32_t bindingIndex, const UniformBufferObject & uniformBufferObject) const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The descriptor set is not yet created ! Unable to write into it.");

			return false;
		}

		if ( !uniformBufferObject.isCreated() )
		{
			Tracer::error(ClassId, "The uniform buffer object is not created !");

			return false;
		}

		const auto descriptorInfo = uniformBufferObject.getDescriptorInfo(0);

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = m_handle;
		writeDescriptorSet.dstBinding = bindingIndex;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		writeDescriptorSet.pImageInfo = nullptr;
		writeDescriptorSet.pBufferInfo = &descriptorInfo;
		writeDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			m_descriptorPool->device()->handle(),
			1, &writeDescriptorSet,
			0, VK_NULL_HANDLE
		);

		return true;
	}

	bool
	DescriptorSet::writeSampler (uint32_t bindingIndex, const Graphics::TextureResource::Abstract & texture) const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The descriptor set is not yet created ! Unable to write into it.");

			return false;
		}

		if ( !texture.isCreated() )
		{
			Tracer::error(ClassId, "The texture is not created !");

			return false;
		}

		/* TODO: Remove this ! */
		const auto descriptorInfo = texture.getDescriptorInfo();

		if ( descriptorInfo.sampler == VK_NULL_HANDLE || descriptorInfo.imageView == VK_NULL_HANDLE || descriptorInfo.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			return false;
		}

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = m_handle;
		writeDescriptorSet.dstBinding = bindingIndex;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writeDescriptorSet.pImageInfo = &descriptorInfo;
		writeDescriptorSet.pBufferInfo = nullptr;
		writeDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			m_descriptorPool->device()->handle(),
			1, &writeDescriptorSet,
			0, VK_NULL_HANDLE
		);

		return true;
	}

	bool
	DescriptorSet::writeCombinedImageSampler (uint32_t bindingIndex, const Graphics::TextureResource::Abstract & texture) const noexcept
	{
		if ( !texture.isCreated() )
		{
			Tracer::error(ClassId, "The texture resource is not created !");

			return false;
		}

		return this->writeCombinedImageSampler(bindingIndex, *texture.image(), *texture.imageView(), *texture.sampler());
	}

	bool
	DescriptorSet::writeCombinedImageSampler (uint32_t bindingIndex, const Image & image, const ImageView & imageView, const Sampler & sampler) const noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The descriptor set is not yet created ! Unable to write into it.");

			return false;
		}

		if ( !image.isCreated() )
		{
			Tracer::error(ClassId, "The image is not created !");

			return false;
		}

		if ( !imageView.isCreated() )
		{
			Tracer::error(ClassId, "The image view is not created !");

			return false;
		}

		if ( !sampler.isCreated() )
		{
			Tracer::error(ClassId, "The sampler is not created !");

			return false;
		}

		VkDescriptorImageInfo descriptorInfo{};
		descriptorInfo.sampler = sampler.handle();
		descriptorInfo.imageView = imageView.handle();
		descriptorInfo.imageLayout = image.currentImageLayout(); /* Should be "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL" when it's a texture. */

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = m_handle;
		writeDescriptorSet.dstBinding = bindingIndex;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.pImageInfo = &descriptorInfo;
		writeDescriptorSet.pBufferInfo = nullptr;
		writeDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			m_descriptorPool->device()->handle(),
			1, &writeDescriptorSet,
			0, VK_NULL_HANDLE
		);

		return true;
	}
}
