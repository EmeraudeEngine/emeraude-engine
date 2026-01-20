/*
 * src/Graphics/BindlessTexturesManager.cpp
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

#include "BindlessTextureManager.hpp"

/* STL inclusions. */
#include <vector>

/* Local inclusions. */
#include "Renderer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/TextureInterface.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

	BindlessTextureManager::BindlessTextureManager (Renderer & renderer) noexcept
		: ServiceInterface{ClassId},
		m_renderer{renderer}
	{

	}

	void
	BindlessTextureManager::setDevice (const std::shared_ptr< Vulkan::Device > & device) noexcept
	{
		m_device = device;
	}

	bool
	BindlessTextureManager::onInitialize () noexcept
	{
		if ( m_device == nullptr )
		{
			Tracer::error(ClassId, "No device set for the bindless textures manager !");

			return false;
		}

		if ( !this->createDescriptorSetLayout() )
		{
			Tracer::error(ClassId, "Failed to create the bindless descriptor set layout !");

			return false;
		}

		if ( !this->createDescriptorPool() )
		{
			Tracer::error(ClassId, "Failed to create the bindless descriptor pool !");

			return false;
		}

		if ( !this->createDescriptorSet() )
		{
			Tracer::error(ClassId, "Failed to create the bindless descriptor set !");

			return false;
		}

		TraceSuccess{ClassId} <<
			"Bindless textures manager initialized successfully with: "
			"1D[" << MaxTextures1D << "], "
			"2D[" << MaxTextures2D << "], "
			"3D[" << MaxTextures3D << "], "
			"Cube[" << MaxTexturesCube << "] textures.";

		return true;
	}

	bool
	BindlessTextureManager::onTerminate () noexcept
	{
		m_descriptorSet.reset();
		m_descriptorPool.reset();
		m_descriptorSetLayout.reset();
		m_device.reset();

		return true;
	}

	bool
	BindlessTextureManager::createDescriptorSetLayout () noexcept
	{
		/* Binding flags for UPDATE_AFTER_BIND and PARTIALLY_BOUND. */
		constexpr VkDescriptorBindingFlags bindingFlags =
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		/* Use the LayoutManager to create the descriptor set layout. */
		auto & layoutManager = m_renderer.layoutManager();

		m_descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout("BindlessTexturesLayout", VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

		if ( m_descriptorSetLayout == nullptr )
		{
			Tracer::error(ClassId, "Failed to prepare bindless descriptor set layout !");

			return false;
		}

		/* Declare each texture array binding with the appropriate flags. */
		/* Binding 0: sampler1D array */
		if ( !m_descriptorSetLayout->declare(VkDescriptorSetLayoutBinding{Texture1DBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures1D, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, bindingFlags) )
		{
			Tracer::error(ClassId, "Failed to declare 1D texture binding !");

			return false;
		}

		/* Binding 1: sampler2D array */
		if ( !m_descriptorSetLayout->declare(VkDescriptorSetLayoutBinding{Texture2DBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures2D, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, bindingFlags) )
		{
			Tracer::error(ClassId, "Failed to declare 2D texture binding !");

			return false;
		}

		/* Binding 2: sampler3D array */
		if ( !m_descriptorSetLayout->declare(VkDescriptorSetLayoutBinding{Texture3DBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures3D, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, bindingFlags) )
		{
			Tracer::error(ClassId, "Failed to declare 3D texture binding !");

			return false;
		}

		/* Binding 3: samplerCube array */
		if ( !m_descriptorSetLayout->declare(VkDescriptorSetLayoutBinding{TextureCubeBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTexturesCube, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, bindingFlags) )
		{
			Tracer::error(ClassId, "Failed to declare cubemap texture binding !");

			return false;
		}

		/* Create the layout on hardware via the LayoutManager. */
		if ( !layoutManager.createDescriptorSetLayout(m_descriptorSetLayout) )
		{
			Tracer::error(ClassId, "Failed to create bindless descriptor set layout on hardware !");

			return false;
		}

		return true;
	}

	bool
	BindlessTextureManager::createDescriptorPool () noexcept
	{
		/* Pool sizes for each texture type. */
		std::vector< VkDescriptorPoolSize > poolSizes{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures1D + MaxTextures2D + MaxTextures3D + MaxTexturesCube}
		};

		/* Create pool with UPDATE_AFTER_BIND flag and FREE_DESCRIPTOR_SET_BIT
		 * to allow individual descriptor set deallocation on shutdown. */
		m_descriptorPool = std::make_shared< Vulkan::DescriptorPool >(
			m_device,
			poolSizes,
			1, /* maxSets - we only need one global descriptor set */
			VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		);

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the bindless descriptor pool on hardware !");

			return false;
		}

		return true;
	}

	bool
	BindlessTextureManager::createDescriptorSet () noexcept
	{
		m_descriptorSet = std::make_unique< Vulkan::DescriptorSet >(m_descriptorPool, m_descriptorSetLayout);

		if ( !m_descriptorSet->create() )
		{
			Tracer::error(ClassId, "Failed to create the bindless descriptor set !");

			return false;
		}

		return true;
	}

	uint32_t
	BindlessTextureManager::allocateIndex (std::queue< uint32_t > & freeIndices, uint32_t & nextIndex, uint32_t maxIndex) noexcept
	{
		if ( !freeIndices.empty() )
		{
			const auto index = freeIndices.front();

			freeIndices.pop();

			return index;
		}

		if ( nextIndex >= maxIndex )
		{
			return UINT32_MAX;
		}

		return nextIndex++;
	}

	void
	BindlessTextureManager::freeIndex (std::queue< uint32_t > & freeIndices, uint32_t index) noexcept
	{
		freeIndices.push(index);
	}

	bool
	BindlessTextureManager::writeTextureToDescriptorSet (uint32_t binding, uint32_t arrayIndex, const Vulkan::TextureInterface & texture) const noexcept
	{
		if ( m_descriptorSet == nullptr || !m_descriptorSet->isCreated() )
		{
			Tracer::error(ClassId, "Descriptor set is not created !");

			return false;
		}

		if ( !texture.isCreated() )
		{
			Tracer::error(ClassId, "Texture is not created !");

			return false;
		}

		const auto descriptorInfo = texture.getDescriptorInfo();

		if ( descriptorInfo.sampler == VK_NULL_HANDLE || descriptorInfo.imageView == VK_NULL_HANDLE )
		{
			Tracer::error(ClassId, "Invalid texture descriptor info !");

			return false;
		}

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = m_descriptorSet->handle();
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstArrayElement = arrayIndex;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.pImageInfo = &descriptorInfo;
		writeDescriptorSet.pBufferInfo = nullptr;
		writeDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			m_device->handle(),
			1, &writeDescriptorSet,
			0, nullptr
		);

		return true;
	}

	uint32_t
	BindlessTextureManager::registerTexture1D (const Vulkan::TextureInterface & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		const auto index = allocateIndex(m_freeIndices1D, m_nextIndex1D, MaxTextures1D);

		if ( index == UINT32_MAX )
		{
			Tracer::error(ClassId, "No more slots available for 1D textures !");

			return UINT32_MAX;
		}

		if ( !this->writeTextureToDescriptorSet(Texture1DBinding, index, texture) )
		{
			freeIndex(m_freeIndices1D, index);

			return UINT32_MAX;
		}

		return index;
	}

	uint32_t
	BindlessTextureManager::registerTexture2D (const Vulkan::TextureInterface & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		const auto index = allocateIndex(m_freeIndices2D, m_nextIndex2D, MaxTextures2D);

		if ( index == UINT32_MAX )
		{
			Tracer::error(ClassId, "No more slots available for 2D textures !");

			return UINT32_MAX;
		}

		if ( !this->writeTextureToDescriptorSet(Texture2DBinding, index, texture) )
		{
			freeIndex(m_freeIndices2D, index);

			return UINT32_MAX;
		}

		return index;
	}

	uint32_t
	BindlessTextureManager::registerTexture3D (const Vulkan::TextureInterface & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		const auto index = allocateIndex(m_freeIndices3D, m_nextIndex3D, MaxTextures3D);

		if ( index == UINT32_MAX )
		{
			Tracer::error(ClassId, "No more slots available for 3D textures !");

			return UINT32_MAX;
		}

		if ( !this->writeTextureToDescriptorSet(Texture3DBinding, index, texture) )
		{
			freeIndex(m_freeIndices3D, index);

			return UINT32_MAX;
		}

		return index;
	}

	uint32_t
	BindlessTextureManager::registerTextureCube (const Vulkan::TextureInterface & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		const auto index = allocateIndex(m_freeIndicesCube, m_nextIndexCube, MaxTexturesCube);

		if ( index == UINT32_MAX )
		{
			Tracer::error(ClassId, "No more slots available for cubemap textures !");

			return UINT32_MAX;
		}

		if ( !this->writeTextureToDescriptorSet(TextureCubeBinding, index, texture) )
		{
			freeIndex(m_freeIndicesCube, index);

			return UINT32_MAX;
		}

		return index;
	}

	void
	BindlessTextureManager::unregisterTexture1D (uint32_t index) noexcept
	{
		if ( index >= MaxTextures1D )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_indexMutex};

		freeIndex(m_freeIndices1D, index);
	}

	void
	BindlessTextureManager::unregisterTexture2D (uint32_t index) noexcept
	{
		if ( index >= MaxTextures2D )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_indexMutex};

		freeIndex(m_freeIndices2D, index);
	}

	void
	BindlessTextureManager::unregisterTexture3D (uint32_t index) noexcept
	{
		if ( index >= MaxTextures3D )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_indexMutex};

		freeIndex(m_freeIndices3D, index);
	}

	void
	BindlessTextureManager::unregisterTextureCube (uint32_t index) noexcept
	{
		if ( index >= MaxTexturesCube )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_indexMutex};

		freeIndex(m_freeIndicesCube, index);
	}

	bool
	BindlessTextureManager::updateTexture1D (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept
	{
		if ( index >= MaxTextures1D )
		{
			TraceError{ClassId} << "Invalid 1D texture index: " << index;

			return false;
		}

		/* NOTE: Protect the descriptor set writing. */
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		return this->writeTextureToDescriptorSet(Texture1DBinding, index, texture);
	}

	bool
	BindlessTextureManager::updateTexture2D (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept
	{
		if ( index >= MaxTextures2D )
		{
			TraceError{ClassId} << "Invalid 2D texture index: " << index;

			return false;
		}

		/* NOTE: Protect the descriptor set writing. */
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		return this->writeTextureToDescriptorSet(Texture2DBinding, index, texture);
	}

	bool
	BindlessTextureManager::updateTexture3D (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept
	{
		if ( index >= MaxTextures3D )
		{
			TraceError{ClassId} << "Invalid 3D texture index: " << index;

			return false;
		}

		/* NOTE: Protect the descriptor set writing. */
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		return this->writeTextureToDescriptorSet(Texture3DBinding, index, texture);
	}

	bool
	BindlessTextureManager::updateTextureCube (uint32_t index, const Vulkan::TextureInterface & texture) const noexcept
	{
		if ( index >= MaxTexturesCube )
		{
			TraceError{ClassId} << "Invalid cubemap texture index: " << index;

			return false;
		}

		/* NOTE: Protect the descriptor set writing. */
		const std::lock_guard< std::mutex > lock{m_indexMutex};

		return this->writeTextureToDescriptorSet(TextureCubeBinding, index, texture);
	}

	const Vulkan::DescriptorSet *
	BindlessTextureManager::descriptorSet () const noexcept
	{
		return m_descriptorSet.get();
	}

	std::shared_ptr< Vulkan::DescriptorSetLayout >
	BindlessTextureManager::descriptorSetLayout () const noexcept
	{
		return m_descriptorSetLayout;
	}
}
