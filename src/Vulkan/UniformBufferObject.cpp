/*
 * src/Vulkan/UniformBufferObject.cpp
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

#include "UniformBufferObject.hpp"

/* Local inclusions. */
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	uint32_t
	UniformBufferObject::maxElementCount () const noexcept
	{
		if ( m_blockAlignedSize == 0 )
		{
			return 1;
		}

		const auto maxUBOSize = this->device()->physicalDevice()->propertiesVK10().limits.maxUniformBufferRange;

		return maxUBOSize / m_blockAlignedSize;
	}

	VkDescriptorBufferInfo
	UniformBufferObject::getDescriptorInfo (uint32_t elementOffset) const noexcept
	{
		if ( m_blockAlignedSize == 0 )
		{
			TraceWarning{ClassId} << "The uniform buffer object '" << this->identifier() << "' has been built with no block size !";

			return this->getDescriptorInfo(0, static_cast< uint32_t >(this->bytes()));
		}

		/* NOTE: Convert element index to byte offset. */
		return this->getDescriptorInfo(elementOffset * m_blockAlignedSize, m_blockAlignedSize);
	}
}
