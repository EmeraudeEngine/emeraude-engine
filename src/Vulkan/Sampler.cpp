/*
 * src/Vulkan/Sampler.cpp
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

#include "Sampler.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	Sampler::Sampler (const std::shared_ptr< Device > & device, Settings & settings, VkSamplerCreateFlags createFlags) noexcept
		: AbstractDeviceDependentObject(device)
	{
		const auto magFilter = settings.get< std::string >(GraphicsTextureMagFilteringKey, DefaultGraphicsTextureFiltering);
		const auto minFilter = settings.get< std::string >(GraphicsTextureMinFilteringKey, DefaultGraphicsTextureFiltering);
		const auto mipmapMode = settings.get< std::string >(GraphicsTextureMipFilteringKey, DefaultGraphicsTextureFiltering);
		const auto mipLevels = settings.get< float >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels);
		const auto anisotropyLevels = settings.get< float >(GraphicsTextureAnisotropyLevelsKey, DefaultGraphicsTextureAnisotropy);

		m_createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = createFlags;
		m_createInfo.magFilter = magFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		m_createInfo.minFilter = minFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		m_createInfo.mipmapMode = mipmapMode == "linear" ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		m_createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		m_createInfo.mipLodBias = 0.0F;
		m_createInfo.anisotropyEnable = anisotropyLevels > 1.0F ? VK_TRUE : VK_FALSE;
		m_createInfo.maxAnisotropy = anisotropyLevels;
		m_createInfo.compareEnable = VK_FALSE;
		m_createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		m_createInfo.minLod = 0.0F;
		m_createInfo.maxLod = mipLevels > 0.0F ? mipLevels : VK_LOD_CLAMP_NONE;
		m_createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		m_createInfo.unnormalizedCoordinates = VK_FALSE;
	}

	Sampler::Sampler (const std::shared_ptr< Device > & device, const VkSamplerCreateInfo & createInfo) noexcept
		: AbstractDeviceDependentObject(device), m_createInfo(createInfo)
	{

	}

	Sampler::~Sampler ()
	{
		this->destroyFromHardware();
	}

	bool
	Sampler::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this sampler !");

			return false;
		}

		const auto result = vkCreateSampler(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a sampler : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Sampler::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the sampler " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if (  m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a sampler");

			vkDestroySampler(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}
}
