/*
 * src/Vulkan/AbstractHostBuffer.cpp
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

#include "AbstractHostBuffer.hpp"

/* STL inclusions. */
#include <cstring>
#include <algorithm>

/* Local inclusions. */
#include "MemoryRegion.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	bool
	AbstractHostBuffer::writeData (const MemoryRegion & memoryRegion) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_memoryAccess};

		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		/* Lock the buffer for access. */
		auto * pointer = this->deviceMemory()->mapMemory(memoryRegion.offset(), memoryRegion.bytes());

		if ( pointer == nullptr )
		{
			TraceError{ClassId} << "Unable to map the buffer from offset " << memoryRegion.offset() << " for " << memoryRegion.bytes() << " bytes.";

			return false;
		}

		/* Raw data copy... */
		std::memcpy(pointer, memoryRegion.source(), memoryRegion.bytes());

		/* Unlock the buffer. */
		this->deviceMemory()->unmapMemory();

		return true;
	}

	bool
	AbstractHostBuffer::writeData (const std::vector< MemoryRegion > & memoryRegions) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_memoryAccess};

		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		if ( memoryRegions.empty() )
		{
			Tracer::error(ClassId, "No memory region to write !");

			return false;
		}

		/* TODO: Check for performance improvement on mapping the buffer once with larger boundaries. */

		/* Raw data copy... */
		return std::ranges::all_of(memoryRegions, [&] (const auto & memoryRegion) {
			/* Lock the buffer for access. */
			auto * pointer = this->deviceMemory()->mapMemory(memoryRegion.offset(), this->bytes());

			if ( pointer == nullptr )
			{
				TraceError{ClassId} << "Unable to map the buffer from offset " << memoryRegion.offset() << " for " << this->bytes() << " bytes.";

				return false;
			}

			/* Raw data copy... */
			std::memcpy(pointer, memoryRegion.source(), memoryRegion.bytes());

			/* Unlock the buffer. */
			this->deviceMemory()->unmapMemory();

			return true;
		});
	}
}
