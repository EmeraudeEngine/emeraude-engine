/*
* src/Vulkan/TextureInterface.cpp
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

#include "TextureInterface.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	constexpr auto TracerTag{"TextureInterface"};

	VkDescriptorImageInfo
	TextureInterface::getDescriptorInfo () const noexcept
	{
		VkDescriptorImageInfo descriptorInfo{};

		if ( const auto & sampler = this->sampler(); sampler == nullptr )
		{
			Tracer::error(TracerTag, "The texture has no sampler !");

			descriptorInfo.sampler = VK_NULL_HANDLE;
		}
		else
		{
			descriptorInfo.sampler = sampler->handle();
		}

		if ( const auto & imageView = this->imageView(); imageView == nullptr )
		{
			Tracer::error(TracerTag, "The texture has no image view !");

			descriptorInfo.imageView = VK_NULL_HANDLE;
		}
		else
		{
			descriptorInfo.imageView = imageView->handle();
		}

		if ( const auto & image = this->image(); image == nullptr )
		{
			Tracer::error(TracerTag, "The texture has no image !");

			descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else
		{
			descriptorInfo.imageLayout = image->currentImageLayout();
		}

		return descriptorInfo;
	}
}
