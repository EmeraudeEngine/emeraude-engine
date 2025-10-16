/*
 * src/Vulkan/ImageView.cpp
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

#include "ImageView.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Image.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	ImageView::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this image view !");

			return false;
		}

		if ( m_image != nullptr && m_image->isCreated() )
		{
			m_createInfo.image = m_image->handle();
			m_createInfo.format = m_image->createInfo().format;
		}
		else
		{
			Tracer::error(ClassId, "The image is null or not created ! Cannot make a view of it !");

			return false;
		}

		const auto result = vkCreateImageView(
			this->device()->handle(),
			&m_createInfo,
			VK_NULL_HANDLE,
			&m_handle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create an image view : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	ImageView::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the image view " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyImageView(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;

			this->setDestroyed();
		}

		return true;
	}
}
