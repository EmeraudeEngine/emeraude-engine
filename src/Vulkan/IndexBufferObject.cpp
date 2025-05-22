/*
 * src/Vulkan/IndexBufferObject.cpp
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

#include "IndexBufferObject.hpp"

/* Local inclusions. */
#include "MemoryRegion.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	IndexBufferObject::IndexBufferObject (const std::shared_ptr< Device > & device, uint32_t indexCount) noexcept
		: AbstractDeviceBuffer(device, 0, indexCount * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
		m_indexCount(indexCount)
	{

	}

	bool
	IndexBufferObject::create (TransferManager & transferManager, const std::vector< uint32_t > & data) noexcept
	{
		const auto bytes = data.size() * sizeof(uint32_t);

		if ( !this->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to allocate " << bytes << " bytes !";

			return false;
		}

		if ( !this->writeData(transferManager, {data.data(), bytes}) )
		{
			TraceError{ClassId} << "Unable to write " << bytes << " bytes into the buffer !";

			return false;
		}

		return true;
	}
}
